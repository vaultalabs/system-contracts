#include <eosio/datastream.hpp>
#include <eosio/eosio.hpp>
#include <eosio/multi_index.hpp>
#include <eosio/privileged.hpp>
#include <eosio/serialize.hpp>
#include <eosio/transaction.hpp>

#include <eosio.system/eosio.system.hpp>
#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

   using eosio::asset;
   using eosio::const_mem_fun;
   using eosio::current_time_point;
   using eosio::indexed_by;
   using eosio::permission_level;
   using eosio::seconds;
   using eosio::time_point_sec;
   using eosio::token;

   /**
    *  This action will buy an exact amount of ram and bill the payer the current market price.
    */
   action_return_buyram system_contract::buyrambytes( const name& payer, const name& receiver, uint32_t bytes ) {
      auto itr = _rammarket.find(ramcore_symbol.raw());
      const int64_t ram_reserve   = itr->base.balance.amount;
      const int64_t eos_reserve   = itr->quote.balance.amount;
      const int64_t cost          = exchange_state::get_bancor_input( ram_reserve, eos_reserve, bytes );
      const int64_t cost_plus_fee = cost / double(0.995);
      return buyram( payer, receiver, asset{ cost_plus_fee, core_symbol() } );
   }

   /**
    * Buy self ram action, ram can only be purchased to itself.
    */
   action_return_buyram system_contract::buyramself( const name& account, const asset& quant ) {
      return buyram( account, account, quant );
   }

   /**
    *  When buying ram the payer irreversibly transfers quant to system contract and only
    *  the receiver may reclaim the tokens via the sellram action. The receiver pays for the
    *  storage of all database records associated with this action.
    *
    *  RAM is a scarce resource whose supply is defined by global properties max_ram_size. RAM is
    *  priced using the bancor algorithm such that price-per-byte with a constant reserve ratio of 100:1.
    */
   action_return_buyram system_contract::buyram( const name& payer, const name& receiver, const asset& quant )
   {
      require_auth( payer );
      update_ram_supply();
      require_recipient(payer);
      require_recipient(receiver);

      check( quant.symbol == core_symbol(), "must buy ram with core token" );
      check( quant.amount > 0, "must purchase a positive amount" );

      asset fee = quant;
      fee.amount = ( fee.amount + 199 ) / 200; /// .5% fee (round up)
      // fee.amount cannot be 0 since that is only possible if quant.amount is 0 which is not allowed by the assert above.
      // If quant.amount == 1, then fee.amount == 1,
      // otherwise if quant.amount > 1, then 0 < fee.amount < quant.amount.
      asset quant_after_fee = quant;
      quant_after_fee.amount -= fee.amount;
      // quant_after_fee.amount should be > 0 if quant.amount > 1.
      // If quant.amount == 1, then quant_after_fee.amount == 0 and the next inline transfer will fail causing the buyram action to fail.
      {
         token::transfer_action transfer_act{ token_account, { {payer, active_permission}, {ram_account, active_permission} } };
         transfer_act.send( payer, ram_account, quant_after_fee, "buy ram" );
      }
      if ( fee.amount > 0 ) {
         token::transfer_action transfer_act{ token_account, { {payer, active_permission} } };
         transfer_act.send( payer, ramfee_account, fee, "ram fee" );
         channel_to_system_fees( ramfee_account, fee );
      }

      int64_t bytes_out;

      const auto& market = _rammarket.get(ramcore_symbol.raw(), "ram market does not exist");
      _rammarket.modify( market, same_payer, [&]( auto& es ) {
         bytes_out = es.direct_convert( quant_after_fee,  ram_symbol ).amount;
      });

      check( bytes_out > 0, "must reserve a positive amount" );

      _gstate.total_ram_bytes_reserved += uint64_t(bytes_out);
      _gstate.total_ram_stake          += quant_after_fee.amount;

      const int64_t ram_bytes = add_ram( receiver, bytes_out );

      // logging
      system_contract::logbuyram_action logbuyram_act{ get_self(), { {get_self(), active_permission} } };
      system_contract::logsystemfee_action logsystemfee_act{ get_self(), { {get_self(), active_permission} } };

      logbuyram_act.send( payer, receiver, quant, bytes_out, ram_bytes, fee );
      logsystemfee_act.send( ram_account, fee, "buy ram" );

      // action return value
      return action_return_buyram{ payer, receiver, quant, bytes_out, ram_bytes, fee };
   }

   void system_contract::logbuyram( const name& payer, const name& receiver, const asset& quantity, int64_t bytes, int64_t ram_bytes, const asset& fee ) {
      require_auth( get_self() );
      require_recipient(payer);
      require_recipient(receiver);
   }

   void system_contract::setramconfig(bool disable_sellram) {
      require_auth( get_self() );
      ramconfig_singleton rc(get_self(), get_self().value);
      rc.set(ram_config{.disable_sellram = disable_sellram}, get_self());
   }

  /**
    *  The system contract now buys and sells RAM allocations at prevailing market prices.
    *  This may result in traders buying RAM today in anticipation of potential shortages
    *  tomorrow. Overall this will result in the market balancing the supply and demand
    *  for RAM over time.
    */
   action_return_sellram system_contract::sellram( const name& account, int64_t bytes ) {
      require_auth( account );
      ramconfig_singleton rc(get_self(), get_self().value);
      if (rc.exists()) {
         check( rc.get().disable_sellram == false, "sellram is disabled");
      }

      update_ram_supply();
      require_recipient(account);
      const int64_t ram_bytes = reduce_ram(account, bytes);

      asset tokens_out;
      auto itr = _rammarket.find(ramcore_symbol.raw());
      _rammarket.modify( itr, same_payer, [&]( auto& es ) {
         /// the cast to int64_t of bytes is safe because we certify bytes is <= quota which is limited by prior purchases
         tokens_out = es.direct_convert( asset(bytes, ram_symbol), core_symbol());
      });

      check( tokens_out.amount > 1, "token amount received from selling ram is too low" );

      _gstate.total_ram_bytes_reserved -= static_cast<decltype(_gstate.total_ram_bytes_reserved)>(bytes); // bytes > 0 is asserted above
      _gstate.total_ram_stake          -= tokens_out.amount;

      //// this shouldn't happen, but just in case it does we should prevent it
      check( _gstate.total_ram_stake >= 0, "error, attempt to unstake more tokens than previously staked" );

      {
         token::transfer_action transfer_act{ token_account, { {ram_account, active_permission}, {account, active_permission} } };
         transfer_act.send( ram_account, account, asset(tokens_out), "sell ram" );
      }
      const int64_t fee = ( tokens_out.amount + 199 ) / 200; /// .5% fee (round up)
      // since tokens_out.amount was asserted to be at least 2 earlier, fee.amount < tokens_out.amount
      if ( fee > 0 ) {
         token::transfer_action transfer_act{ token_account, { {account, active_permission} } };
         transfer_act.send( account, ramfee_account, asset(fee, core_symbol()), "sell ram fee" );
         channel_to_system_fees( ramfee_account, asset(fee, core_symbol() ));
      }

      // logging
      system_contract::logsellram_action logsellram_act{ get_self(), { {get_self(), active_permission} } };
      system_contract::logsystemfee_action logsystemfee_act{ get_self(), { {get_self(), active_permission} } };

      logsellram_act.send( account, tokens_out, bytes, ram_bytes, asset(fee, core_symbol() ) );
      logsystemfee_act.send( ram_account, asset(fee, core_symbol() ), "sell ram" );

      // action return value
      return action_return_sellram{ account, tokens_out, bytes, ram_bytes, asset(fee, core_symbol() ) };
   }

   void system_contract::logsellram( const name& account, const asset& quantity, int64_t bytes, int64_t ram_bytes, const asset& fee ) {
      require_auth( get_self() );
      require_recipient(account);
   }

   action_return_ramtransfer system_contract::giftram( const name from, const name to, int64_t bytes, const std::string& memo  ) {
      require_auth( from );
      require_recipient(from);
      require_recipient(to);
      
      check( bytes > 0, "must gift positive bytes" );
      check(is_account(to), "to=" + to.to_string() + " account does not exist");

      // add ram gift to `gifted_ram_table`
      gifted_ram_table giftedram(get_self(), get_self().value);
      auto giftedram_itr = giftedram.find(to.value);
      if (giftedram_itr == giftedram.end()) {
         giftedram.emplace(from, [&](auto& res) { // gifter pays the ram costs for gift storage
            res.giftee    = to;
            res.gifter    = from;
            res.ram_bytes = bytes;
         });
      } else {
         check(giftedram_itr->gifter == from,
               "A single RAM gifter is allowed at any one time per account, currently holding RAM gifted by: " +
                  giftedram_itr->gifter.to_string());
         giftedram.modify(giftedram_itr, same_payer, [&](auto& res) {
            res.ram_bytes += bytes;
         });
      }

      return ramtransfer( from, to, bytes, memo );
   }

   action_return_ramtransfer system_contract::ungiftram( const name from, const name to, const std::string& memo ) {
      require_auth( from );
      require_recipient(from);
      require_recipient(to);

      check(is_account(to), "to=" + to.to_string() + " account does not exist");

      // check the amount to return from `gifted_ram_table`
      gifted_ram_table giftedram(get_self(), get_self().value);
      auto giftedram_itr = giftedram.find(from.value);
      check(giftedram_itr != giftedram.end(), "Account " + from.to_string() + " does not hold any gifted RAM");
      check(giftedram_itr->gifter == to, "Returning RAM to wrong gifter, should be: " + giftedram_itr->gifter.to_string());
      int64_t returned_bytes = giftedram_itr->ram_bytes;

      // we erase the `gifted_ram_table` row before the call to `ramtransfer`, so the `reduce_ram()` will work even
      // if we just have `returned_bytes` available in `user_resources_table`
      giftedram.erase(giftedram_itr);

      return ramtransfer( from, to, returned_bytes, memo );
   }

   /**
    * This action will transfer RAM bytes from one account to another.
    */
   action_return_ramtransfer system_contract::ramtransfer( const name& from, const name& to, int64_t bytes, const std::string& memo ) {
      require_auth( from );
      update_ram_supply();
      check( memo.size() <= 256, "memo has more than 256 bytes" );
      const int64_t from_ram_bytes = reduce_ram( from, bytes );
      const int64_t to_ram_bytes = add_ram( to, bytes );
      require_recipient( from );
      require_recipient( to );

      // action return value
      return action_return_ramtransfer{ from, to, bytes, from_ram_bytes, to_ram_bytes };
   }

   /**
    * This action will burn RAM bytes from owner account.
    */
   action_return_ramtransfer system_contract::ramburn( const name& owner, int64_t bytes, const std::string& memo ) {
      require_auth( owner );
      return ramtransfer( owner, null_account, bytes, memo );
   }

   /**
    * This action will buy and then burn the purchased RAM bytes.
    */
   action_return_buyram system_contract::buyramburn( const name& payer, const asset& quantity, const std::string& memo ) {
      require_auth( payer );
      check( quantity.symbol == core_symbol(), "quantity must be core token" );
      check( quantity.amount > 0, "quantity must be positive" );

      const auto return_buyram = buyram( payer, payer, quantity );
      ramburn( payer, return_buyram.bytes_purchased, memo );

      return return_buyram;
   }

   [[eosio::action]]
   void system_contract::logramchange( const name& owner, int64_t bytes, int64_t ram_bytes )
   {
      require_auth( get_self() );
      require_recipient( owner );
   }

   // this is called when transfering or selling ram, and encumbered (gifted) ram cannot be sold or transferred.
   // so if we hold some gifted ram, deduct the amount from the ram available to be sold or transfered.
   int64_t system_contract::reduce_ram( const name& owner, int64_t bytes ) {
      check( bytes > 0, "cannot reduce negative byte" );

      user_resources_table userres( get_self(), owner.value );
      auto res_itr = userres.find( owner.value );
      check( res_itr != userres.end(), "no resource row" );

      gifted_ram_table giftedram( get_self(), get_self().value );
      auto giftedram_itr = giftedram.find( owner.value );
      bool holding_gifted_ram = (giftedram_itr != giftedram.end());

      auto available_bytes = holding_gifted_ram ? res_itr->ram_bytes - giftedram_itr->ram_bytes : res_itr->ram_bytes;
      check( available_bytes >= bytes, "insufficient quota" );

      userres.modify( res_itr, same_payer, [&]( auto& res ) {
          res.ram_bytes -= bytes;
      });
      set_resource_ram_bytes_limits( owner, res_itr->ram_bytes );

      // logging
      system_contract::logramchange_action logramchange_act{ get_self(), { {get_self(), active_permission} }};
      logramchange_act.send( owner, -bytes, res_itr->ram_bytes );
      return res_itr->ram_bytes;
   }

   int64_t system_contract::add_ram( const name& owner, int64_t bytes ) {
      check( bytes > 0, "cannot add negative byte" );
      check( is_account(owner), "owner=" + owner.to_string() + " account does not exist");
      user_resources_table userres( get_self(), owner.value );
      auto res_itr = userres.find( owner.value );

      int64_t updated_ram_bytes = 0;
      if ( res_itr == userres.end() ) {
         // only when `owner == null_account` can the row not be found (see `ramburn`)
         userres.emplace( owner, [&]( auto& res ) {
            res.owner = owner;
            res.net_weight = asset( 0, core_symbol() );
            res.cpu_weight = asset( 0, core_symbol() );
            res.ram_bytes = bytes;
         });
         updated_ram_bytes = bytes;
      } else {
         // row should exist as it is always created when creating the account, see `native::newaccount`
         userres.modify( res_itr, same_payer, [&]( auto& res ) {
            res.ram_bytes += bytes;
         });
         updated_ram_bytes = res_itr->ram_bytes;
      }

      set_resource_ram_bytes_limits( owner, updated_ram_bytes );

      // logging
      system_contract::logramchange_action logramchange_act{ get_self(), { {get_self(), active_permission} } };
      logramchange_act.send( owner, bytes, updated_ram_bytes );
      return updated_ram_bytes;
   }

   void system_contract::set_resource_ram_bytes_limits( const name& owner, int64_t res_bytes ) {
      auto voter_itr = _voters.find( owner.value );
      if ( voter_itr == _voters.end() || !has_field( voter_itr->flags1, voter_info::flags1_fields::ram_managed ) ) {
         int64_t ram_bytes, net, cpu;
         get_resource_limits( owner, ram_bytes, net, cpu );
         set_resource_limits( owner, res_bytes + ram_gift_bytes, net, cpu );
      }
   }

   std::pair<int64_t, int64_t> get_b1_vesting_info() {
      const int64_t base_time = 1527811200; /// Friday, June 1, 2018 12:00:00 AM UTC
      const int64_t current_time = 1638921540; /// Tuesday, December 7, 2021 11:59:00 PM UTC
      const int64_t total_vesting = 100'000'000'0000ll;
      const int64_t vested = int64_t(total_vesting * double(current_time - base_time) / (10*seconds_per_year) );
      return { total_vesting, vested };
   }

   void validate_b1_vesting( int64_t new_stake, asset stake_change ) {
      const auto [total_vesting, vested] = get_b1_vesting_info();
      auto unvestable = total_vesting - vested;

      auto hasAlreadyUnvested = new_stake < unvestable
            && stake_change.amount < 0
            && new_stake + std::abs(stake_change.amount) < unvestable;
      if(hasAlreadyUnvested) return;

      check( new_stake >= unvestable, "b1 can only claim what has already vested" );
   }

   void system_contract::changebw( name from, const name& receiver,
                                   const asset& stake_net_delta, const asset& stake_cpu_delta, bool transfer )
   {
      require_auth( from );
      check( stake_net_delta.amount != 0 || stake_cpu_delta.amount != 0, "should stake non-zero amount" );
      check( std::abs( (stake_net_delta + stake_cpu_delta).amount )
             >= std::max( std::abs( stake_net_delta.amount ), std::abs( stake_cpu_delta.amount ) ),
             "net and cpu deltas cannot be opposite signs" );

      name source_stake_from = from;
      if ( transfer ) {
         from = receiver;
      }

      update_stake_delegated( from, receiver, stake_net_delta, stake_cpu_delta );
      update_user_resources( from, receiver, stake_net_delta, stake_cpu_delta );

      // create refund or update from existing refund
      if ( stake_account != source_stake_from ) { //for eosio both transfer and refund make no sense
         refunds_table refunds_tbl( get_self(), from.value );
         auto req = refunds_tbl.find( from.value );

         //create/update/delete refund
         auto net_balance = stake_net_delta;
         auto cpu_balance = stake_cpu_delta;

         // net and cpu are same sign by assertions in delegatebw and undelegatebw
         // redundant assertion also at start of changebw to protect against misuse of changebw
         bool is_undelegating = (net_balance.amount + cpu_balance.amount ) < 0;
         bool is_delegating_to_self = (!transfer && from == receiver);

         if( is_delegating_to_self || is_undelegating ) {
            if ( req != refunds_tbl.end() ) { //need to update refund
               refunds_tbl.modify( req, same_payer, [&]( refund_request& r ) {
                  if ( net_balance.amount < 0 || cpu_balance.amount < 0 ) {
                     r.request_time = current_time_point();
                  }
                  r.net_amount -= net_balance;
                  if ( r.net_amount.amount < 0 ) {
                     net_balance = -r.net_amount;
                     r.net_amount.amount = 0;
                  } else {
                     net_balance.amount = 0;
                  }
                  r.cpu_amount -= cpu_balance;
                  if ( r.cpu_amount.amount < 0 ){
                     cpu_balance = -r.cpu_amount;
                     r.cpu_amount.amount = 0;
                  } else {
                     cpu_balance.amount = 0;
                  }
               });

               check( 0 <= req->net_amount.amount, "negative net refund amount" ); //should never happen
               check( 0 <= req->cpu_amount.amount, "negative cpu refund amount" ); //should never happen

               if ( req->is_empty() ) {
                  refunds_tbl.erase( req );
               }
            } else if ( net_balance.amount < 0 || cpu_balance.amount < 0 ) { //need to create refund
               refunds_tbl.emplace( from, [&]( refund_request& r ) {
                  r.owner = from;
                  if ( net_balance.amount < 0 ) {
                     r.net_amount = -net_balance;
                     net_balance.amount = 0;
                  } else {
                     r.net_amount = asset( 0, core_symbol() );
                  }
                  if ( cpu_balance.amount < 0 ) {
                     r.cpu_amount = -cpu_balance;
                     cpu_balance.amount = 0;
                  } else {
                     r.cpu_amount = asset( 0, core_symbol() );
                  }
                  r.request_time = current_time_point();
               });
            } // else stake increase requested with no existing row in refunds_tbl -> nothing to do with refunds_tbl
         } /// end if is_delegating_to_self || is_undelegating

         auto transfer_amount = net_balance + cpu_balance;
         if ( 0 < transfer_amount.amount ) {
            token::transfer_action transfer_act{ token_account, { {source_stake_from, active_permission} } };
            transfer_act.send( source_stake_from, stake_account, asset(transfer_amount), "stake bandwidth" );
         }
      }

      vote_stake_updater( from );
      const int64_t staked = update_voting_power( from, stake_net_delta + stake_cpu_delta );
      if ( from == "b1"_n ) {
         validate_b1_vesting( staked, stake_net_delta + stake_cpu_delta );
      }
   }

   void system_contract::update_stake_delegated( const name from, const name receiver, const asset stake_net_delta, const asset stake_cpu_delta )
   {
      del_bandwidth_table del_tbl( get_self(), from.value );
      auto itr = del_tbl.find( receiver.value );
      if( itr == del_tbl.end() ) {
         itr = del_tbl.emplace( from, [&]( auto& dbo ){
               dbo.from          = from;
               dbo.to            = receiver;
               dbo.net_weight    = stake_net_delta;
               dbo.cpu_weight    = stake_cpu_delta;
            });
      } else {
         del_tbl.modify( itr, same_payer, [&]( auto& dbo ){
               dbo.net_weight    += stake_net_delta;
               dbo.cpu_weight    += stake_cpu_delta;
            });
      }
      check( 0 <= itr->net_weight.amount, "insufficient staked net bandwidth" );
      check( 0 <= itr->cpu_weight.amount, "insufficient staked cpu bandwidth" );
      if ( itr->is_empty() ) {
         del_tbl.erase( itr );
      }
   }

   void system_contract::update_user_resources( const name from, const name receiver, const asset stake_net_delta, const asset stake_cpu_delta )
   {
      user_resources_table   totals_tbl( get_self(), receiver.value );
      auto tot_itr = totals_tbl.find( receiver.value );
      if( tot_itr ==  totals_tbl.end() ) {
         tot_itr = totals_tbl.emplace( from, [&]( auto& tot ) {
               tot.owner = receiver;
               tot.net_weight    = stake_net_delta;
               tot.cpu_weight    = stake_cpu_delta;
            });
      } else {
         totals_tbl.modify( tot_itr, from == receiver ? from : same_payer, [&]( auto& tot ) {
               tot.net_weight    += stake_net_delta;
               tot.cpu_weight    += stake_cpu_delta;
            });
      }
      check( 0 <= tot_itr->net_weight.amount, "insufficient staked total net bandwidth" );
      check( 0 <= tot_itr->cpu_weight.amount, "insufficient staked total cpu bandwidth" );

      {
         bool ram_managed = false;
         bool net_managed = false;
         bool cpu_managed = false;

         auto voter_itr = _voters.find( receiver.value );
         if( voter_itr != _voters.end() ) {
            ram_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::ram_managed );
            net_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::net_managed );
            cpu_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::cpu_managed );
         }

         if( !(net_managed && cpu_managed) ) {
            int64_t ram_bytes, net, cpu;
            get_resource_limits( receiver, ram_bytes, net, cpu );

            set_resource_limits( receiver,
                                 ram_managed ? ram_bytes : std::max( tot_itr->ram_bytes + ram_gift_bytes, ram_bytes ),
                                 net_managed ? net : tot_itr->net_weight.amount,
                                 cpu_managed ? cpu : tot_itr->cpu_weight.amount );
         }
      }

      if ( tot_itr->is_empty() ) {
         totals_tbl.erase( tot_itr );
      } // tot_itr can be invalid, should go out of scope
   }

   int64_t system_contract::update_voting_power( const name& voter, const asset& total_update )
   {
      auto voter_itr = _voters.find( voter.value );
      if( voter_itr == _voters.end() ) {
         voter_itr = _voters.emplace( voter, [&]( auto& v ) {
            v.owner  = voter;
            v.staked = total_update.amount;
         });
      } else {
         _voters.modify( voter_itr, same_payer, [&]( auto& v ) {
            v.staked += total_update.amount;
         });
      }

      check( 0 <= voter_itr->staked, "stake for voting cannot be negative" );

      if( voter_itr->producers.size() || voter_itr->proxy ) {
         update_votes( voter, voter_itr->proxy, voter_itr->producers, false );
      }
      return voter_itr->staked;
   }

   void system_contract::delegatebw( const name& from, const name& receiver,
                                     const asset& stake_net_quantity,
                                     const asset& stake_cpu_quantity, bool transfer )
   {
      asset zero_asset( 0, core_symbol() );
      check( stake_cpu_quantity >= zero_asset, "must stake a positive amount" );
      check( stake_net_quantity >= zero_asset, "must stake a positive amount" );
      check( stake_net_quantity.amount + stake_cpu_quantity.amount > 0, "must stake a positive amount" );
      check( !transfer || from != receiver, "cannot use transfer flag if delegating to self" );

      changebw( from, receiver, stake_net_quantity, stake_cpu_quantity, transfer);
   } // delegatebw

   void system_contract::undelegatebw( const name& from, const name& receiver,
                                       const asset& unstake_net_quantity, const asset& unstake_cpu_quantity )
   {
      asset zero_asset( 0, core_symbol() );
      check( unstake_cpu_quantity >= zero_asset, "must unstake a positive amount" );
      check( unstake_net_quantity >= zero_asset, "must unstake a positive amount" );
      check( unstake_cpu_quantity.amount + unstake_net_quantity.amount > 0, "must unstake a positive amount" );
      check( _gstate.thresh_activated_stake_time != time_point(),
             "cannot undelegate bandwidth until the chain is activated (at least 15% of all tokens participate in voting)" );

      changebw( from, receiver, -unstake_net_quantity, -unstake_cpu_quantity, false);
   } // undelegatebw

   void system_contract::refund( const name& owner ) {
      require_auth( owner );

      refunds_table refunds_tbl( get_self(), owner.value );
      auto req = refunds_tbl.find( owner.value );
      check( req != refunds_tbl.end(), "refund request not found" );
      check( req->request_time + seconds(refund_delay_sec) <= current_time_point(),
             "refund is not available yet" );
      token::transfer_action transfer_act{ token_account, { {stake_account, active_permission}, {req->owner, active_permission} } };
      transfer_act.send( stake_account, req->owner, req->net_amount + req->cpu_amount, "unstake" );
      refunds_tbl.erase( req );
   }

   void system_contract::unvest(const name account, const asset unvest_net_quantity, const asset unvest_cpu_quantity)
   {
      require_auth( get_self() );

      check( account == "b1"_n, "only b1 account can unvest");

      check( unvest_cpu_quantity.amount >= 0, "must unvest a positive amount" );
      check( unvest_net_quantity.amount >= 0, "must unvest a positive amount" );

      const auto [total_vesting, vested] = get_b1_vesting_info();
      const asset unvesting = unvest_net_quantity + unvest_cpu_quantity;
      check( unvesting.amount <= total_vesting - vested , "can only unvest what is not already vested");

      // reduce staked from account
      update_voting_power( account, -unvesting );
      update_stake_delegated( account, account, -unvest_net_quantity, -unvest_cpu_quantity );
      update_user_resources( account, account, -unvest_net_quantity, -unvest_cpu_quantity );
      vote_stake_updater( account );

      // transfer unvested tokens to `eosio`
      token::transfer_action transfer_act{ token_account, { {stake_account, active_permission} } };
      transfer_act.send( stake_account, get_self(), unvesting, "unvest" );

      // retire unvested tokens
      token::retire_action retire_act{ token_account, { {"eosio"_n, active_permission} } };
      retire_act.send( unvesting, "unvest" );
   } // unvest

} //namespace eosiosystem
