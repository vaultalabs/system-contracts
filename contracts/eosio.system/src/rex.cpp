#include <eosio.system/eosio.system.hpp>
#include <eosio.token/eosio.token.hpp>
#include <eosio.system/rex.results.hpp>

namespace eosiosystem {

   using eosio::current_time_point;
   using eosio::token;
   using eosio::seconds;

   void system_contract::setrexmature(const std::optional<uint32_t> num_of_maturity_buckets, const std::optional<bool> sell_matured_rex, const std::optional<bool> buy_rex_to_savings )
   {
      require_auth(get_self());

      auto state = _rexmaturity.get_or_default();

      check(*num_of_maturity_buckets > 0, "num_of_maturity_buckets must be positive");
      check(*num_of_maturity_buckets <= 30, "num_of_maturity_buckets must be less than or equal to 30");

      if ( num_of_maturity_buckets ) state.num_of_maturity_buckets = *num_of_maturity_buckets;
      if ( sell_matured_rex ) state.sell_matured_rex = *sell_matured_rex;
      if ( buy_rex_to_savings ) state.buy_rex_to_savings = *buy_rex_to_savings;
      _rexmaturity.set(state, get_self());
   }

   void system_contract::deposit( const name& owner, const asset& amount )
   {
      check(
         eosio::get_sender() == "core.vaulta"_n,
         "EOS has been rebranded to Vaulta. This action must now be called from the core.vaulta contract."
      );

      require_auth( owner );

      check( amount.symbol == core_symbol(), "must deposit core token" );
      check( 0 < amount.amount, "must deposit a positive amount" );
      // inline transfer from owner's token balance
      {
         token::transfer_action transfer_act{ token_account, { owner, active_permission } };
         transfer_act.send( owner, rex_account, amount, "deposit to REX fund" );
      }
      transfer_to_fund( owner, amount );
   }

   void system_contract::withdraw( const name& owner, const asset& amount )
   {
      check(
         eosio::get_sender() == "core.vaulta"_n,
         "EOS has been rebranded to Vaulta. This action must now be called from the core.vaulta contract."
      );

      require_auth( owner );

      check( amount.symbol == core_symbol(), "must withdraw core token" );
      check( 0 < amount.amount, "must withdraw a positive amount" );
      update_rex_account( owner, asset( 0, core_symbol() ), asset( 0, core_symbol() ) );
      transfer_from_fund( owner, amount );
      // inline transfer to owner's token balance
      {
         token::transfer_action transfer_act{ token_account, { rex_account, active_permission } };
         transfer_act.send( rex_account, owner, amount, "withdraw from REX fund" );
      }
   }

   void system_contract::buyrex( const name& from, const asset& amount )
   {
      require_auth( from );

      check( amount.symbol == core_symbol(), "asset must be core token" );
      check( 0 < amount.amount, "must use positive amount" );
      transfer_from_fund( from, amount );
      const asset rex_received    = add_to_rex_pool( amount );
      const asset delta_rex_stake = add_to_rex_balance( from, amount, rex_received );
      runrex(2);
      update_rex_account( from, asset( 0, core_symbol() ), delta_rex_stake );

      process_buy_rex_to_savings( from, rex_received );
      process_sell_matured_rex( from );

      // dummy action added so that amount of REX tokens purchased shows up in action trace
      rex_results::buyresult_action buyrex_act( rex_account, std::vector<eosio::permission_level>{ } );
      buyrex_act.send( rex_received );
   }

   void system_contract::unstaketorex( const name& owner, const name& receiver, const asset& from_net, const asset& from_cpu )
   {
      check(
         eosio::get_sender() == "core.vaulta"_n,
         "EOS has been rebranded to Vaulta. This action must now be called from the core.vaulta contract."
      );

      require_auth( owner );

      check( from_net.symbol == core_symbol() && from_cpu.symbol == core_symbol(), "asset must be core token" );
      check( (0 <= from_net.amount) && (0 <= from_cpu.amount) && (0 < from_net.amount || 0 < from_cpu.amount),
             "must unstake a positive amount to buy rex" );

      {
         del_bandwidth_table dbw_table( get_self(), owner.value );
         auto del_itr = dbw_table.require_find( receiver.value, "delegated bandwidth record does not exist" );
         check( from_net.amount <= del_itr->net_weight.amount, "amount exceeds tokens staked for net");
         check( from_cpu.amount <= del_itr->cpu_weight.amount, "amount exceeds tokens staked for cpu");
         dbw_table.modify( del_itr, same_payer, [&]( delegated_bandwidth& dbw ) {
            dbw.net_weight.amount -= from_net.amount;
            dbw.cpu_weight.amount -= from_cpu.amount;
         });
         if ( del_itr->is_empty() ) {
            dbw_table.erase( del_itr );
         }
      }

      update_resource_limits( name(0), receiver, -from_net.amount, -from_cpu.amount );

      const asset payment = from_net + from_cpu;
      // inline transfer from stake_account to rex_account
      {
         token::transfer_action transfer_act{ token_account, { stake_account, active_permission } };
         transfer_act.send( stake_account, rex_account, payment, "buy REX with staked tokens" );
      }
      const asset rex_received = add_to_rex_pool( payment );
      auto rex_stake_delta = add_to_rex_balance( owner, payment, rex_received );
      runrex(2);
      update_rex_account( owner, asset( 0, core_symbol() ), rex_stake_delta - payment, true );

      process_buy_rex_to_savings( owner, rex_received );
      process_sell_matured_rex( owner );

      // dummy action added so that amount of REX tokens purchased shows up in action trace
      rex_results::buyresult_action buyrex_act( rex_account, std::vector<eosio::permission_level>{ } );
      buyrex_act.send( rex_received );
   }

   void system_contract::sellrex( const name& from, const asset& rex )
   {
      require_auth( from );
      sell_rex( from, rex );
      process_sell_matured_rex( from );
   }

   void system_contract::sell_rex( const name& from, const asset& rex )
   {
      runrex(2);

      auto bitr = _rexbalance.require_find( from.value, "user must first buyrex" );
      check( rex.amount > 0 && rex.symbol == bitr->rex_balance.symbol,
             "asset must be a positive amount of (REX, 4)" );
      process_rex_maturities( bitr );
      check( rex.amount <= bitr->matured_rex, "insufficient available rex" );

      const auto current_order = fill_rex_order( bitr, rex );
      if ( current_order.success && current_order.proceeds.amount == 0 ) {
         check( false, "proceeds are negligible" );
      }
      asset pending_sell_order = update_rex_account( from, current_order.proceeds, current_order.stake_change );
      if ( !current_order.success ) {
         if ( from == "b1"_n ) {
            check( false, "b1 sellrex orders should not be queued" );
         }
         /**
          * REX order couldn't be filled and is added to queue.
          * If account already has an open order, requested rex is added to existing order.
          */
         auto oitr = _rexorders.find( from.value );
         if ( oitr == _rexorders.end() ) {
            oitr = _rexorders.emplace( from, [&]( auto& order ) {
               order.owner         = from;
               order.rex_requested = rex;
               order.is_open       = true;
               order.proceeds      = asset( 0, core_symbol() );
               order.stake_change  = asset( 0, core_symbol() );
               order.order_time    = current_time_point();
            });
         } else {
            _rexorders.modify( oitr, same_payer, [&]( auto& order ) {
               order.rex_requested.amount += rex.amount;
            });
         }
         pending_sell_order.amount = oitr->rex_requested.amount;
      }
      check( pending_sell_order.amount <= bitr->matured_rex, "insufficient funds for current and scheduled orders" );
      // dummy action added so that sell order proceeds show up in action trace
      if ( current_order.success ) {
         rex_results::sellresult_action sellrex_act( rex_account, std::vector<eosio::permission_level>{ } );
         sellrex_act.send( current_order.proceeds );
      }
   }

   void system_contract::cnclrexorder( const name& owner )
   {
      require_auth( owner );

      auto itr = _rexorders.require_find( owner.value, "no sellrex order is scheduled" );
      check( itr->is_open, "sellrex order has been filled and cannot be canceled" );
      _rexorders.erase( itr );
   }

   void system_contract::rentcpu( const name& from, const name& receiver, const asset& loan_payment, const asset& loan_fund )
   {
      require_auth( from );

      rex_cpu_loan_table cpu_loans( get_self(), get_self().value );
      int64_t rented_tokens = rent_rex( cpu_loans, from, receiver, loan_payment, loan_fund );
      update_resource_limits( from, receiver, 0, rented_tokens );
   }

   void system_contract::rentnet( const name& from, const name& receiver, const asset& loan_payment, const asset& loan_fund )
   {
      require_auth( from );

      rex_net_loan_table net_loans( get_self(), get_self().value );
      int64_t rented_tokens = rent_rex( net_loans, from, receiver, loan_payment, loan_fund );
      update_resource_limits( from, receiver, rented_tokens, 0 );
   }

   void system_contract::fundcpuloan( const name& from, uint64_t loan_num, const asset& payment )
   {
      require_auth( from );

      rex_cpu_loan_table cpu_loans( get_self(), get_self().value );
      fund_rex_loan( cpu_loans, from, loan_num, payment  );
   }

   void system_contract::fundnetloan( const name& from, uint64_t loan_num, const asset& payment )
   {
      require_auth( from );

      rex_net_loan_table net_loans( get_self(), get_self().value );
      fund_rex_loan( net_loans, from, loan_num, payment );
   }

   void system_contract::defcpuloan( const name& from, uint64_t loan_num, const asset& amount )
   {
      require_auth( from );

      rex_cpu_loan_table cpu_loans( get_self(), get_self().value );
      defund_rex_loan( cpu_loans, from, loan_num, amount );
   }

   void system_contract::defnetloan( const name& from, uint64_t loan_num, const asset& amount )
   {
      require_auth( from );

      rex_net_loan_table net_loans( get_self(), get_self().value );
      defund_rex_loan( net_loans, from, loan_num, amount );
   }

   void system_contract::updaterex( const name& owner )
   {
      require_auth( owner );

      runrex(2);

      auto itr = _rexbalance.require_find( owner.value, "account has no REX balance" );
      const asset init_stake = itr->vote_stake;

      auto rexpool_itr = _rexpool.begin();
      const int64_t total_rex      = rexpool_itr->total_rex.amount;
      const int64_t total_lendable = rexpool_itr->total_lendable.amount;
      const int64_t rex_balance    = itr->rex_balance.amount;

      asset current_stake( 0, core_symbol() );
      if ( total_rex > 0 ) {
         current_stake.amount = ( uint128_t(rex_balance) * total_lendable ) / total_rex;
      }
      _rexbalance.modify( itr, same_payer, [&]( auto& rb ) {
         rb.vote_stake = current_stake;
      });

      update_rex_account( owner, asset( 0, core_symbol() ), current_stake - init_stake, true );
      process_rex_maturities( itr );
   }

   void system_contract::setrex( const asset& balance )
   {
      require_auth( "eosio"_n );

      check( balance.amount > 0, "balance must be set to have a positive amount" );
      check( balance.symbol == core_symbol(), "balance symbol must be core symbol" );
      check( rex_system_initialized(), "rex system is not initialized" );
      _rexpool.modify( _rexpool.begin(), same_payer, [&]( auto& pool ) {
         pool.total_rent = balance;
      });
   }

   void system_contract::rexexec( const name& user, uint16_t max )
   {
      require_auth( user );

      runrex( max );
   }

   void system_contract::consolidate( const name& owner )
   {
      require_auth( owner );

      runrex(2);

      auto bitr = _rexbalance.require_find( owner.value, "account has no REX balance" );
      asset rex_in_sell_order = update_rex_account( owner, asset( 0, core_symbol() ), asset( 0, core_symbol() ) );
      consolidate_rex_balance( bitr, rex_in_sell_order );
   }

   void system_contract::mvtosavings( const name& owner, const asset& rex )
   {
      require_auth( owner );

      runrex(2);

      auto bitr = _rexbalance.require_find( owner.value, "account has no REX balance" );
      check( rex.amount > 0 && rex.symbol == bitr->rex_balance.symbol, "asset must be a positive amount of (REX, 4)" );
      const asset   rex_in_sell_order = update_rex_account( owner, asset( 0, core_symbol() ), asset( 0, core_symbol() ) );
      const int64_t rex_in_savings    = read_rex_savings( bitr );
      check( rex.amount + rex_in_sell_order.amount + rex_in_savings <= bitr->rex_balance.amount,
             "insufficient REX balance" );
      process_rex_maturities( bitr );
      _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
         int64_t moved_rex = 0;
         while ( !rb.rex_maturities.empty() && moved_rex < rex.amount) {
            const int64_t d_rex = std::min( rex.amount - moved_rex, rb.rex_maturities.back().second );
            rb.rex_maturities.back().second -= d_rex;
            moved_rex                       += d_rex;
            if ( rb.rex_maturities.back().second == 0 ) {
               rb.rex_maturities.pop_back();
            }
         }
         if ( moved_rex < rex.amount ) {
            const int64_t d_rex = rex.amount - moved_rex;
            rb.matured_rex    -= d_rex;
            moved_rex         += d_rex;
            check( rex_in_sell_order.amount <= rb.matured_rex, "logic error in mvtosavings" );
         }
         check( moved_rex == rex.amount, "programmer error in mvtosavings" );
      });
      put_rex_savings( bitr, rex_in_savings + rex.amount );
   }

   void system_contract::mvfrsavings( const name& owner, const asset& rex )
   {
      require_auth( owner );

      runrex(2);

      auto bitr = _rexbalance.require_find( owner.value, "account has no REX balance" );
      check( rex.amount > 0 && rex.symbol == bitr->rex_balance.symbol, "asset must be a positive amount of (REX, 4)" );
      const int64_t rex_in_savings = read_rex_savings( bitr );
      check( rex.amount <= rex_in_savings, "insufficient REX in savings" );
      process_rex_maturities( bitr );
      _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
         const time_point_sec maturity = get_rex_maturity();
         if ( !rb.rex_maturities.empty() && rb.rex_maturities.back().first == maturity ) {
            rb.rex_maturities.back().second += rex.amount;
         } else {
            rb.rex_maturities.emplace_back( pair_time_point_sec_int64 { maturity, rex.amount } );
         }
      });
      put_rex_savings( bitr, rex_in_savings - rex.amount );
      update_rex_account( owner, asset( 0, core_symbol() ), asset( 0, core_symbol() ) );
   }

   void system_contract::closerex( const name& owner )
   {
      require_auth( owner );

      if ( rex_system_initialized() )
         runrex(2);

      update_rex_account( owner, asset( 0, core_symbol() ), asset( 0, core_symbol() ) );

      /// check for any outstanding loans or rex fund
      {
         rex_cpu_loan_table cpu_loans( get_self(), get_self().value );
         auto cpu_idx = cpu_loans.get_index<"byowner"_n>();
         bool no_outstanding_cpu_loans = ( cpu_idx.find( owner.value ) == cpu_idx.end() );

         rex_net_loan_table net_loans( get_self(), get_self().value );
         auto net_idx = net_loans.get_index<"byowner"_n>();
         bool no_outstanding_net_loans = ( net_idx.find( owner.value ) == net_idx.end() );

         auto fund_itr = _rexfunds.find( owner.value );
         bool no_outstanding_rex_fund = ( fund_itr != _rexfunds.end() ) && ( fund_itr->balance.amount == 0 );

         if ( no_outstanding_cpu_loans && no_outstanding_net_loans && no_outstanding_rex_fund ) {
            _rexfunds.erase( fund_itr );
         }
      }

      /// check for remaining rex balance
      {
         auto rex_itr = _rexbalance.find( owner.value );
         if ( rex_itr != _rexbalance.end() ) {
            check( rex_itr->rex_balance.amount == 0, "account has remaining REX balance, must sell first");
            _rexbalance.erase( rex_itr );
         }
      }
   }

   /**
    * @brief Updates REX pool and deposits token to eosio.rex
    *
    * @param payer - the payer of donated funds.
    * @param quantity - the quantity of tokens to donated to REX with.
    * @param memo - the memo string to accompany the transfer.
    */
   void system_contract::donatetorex( const name& payer, const asset& quantity, const std::string& memo )
   {
      require_auth( payer );
      check( rex_available(), "rex system is not initialized" );
      check( quantity.symbol == core_symbol(), "quantity must be core token" );
      check( quantity.amount > 0, "quantity must be positive" );

      add_to_rex_return_pool( quantity );
      // inline transfer to rex_account
      token::transfer_action transfer_act{ token_account, { payer, active_permission } };
      transfer_act.send( payer, rex_account, quantity, memo );
   }

   /**
    * @brief Updates account NET and CPU resource limits
    *
    * @param from - account charged for RAM if there is a need
    * @param receiver - account whose resource limits are updated
    * @param delta_net - change in NET bandwidth limit
    * @param delta_cpu - change in CPU bandwidth limit
    */
   void system_contract::update_resource_limits( const name& from, const name& receiver, int64_t delta_net, int64_t delta_cpu )
   {
      if ( delta_cpu == 0 && delta_net == 0 ) { // nothing to update
         return;
      }

      user_resources_table totals_tbl( get_self(), receiver.value );
      auto tot_itr = totals_tbl.find( receiver.value );
      if ( tot_itr == totals_tbl.end() ) {
         check( 0 <= delta_net && 0 <= delta_cpu, "logic error, should not occur");
         tot_itr = totals_tbl.emplace( from, [&]( auto& tot ) {
            tot.owner      = receiver;
            tot.net_weight = asset( delta_net, core_symbol() );
            tot.cpu_weight = asset( delta_cpu, core_symbol() );
         });
      } else {
         totals_tbl.modify( tot_itr, same_payer, [&]( auto& tot ) {
            tot.net_weight.amount += delta_net;
            tot.cpu_weight.amount += delta_cpu;
         });
      }
      check( 0 <= tot_itr->net_weight.amount, "insufficient staked total net bandwidth" );
      check( 0 <= tot_itr->cpu_weight.amount, "insufficient staked total cpu bandwidth" );

      {
         bool net_managed = false;
         bool cpu_managed = false;

         auto voter_itr = _voters.find( receiver.value );
         if( voter_itr != _voters.end() ) {
            net_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::net_managed );
            cpu_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::cpu_managed );
         }

         if( !(net_managed && cpu_managed) ) {
            int64_t ram_bytes = 0, net = 0, cpu = 0;
            get_resource_limits( receiver, ram_bytes, net, cpu );

            set_resource_limits( receiver,
                                 ram_bytes,
                                 net_managed ? net : tot_itr->net_weight.amount,
                                 cpu_managed ? cpu : tot_itr->cpu_weight.amount );
         }
      }

      if ( tot_itr->is_empty() ) {
         totals_tbl.erase( tot_itr );
      }
   }

   /**
    * @brief Checks if CPU and Network loans are available
    *
    * Loans are available if 1) REX pool lendable balance is nonempty, and 2) there are no
    * unfilled sellrex orders.
    */
   bool system_contract::rex_loans_available()const
   {
      if ( !rex_available() ) {
         return false;
      } else {
         if ( _rexorders.begin() == _rexorders.end() ) {
            return true; // no outstanding sellrex orders
         } else {
            auto idx = _rexorders.get_index<"bytime"_n>();
            return !idx.begin()->is_open; // no outstanding unfilled sellrex orders
         }
      }
   }

   /**
    * @brief Updates rex_pool balances upon creating a new loan or renewing an existing one
    *
    * @param payment - loan fee paid
    * @param rented_tokens - amount of tokens to be staked to loan receiver
    * @param new_loan - flag indicating whether the loan is new or being renewed
    */
   void system_contract::add_loan_to_rex_pool( const asset& payment, int64_t rented_tokens, bool new_loan )
   {
      channel_to_system_fees( get_self(), payment );
      _rexpool.modify( _rexpool.begin(), same_payer, [&]( auto& rt ) {
         // add payment to total_rent
         rt.total_rent.amount    += payment.amount;
         // move rented_tokens from total_unlent to total_lent
         rt.total_unlent.amount  -= rented_tokens;
         rt.total_lent.amount    += rented_tokens;
         // increment loan_num if a new loan is being created
         if ( new_loan ) {
            rt.loan_num++;
         }
      });
   }

   /**
    * @brief Updates rex_pool balances upon closing an expired loan
    *
    * @param loan - loan to be closed
    */
   void system_contract::remove_loan_from_rex_pool( const rex_loan& loan )
   {
      const auto& pool = _rexpool.begin();
      const int64_t delta_total_rent = exchange_state::get_bancor_output( pool->total_unlent.amount,
                                                                          pool->total_rent.amount,
                                                                          loan.total_staked.amount );
      _rexpool.modify( pool, same_payer, [&]( auto& rt ) {
         // deduct calculated delta_total_rent from total_rent
         rt.total_rent.amount    -= delta_total_rent;
         // move rented tokens from total_lent to total_unlent
         rt.total_unlent.amount  += loan.total_staked.amount;
         rt.total_lent.amount    -= loan.total_staked.amount;
         rt.total_lendable.amount = rt.total_unlent.amount + rt.total_lent.amount;
      });
   }

   /**
    * @brief Updates the fields of an existing loan that is being renewed
    */
   template <typename Index, typename Iterator>
   int64_t system_contract::update_renewed_loan( Index& idx, const Iterator& itr, int64_t rented_tokens )
   {
      int64_t delta_stake = rented_tokens - itr->total_staked.amount;
      idx.modify ( itr, same_payer, [&]( auto& loan ) {
         loan.total_staked.amount = rented_tokens;
         loan.expiration         += eosio::days(30);
         loan.balance.amount     -= loan.payment.amount;
      });
      return delta_stake;
   }

   /**
    * @brief Performs maintenance operations on expired NET and CPU loans and sellrex orders
    *
    * @param max - maximum number of each of the three categories to be processed
    */
   void system_contract::runrex( uint16_t max )
   {
      check( rex_system_initialized(), "rex system not initialized yet" );

      update_rex_pool();

      const auto& pool = _rexpool.begin();

      auto process_expired_loan = [&]( auto& idx, const auto& itr ) -> std::pair<bool, int64_t> {
         /// update rex_pool in order to delete existing loan
         remove_loan_from_rex_pool( *itr );
         bool    delete_loan   = false;
         int64_t delta_stake   = 0;
         /// calculate rented tokens at current price
         int64_t rented_tokens = exchange_state::get_bancor_output( pool->total_rent.amount,
                                                                    pool->total_unlent.amount,
                                                                    itr->payment.amount );
         /// conditions for loan renewal
         bool renew_loan = itr->payment <= itr->balance        /// loan has sufficient balance
                        && itr->payment.amount < rented_tokens /// loan has favorable return
                        && rex_loans_available();              /// no pending sell orders
         if ( renew_loan ) {
            /// update rex_pool in order to account for renewed loan
            add_loan_to_rex_pool( itr->payment, rented_tokens, false );
            /// update renewed loan fields
            delta_stake = update_renewed_loan( idx, itr, rented_tokens );
         } else {
            delete_loan = true;
            delta_stake = -( itr->total_staked.amount );
            /// refund "from" account if the closed loan balance is positive
            if ( itr->balance.amount > 0 ) {
               transfer_to_fund( itr->from, itr->balance );
            }
         }

         return { delete_loan, delta_stake };
      };

      /// process cpu loans
      {
         rex_cpu_loan_table cpu_loans( get_self(), get_self().value );
         auto cpu_idx = cpu_loans.get_index<"byexpr"_n>();
         for ( uint16_t i = 0; i < max; ++i ) {
            auto itr = cpu_idx.begin();
            if ( itr == cpu_idx.end() || itr->expiration > current_time_point() ) break;

            auto result = process_expired_loan( cpu_idx, itr );
            if ( result.second != 0 )
               update_resource_limits( itr->from, itr->receiver, 0, result.second );

            if ( result.first )
               cpu_idx.erase( itr );
         }
      }

      /// process net loans
      {
         rex_net_loan_table net_loans( get_self(), get_self().value );
         auto net_idx = net_loans.get_index<"byexpr"_n>();
         for ( uint16_t i = 0; i < max; ++i ) {
            auto itr = net_idx.begin();
            if ( itr == net_idx.end() || itr->expiration > current_time_point() ) break;

            auto result = process_expired_loan( net_idx, itr );
            if ( result.second != 0 )
               update_resource_limits( itr->from, itr->receiver, result.second, 0 );

            if ( result.first )
               net_idx.erase( itr );
         }
      }

      /// process sellrex orders
      if ( _rexorders.begin() != _rexorders.end() ) {
         auto idx  = _rexorders.get_index<"bytime"_n>();
         auto oitr = idx.begin();
         for ( uint16_t i = 0; i < max; ++i ) {
            if ( oitr == idx.end() || !oitr->is_open ) break;
            auto next = oitr;
            ++next;
            auto bitr = _rexbalance.find( oitr->owner.value );
            if ( bitr != _rexbalance.end() ) { // should always be true
               auto result = fill_rex_order( bitr, oitr->rex_requested );
               if ( result.success ) {
                  const name order_owner = oitr->owner;
                  idx.modify( oitr, same_payer, [&]( auto& order ) {
                     order.proceeds.amount     = result.proceeds.amount;
                     order.stake_change.amount = result.stake_change.amount;
                     order.close();
                  });
                  /// send dummy action to show owner and proceeds of filled sellrex order
                  rex_results::orderresult_action order_act( rex_account, std::vector<eosio::permission_level>{ } );
                  order_act.send( order_owner, result.proceeds );
               }
            }
            oitr = next;
         }
      }

   }

   /**
    * @brief Adds returns from the REX return pool to the REX pool
    */
   void system_contract::update_rex_pool()
   {
      auto get_elapsed_intervals = [&]( const time_point_sec& t1, const time_point_sec& t0 ) -> uint32_t {
         return ( t1.sec_since_epoch() - t0.sec_since_epoch() ) / rex_return_pool::dist_interval;
      };

      const time_point_sec ct             = current_time_point();
      const uint32_t       cts            = ct.sec_since_epoch();
      const time_point_sec effective_time{cts - cts % rex_return_pool::dist_interval};

      const auto ret_pool_elem    = _rexretpool.begin();
      const auto ret_buckets_elem = _rexretbuckets.begin();

      if ( ret_pool_elem == _rexretpool.end() || effective_time <= ret_pool_elem->last_dist_time ) {
         return;
      }

      const int64_t  current_rate      = ret_pool_elem->current_rate_of_increase;
      const uint32_t elapsed_intervals = get_elapsed_intervals( effective_time, ret_pool_elem->last_dist_time );
      int64_t        change_estimate   = current_rate * elapsed_intervals;

      {
         const bool new_return_bucket = ret_pool_elem->pending_bucket_time <= effective_time;
         int64_t        new_bucket_rate = 0;
         time_point_sec new_bucket_time = time_point_sec::min();
         _rexretpool.modify( ret_pool_elem, same_payer, [&]( auto& rp ) {
            if ( new_return_bucket ) {
               int64_t remainder = rp.pending_bucket_proceeds % rex_return_pool::total_intervals;
               new_bucket_rate   = ( rp.pending_bucket_proceeds - remainder ) / rex_return_pool::total_intervals;
               new_bucket_time   = rp.pending_bucket_time;
               rp.current_rate_of_increase += new_bucket_rate;
               change_estimate             += remainder + new_bucket_rate * get_elapsed_intervals( effective_time, rp.pending_bucket_time );
               rp.pending_bucket_proceeds   = 0;
               rp.pending_bucket_time       = time_point_sec::maximum();
               if ( new_bucket_time < rp.oldest_bucket_time ) {
                  rp.oldest_bucket_time = new_bucket_time;
               }
            }
            rp.proceeds      -= change_estimate;
            rp.last_dist_time = effective_time;
         });

         if ( new_return_bucket ) {
            _rexretbuckets.modify( ret_buckets_elem, same_payer, [&]( auto& rb ) {
               auto iter = std::lower_bound(rb.return_buckets.begin(), rb.return_buckets.end(), new_bucket_time, [](const pair_time_point_sec_int64& bucket, time_point_sec first) {
                  return bucket.first < first;
               });
               if ((iter != rb.return_buckets.end()) && (iter->first == new_bucket_time)) {
                  iter->second = new_bucket_rate;
               } else {
                  rb.return_buckets.insert(iter, pair_time_point_sec_int64{new_bucket_time, new_bucket_rate});
               }
            });
         }
      }

      const time_point_sec time_threshold = effective_time - seconds(rex_return_pool::total_intervals * rex_return_pool::dist_interval);
      if ( ret_pool_elem->oldest_bucket_time <= time_threshold ) {
         int64_t expired_rate = 0;
         int64_t surplus      = 0;
         _rexretbuckets.modify( ret_buckets_elem, same_payer, [&]( auto& rb ) {
            auto& return_buckets = rb.return_buckets;
            auto iter = return_buckets.begin();
            for (; iter != return_buckets.end() && iter->first <= time_threshold; ++iter) {
               const uint32_t overtime = get_elapsed_intervals( effective_time,
                                                                iter->first + seconds(rex_return_pool::total_intervals * rex_return_pool::dist_interval) );
               surplus      += iter->second * overtime;
               expired_rate += iter->second;
            }
            return_buckets.erase(return_buckets.begin(), iter);
         });

         _rexretpool.modify( ret_pool_elem, same_payer, [&]( auto& rp ) {
            if ( !ret_buckets_elem->return_buckets.empty() ) {
               rp.oldest_bucket_time = ret_buckets_elem->return_buckets.begin()->first;
            } else {
               rp.oldest_bucket_time = time_point_sec::min();
            }
            if ( expired_rate > 0) {
               rp.current_rate_of_increase -= expired_rate;
            }
            if ( surplus > 0 ) {
               change_estimate -= surplus;
               rp.proceeds     += surplus;
            }
         });
      }

      if ( change_estimate > 0 && ret_pool_elem->proceeds < 0 ) {
         _rexretpool.modify( ret_pool_elem, same_payer, [&]( auto& rp ) {
            change_estimate += rp.proceeds;
            rp.proceeds      = 0;
         });
      }

      if ( change_estimate > 0 ) {
         _rexpool.modify( _rexpool.begin(), same_payer, [&]( auto& pool ) {
            pool.total_unlent.amount += change_estimate;
            pool.total_lendable       = pool.total_unlent + pool.total_lent;
         });
      }
   }

   template <typename T>
   int64_t system_contract::rent_rex( T& table, const name& from, const name& receiver, const asset& payment, const asset& fund )
   {
      runrex(2);

      check( rex_loans_available(), "rex loans are currently not available" );
      check( payment.symbol == core_symbol() && fund.symbol == core_symbol(), "must use core token" );
      check( 0 < payment.amount && 0 <= fund.amount, "must use positive asset amount" );

      transfer_from_fund( from, payment + fund );

      const auto& pool = _rexpool.begin(); /// already checked that _rexpool.begin() != _rexpool.end() in rex_loans_available()

      int64_t rented_tokens = exchange_state::get_bancor_output( pool->total_rent.amount,
                                                                 pool->total_unlent.amount,
                                                                 payment.amount );
      check( payment.amount < rented_tokens, "loan price does not favor renting" );
      add_loan_to_rex_pool( payment, rented_tokens, true );

      table.emplace( from, [&]( auto& c ) {
         c.from         = from;
         c.receiver     = receiver;
         c.payment      = payment;
         c.balance      = fund;
         c.total_staked = asset( rented_tokens, core_symbol() );
         c.expiration   = current_time_point() + eosio::days(30);
         c.loan_num     = pool->loan_num;
      });

      rex_results::rentresult_action rentresult_act{ rex_account, std::vector<eosio::permission_level>{ } };
      rentresult_act.send( asset{ rented_tokens, core_symbol() } );

      // logging
      system_contract::logsystemfee_action logsystemfee_act{ get_self(), { {get_self(), active_permission} } };
      logsystemfee_act.send( rex_account, payment, "rent rex" );
      return rented_tokens;
   }

   /**
    * @brief Processes a sellrex order and returns object containing the results
    *
    * Processes an incoming or already scheduled sellrex order. If REX pool has enough core
    * tokens not frozen in loans, order is filled. In this case, REX pool totals, user rex_balance
    * and user vote_stake are updated. However, this function does not update user voting power. The
    * function returns success flag, order proceeds, and vote stake delta. These are used later in a
    * different function to complete order processing, i.e. transfer proceeds to user REX fund and
    * update user vote weight.
    *
    * @param bitr - iterator pointing to rex_balance database record
    * @param rex - amount of rex to be sold
    *
    * @return rex_order_outcome - a struct containing success flag, order proceeds, and resultant
    * vote stake change
    */
   rex_order_outcome system_contract::fill_rex_order( const rex_balance_table::const_iterator& bitr, const asset& rex )
   {
      auto rexpool_itr = _rexpool.begin();
      const int64_t S0 = rexpool_itr->total_lendable.amount;
      const int64_t R0 = rexpool_itr->total_rex.amount;
      const int64_t p  = (uint128_t(rex.amount) * S0) / R0;
      const int64_t R1 = R0 - rex.amount;
      const int64_t S1 = S0 - p;
      asset proceeds( p, core_symbol() );
      asset stake_change( 0, core_symbol() );
      bool  success = false;

      const int64_t unlent_lower_bound = rexpool_itr->total_lent.amount / 10;
      const int64_t available_unlent   = rexpool_itr->total_unlent.amount - unlent_lower_bound; // available_unlent <= 0 is possible
      if ( proceeds.amount <= available_unlent ) {
         const int64_t init_vote_stake_amount = bitr->vote_stake.amount;
         const int64_t current_stake_value    = ( uint128_t(bitr->rex_balance.amount) * S0 ) / R0;
         _rexpool.modify( rexpool_itr, same_payer, [&]( auto& rt ) {
            rt.total_rex.amount      = R1;
            rt.total_lendable.amount = S1;
            rt.total_unlent.amount   = rt.total_lendable.amount - rt.total_lent.amount;
         });
         _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
            rb.vote_stake.amount   = current_stake_value - proceeds.amount;
            rb.rex_balance.amount -= rex.amount;
            rb.matured_rex        -= rex.amount;
         });
         stake_change.amount = bitr->vote_stake.amount - init_vote_stake_amount;
         success = true;
      } else {
         proceeds.amount = 0;
      }

      return { success, proceeds, stake_change };
   }

   template <typename T>
   void system_contract::fund_rex_loan( T& table, const name& from, uint64_t loan_num, const asset& payment  )
   {
      check( payment.symbol == core_symbol(), "must use core token" );
      transfer_from_fund( from, payment );
      auto itr = table.require_find( loan_num, "loan not found" );
      check( itr->from == from, "user must be loan creator" );
      check( itr->expiration > current_time_point(), "loan has already expired" );
      table.modify( itr, same_payer, [&]( auto& loan ) {
         loan.balance.amount += payment.amount;
      });
   }

   template <typename T>
   void system_contract::defund_rex_loan( T& table, const name& from, uint64_t loan_num, const asset& amount  )
   {
      check( amount.symbol == core_symbol(), "must use core token" );
      auto itr = table.require_find( loan_num, "loan not found" );
      check( itr->from == from, "user must be loan creator" );
      check( itr->expiration > current_time_point(), "loan has already expired" );
      check( itr->balance >= amount, "insufficent loan balance" );
      table.modify( itr, same_payer, [&]( auto& loan ) {
         loan.balance.amount -= amount.amount;
      });
      transfer_to_fund( from, amount );
   }

   /**
    * @brief Transfers tokens from owner REX fund
    *
    * @pre - owner REX fund has sufficient balance
    *
    * @param owner - owner account name
    * @param amount - tokens to be transferred out of REX fund
    */
   void system_contract::transfer_from_fund( const name& owner, const asset& amount )
   {
      check( 0 < amount.amount && amount.symbol == core_symbol(), "must transfer positive amount from REX fund" );
      auto itr = _rexfunds.require_find( owner.value, "must deposit to REX fund first" );
      check( amount <= itr->balance, "insufficient funds" );
      _rexfunds.modify( itr, same_payer, [&]( auto& fund ) {
         fund.balance.amount -= amount.amount;
      });
   }

   /**
    * @brief Transfers tokens to owner REX fund
    *
    * @param owner - owner account name
    * @param amount - tokens to be transferred to REX fund
    */
   void system_contract::transfer_to_fund( const name& owner, const asset& amount )
   {
      check( 0 < amount.amount && amount.symbol == core_symbol(), "must transfer positive amount to REX fund" );
      auto itr = _rexfunds.find( owner.value );
      if ( itr == _rexfunds.end() ) {
         _rexfunds.emplace( owner, [&]( auto& fund ) {
            fund.owner   = owner;
            fund.balance = amount;
         });
      } else {
         _rexfunds.modify( itr, same_payer, [&]( auto& fund ) {
            fund.balance.amount += amount.amount;
         });
      }
   }

   /**
    * @brief Processes owner filled sellrex order and updates vote weight
    *
    * Checks if user has a scheduled sellrex order that has been filled, completes its processing,
    * and deletes it. Processing entails transferring proceeds to user REX fund and updating user
    * vote weight. Additional proceeds and stake change can be passed as arguments. This function
    * is called only by actions pushed by owner.
    *
    * @param owner - owner account name
    * @param proceeds - additional proceeds to be transferred to owner REX fund
    * @param delta_stake - additional stake to be added to owner vote weight
    * @param force_vote_update - if true, vote weight is updated even if vote stake didn't change
    *
    * @return asset - REX amount of owner unfilled sell order if one exists
    */
   asset system_contract::update_rex_account( const name& owner, const asset& proceeds, const asset& delta_stake, bool force_vote_update )
   {
      asset to_fund( proceeds );
      asset to_stake( delta_stake );
      asset rex_in_sell_order( 0, rex_symbol );
      auto itr = _rexorders.find( owner.value );
      if ( itr != _rexorders.end() ) {
         if ( itr->is_open ) {
            rex_in_sell_order.amount = itr->rex_requested.amount;
         } else {
            to_fund.amount  += itr->proceeds.amount;
            to_stake.amount += itr->stake_change.amount;
            _rexorders.erase( itr );
         }
      }

      if ( to_fund.amount > 0 )
         transfer_to_fund( owner, to_fund );
      if ( force_vote_update || to_stake.amount != 0 )
         update_voting_power( owner, to_stake );

      return rex_in_sell_order;
   }

   /**
    * @brief Calculates maturity time of purchased REX tokens which is {num_of_maturity_buckets} days from end
    * of the day UTC
    *
    * @return time_point_sec
    */
   time_point_sec system_contract::get_rex_maturity( const name& system_account_name )
   {
      rex_maturity_singleton _rexmaturity(system_account_name, system_account_name.value);
      const uint32_t num_of_maturity_buckets = _rexmaturity.get_or_default().num_of_maturity_buckets; // default 5
      static const uint32_t now = current_time_point().sec_since_epoch();
      static const uint32_t r   = now % seconds_per_day;
      static const time_point_sec rms{ now - r + num_of_maturity_buckets * seconds_per_day };
      return rms;
   }

   /**
    * @brief Updates REX owner maturity buckets
    *
    * @param bitr - iterator pointing to rex_balance object
    */
   void system_contract::process_rex_maturities( const rex_balance_table::const_iterator& bitr )
   {
      const time_point_sec now = current_time_point();
      _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
         while ( !rb.rex_maturities.empty() && rb.rex_maturities.front().first <= now ) {
            rb.matured_rex += rb.rex_maturities.front().second;
            rb.rex_maturities.erase(rb.rex_maturities.begin());
         }
      });
   }

   /**
    * @brief Sells matured REX tokens
    *        https://github.com/eosnetworkfoundation/eos-system-contracts/issues/134
    *
    * @param owner - owner account name
    */
   void system_contract::process_sell_matured_rex( const name owner )
   {
      const auto rex_maturity_state = _rexmaturity.get_or_default();
      if ( rex_maturity_state.sell_matured_rex == false ) return; // skip selling matured REX

      const auto itr = _rexbalance.find( owner.value );
      if ( itr->matured_rex > 0 ) {
         sell_rex(owner, asset(itr->matured_rex, rex_symbol));
      }
   }

   /**
    * @brief Move new REX tokens to savings
    *        https://github.com/eosnetworkfoundation/eos-system-contracts/issues/135
    *
    * @param owner - owner account name
    * @param rex - amount of REX tokens to be moved to savings
    */
   void system_contract::process_buy_rex_to_savings( const name owner, const asset rex )
   {
      const auto rex_maturity_state = _rexmaturity.get_or_default();
      if ( rex_maturity_state.buy_rex_to_savings && rex.amount > 0 ) {
         mvtosavings( owner, rex );
      }
   }

   /**
    * @brief Consolidates REX maturity buckets into one
    *
    * @param bitr - iterator pointing to rex_balance object
    * @param rex_in_sell_order - REX tokens in owner unfilled sell order, if one exists
    */
   void system_contract::consolidate_rex_balance( const rex_balance_table::const_iterator& bitr,
                                                  const asset& rex_in_sell_order )
   {
      const int64_t rex_in_savings = read_rex_savings( bitr );
      _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
         int64_t total  = rb.matured_rex - rex_in_sell_order.amount;
         rb.matured_rex = rex_in_sell_order.amount;
         while ( !rb.rex_maturities.empty() ) {
            total += rb.rex_maturities.front().second;
            rb.rex_maturities.erase(rb.rex_maturities.begin());
         }
         if ( total > 0 ) {
            rb.rex_maturities.emplace_back( pair_time_point_sec_int64{ get_rex_maturity(), total } );
         }
      });
      put_rex_savings( bitr, rex_in_savings );
   }

   /**
    * @brief Updates REX pool balances upon REX purchase
    *
    * @param payment - amount of core tokens paid
    *
    * @return asset - calculated amount of REX tokens purchased
    */
   asset system_contract::add_to_rex_pool( const asset& payment )
   {
      /**
       * If CORE_SYMBOL is (EOS,4), maximum supply is 10^10 tokens (10 billion tokens), i.e., maximum amount
       * of indivisible units is 10^14. rex_ratio = 10^4 sets the upper bound on (REX,4) indivisible units to
       * 10^18 and that is within the maximum allowable amount field of asset type which is set to 2^62
       * (approximately 4.6 * 10^18). For a different CORE_SYMBOL, and in order for maximum (REX,4) amount not
       * to exceed that limit, maximum amount of indivisible units cannot be set to a value larger than 4 * 10^14.
       * If precision of CORE_SYMBOL is 4, that corresponds to a maximum supply of 40 billion tokens.
       */
      const int64_t rex_ratio = 10000;
      const asset   init_total_rent( 20'000'0000, core_symbol() ); /// base balance prevents renting profitably until at least a minimum number of core_symbol() is made available
      asset rex_received( 0, rex_symbol );
      auto itr = _rexpool.begin();
      if ( !rex_system_initialized() ) {
         /// initialize REX pool
         _rexpool.emplace( get_self(), [&]( auto& rp ) {
            rex_received.amount = payment.amount * rex_ratio;
            rp.total_lendable   = payment;
            rp.total_lent       = asset( 0, core_symbol() );
            rp.total_unlent     = rp.total_lendable - rp.total_lent;
            rp.total_rent       = init_total_rent;
            rp.total_rex        = rex_received;
            rp.namebid_proceeds = asset( 0, core_symbol() );
         });
      } else if ( !rex_available() ) { /// should be a rare corner case, REX pool is initialized but empty
         _rexpool.modify( itr, same_payer, [&]( auto& rp ) {
            rex_received.amount      = payment.amount * rex_ratio;
            rp.total_lendable.amount = payment.amount;
            rp.total_lent.amount     = 0;
            rp.total_unlent.amount   = rp.total_lendable.amount - rp.total_lent.amount;
            rp.total_rent.amount     = init_total_rent.amount;
            rp.total_rex.amount      = rex_received.amount;
         });
      } else {
         /// total_lendable > 0 if total_rex > 0 except in a rare case and due to rounding errors
         check( itr->total_lendable.amount > 0, "lendable REX pool is empty" );
         const int64_t S0 = itr->total_lendable.amount;
         const int64_t S1 = S0 + payment.amount;
         const int64_t R0 = itr->total_rex.amount;
         const int64_t R1 = (uint128_t(S1) * R0) / S0;
         rex_received.amount = R1 - R0;
         _rexpool.modify( itr, same_payer, [&]( auto& rp ) {
            rp.total_lendable.amount = S1;
            rp.total_rex.amount      = R1;
            rp.total_unlent.amount   = rp.total_lendable.amount - rp.total_lent.amount;
            check( rp.total_unlent.amount >= 0, "programmer error, this should never go negative" );
         });
      }

      return rex_received;
   }

   /**
    * @brief Adds an amount of core tokens to the REX return pool
    *
    * @param fee - amount to be added
    */
   void system_contract::add_to_rex_return_pool( const asset& fee )
   {
      update_rex_pool();
      if ( fee.amount <= 0 ) {
         return;
      }

      const time_point_sec ct              = current_time_point();
      const uint32_t       cts             = ct.sec_since_epoch();
      const uint32_t       bucket_interval = rex_return_pool::hours_per_bucket * seconds_per_hour;
      const time_point_sec effective_time{cts - cts % bucket_interval + bucket_interval};
      const auto return_pool_elem = _rexretpool.begin();
      if ( return_pool_elem == _rexretpool.end() ) {
         _rexretpool.emplace( get_self(), [&]( auto& rp ) {
            rp.last_dist_time          = effective_time;
            rp.pending_bucket_proceeds = fee.amount;
            rp.pending_bucket_time     = effective_time;
            rp.proceeds                = fee.amount;
         });
         _rexretbuckets.emplace( get_self(), [&]( auto& rb ) { } );
      } else {
         _rexretpool.modify( return_pool_elem, same_payer, [&]( auto& rp ) {
            rp.pending_bucket_proceeds += fee.amount;
            rp.proceeds                += fee.amount;
            if ( rp.pending_bucket_time == time_point_sec::maximum() ) {
               rp.pending_bucket_time = effective_time;
            }
         });
      }
   }

   /**
    * @brief Updates owner REX balance upon buying REX tokens
    *
    * @param owner - account name of REX owner
    * @param payment - amount core tokens paid to buy REX
    * @param rex_received - amount of purchased REX tokens
    *
    * @return asset - change in owner REX vote stake
    */
   asset system_contract::add_to_rex_balance( const name& owner, const asset& payment, const asset& rex_received )
   {
      asset init_rex_stake( 0, core_symbol() );
      asset current_rex_stake( 0, core_symbol() );
      auto bitr = _rexbalance.find( owner.value );
      if ( bitr == _rexbalance.end() ) {
         bitr = _rexbalance.emplace( owner, [&]( auto& rb ) {
            rb.owner       = owner;
            rb.vote_stake  = payment;
            rb.rex_balance = rex_received;
         });
         current_rex_stake.amount = payment.amount;
      } else {
         init_rex_stake.amount = bitr->vote_stake.amount;
         _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
            rb.rex_balance.amount += rex_received.amount;
            rb.vote_stake.amount   = ( uint128_t(rb.rex_balance.amount) * _rexpool.begin()->total_lendable.amount )
                                     / _rexpool.begin()->total_rex.amount;
         });
         current_rex_stake.amount = bitr->vote_stake.amount;
      }

      const int64_t rex_in_savings = read_rex_savings( bitr );
      process_rex_maturities( bitr );
      _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
         const time_point_sec maturity = get_rex_maturity();
         if ( !rb.rex_maturities.empty() && rb.rex_maturities.back().first == maturity ) {
            rb.rex_maturities.back().second += rex_received.amount;
         } else {
            rb.rex_maturities.emplace_back( pair_time_point_sec_int64 { maturity, rex_received.amount } );
         }
      });
      put_rex_savings( bitr, rex_in_savings );
      return current_rex_stake - init_rex_stake;
   }

   /**
    * @brief Reads amount of REX in savings bucket and removes the bucket from maturities
    *
    * Reads and (temporarily) removes REX savings bucket from REX maturities in order to
    * allow uniform processing of remaining buckets as savings is a special case. This
    * function is used in conjunction with put_rex_savings.
    *
    * @param bitr - iterator pointing to rex_balance object
    *
    * @return int64_t - amount of REX in savings bucket
    */
   int64_t system_contract::read_rex_savings( const rex_balance_table::const_iterator& bitr )
   {
      int64_t rex_in_savings = 0;
      static const time_point_sec end_of_days = time_point_sec::maximum();
      if ( !bitr->rex_maturities.empty() && bitr->rex_maturities.back().first == end_of_days ) {
         _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
            rex_in_savings = rb.rex_maturities.back().second;
            rb.rex_maturities.pop_back();
         });
      }
      return rex_in_savings;
   }

   /**
    * @brief Adds a specified REX amount to savings bucket
    *
    * @param bitr - iterator pointing to rex_balance object
    * @param rex - amount of REX to be added
    */
   void system_contract::put_rex_savings( const rex_balance_table::const_iterator& bitr, int64_t rex )
   {
      if ( rex == 0 ) return;
      static const time_point_sec end_of_days = time_point_sec::maximum();
      _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
         if ( !rb.rex_maturities.empty() && rb.rex_maturities.back().first == end_of_days ) {
            rb.rex_maturities.back().second += rex;
         } else {
            rb.rex_maturities.emplace_back( pair_time_point_sec_int64{ end_of_days, rex } );
         }
      });
   }

   /**
    * @brief Updates voter REX vote stake to the current value of REX tokens held
    *
    * @param voter - account name of voter
    */
   void system_contract::update_rex_stake( const name& voter )
   {
      int64_t delta_stake = 0;
      auto bitr = _rexbalance.find( voter.value );
      if ( bitr != _rexbalance.end() && rex_available() ) {
         asset init_vote_stake = bitr->vote_stake;
         asset current_vote_stake( 0, core_symbol() );
         current_vote_stake.amount = ( uint128_t(bitr->rex_balance.amount) * _rexpool.begin()->total_lendable.amount )
                                     / _rexpool.begin()->total_rex.amount;
         _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
            rb.vote_stake.amount = current_vote_stake.amount;
         });
         delta_stake = current_vote_stake.amount - init_vote_stake.amount;
      }

      if ( delta_stake != 0 ) {
         auto vitr = _voters.find( voter.value );
         if ( vitr != _voters.end() ) {
            _voters.modify( vitr, same_payer, [&]( auto& vinfo ) {
               vinfo.staked += delta_stake;
            });
         }
      }
   }

}; /// namespace eosiosystem
