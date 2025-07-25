#include <boost/test/unit_test.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fc/log/logger.hpp>
#include <eosio/chain/exceptions.hpp>

#include "eosio.system_tester.hpp"
struct _abi_hash {
   name owner;
   fc::sha256 hash;
};
FC_REFLECT( _abi_hash, (owner)(hash) );

struct connector {
   asset balance;
   double weight = .5;
};
FC_REFLECT( connector, (balance)(weight) );

using namespace eosio_system;

bool within_error(int64_t a, int64_t b, int64_t err) { return std::abs(a - b) <= err; };
bool within_one(int64_t a, int64_t b) { return within_error(a, b, 1); }

// Split the tests into multiple suites so that they can run in parallel in CICD to
// reduce overall CICD time..
// Each suite is grouped by functionality and takes approximately the same amount of time.

BOOST_AUTO_TEST_SUITE(eosio_system_stake_tests)

BOOST_FIXTURE_TEST_CASE( buysell, eosio_system_tester ) try {

   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( "alice1111111" ) );

   transfer( "eosio", "alice1111111", core_sym::from_string("1000.0000"), "eosio" );
   BOOST_REQUIRE_EQUAL( success(), stake( "eosio", "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );

   auto total = get_total_stake( "alice1111111" );
   auto init_bytes =  total["ram_bytes"].as_uint64();

   const asset initial_ram_balance = get_balance("eosio.ram"_n);
   const asset initial_fees_balance = get_balance("eosio.fees"_n);
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("200.0000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("800.0000"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( initial_ram_balance + core_sym::from_string("199.0000"), get_balance("eosio.ram"_n) );
   BOOST_REQUIRE_EQUAL( initial_fees_balance + core_sym::from_string("1.0000"), get_balance("eosio.fees"_n) );

   total = get_total_stake( "alice1111111" );
   auto bytes = total["ram_bytes"].as_uint64();
   auto bought_bytes = bytes - init_bytes;
   wdump((init_bytes)(bought_bytes)(bytes) );

   BOOST_REQUIRE_EQUAL( true, 0 < bought_bytes );

   BOOST_REQUIRE_EQUAL( success(), sellram( "alice1111111", bought_bytes ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("998.0049"), get_balance( "alice1111111" ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( true, total["ram_bytes"].as_uint64() == init_bytes );

   transfer( "eosio", "alice1111111", core_sym::from_string("100000000.0000"), "eosio" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("100000998.0049"), get_balance( "alice1111111" ) );
   // alice buys ram for 10000000.0000, 0.5% = 50000.0000 go to ramfee
   // after fee 9950000.0000 go to bought bytes
   // when selling back bought bytes, pay 0.5% fee and get back 99.5% of 9950000.0000 = 9900250.0000
   // expected account after that is 90000998.0049 + 9900250.0000 = 99901248.0049 with a difference
   // of order 0.0001 due to rounding errors
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("10000000.0000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("90000998.0049"), get_balance( "alice1111111" ) );

   total = get_total_stake( "alice1111111" );
   bytes = total["ram_bytes"].as_uint64();
   bought_bytes = bytes - init_bytes;
   wdump((init_bytes)(bought_bytes)(bytes) );

   BOOST_REQUIRE_EQUAL( success(), sellram( "alice1111111", bought_bytes ) );
   total = get_total_stake( "alice1111111" );

   bytes = total["ram_bytes"].as_uint64();
   bought_bytes = bytes - init_bytes;
   wdump((init_bytes)(bought_bytes)(bytes) );

   BOOST_REQUIRE_EQUAL( true, total["ram_bytes"].as_uint64() == init_bytes );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("99901248.0048"), get_balance( "alice1111111" ) );

   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("100.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("100.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("100.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("100.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("100.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("10.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("10.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("10.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("30.0000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("99900688.0048"), get_balance( "alice1111111" ) );

   auto newtotal = get_total_stake( "alice1111111" );

   auto newbytes = newtotal["ram_bytes"].as_uint64();
   bought_bytes = newbytes - bytes;
   wdump((newbytes)(bytes)(bought_bytes) );

   BOOST_REQUIRE_EQUAL( success(), sellram( "alice1111111", bought_bytes ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("99901242.4187"), get_balance( "alice1111111" ) );

   newtotal = get_total_stake( "alice1111111" );
   auto startbytes = newtotal["ram_bytes"].as_uint64();

   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("10000000.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("10000000.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("10000000.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("10000000.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("10000000.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("100000.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("100000.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("100000.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("300000.0000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("49301242.4187"), get_balance( "alice1111111" ) );

   auto finaltotal = get_total_stake( "alice1111111" );
   auto endbytes = finaltotal["ram_bytes"].as_uint64();

   bought_bytes = endbytes - startbytes;
   wdump((startbytes)(endbytes)(bought_bytes) );

   BOOST_REQUIRE_EQUAL( success(), sellram( "alice1111111", bought_bytes ) );

   BOOST_REQUIRE_EQUAL( false, get_row_by_account( config::system_account_name, config::system_account_name,
                                                   "rammarket"_n, account_name(symbol{SY(4,RAMCORE)}.value()) ).empty() );

   auto get_ram_market = [this]() -> fc::variant {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name,
                                              "rammarket"_n, account_name(symbol{SY(4,RAMCORE)}.value()) );
      BOOST_REQUIRE( !data.empty() );
      return abi_ser.binary_to_variant("exchange_state", data, abi_serializer::create_yield_function(abi_serializer_max_time));
   };

   {
      transfer( config::system_account_name, "alice1111111"_n, core_sym::from_string("10000000.0000"), config::system_account_name );
      uint64_t bytes0 = get_total_stake( "alice1111111" )["ram_bytes"].as_uint64();

      auto market = get_ram_market();
      const asset r0 = market["base"].as<connector>().balance;
      const asset e0 = market["quote"].as<connector>().balance;
      BOOST_REQUIRE_EQUAL( asset::from_string("0 RAM").get_symbol(),     r0.get_symbol() );
      BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000").get_symbol(), e0.get_symbol() );

      const asset payment = core_sym::from_string("10000000.0000");
      BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", payment ) );
      uint64_t bytes1 = get_total_stake( "alice1111111" )["ram_bytes"].as_uint64();

      const int64_t fee = (payment.get_amount() + 199) / 200;
      const double net_payment = payment.get_amount() - fee;
      const int64_t expected_delta = net_payment * r0.get_amount() / ( net_payment + e0.get_amount() );

      BOOST_REQUIRE_EQUAL( expected_delta, bytes1 -  bytes0 );
   }

   {
      transfer( config::system_account_name, "bob111111111"_n, core_sym::from_string("100000.0000"), config::system_account_name );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("must reserve a positive amount"),
                           buyrambytes( "bob111111111", "bob111111111", 1 ) );

      uint64_t bytes0 = get_total_stake( "bob111111111" )["ram_bytes"].as_uint64();
      BOOST_REQUIRE_EQUAL( success(), buyrambytes( "bob111111111", "bob111111111", 1024 ) );
      uint64_t bytes1 = get_total_stake( "bob111111111" )["ram_bytes"].as_uint64();
      BOOST_REQUIRE( within_one( 1024, bytes1 - bytes0 ) );

      BOOST_REQUIRE_EQUAL( success(), buyrambytes( "bob111111111", "bob111111111", 1024 * 1024) );
      uint64_t bytes2 = get_total_stake( "bob111111111" )["ram_bytes"].as_uint64();
      BOOST_REQUIRE( within_one( 1024 * 1024, bytes2 - bytes1 ) );
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( stake_unstake, eosio_system_tester ) try {
   cross_15_percent_threshold();

   produce_blocks( 10 );
   produce_block( fc::hours(3*24) );

   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( "alice1111111" ) );
   transfer( "eosio", "alice1111111", core_sym::from_string("1000.0000"), "eosio" );

   BOOST_REQUIRE_EQUAL( core_sym::from_string("1000.0000"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( success(), stake( "eosio", "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );

   auto total = get_total_stake("alice1111111");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());

   const auto init_eosio_stake_balance = get_balance( "eosio.stake"_n );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( init_eosio_stake_balance + core_sym::from_string("300.0000"), get_balance( "eosio.stake"_n ) );
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );

   produce_block( fc::hours(3*24-1) );
   produce_blocks(1);
   // testing balance still the same
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( init_eosio_stake_balance + core_sym::from_string("300.0000"), get_balance( "eosio.stake"_n ) );
   // call refund expected to fail too early
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("refund is not available yet"),
                       push_action( "alice1111111"_n, "refund"_n, mvo()("owner", "alice1111111") ) );

   // after 1 hour refund ready
   produce_block( fc::hours(1) );
   produce_blocks(1);
   // now we can do the refund
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "refund"_n, mvo()("owner", "alice1111111") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1000.0000"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( init_eosio_stake_balance, get_balance( "eosio.stake"_n ) );

   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "bob111111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );
   total = get_total_stake("bob111111111");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());

   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000").get_amount(), total["net_weight"].as<asset>().get_amount() );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000").get_amount(), total["cpu_weight"].as<asset>().get_amount() );

   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("300.0000")), get_voter_info( "alice1111111" ) );

   auto bytes = total["ram_bytes"].as_uint64();
   BOOST_REQUIRE_EQUAL( true, 0 < bytes );

   //unstake from bob111111111
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "bob111111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );
   total = get_total_stake("bob111111111");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("10.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("10.0000"), total["cpu_weight"].as<asset>());
   produce_block( fc::hours(3*24-1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );
   //after 3 days funds should be released
   produce_block( fc::hours(1) );
   produce_blocks(1);

   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("0.0000") ), get_voter_info( "alice1111111" ) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "refund"_n, mvo()("owner", "alice1111111") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1000.0000"), get_balance( "alice1111111" ) );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( stake_unstake_with_transfer, eosio_system_tester ) try {
   cross_15_percent_threshold();

   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( "alice1111111" ) );

   //eosio stakes for alice with transfer flag

   transfer( "eosio", "bob111111111", core_sym::from_string("1000.0000"), "eosio" );
   BOOST_REQUIRE_EQUAL( success(), stake_with_transfer( "bob111111111"_n, "alice1111111"_n, core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );

   //check that alice has both bandwidth and voting power
   auto total = get_total_stake("alice1111111");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("300.0000")), get_voter_info( "alice1111111" ) );

   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( "alice1111111" ) );

   //alice stakes for herself
   transfer( "eosio", "alice1111111", core_sym::from_string("1000.0000"), "eosio" );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );
   //now alice's stake should be equal to transferred from eosio + own stake
   total = get_total_stake("alice1111111");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("410.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("600.0000")), get_voter_info( "alice1111111" ) );

   //alice can unstake everything (including what was transferred)
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "alice1111111", core_sym::from_string("400.0000"), core_sym::from_string("200.0000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );

   produce_block( fc::hours(3*24-1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );
   //after 3 days funds should be released

   produce_block( fc::hours(1) );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "refund"_n, mvo()("owner", "alice1111111") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1300.0000"), get_balance( "alice1111111" ) );

   //stake should be equal to what was staked in constructor, voting power should be 0
   total = get_total_stake("alice1111111");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("10.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("10.0000"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("0.0000")), get_voter_info( "alice1111111" ) );

   // Now alice stakes to bob with transfer flag
   BOOST_REQUIRE_EQUAL( success(), stake_with_transfer( "alice1111111"_n, "bob111111111"_n, core_sym::from_string("100.0000"), core_sym::from_string("100.0000") ) );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( stake_to_self_with_transfer, eosio_system_tester ) try {
   cross_15_percent_threshold();

   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( "alice1111111" ) );
   transfer( "eosio", "alice1111111", core_sym::from_string("1000.0000"), "eosio" );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg("cannot use transfer flag if delegating to self"),
                        stake_with_transfer( "alice1111111"_n, "alice1111111"_n, core_sym::from_string("200.0000"), core_sym::from_string("100.0000") )
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( stake_while_pending_refund, eosio_system_tester ) try {
   cross_15_percent_threshold();

   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( "alice1111111" ) );

   //eosio stakes for alice with transfer flag
   transfer( "eosio", "bob111111111", core_sym::from_string("1000.0000"), "eosio" );
   BOOST_REQUIRE_EQUAL( success(), stake_with_transfer( "bob111111111"_n, "alice1111111"_n, core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );

   //check that alice has both bandwidth and voting power
   auto total = get_total_stake("alice1111111");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("300.0000")), get_voter_info( "alice1111111" ) );

   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( "alice1111111" ) );

   //alice stakes for herself
   transfer( "eosio", "alice1111111", core_sym::from_string("1000.0000"), "eosio" );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );
   //now alice's stake should be equal to transferred from eosio + own stake
   total = get_total_stake("alice1111111");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("410.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("600.0000")), get_voter_info( "alice1111111" ) );

   //alice can unstake everything (including what was transferred)
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "alice1111111", core_sym::from_string("400.0000"), core_sym::from_string("200.0000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );

   produce_block( fc::hours(3*24-1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );
   //after 3 days funds should be released

   produce_block( fc::hours(1) );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "refund"_n, mvo()("owner", "alice1111111") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1300.0000"), get_balance( "alice1111111" ) );

   //stake should be equal to what was staked in constructor, voting power should be 0
   total = get_total_stake("alice1111111");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("10.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("10.0000"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("0.0000")), get_voter_info( "alice1111111" ) );

   // Now alice stakes to bob with transfer flag
   BOOST_REQUIRE_EQUAL( success(), stake_with_transfer( "alice1111111"_n, "bob111111111"_n, core_sym::from_string("100.0000"), core_sym::from_string("100.0000") ) );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( fail_without_auth, eosio_system_tester ) try {
   cross_15_percent_threshold();

   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );

   BOOST_REQUIRE_EQUAL( success(), stake( "eosio", "alice1111111", core_sym::from_string("2000.0000"), core_sym::from_string("1000.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "bob111111111", core_sym::from_string("10.0000"), core_sym::from_string("10.0000") ) );

   BOOST_REQUIRE_EQUAL( error("missing authority of alice1111111"),
                        push_action( "alice1111111"_n, "delegatebw"_n, mvo()
                                    ("from",     "alice1111111")
                                    ("receiver", "bob111111111")
                                    ("stake_net_quantity", core_sym::from_string("10.0000"))
                                    ("stake_cpu_quantity", core_sym::from_string("10.0000"))
                                    ("transfer", 0 )
                                    ,false
                        )
   );

   BOOST_REQUIRE_EQUAL( error("missing authority of alice1111111"),
                        push_action("alice1111111"_n, "undelegatebw"_n, mvo()
                                    ("from",     "alice1111111")
                                    ("receiver", "bob111111111")
                                    ("unstake_net_quantity", core_sym::from_string("200.0000"))
                                    ("unstake_cpu_quantity", core_sym::from_string("100.0000"))
                                    ("transfer", 0 )
                                    ,false
                        )
   );
   //REQUIRE_MATCHING_OBJECT( , get_voter_info( "alice1111111" ) );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( stake_negative, eosio_system_tester ) try {
   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg("must stake a positive amount"),
                        stake( "alice1111111", core_sym::from_string("-0.0001"), core_sym::from_string("0.0000") )
   );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg("must stake a positive amount"),
                        stake( "alice1111111", core_sym::from_string("0.0000"), core_sym::from_string("-0.0001") )
   );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg("must stake a positive amount"),
                        stake( "alice1111111", core_sym::from_string("00.0000"), core_sym::from_string("00.0000") )
   );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg("must stake a positive amount"),
                        stake( "alice1111111", core_sym::from_string("0.0000"), core_sym::from_string("00.0000") )

   );

   BOOST_REQUIRE_EQUAL( true, get_voter_info( "alice1111111" ).is_null() );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( unstake_negative, eosio_system_tester ) try {
   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );

   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "bob111111111", core_sym::from_string("200.0001"), core_sym::from_string("100.0001") ) );

   auto total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0001"), total["net_weight"].as<asset>());
   auto vinfo = get_voter_info("alice1111111" );
   wdump((vinfo));
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("300.0002") ), get_voter_info( "alice1111111" ) );


   BOOST_REQUIRE_EQUAL( wasm_assert_msg("must unstake a positive amount"),
                        unstake( "alice1111111", "bob111111111", core_sym::from_string("-1.0000"), core_sym::from_string("0.0000") )
   );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg("must unstake a positive amount"),
                        unstake( "alice1111111", "bob111111111", core_sym::from_string("0.0000"), core_sym::from_string("-1.0000") )
   );

   //unstake all zeros
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("must unstake a positive amount"),
                        unstake( "alice1111111", "bob111111111", core_sym::from_string("0.0000"), core_sym::from_string("0.0000") )

   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( unstake_more_than_at_stake, eosio_system_tester ) try {
   cross_15_percent_threshold();

   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );

   auto total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());

   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );

   //trying to unstake more net bandwidth than at stake

   BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient staked net bandwidth"),
                        unstake( "alice1111111", core_sym::from_string("200.0001"), core_sym::from_string("0.0000") )
   );

   //trying to unstake more cpu bandwidth than at stake
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient staked cpu bandwidth"),
                        unstake( "alice1111111", core_sym::from_string("0.0000"), core_sym::from_string("100.0001") )

   );

   //check that nothing has changed
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( delegate_to_another_user, eosio_system_tester ) try {
   cross_15_percent_threshold();

   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );

   BOOST_REQUIRE_EQUAL( success(), stake ( "alice1111111", "bob111111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );

   auto total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );
   //all voting power goes to alice1111111
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("300.0000") ), get_voter_info( "alice1111111" ) );
   //but not to bob111111111
   BOOST_REQUIRE_EQUAL( true, get_voter_info( "bob111111111" ).is_null() );

   //bob111111111 should not be able to unstake what was staked by alice1111111
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient staked cpu bandwidth"),
                        unstake( "bob111111111", core_sym::from_string("0.0000"), core_sym::from_string("10.0000") )

   );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient staked net bandwidth"),
                        unstake( "bob111111111", core_sym::from_string("10.0000"),  core_sym::from_string("0.0000") )
   );

   issue_and_transfer( "carol1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol1111111", "bob111111111", core_sym::from_string("20.0000"), core_sym::from_string("10.0000") ) );
   total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("230.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("120.0000"), total["cpu_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("970.0000"), get_balance( "carol1111111" ) );
   REQUIRE_MATCHING_OBJECT( voter( "carol1111111", core_sym::from_string("30.0000") ), get_voter_info( "carol1111111" ) );

   //alice1111111 should not be able to unstake money staked by carol1111111

   BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient staked net bandwidth"),
                        unstake( "alice1111111", "bob111111111", core_sym::from_string("2001.0000"), core_sym::from_string("1.0000") )
   );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient staked cpu bandwidth"),
                        unstake( "alice1111111", "bob111111111", core_sym::from_string("1.0000"), core_sym::from_string("101.0000") )

   );

   total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("230.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("120.0000"), total["cpu_weight"].as<asset>());
   //balance should not change after unsuccessful attempts to unstake
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );
   //voting power too
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("300.0000") ), get_voter_info( "alice1111111" ) );
   REQUIRE_MATCHING_OBJECT( voter( "carol1111111", core_sym::from_string("30.0000") ), get_voter_info( "carol1111111" ) );
   BOOST_REQUIRE_EQUAL( true, get_voter_info( "bob111111111" ).is_null() );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( stake_unstake_separate, eosio_system_tester ) try {
   cross_15_percent_threshold();

   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1000.0000"), get_balance( "alice1111111" ) );

   //everything at once
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", core_sym::from_string("10.0000"), core_sym::from_string("20.0000") ) );
   auto total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("20.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("30.0000"), total["cpu_weight"].as<asset>());

   //cpu
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", core_sym::from_string("100.0000"), core_sym::from_string("0.0000") ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("120.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("30.0000"), total["cpu_weight"].as<asset>());

   //net
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", core_sym::from_string("0.0000"), core_sym::from_string("200.0000") ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("120.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("230.0000"), total["cpu_weight"].as<asset>());

   //unstake cpu
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", core_sym::from_string("100.0000"), core_sym::from_string("0.0000") ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("20.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("230.0000"), total["cpu_weight"].as<asset>());

   //unstake net
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", core_sym::from_string("0.0000"), core_sym::from_string("200.0000") ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("20.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("30.0000"), total["cpu_weight"].as<asset>());
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( adding_stake_partial_unstake, eosio_system_tester ) try {
   cross_15_percent_threshold();

   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "bob111111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );

   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("300.0000") ), get_voter_info( "alice1111111" ) );

   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "bob111111111", core_sym::from_string("100.0000"), core_sym::from_string("50.0000") ) );

   auto total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("310.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("160.0000"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("450.0000") ), get_voter_info( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("550.0000"), get_balance( "alice1111111" ) );

   //unstake a share
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "bob111111111", core_sym::from_string("150.0000"), core_sym::from_string("75.0000") ) );

   total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("160.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("85.0000"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("225.0000") ), get_voter_info( "alice1111111" ) );

   //unstake more
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "bob111111111", core_sym::from_string("50.0000"), core_sym::from_string("25.0000") ) );
   total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("60.0000"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("150.0000") ), get_voter_info( "alice1111111" ) );

   //combined amount should be available only in 3 days
   produce_block( fc::days(2) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("550.0000"), get_balance( "alice1111111" ) );
   produce_block( fc::days(1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "refund"_n, mvo()("owner", "alice1111111") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("850.0000"), get_balance( "alice1111111" ) );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( stake_from_refund, eosio_system_tester ) try {
   cross_15_percent_threshold();

   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );

   auto total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());

   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "bob111111111", core_sym::from_string("50.0000"), core_sym::from_string("50.0000") ) );

   total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("60.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("60.0000"), total["cpu_weight"].as<asset>());

   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("400.0000") ), get_voter_info( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("600.0000"), get_balance( "alice1111111" ) );

   //unstake a share
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "alice1111111", core_sym::from_string("100.0000"), core_sym::from_string("50.0000") ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("60.0000"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("250.0000") ), get_voter_info( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("600.0000"), get_balance( "alice1111111" ) );
   auto refund = get_refund_request( "alice1111111"_n );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("100.0000"), refund["net_amount"].as<asset>() );
   BOOST_REQUIRE_EQUAL( core_sym::from_string( "50.0000"), refund["cpu_amount"].as<asset>() );
   //XXX auto request_time = refund["request_time"].as_int64();

   //alice delegates to bob, should pull from liquid balance not refund
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "bob111111111", core_sym::from_string("50.0000"), core_sym::from_string("50.0000") ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("60.0000"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("350.0000") ), get_voter_info( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("500.0000"), get_balance( "alice1111111" ) );
   refund = get_refund_request( "alice1111111"_n );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("100.0000"), refund["net_amount"].as<asset>() );
   BOOST_REQUIRE_EQUAL( core_sym::from_string( "50.0000"), refund["cpu_amount"].as<asset>() );

   //stake less than pending refund, entire amount should be taken from refund
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "alice1111111", core_sym::from_string("50.0000"), core_sym::from_string("25.0000") ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("160.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("85.0000"), total["cpu_weight"].as<asset>());
   refund = get_refund_request( "alice1111111"_n );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("50.0000"), refund["net_amount"].as<asset>() );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("25.0000"), refund["cpu_amount"].as<asset>() );
   //request time should stay the same
   //BOOST_REQUIRE_EQUAL( request_time, refund["request_time"].as_int64() );
   //balance should stay the same
   BOOST_REQUIRE_EQUAL( core_sym::from_string("500.0000"), get_balance( "alice1111111" ) );

   //stake exactly pending refund amount
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "alice1111111", core_sym::from_string("50.0000"), core_sym::from_string("25.0000") ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
   //pending refund should be removed
   refund = get_refund_request( "alice1111111"_n );
   BOOST_TEST_REQUIRE( refund.is_null() );
   //balance should stay the same
   BOOST_REQUIRE_EQUAL( core_sym::from_string("500.0000"), get_balance( "alice1111111" ) );

   //create pending refund again
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("10.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("10.0000"), total["cpu_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("500.0000"), get_balance( "alice1111111" ) );
   refund = get_refund_request( "alice1111111"_n );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("200.0000"), refund["net_amount"].as<asset>() );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("100.0000"), refund["cpu_amount"].as<asset>() );

   //stake more than pending refund
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "alice1111111", core_sym::from_string("300.0000"), core_sym::from_string("200.0000") ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("310.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("700.0000") ), get_voter_info( "alice1111111" ) );
   refund = get_refund_request( "alice1111111"_n );
   BOOST_TEST_REQUIRE( refund.is_null() );
   //200 core tokens should be taken from alice's account
   BOOST_REQUIRE_EQUAL( core_sym::from_string("300.0000"), get_balance( "alice1111111" ) );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( stake_to_another_user_not_from_refund, eosio_system_tester ) try {
   cross_15_percent_threshold();

   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );

   auto total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );

   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("300.0000") ), get_voter_info( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );

   //unstake
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );
   auto refund = get_refund_request( "alice1111111"_n );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("200.0000"), refund["net_amount"].as<asset>() );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("100.0000"), refund["cpu_amount"].as<asset>() );
   //auto orig_request_time = refund["request_time"].as_int64();

   //stake to another user
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "bob111111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );
   total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
   //stake should be taken from alice's balance, and refund request should stay the same
   BOOST_REQUIRE_EQUAL( core_sym::from_string("400.0000"), get_balance( "alice1111111" ) );
   refund = get_refund_request( "alice1111111"_n );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("200.0000"), refund["net_amount"].as<asset>() );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("100.0000"), refund["cpu_amount"].as<asset>() );
   //BOOST_REQUIRE_EQUAL( orig_request_time, refund["request_time"].as_int64() );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE(eosio_system_producer_tests)

// Tests for voting
BOOST_FIXTURE_TEST_CASE( producer_register_unregister, eosio_system_tester ) try {
   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );

   //fc::variant params = producer_parameters_example(1);
   auto key =  fc::crypto::public_key( std::string("EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV") ); // cspell:disable-line
   BOOST_REQUIRE_EQUAL( success(), push_action("alice1111111"_n, "regproducer"_n, mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", key )
                                               ("url", "http://block.one")
                                               ("location", 1)
                        )
   );

   auto info = get_producer_info( "alice1111111" );
   BOOST_REQUIRE_EQUAL( "alice1111111", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "http://block.one", info["url"].as_string() );

   //change parameters one by one to check for things like #3783
   //fc::variant params2 = producer_parameters_example(2);
   BOOST_REQUIRE_EQUAL( success(), push_action("alice1111111"_n, "regproducer"_n, mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", key )
                                               ("url", "http://block.two")
                                               ("location", 1)
                        )
   );
   info = get_producer_info( "alice1111111" );
   BOOST_REQUIRE_EQUAL( "alice1111111", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( key, fc::crypto::public_key(info["producer_key"].as_string()) );
   BOOST_REQUIRE_EQUAL( "http://block.two", info["url"].as_string() );
   BOOST_REQUIRE_EQUAL( 1, info["location"].as_int64() );

   auto key2 =  fc::crypto::public_key( std::string("EOS5jnmSKrzdBHE9n8hw58y7yxFWBC8SNiG7m8S1crJH3KvAnf9o6") ); // cspell:disable-line
   BOOST_REQUIRE_EQUAL( success(), push_action("alice1111111"_n, "regproducer"_n, mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", key2 )
                                               ("url", "http://block.two")
                                               ("location", 2)
                        )
   );
   info = get_producer_info( "alice1111111" );
   BOOST_REQUIRE_EQUAL( "alice1111111", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( key2, fc::crypto::public_key(info["producer_key"].as_string()) );
   BOOST_REQUIRE_EQUAL( "http://block.two", info["url"].as_string() );
   BOOST_REQUIRE_EQUAL( 2, info["location"].as_int64() );

   //unregister producer
   BOOST_REQUIRE_EQUAL( success(), push_action("alice1111111"_n, "unregprod"_n, mvo()
                                               ("producer",  "alice1111111")
                        )
   );
   info = get_producer_info( "alice1111111" );
   //key should be empty
   BOOST_REQUIRE_EQUAL( fc::crypto::public_key(), fc::crypto::public_key(info["producer_key"].as_string()) );
   //everything else should stay the same
   BOOST_REQUIRE_EQUAL( "alice1111111", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "http://block.two", info["url"].as_string() );

   //unregister bob111111111 who is not a producer
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "producer not found" ),
                        push_action( "bob111111111"_n, "unregprod"_n, mvo()
                                     ("producer",  "bob111111111")
                        )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( producer_wtmsig, eosio_system_tester ) try {
   cross_15_percent_threshold();

   BOOST_REQUIRE_EQUAL( control->active_producers().version, 0u );

   issue_and_transfer( "alice1111111"_n, core_sym::from_string("200000000.0000"),  config::system_account_name );
   block_signing_authority_v0 alice_signing_authority;
   alice_signing_authority.threshold = 1;
   alice_signing_authority.keys.push_back( {.key = get_public_key( "alice1111111"_n, "bs1"), .weight = 1} );
   alice_signing_authority.keys.push_back( {.key = get_public_key( "alice1111111"_n, "bs2"), .weight = 1} );
   producer_authority alice_producer_authority = {.producer_name = "alice1111111"_n, .authority = alice_signing_authority};
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproducer2"_n, mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_authority", alice_producer_authority.get_abi_variant()["authority"])
                                               ("url", "http://block.one")
                                               ("location", 0 )
                        )
   );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111"_n, core_sym::from_string("100000000.0000"), core_sym::from_string("100000000.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), vote( "alice1111111"_n, { "alice1111111"_n } ) );

   block_signing_private_keys.emplace(get_public_key("alice1111111"_n, "bs1"), get_private_key("alice1111111"_n, "bs1"));

   auto alice_prod_info = get_producer_info( "alice1111111"_n );
   wdump((alice_prod_info));
   BOOST_REQUIRE_EQUAL( alice_prod_info["is_active"], true );

   produce_block();
   produce_block( fc::minutes(2) );
   produce_blocks(config::producer_repetitions * 2);
   BOOST_REQUIRE_EQUAL( control->active_producers().version, 1u );
   produce_block();
   BOOST_REQUIRE_EQUAL( control->pending_block_producer(), "alice1111111"_n );
   produce_block();

   alice_signing_authority.threshold = 0;
   alice_producer_authority.authority = alice_signing_authority;

   // Ensure an authority with a threshold of 0 is rejected.
   BOOST_REQUIRE_EQUAL( error("assertion failure with message: invalid producer authority"),
                        push_action( "alice1111111"_n, "regproducer2"_n, mvo()
                                       ("producer",  "alice1111111")
                                       ("producer_authority", alice_producer_authority.get_abi_variant()["authority"])
                                       ("url", "http://block.one")
                                       ("location", 0 )
                        )
   );

   // Ensure an authority that is not satisfiable is rejected.
   alice_signing_authority.threshold = 3;
   alice_producer_authority.authority = alice_signing_authority;
   BOOST_REQUIRE_EQUAL( error("assertion failure with message: invalid producer authority"),
                        push_action( "alice1111111"_n, "regproducer2"_n, mvo()
                                       ("producer",  "alice1111111")
                                       ("producer_authority", alice_producer_authority.get_abi_variant()["authority"])
                                       ("url", "http://block.one")
                                       ("location", 0 )
                        )
   );

   // Ensure an authority with duplicate keys is rejected.
   alice_signing_authority.threshold = 1;
   alice_signing_authority.keys[1] = alice_signing_authority.keys[0];
   alice_producer_authority.authority = alice_signing_authority;
   BOOST_REQUIRE_EQUAL( error("assertion failure with message: invalid producer authority"),
                        push_action( "alice1111111"_n, "regproducer2"_n, mvo()
                                       ("producer",  "alice1111111")
                                       ("producer_authority", alice_producer_authority.get_abi_variant()["authority"])
                                       ("url", "http://block.one")
                                       ("location", 0 )
                        )
   );

   // However, an authority with an invalid key is okay.
   alice_signing_authority.keys[1] = {};
   alice_producer_authority.authority = alice_signing_authority;
   BOOST_REQUIRE_EQUAL( success(),
                        push_action( "alice1111111"_n, "regproducer2"_n, mvo()
                                       ("producer",  "alice1111111")
                                       ("producer_authority", alice_producer_authority.get_abi_variant()["authority"])
                                       ("url", "http://block.one")
                                       ("location", 0 )
                        )
   );

   produce_block();
   produce_block( fc::minutes(2) );
   produce_blocks(config::producer_repetitions * 2);
   BOOST_REQUIRE_EQUAL( control->active_producers().version, 2u );
   produce_block();
   BOOST_REQUIRE_EQUAL( control->pending_block_producer(), "alice1111111"_n );
   produce_block();

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( producer_wtmsig_transition, eosio_system_tester ) try {
   cross_15_percent_threshold();

   BOOST_REQUIRE_EQUAL( control->active_producers().version, 0u );

   set_code( config::system_account_name, contracts::util::system_wasm_v1_8() );
   set_abi(  config::system_account_name, contracts::util::system_abi_v1_8().data() );

   issue_and_transfer( "alice1111111"_n, core_sym::from_string("200000000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproducer"_n, mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( "alice1111111"_n, "active") )
                                               ("url","")
                                               ("location", 0)
                        )
   );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111"_n, core_sym::from_string("100000000.0000"), core_sym::from_string("100000000.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), vote( "alice1111111"_n, { "alice1111111"_n } ) );

   //auto alice_prod_info1 = get_producer_info( "alice1111111"_n );
   //wdump((alice_prod_info1));

   produce_block();
   produce_block( fc::minutes(2) );
   produce_blocks(config::producer_repetitions * 2);
   BOOST_REQUIRE_EQUAL( control->active_producers().version, 1u );

   auto convert_to_block_timestamp = [](const fc::variant& timestamp) -> eosio::chain::block_timestamp_type {
      return fc::time_point::from_iso_string(timestamp.as_string());
   };

   const auto schedule_update1 = convert_to_block_timestamp(get_global_state()["last_producer_schedule_update"]);

   const auto& rlm = control->get_resource_limits_manager();

   auto alice_initial_ram_usage = rlm.get_account_ram_usage("alice1111111"_n);

   set_code( config::system_account_name, contracts::system_wasm() );
   set_abi(  config::system_account_name, contracts::system_abi().data() );
   produce_block();
   BOOST_REQUIRE_EQUAL( control->pending_block_producer(), "alice1111111"_n );

   auto alice_prod_info2 = get_producer_info( "alice1111111"_n );
   BOOST_REQUIRE_EQUAL( alice_prod_info2["is_active"], true );

   produce_block( fc::minutes(2) );
   const auto schedule_update2 = convert_to_block_timestamp(get_global_state()["last_producer_schedule_update"]);
   BOOST_REQUIRE( schedule_update1 < schedule_update2 ); // Ensure last_producer_schedule_update is increasing.

   // Producing the above block would trigger the bug in v1.9.0 that sets the default block_signing_authority
   // in the producer object of the currently active producer alice1111111.
   // However, starting in v1.9.1, the producer object does not have a default block_signing_authority added to the
   // serialization of the producer object if it was not already present in the binary extension field
   // producer_authority to begin with. This is verified below by ensuring the RAM usage of alice (who pays for the
   // producer object) does not increase.

   auto alice_ram_usage = rlm.get_account_ram_usage("alice1111111"_n);
   BOOST_CHECK_EQUAL( alice_initial_ram_usage, alice_ram_usage );

   auto alice_prod_info3 = get_producer_info( "alice1111111"_n );
   if( alice_prod_info3.get_object().contains("producer_authority") ) {
      BOOST_CHECK_EQUAL( alice_prod_info3["producer_authority"][1]["threshold"], 0 );
   }

   produce_block( fc::minutes(2) );
   const auto schedule_update3 = convert_to_block_timestamp(get_global_state()["last_producer_schedule_update"]);

   // The bug in v1.9.0 would cause alice to have an invalid producer authority (the default block_signing_authority).
   // The v1.9.0 system contract would have attempted to set a proposed producer schedule including this invalid
   // authority which would be rejected by the EOSIO native system and cause the onblock transaction to continue to fail.
   // This could be observed by noticing that last_producer_schedule_update was not being updated even though it should.
   // However, starting in v1.9.1, update_elected_producers is smarter about the producer schedule it constructs to
   // propose to the system. It will recognize the default constructed authority (which shouldn't be created by the
   // v1.9.1 system contract but may still exist in the tables if it was constructed by the buggy v1.9.0 system contract)
   // and instead resort to constructing the block signing authority from the single producer key in the table.
   // So newer system contracts should see onblock continue to function, which is verified by the check below.

   BOOST_CHECK( schedule_update2 < schedule_update3 ); // Ensure last_producer_schedule_update is increasing.

   // But even if the buggy v1.9.0 system contract was running, it should always still be possible to recover
   // by having the producer with the invalid authority simply call regproducer or regproducer2 to correct their
   // block signing authority.

   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproducer"_n, mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( "alice1111111"_n, "active") )
                                               ("url","")
                                               ("location", 0)
                        )
   );

   produce_block();
   produce_block( fc::minutes(2) );

   auto alice_prod_info4 = get_producer_info( "alice1111111"_n );
   BOOST_REQUIRE_EQUAL( alice_prod_info4["is_active"], true );
   const auto schedule_update4 = convert_to_block_timestamp(get_global_state()["last_producer_schedule_update"]);
   BOOST_REQUIRE( schedule_update2 < schedule_update4 );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( vote_for_producer, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   cross_15_percent_threshold();

   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproducer"_n, mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( "alice1111111"_n, "active") )
                                               ("url", "http://block.one")
                                               ("location", 0 )
                        )
   );
   auto prod = get_producer_info( "alice1111111" );
   BOOST_REQUIRE_EQUAL( "alice1111111", prod["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, prod["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "http://block.one", prod["url"].as_string() );

   issue_and_transfer( "bob111111111", core_sym::from_string("2000.0000"),  config::system_account_name );
   issue_and_transfer( "carol1111111", core_sym::from_string("3000.0000"),  config::system_account_name );

   //bob111111111 makes stake
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", core_sym::from_string("11.0000"), core_sym::from_string("0.1111") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1988.8889"), get_balance( "bob111111111" ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", core_sym::from_string("11.1111") ), get_voter_info( "bob111111111" ) );

   //bob111111111 votes for alice1111111
   BOOST_REQUIRE_EQUAL( success(), vote( "bob111111111"_n, { "alice1111111"_n } ) );

   //check that producer parameters stay the same after voting
   prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("11.1111")) == prod["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "alice1111111", prod["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( "http://block.one", prod["url"].as_string() );

   //carol1111111 makes stake
   BOOST_REQUIRE_EQUAL( success(), stake( "carol1111111", core_sym::from_string("22.0000"), core_sym::from_string("0.2222") ) );
   REQUIRE_MATCHING_OBJECT( voter( "carol1111111", core_sym::from_string("22.2222") ), get_voter_info( "carol1111111" ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("2977.7778"), get_balance( "carol1111111" ) );
   //carol1111111 votes for alice1111111
   BOOST_REQUIRE_EQUAL( success(), vote( "carol1111111"_n, { "alice1111111"_n } ) );

   //new stake votes be added to alice1111111's total_votes
   prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("33.3333")) == prod["total_votes"].as_double() );

   //bob111111111 increases his stake
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", core_sym::from_string("33.0000"), core_sym::from_string("0.3333") ) );
   //alice1111111 stake with transfer to bob111111111
   BOOST_REQUIRE_EQUAL( success(), stake_with_transfer( "alice1111111"_n, "bob111111111"_n, core_sym::from_string("22.0000"), core_sym::from_string("0.2222") ) );
   //should increase alice1111111's total_votes
   prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("88.8888")) == prod["total_votes"].as_double() );

   //carol1111111 unstakes part of the stake
   BOOST_REQUIRE_EQUAL( success(), unstake( "carol1111111", core_sym::from_string("2.0000"), core_sym::from_string("0.0002")/*"2.0000 SYS", "0.0002 SYS"*/ ) );

   //should decrease alice1111111's total_votes
   prod = get_producer_info( "alice1111111" );
   wdump((prod));
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("86.8886")) == prod["total_votes"].as_double() );

   //bob111111111 revokes his vote
   BOOST_REQUIRE_EQUAL( success(), vote( "bob111111111"_n, vector<account_name>() ) );

   //should decrease alice1111111's total_votes
   prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("20.2220")) == prod["total_votes"].as_double() );
   //but eos should still be at stake
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1955.5556"), get_balance( "bob111111111" ) );

   //carol1111111 unstakes rest of eos
   BOOST_REQUIRE_EQUAL( success(), unstake( "carol1111111", core_sym::from_string("20.0000"), core_sym::from_string("0.2220") ) );
   //should decrease alice1111111's total_votes to zero
   prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( 0.0 == prod["total_votes"].as_double() );

   //carol1111111 should receive funds in 3 days
   produce_block( fc::days(3) );
   produce_block();

   // do a bid refund for carol
   BOOST_REQUIRE_EQUAL( success(), push_action( "carol1111111"_n, "refund"_n, mvo()("owner", "carol1111111") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("3000.0000"), get_balance( "carol1111111" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( unregistered_producer_voting, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   issue_and_transfer( "bob111111111", core_sym::from_string("2000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", core_sym::from_string("13.0000"), core_sym::from_string("0.5791") ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", core_sym::from_string("13.5791") ), get_voter_info( "bob111111111" ) );

   //bob111111111 should not be able to vote for alice1111111 who is not a producer
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "producer alice1111111 is not registered" ),
                        vote( "bob111111111"_n, { "alice1111111"_n } ) );

   //alice1111111 registers as a producer
   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproducer"_n, mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( "alice1111111"_n, "active") )
                                               ("url", "")
                                               ("location", 0)
                        )
   );
   //and then unregisters
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "unregprod"_n, mvo()
                                               ("producer",  "alice1111111")
                        )
   );
   //key should be empty
   auto prod = get_producer_info( "alice1111111" );
   BOOST_REQUIRE_EQUAL( fc::crypto::public_key(), fc::crypto::public_key(prod["producer_key"].as_string()) );

   //bob111111111 should not be able to vote for alice1111111 who is an unregistered producer
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "producer alice1111111 is not currently registered" ),
                        vote( "bob111111111"_n, { "alice1111111"_n } ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( more_than_30_producer_voting, eosio_system_tester ) try {
   issue_and_transfer( "bob111111111", core_sym::from_string("2000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", core_sym::from_string("13.0000"), core_sym::from_string("0.5791") ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", core_sym::from_string("13.5791") ), get_voter_info( "bob111111111" ) );

   //bob111111111 should not be able to vote for alice1111111 who is not a producer
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "attempt to vote for too many producers" ),
                        vote( "bob111111111"_n, vector<account_name>(31, "alice1111111"_n) ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_same_producer_30_times, eosio_system_tester ) try {
   issue_and_transfer( "bob111111111", core_sym::from_string("2000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", core_sym::from_string("50.0000"), core_sym::from_string("50.0000") ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", core_sym::from_string("100.0000") ), get_voter_info( "bob111111111" ) );

   //alice1111111 becomes a producer
   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproducer"_n, mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key("alice1111111"_n, "active") )
                                               ("url", "")
                                               ("location", 0)
                        )
   );

   //bob111111111 should not be able to vote for alice1111111 who is not a producer
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "producer votes must be unique and sorted" ),
                        vote( "bob111111111"_n, vector<account_name>(30, "alice1111111"_n) ) );

   auto prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( 0 == prod["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( producer_keep_votes, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   vector<char> key = fc::raw::pack( get_public_key( "alice1111111"_n, "active" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproducer"_n, mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( "alice1111111"_n, "active") )
                                               ("url", "")
                                               ("location", 0)
                        )
   );

   //bob111111111 makes stake
   issue_and_transfer( "bob111111111", core_sym::from_string("2000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", core_sym::from_string("13.0000"), core_sym::from_string("0.5791") ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", core_sym::from_string("13.5791") ), get_voter_info( "bob111111111" ) );

   //bob111111111 votes for alice1111111
   BOOST_REQUIRE_EQUAL( success(), vote("bob111111111"_n, { "alice1111111"_n } ) );

   auto prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("13.5791")) == prod["total_votes"].as_double() );

   //unregister producer
   BOOST_REQUIRE_EQUAL( success(), push_action("alice1111111"_n, "unregprod"_n, mvo()
                                               ("producer",  "alice1111111")
                        )
   );
   prod = get_producer_info( "alice1111111" );
   //key should be empty
   BOOST_REQUIRE_EQUAL( fc::crypto::public_key(), fc::crypto::public_key(prod["producer_key"].as_string()) );
   //check parameters just in case
   //REQUIRE_MATCHING_OBJECT( params, prod["prefs"]);
   //votes should stay the same
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("13.5791")), prod["total_votes"].as_double() );

   //regtister the same producer again
   params = producer_parameters_example(2);
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproducer"_n, mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( "alice1111111"_n, "active") )
                                               ("url", "")
                                               ("location", 0)
                        )
   );
   prod = get_producer_info( "alice1111111" );
   //votes should stay the same
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("13.5791")), prod["total_votes"].as_double() );

   //change parameters
   params = producer_parameters_example(3);
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproducer"_n, mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( "alice1111111"_n, "active") )
                                               ("url","")
                                               ("location", 0)
                        )
   );
   prod = get_producer_info( "alice1111111" );
   //votes should stay the same
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("13.5791")), prod["total_votes"].as_double() );
   //check parameters just in case
   //REQUIRE_MATCHING_OBJECT( params, prod["prefs"]);

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_for_two_producers, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   //alice1111111 becomes a producer
   fc::variant params = producer_parameters_example(1);
   auto key = get_public_key( "alice1111111"_n, "active" );
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproducer"_n, mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( "alice1111111"_n, "active") )
                                               ("url","")
                                               ("location", 0)
                        )
   );
   //bob111111111 becomes a producer
   params = producer_parameters_example(2);
   key = get_public_key( "bob111111111"_n, "active" );
   BOOST_REQUIRE_EQUAL( success(), push_action( "bob111111111"_n, "regproducer"_n, mvo()
                                               ("producer",  "bob111111111")
                                               ("producer_key", get_public_key( "alice1111111"_n, "active") )
                                               ("url","")
                                               ("location", 0)
                        )
   );

   //carol1111111 votes for alice1111111 and bob111111111
   issue_and_transfer( "carol1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol1111111", core_sym::from_string("15.0005"), core_sym::from_string("5.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), vote( "carol1111111"_n, { "alice1111111"_n, "bob111111111"_n } ) );

   auto alice_info = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("20.0005")) == alice_info["total_votes"].as_double() );
   auto bob_info = get_producer_info( "bob111111111" );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("20.0005")) == bob_info["total_votes"].as_double() );

   //carol1111111 votes for alice1111111 (but revokes vote for bob111111111)
   BOOST_REQUIRE_EQUAL( success(), vote( "carol1111111"_n, { "alice1111111"_n } ) );

   alice_info = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("20.0005")) == alice_info["total_votes"].as_double() );
   bob_info = get_producer_info( "bob111111111" );
   BOOST_TEST_REQUIRE( 0 == bob_info["total_votes"].as_double() );

   //alice1111111 votes for herself and bob111111111
   issue_and_transfer( "alice1111111", core_sym::from_string("2.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", core_sym::from_string("1.0000"), core_sym::from_string("1.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), vote("alice1111111"_n, { "alice1111111"_n, "bob111111111"_n } ) );

   alice_info = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("22.0005")) == alice_info["total_votes"].as_double() );

   bob_info = get_producer_info( "bob111111111" );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("2.0000")) == bob_info["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_register_unregister_keeps_stake, eosio_system_tester ) try {
   //register proxy by first action for this user ever
   BOOST_REQUIRE_EQUAL( success(), push_action("alice1111111"_n, "regproxy"_n, mvo()
                                               ("proxy",  "alice1111111")
                                               ("isproxy", true )
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111"_n ), get_voter_info( "alice1111111" ) );

   //unregister proxy
   BOOST_REQUIRE_EQUAL( success(), push_action("alice1111111"_n, "regproxy"_n, mvo()
                                               ("proxy",  "alice1111111")
                                               ("isproxy", false)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111" ), get_voter_info( "alice1111111" ) );

   //stake and then register as a proxy
   issue_and_transfer( "bob111111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", core_sym::from_string("200.0002"), core_sym::from_string("100.0001") ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( "bob111111111"_n, "regproxy"_n, mvo()
                                               ("proxy",  "bob111111111")
                                               ("isproxy", true)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "bob111111111"_n )( "staked", 3000003 ), get_voter_info( "bob111111111" ) );
   //unrgister and check that stake is still in place
   BOOST_REQUIRE_EQUAL( success(), push_action( "bob111111111"_n, "regproxy"_n, mvo()
                                               ("proxy",  "bob111111111")
                                               ("isproxy", false)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", core_sym::from_string("300.0003") ), get_voter_info( "bob111111111" ) );

   //register as a proxy and then stake
   BOOST_REQUIRE_EQUAL( success(), push_action( "carol1111111"_n, "regproxy"_n, mvo()
                                               ("proxy",  "carol1111111")
                                               ("isproxy", true)
                        )
   );
   issue_and_transfer( "carol1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol1111111", core_sym::from_string("246.0002"), core_sym::from_string("531.0001") ) );
   //check that both proxy flag and stake a correct
   REQUIRE_MATCHING_OBJECT( proxy( "carol1111111"_n )( "staked", 7770003 ), get_voter_info( "carol1111111" ) );

   //unregister
   BOOST_REQUIRE_EQUAL( success(), push_action( "carol1111111"_n, "regproxy"_n, mvo()
                                                ("proxy",  "carol1111111")
                                                ("isproxy", false)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "carol1111111", core_sym::from_string("777.0003") ), get_voter_info( "carol1111111" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_stake_unstake_keeps_proxy_flag, eosio_system_tester ) try {
   cross_15_percent_threshold();

   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproxy"_n, mvo()
                                               ("proxy",  "alice1111111")
                                               ("isproxy", true)
                        )
   );
   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111"_n ), get_voter_info( "alice1111111" ) );

   //stake
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", core_sym::from_string("100.0000"), core_sym::from_string("50.0000") ) );
   //check that account is still a proxy
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111"_n )( "staked", 1500000 ), get_voter_info( "alice1111111" ) );

   //stake more
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", core_sym::from_string("30.0000"), core_sym::from_string("20.0000") ) );
   //check that account is still a proxy
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111"_n )("staked", 2000000 ), get_voter_info( "alice1111111" ) );

   //unstake more
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", core_sym::from_string("65.0000"), core_sym::from_string("35.0000") ) );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111"_n )("staked", 1000000 ), get_voter_info( "alice1111111" ) );

   //unstake the rest
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", core_sym::from_string("65.0000"), core_sym::from_string("35.0000") ) );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111"_n )( "staked", 0 ), get_voter_info( "alice1111111" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_actions_affect_producers, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   cross_15_percent_threshold();

   create_accounts_with_resources( {  "defproducer1"_n, "defproducer2"_n, "defproducer3"_n } );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer1"_n, 1) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer2"_n, 2) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer3"_n, 3) );

   //register as a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproxy"_n, mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy", true)
                        )
   );

   //accumulate proxied votes
   issue_and_transfer( "bob111111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", core_sym::from_string("100.0002"), core_sym::from_string("50.0001") ) );
   BOOST_REQUIRE_EQUAL( success(), vote("bob111111111"_n, vector<account_name>(), "alice1111111"_n ) );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111"_n )( "proxied_vote_weight", stake2votes(core_sym::from_string("150.0003")) ), get_voter_info( "alice1111111" ) );

   //vote for producers
   BOOST_REQUIRE_EQUAL( success(), vote("alice1111111"_n, { "defproducer1"_n, "defproducer2"_n } ) );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("150.0003")) == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("150.0003")) == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 0 == get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //vote for another producers
   BOOST_REQUIRE_EQUAL( success(), vote( "alice1111111"_n, { "defproducer1"_n, "defproducer3"_n } ) );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("150.0003")) == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("150.0003")) == get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //unregister proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproxy"_n, mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy", false)
                        )
   );
   //REQUIRE_MATCHING_OBJECT( voter( "alice1111111" )( "proxied_vote_weight", stake2votes(core_sym::from_string("150.0003")) ), get_voter_info( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //register proxy again
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproxy"_n, mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy", true)
                        )
   );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("150.0003")) == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("150.0003")) == get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //stake increase by proxy itself affects producers
   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", core_sym::from_string("30.0001"), core_sym::from_string("20.0001") ) );
   BOOST_REQUIRE_EQUAL( stake2votes(core_sym::from_string("200.0005")), get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( stake2votes(core_sym::from_string("200.0005")), get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //stake decrease by proxy itself affects producers
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", core_sym::from_string("10.0001"), core_sym::from_string("10.0001") ) );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("180.0003")) == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("180.0003")) == get_producer_info( "defproducer3" )["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(producer_pay, eosio_system_tester, * boost::unit_test::tolerance(1e-10)) try {

   const double continuous_rate = std::log1p(double(0.05));
   const double usecs_per_year  = 52 * 7 * 24 * 3600 * 1000000ll;
   const double secs_per_year   = 52 * 7 * 24 * 3600;


   const asset large_asset = core_sym::from_string("80.0000");
   create_account_with_resources( "defproducera"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );
   create_account_with_resources( "defproducerb"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );
   create_account_with_resources( "defproducerc"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );

   create_account_with_resources( "producvotera"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );
   create_account_with_resources( "producvoterb"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );

   BOOST_REQUIRE_EQUAL(success(), regproducer("defproducera"_n));
   produce_block(fc::hours(24));
   auto prod = get_producer_info( "defproducera"_n );
   BOOST_REQUIRE_EQUAL("defproducera", prod["owner"].as_string());
   BOOST_REQUIRE_EQUAL(0, prod["total_votes"].as_double());

   transfer( config::system_account_name, "producvotera", core_sym::from_string("400000000.0000"), config::system_account_name);
   BOOST_REQUIRE_EQUAL(success(), stake("producvotera", core_sym::from_string("100000000.0000"), core_sym::from_string("100000000.0000")));
   BOOST_REQUIRE_EQUAL(success(), vote( "producvotera"_n, { "defproducera"_n }));
   // defproducera is the only active producer
   // produce enough blocks so new schedule kicks in and defproducera produces some blocks
   {
      produce_blocks(50);

      const auto     initial_global_state      = get_global_state();
      const uint64_t initial_claim_time        = microseconds_since_epoch_of_iso_string( initial_global_state["last_pervote_bucket_fill"] );
      const int64_t  initial_pervote_bucket    = initial_global_state["pervote_bucket"].as<int64_t>();
      const int64_t  initial_perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const int64_t  initial_savings           = get_balance("eosio.saving"_n).get_amount();
      const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();

      prod = get_producer_info("defproducera");
      const uint32_t unpaid_blocks = prod["unpaid_blocks"].as<uint32_t>();
      BOOST_REQUIRE(1 < unpaid_blocks);

      BOOST_REQUIRE_EQUAL(initial_tot_unpaid_blocks, unpaid_blocks);

      const asset initial_supply  = get_token_supply();
      const asset initial_balance = get_balance("defproducera"_n);

      BOOST_REQUIRE_EQUAL(success(), push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));

      const auto     global_state      = get_global_state();
      const uint64_t claim_time        = microseconds_since_epoch_of_iso_string( global_state["last_pervote_bucket_fill"] );
      const int64_t  pervote_bucket    = global_state["pervote_bucket"].as<int64_t>();
      [[maybe_unused]] const int64_t  perblock_bucket   = global_state["perblock_bucket"].as<int64_t>();
      const int64_t  savings           = get_balance("eosio.saving"_n).get_amount();
      const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();

      prod = get_producer_info("defproducera");
      BOOST_REQUIRE_EQUAL(1, prod["unpaid_blocks"].as<uint32_t>());
      BOOST_REQUIRE_EQUAL(1, tot_unpaid_blocks);
      const asset supply  = get_token_supply();
      const asset balance = get_balance("defproducera"_n);

      BOOST_REQUIRE_EQUAL(claim_time, microseconds_since_epoch_of_iso_string( prod["last_claim_time"] ));

      auto usecs_between_fills = claim_time - initial_claim_time;
      int32_t secs_between_fills = usecs_between_fills/1000000;

      BOOST_REQUIRE_EQUAL(0, initial_savings);
      BOOST_REQUIRE_EQUAL(0, initial_perblock_bucket);
      BOOST_REQUIRE_EQUAL(0, initial_pervote_bucket);

      BOOST_REQUIRE_EQUAL(int64_t( ( initial_supply.get_amount() * double(secs_between_fills) * continuous_rate ) / secs_per_year ),
                          supply.get_amount() - initial_supply.get_amount());
      BOOST_REQUIRE(within_one(int64_t( ( initial_supply.get_amount() * double(secs_between_fills) * (4.   * continuous_rate/ 5.) / secs_per_year ) ),
                               savings - initial_savings));
      BOOST_REQUIRE(within_one(int64_t( ( initial_supply.get_amount() * double(secs_between_fills) * (0.25 * continuous_rate/ 5.) / secs_per_year ) ),
                               balance.get_amount() - initial_balance.get_amount()));

      int64_t from_perblock_bucket = int64_t( initial_supply.get_amount() * double(secs_between_fills) * (0.25 * continuous_rate/ 5.) / secs_per_year ) ;
      int64_t from_pervote_bucket  = int64_t( initial_supply.get_amount() * double(secs_between_fills) * (0.75 * continuous_rate/ 5.) / secs_per_year ) ;


      if (from_pervote_bucket >= 100 * 10000) {
         BOOST_REQUIRE_EQUAL(from_perblock_bucket + from_pervote_bucket, balance.get_amount() - initial_balance.get_amount());
         BOOST_REQUIRE_EQUAL(0, pervote_bucket);
      } else {
         BOOST_REQUIRE_EQUAL(from_perblock_bucket, balance.get_amount() - initial_balance.get_amount());
         BOOST_REQUIRE_EQUAL(from_pervote_bucket, pervote_bucket);
      }
   }

   {
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                          push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));
   }

   // defproducera waits for 23 hours and 55 minutes, can't claim rewards yet
   {
      produce_block(fc::seconds(23 * 3600 + 55 * 60));
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                          push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));
   }

   // wait 5 more minutes, defproducera can now claim rewards again
   {
      produce_block(fc::seconds(5 * 60));

      const auto     initial_global_state      = get_global_state();
      const uint64_t initial_claim_time        = microseconds_since_epoch_of_iso_string( initial_global_state["last_pervote_bucket_fill"] );
      const int64_t  initial_pervote_bucket    = initial_global_state["pervote_bucket"].as<int64_t>();
      [[maybe_unused]] const int64_t  initial_perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const int64_t  initial_savings           = get_balance("eosio.saving"_n).get_amount();
      const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();
      const double   initial_tot_vote_weight   = initial_global_state["total_producer_vote_weight"].as<double>();

      prod = get_producer_info("defproducera");
      const uint32_t unpaid_blocks = prod["unpaid_blocks"].as<uint32_t>();
      BOOST_REQUIRE(1 < unpaid_blocks);
      BOOST_REQUIRE_EQUAL(initial_tot_unpaid_blocks, unpaid_blocks);
      BOOST_REQUIRE(0 < prod["total_votes"].as<double>());
      BOOST_TEST(initial_tot_vote_weight, prod["total_votes"].as<double>());
      BOOST_REQUIRE(0 < microseconds_since_epoch_of_iso_string( prod["last_claim_time"] ));

      BOOST_REQUIRE_EQUAL(initial_tot_unpaid_blocks, unpaid_blocks);

      const asset initial_supply  = get_token_supply();
      const asset initial_balance = get_balance("defproducera"_n);

      BOOST_REQUIRE_EQUAL(success(), push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));

      const auto global_state          = get_global_state();
      const uint64_t claim_time        = microseconds_since_epoch_of_iso_string( global_state["last_pervote_bucket_fill"] );
      const int64_t  pervote_bucket    = global_state["pervote_bucket"].as<int64_t>();
      [[maybe_unused]] const int64_t  perblock_bucket   = global_state["perblock_bucket"].as<int64_t>();
      const int64_t  savings           = get_balance("eosio.saving"_n).get_amount();
      const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();

      prod = get_producer_info("defproducera");
      BOOST_REQUIRE_EQUAL(1, prod["unpaid_blocks"].as<uint32_t>());
      BOOST_REQUIRE_EQUAL(1, tot_unpaid_blocks);
      const asset supply  = get_token_supply();
      const asset balance = get_balance("defproducera"_n);

      BOOST_REQUIRE_EQUAL(claim_time, microseconds_since_epoch_of_iso_string( prod["last_claim_time"] ));
      auto usecs_between_fills = claim_time - initial_claim_time;

      BOOST_REQUIRE_EQUAL( int64_t( initial_supply.get_amount() * double(usecs_between_fills) * continuous_rate / usecs_per_year ),
                           supply.get_amount() - initial_supply.get_amount() );
      BOOST_REQUIRE_EQUAL( (supply.get_amount() - initial_supply.get_amount()) - (supply.get_amount() - initial_supply.get_amount()) / 5,
                           savings - initial_savings );

      int64_t to_producer        = int64_t( initial_supply.get_amount() * double(usecs_between_fills) * continuous_rate / usecs_per_year ) / 5;
      int64_t to_perblock_bucket = to_producer / 4;
      int64_t to_pervote_bucket  = to_producer - to_perblock_bucket;

      if (to_pervote_bucket + initial_pervote_bucket >= 100 * 10000) {
         BOOST_REQUIRE_EQUAL(to_perblock_bucket + to_pervote_bucket + initial_pervote_bucket, balance.get_amount() - initial_balance.get_amount());
         BOOST_REQUIRE_EQUAL(0, pervote_bucket);
      } else {
         BOOST_REQUIRE_EQUAL(to_perblock_bucket, balance.get_amount() - initial_balance.get_amount());
         BOOST_REQUIRE_EQUAL(to_pervote_bucket + initial_pervote_bucket, pervote_bucket);
      }
   }

   // defproducerb tries to claim rewards but he's not on the list
   {
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("producer not registered"),
                          push_action("defproducerb"_n, "claimrewards"_n, mvo()("owner", "defproducerb")));
   }

   // test stability over a year
   {
      regproducer("defproducerb"_n);
      regproducer("defproducerc"_n);
      produce_block(fc::hours(24));
      const asset   initial_supply  = get_token_supply();
      const int64_t initial_savings = get_balance("eosio.saving"_n).get_amount();
      for (uint32_t i = 0; i < 7 * 52; ++i) {
         produce_block(fc::seconds(8 * 3600));
         BOOST_REQUIRE_EQUAL(success(), push_action("defproducerc"_n, "claimrewards"_n, mvo()("owner", "defproducerc")));
         produce_block(fc::seconds(8 * 3600));
         BOOST_REQUIRE_EQUAL(success(), push_action("defproducerb"_n, "claimrewards"_n, mvo()("owner", "defproducerb")));
         produce_block(fc::seconds(8 * 3600));
         BOOST_REQUIRE_EQUAL(success(), push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));
      }
      const asset   supply  = get_token_supply();
      const int64_t savings = get_balance("eosio.saving"_n).get_amount();
      // Amount issued per year is very close to the 5% inflation target. Small difference (500 tokens out of 50'000'000 issued)
      // is due to compounding every 8 hours in this test as opposed to theoretical continuous compounding
      BOOST_REQUIRE(500 * 10000 > int64_t(double(initial_supply.get_amount()) * double(0.05)) - (supply.get_amount() - initial_supply.get_amount()));
      BOOST_REQUIRE(500 * 10000 > int64_t(double(initial_supply.get_amount()) * double(0.04)) - (savings - initial_savings));
   }

   // test claimrewards when max supply is reached
   {
      produce_block(fc::hours(24));

      const asset before_supply = get_token_supply();
      const asset before_system_balance = get_balance(config::system_account_name);
      const asset before_producer_balance = get_balance("defproducera"_n);

      setmaxsupply( before_supply );
      BOOST_REQUIRE_EQUAL(success(), push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));

      const asset after_supply = get_token_supply();
      const asset after_system_balance = get_balance(config::system_account_name);
      const asset after_producer_balance = get_balance("defproducera"_n);

      BOOST_REQUIRE_EQUAL(after_supply.get_amount(), before_supply.get_amount());
      BOOST_REQUIRE_EQUAL(after_system_balance.get_amount() - before_system_balance.get_amount(), -1407793756);
      BOOST_REQUIRE_EQUAL(after_producer_balance.get_amount() - before_producer_balance.get_amount(), 281558751);
   }

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE(eosio_system_inflation_tests)

BOOST_FIXTURE_TEST_CASE(change_inflation, eosio_system_tester) try {

   {
      BOOST_REQUIRE_EQUAL( success(),
                           setinflation(0, 10000, 10000) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("annual_rate can't be negative"),
                           setinflation(-1, 10000, 10000) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("inflation_pay_factor must not be less than 10000"),
                           setinflation(1, 9999, 10000) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("votepay_factor must not be less than 10000"),
                           setinflation(1, 10000, 9999) );

      BOOST_REQUIRE_EQUAL( success(),
                           setpayfactor(10000, 10000) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("inflation_pay_factor must not be less than 10000"),
                           setpayfactor(9999, 10000) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("votepay_factor must not be less than 10000"),
                           setpayfactor(10000, 9999) );
   }

   {
      const asset large_asset = core_sym::from_string("80.0000");
      create_account_with_resources( "defproducera"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );
      create_account_with_resources( "defproducerb"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );
      create_account_with_resources( "defproducerc"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );

      create_account_with_resources( "producvotera"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );
      create_account_with_resources( "producvoterb"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );

      BOOST_REQUIRE_EQUAL(success(), regproducer("defproducera"_n));
      BOOST_REQUIRE_EQUAL(success(), regproducer("defproducerb"_n));
      BOOST_REQUIRE_EQUAL(success(), regproducer("defproducerc"_n));

      produce_block(fc::hours(24));

      transfer( config::system_account_name, "producvotera", core_sym::from_string("400000000.0000"), config::system_account_name);
      BOOST_REQUIRE_EQUAL(success(), stake("producvotera", core_sym::from_string("100000000.0000"), core_sym::from_string("100000000.0000")));
      BOOST_REQUIRE_EQUAL(success(), vote( "producvotera"_n, { "defproducera"_n,"defproducerb"_n,"defproducerc"_n }));

      auto run_for_1year = [this](int64_t annual_rate, int64_t inflation_pay_factor, int64_t votepay_factor) {

         double inflation = double(annual_rate)/double(10000);

         BOOST_REQUIRE_EQUAL(success(), setinflation(
            annual_rate,
            inflation_pay_factor,
            votepay_factor
         ));

         produce_block(fc::hours(24));
         const asset   initial_supply  = get_token_supply();
         const int64_t initial_savings = get_balance("eosio.saving"_n).get_amount();
         for (uint32_t i = 0; i < 7 * 52; ++i) {
            produce_block(fc::seconds(8 * 3600));
            BOOST_REQUIRE_EQUAL(success(), push_action("defproducerc"_n, "claimrewards"_n, mvo()("owner", "defproducerc")));
            produce_block(fc::seconds(8 * 3600));
            BOOST_REQUIRE_EQUAL(success(), push_action("defproducerb"_n, "claimrewards"_n, mvo()("owner", "defproducerb")));
            produce_block(fc::seconds(8 * 3600));
            BOOST_REQUIRE_EQUAL(success(), push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));
         }
         const asset   final_supply  = get_token_supply();
         const int64_t final_savings = get_balance("eosio.saving"_n).get_amount();

         double computed_new_tokens = double(final_supply.get_amount() - initial_supply.get_amount());
         double theoretical_new_tokens = double(initial_supply.get_amount())*inflation;
         double diff_new_tokens = std::abs(theoretical_new_tokens-computed_new_tokens);

         if( annual_rate > 0 ) {
            //Error should be less than 0.3%
            BOOST_REQUIRE( diff_new_tokens/theoretical_new_tokens < double(0.003) );
         } else {
            BOOST_REQUIRE_EQUAL( computed_new_tokens, 0 );
            BOOST_REQUIRE_EQUAL( theoretical_new_tokens, 0 );
         }

         double savings_inflation = inflation - inflation * 10000 / inflation_pay_factor;

         double computed_savings_tokens = double(final_savings-initial_savings);
         double theoretical_savings_tokens = double(initial_supply.get_amount())*savings_inflation;
         double diff_savings_tokens = std::abs(theoretical_savings_tokens-computed_savings_tokens);

         if( annual_rate > 0 ) {
            //Error should be less than 0.3%
            BOOST_REQUIRE( diff_savings_tokens/theoretical_savings_tokens < double(0.003) );
         } else {
            BOOST_REQUIRE_EQUAL( computed_savings_tokens, 0 );
            BOOST_REQUIRE_EQUAL( theoretical_savings_tokens, 0 );
         }
      };

      // 1% inflation for 1 year. 50% savings / 50% bp reward. 10000 / 50000 = 0.2 => 20% blockpay, 80% votepay
      run_for_1year(100, 20000, 50000);

      // 3% inflation for 1 year. 66.6% savings / 33.33% bp reward. 10000/13333 = 0.75 => 75% blockpay, 25% votepay
      run_for_1year(300, 30000, 13333);

      // 0% inflation for 1 year
      run_for_1year(0, 30000, 50000);
   }

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE(extreme_inflation) try {
   eosio_system_tester t(eosio_system_tester::setup_level::minimal);
   symbol core_symbol{CORE_SYM};
   const asset max_supply = asset((1ll << 62) - 1, core_symbol);
   t.create_currency( "eosio.token"_n, config::system_account_name, max_supply );
   t.issue( asset(10000000000000, core_symbol) );
   t.deploy_contract();
   t.produce_block();
   t.create_account_with_resources("defproducera"_n, config::system_account_name, core_sym::from_string("1.0000"), false);
   BOOST_REQUIRE_EQUAL(t.success(), t.regproducer("defproducera"_n));
   t.transfer( config::system_account_name, "defproducera", core_sym::from_string("200000000.0000"), config::system_account_name);
   BOOST_REQUIRE_EQUAL(t.success(), t.stake("defproducera", core_sym::from_string("100000000.0000"), core_sym::from_string("100000000.0000")));
   BOOST_REQUIRE_EQUAL(t.success(), t.vote( "defproducera"_n, { "defproducera"_n }));
   t.produce_blocks(4);
   t.produce_block(fc::hours(24));

   BOOST_REQUIRE_EQUAL(t.success(), t.push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));
   t.produce_block();
   const asset current_supply = t.get_token_supply();
   t.issue( max_supply - current_supply );

   // empty system balance
   // claimrewards operates by either `issue` new tokens or using the existing system balance
   const asset system_balance = t.get_balance(config::system_account_name);
   t.transfer( config::system_account_name, "eosio.null"_n, system_balance, config::system_account_name);
   BOOST_REQUIRE_EQUAL(t.get_balance(config::system_account_name).get_amount(), 0);
   BOOST_REQUIRE_EQUAL(t.get_token_supply().get_amount() - max_supply.get_amount(), 0);

   // set maximum inflation
   BOOST_REQUIRE_EQUAL(t.success(), t.setinflation(std::numeric_limits<int64_t>::max(), 50000, 40000));
   t.produce_block();

   t.produce_block(fc::hours(10*24));
   BOOST_REQUIRE_EQUAL(t.wasm_assert_msg("insufficient system token balance for claiming rewards"), t.push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));

   t.produce_block(fc::hours(11*24));
   BOOST_REQUIRE_EQUAL(t.wasm_assert_msg("magnitude of asset amount must be less than 2^62"), t.push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));

   t.produce_block(fc::hours(24));
   BOOST_REQUIRE_EQUAL(t.wasm_assert_msg("overflow in calculating new tokens to be issued; inflation rate is too high"), t.push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));
   BOOST_REQUIRE_EQUAL(t.success(), t.setinflation(500, 50000, 40000));
   BOOST_REQUIRE_EQUAL(t.wasm_assert_msg("insufficient system token balance for claiming rewards"), t.push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE(eosio_system_multiple_producer_pay_tests)

BOOST_FIXTURE_TEST_CASE(multiple_producer_pay, eosio_system_tester, * boost::unit_test::tolerance(1e-10)) try {

   const int64_t secs_per_year  = 52 * 7 * 24 * 3600;
   const double  usecs_per_year = secs_per_year * 1000000;
   const double  cont_rate      = std::log1p(double(0.05));

   const asset net = core_sym::from_string("80.0000");
   const asset cpu = core_sym::from_string("80.0000");
   const std::vector<account_name> voters = { "producvotera"_n, "producvoterb"_n, "producvoterc"_n, "producvoterd"_n };
   for (const auto& v: voters) {
      create_account_with_resources( v, config::system_account_name, core_sym::from_string("1.0000"), false, net, cpu );
      transfer( config::system_account_name, v, core_sym::from_string("100000000.0000"), config::system_account_name );
      BOOST_REQUIRE_EQUAL(success(), stake(v, core_sym::from_string("30000000.0000"), core_sym::from_string("30000000.0000")) );
   }

   // create accounts {defproducera, defproducerb, ..., defproducerz, abcproducera, ..., defproducern} and register as producers
   std::vector<account_name> producer_names;
   {
      producer_names.reserve('z' - 'a' + 1);
      {
         const std::string root("defproducer");
         for ( char c = 'a'; c <= 'z'; ++c ) {
            producer_names.emplace_back(root + std::string(1, c));
         }
      }
      {
         const std::string root("abcproducer");
         for ( char c = 'a'; c <= 'n'; ++c ) {
            producer_names.emplace_back(root + std::string(1, c));
         }
      }
      setup_producer_accounts(producer_names);
      for (const auto& p: producer_names) {
         BOOST_REQUIRE_EQUAL( success(), regproducer(p) );
         produce_blocks(1);
         ilog( "------ get pro----------" );
         wdump((p));
         BOOST_TEST(0 == get_producer_info(p)["total_votes"].as<double>());
      }
   }

   produce_block( fc::hours(24) );

   // producvotera votes for defproducera ... defproducerj
   // producvoterb votes for defproducera ... defproduceru
   // producvoterc votes for defproducera ... defproducerz
   // producvoterd votes for abcproducera ... abcproducern
   {
      BOOST_REQUIRE_EQUAL(success(), vote("producvotera"_n, vector<account_name>(producer_names.begin(), producer_names.begin()+10)));
      BOOST_REQUIRE_EQUAL(success(), vote("producvoterb"_n, vector<account_name>(producer_names.begin(), producer_names.begin()+21)));
      BOOST_REQUIRE_EQUAL(success(), vote("producvoterc"_n, vector<account_name>(producer_names.begin(), producer_names.begin()+26)));
      BOOST_REQUIRE_EQUAL(success(), vote("producvoterd"_n, vector<account_name>(producer_names.begin()+26, producer_names.end())));
   }

   {
      auto proda = get_producer_info( "defproducera"_n );
      auto prodj = get_producer_info( "defproducerj"_n );
      auto prodk = get_producer_info( "defproducerk"_n );
      auto produ = get_producer_info( "defproduceru"_n );
      auto prodv = get_producer_info( "defproducerv"_n );
      auto prodz = get_producer_info( "defproducerz"_n );

      BOOST_REQUIRE (0 == proda["unpaid_blocks"].as<uint32_t>() && 0 == prodz["unpaid_blocks"].as<uint32_t>());

      // check vote ratios
      BOOST_REQUIRE ( 0 < proda["total_votes"].as<double>() && 0 < prodz["total_votes"].as<double>() );
      BOOST_TEST( proda["total_votes"].as<double>() == prodj["total_votes"].as<double>() );
      BOOST_TEST( prodk["total_votes"].as<double>() == produ["total_votes"].as<double>() );
      BOOST_TEST( prodv["total_votes"].as<double>() == prodz["total_votes"].as<double>() );
      BOOST_TEST( 2 * proda["total_votes"].as<double>() == 3 * produ["total_votes"].as<double>() );
      BOOST_TEST( proda["total_votes"].as<double>() ==  3 * prodz["total_votes"].as<double>() );
   }

   // give a chance for everyone to produce blocks
   {
      produce_blocks(23 * 12 + 20);
      bool all_21_produced = true;
      for (uint32_t i = 0; i < 21; ++i) {
         if (0 == get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            all_21_produced = false;
         }
      }
      bool rest_didnt_produce = true;
      for (uint32_t i = 21; i < producer_names.size(); ++i) {
         if (0 < get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            rest_didnt_produce = false;
         }
      }
      BOOST_REQUIRE(all_21_produced && rest_didnt_produce);
   }

   std::vector<double> vote_shares(producer_names.size());
   {
      double total_votes = 0;
      for (uint32_t i = 0; i < producer_names.size(); ++i) {
         vote_shares[i] = get_producer_info(producer_names[i])["total_votes"].as<double>();
         total_votes += vote_shares[i];
      }
      BOOST_TEST(total_votes == get_global_state()["total_producer_vote_weight"].as<double>());
      std::for_each(vote_shares.begin(), vote_shares.end(), [total_votes](double& x) { x /= total_votes; });

      BOOST_TEST(double(1) == std::accumulate(vote_shares.begin(), vote_shares.end(), double(0)));
      BOOST_TEST(double(3./71.) == vote_shares.front());
      BOOST_TEST(double(1./71.) == vote_shares.back());
   }

   {
      const uint32_t prod_index = 2;
      const auto prod_name = producer_names[prod_index];

      const auto     initial_global_state      = get_global_state();
      const uint64_t initial_claim_time        = microseconds_since_epoch_of_iso_string( initial_global_state["last_pervote_bucket_fill"] );
      [[maybe_unused]] const int64_t  initial_pervote_bucket    = initial_global_state["pervote_bucket"].as<int64_t>();
      [[maybe_unused]] const int64_t  initial_perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const int64_t  initial_savings           = get_balance("eosio.saving"_n).get_amount();
      const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();
      const asset    initial_supply            = get_token_supply();
      [[maybe_unused]] const asset    initial_bpay_balance      = get_balance("eosio.bpay"_n);
      [[maybe_unused]] const asset    initial_vpay_balance      = get_balance("eosio.vpay"_n);
      const asset    initial_balance           = get_balance(prod_name);
      const uint32_t initial_unpaid_blocks     = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

      BOOST_REQUIRE_EQUAL(success(), push_action(prod_name, "claimrewards"_n, mvo()("owner", prod_name)));

      const auto     global_state      = get_global_state();
      const uint64_t claim_time        = microseconds_since_epoch_of_iso_string( global_state["last_pervote_bucket_fill"] );
      const int64_t  pervote_bucket    = global_state["pervote_bucket"].as<int64_t>();
      const int64_t  perblock_bucket   = global_state["perblock_bucket"].as<int64_t>();
      const int64_t  savings           = get_balance("eosio.saving"_n).get_amount();
      const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();
      const asset    supply            = get_token_supply();
      const asset    bpay_balance      = get_balance("eosio.bpay"_n);
      const asset    vpay_balance      = get_balance("eosio.vpay"_n);
      const asset    balance           = get_balance(prod_name);
      const uint32_t unpaid_blocks     = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

      const uint64_t usecs_between_fills = claim_time - initial_claim_time;
      [[maybe_unused]] const int32_t secs_between_fills = static_cast<int32_t>(usecs_between_fills / 1000000);

      const double expected_supply_growth = initial_supply.get_amount() * double(usecs_between_fills) * cont_rate / usecs_per_year;
      BOOST_REQUIRE_EQUAL( int64_t(expected_supply_growth), supply.get_amount() - initial_supply.get_amount() );

      BOOST_REQUIRE_EQUAL( int64_t(expected_supply_growth) - int64_t(expected_supply_growth)/5, savings - initial_savings );

      const int64_t expected_perblock_bucket = initial_supply.get_amount() * double(usecs_between_fills) * (0.25 * cont_rate/ 5.) / usecs_per_year;
      const int64_t expected_pervote_bucket  = initial_supply.get_amount() * double(usecs_between_fills) * (0.75 * cont_rate/ 5.) / usecs_per_year;

      const int64_t from_perblock_bucket = initial_unpaid_blocks * expected_perblock_bucket / initial_tot_unpaid_blocks ;
      const int64_t from_pervote_bucket  = vote_shares[prod_index] * expected_pervote_bucket;

      BOOST_REQUIRE( 1 >= abs(int32_t(initial_tot_unpaid_blocks - tot_unpaid_blocks) - int32_t(initial_unpaid_blocks - unpaid_blocks)) );

      if (from_pervote_bucket >= 100 * 10000) {
         BOOST_REQUIRE( within_one( from_perblock_bucket + from_pervote_bucket, balance.get_amount() - initial_balance.get_amount() ) );
         BOOST_REQUIRE( within_one( expected_pervote_bucket - from_pervote_bucket, pervote_bucket ) );
      } else {
         BOOST_REQUIRE( within_one( from_perblock_bucket, balance.get_amount() - initial_balance.get_amount() ) );
         BOOST_REQUIRE( within_one( expected_pervote_bucket, pervote_bucket ) );
         BOOST_REQUIRE( within_one( expected_pervote_bucket, vpay_balance.get_amount() ) );
         BOOST_REQUIRE( within_one( perblock_bucket, bpay_balance.get_amount() ) );
      }

      produce_blocks(5);

      BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                          push_action(prod_name, "claimrewards"_n, mvo()("owner", prod_name)));
   }

   {
      const uint32_t prod_index = 23;
      const auto prod_name = producer_names[prod_index];
      BOOST_REQUIRE_EQUAL(success(),
                          push_action(prod_name, "claimrewards"_n, mvo()("owner", prod_name)));
      BOOST_REQUIRE_EQUAL(0, get_balance(prod_name).get_amount());
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                          push_action(prod_name, "claimrewards"_n, mvo()("owner", prod_name)));
   }

   // Wait for 23 hours. By now, pervote_bucket has grown enough
   // that a producer's share is more than 100 tokens.
   produce_block(fc::seconds(23 * 3600));

   {
      const uint32_t prod_index = 15;
      const auto prod_name = producer_names[prod_index];

      const auto     initial_global_state      = get_global_state();
      const uint64_t initial_claim_time        = microseconds_since_epoch_of_iso_string( initial_global_state["last_pervote_bucket_fill"] );
      const int64_t  initial_pervote_bucket    = initial_global_state["pervote_bucket"].as<int64_t>();
      const int64_t  initial_perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const int64_t  initial_savings           = get_balance("eosio.saving"_n).get_amount();
      const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();
      const asset    initial_supply            = get_token_supply();
      [[maybe_unused]] const asset    initial_bpay_balance      = get_balance("eosio.bpay"_n);
      [[maybe_unused]] const asset    initial_vpay_balance      = get_balance("eosio.vpay"_n);
      const asset    initial_balance           = get_balance(prod_name);
      const uint32_t initial_unpaid_blocks     = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

      BOOST_REQUIRE_EQUAL(success(), push_action(prod_name, "claimrewards"_n, mvo()("owner", prod_name)));

      const auto     global_state      = get_global_state();
      const uint64_t claim_time        = microseconds_since_epoch_of_iso_string( global_state["last_pervote_bucket_fill"] );
      const int64_t  pervote_bucket    = global_state["pervote_bucket"].as<int64_t>();
      const int64_t  perblock_bucket   = global_state["perblock_bucket"].as<int64_t>();
      const int64_t  savings           = get_balance("eosio.saving"_n).get_amount();
      const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();
      const asset    supply            = get_token_supply();
      const asset    bpay_balance      = get_balance("eosio.bpay"_n);
      const asset    vpay_balance      = get_balance("eosio.vpay"_n);
      const asset    balance           = get_balance(prod_name);
      const uint32_t unpaid_blocks     = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

      const uint64_t usecs_between_fills = claim_time - initial_claim_time;

      const double expected_supply_growth = initial_supply.get_amount() * double(usecs_between_fills) * cont_rate / usecs_per_year;
      BOOST_REQUIRE_EQUAL( int64_t(expected_supply_growth), supply.get_amount() - initial_supply.get_amount() );
      BOOST_REQUIRE_EQUAL( int64_t(expected_supply_growth) - int64_t(expected_supply_growth)/5, savings - initial_savings );

      const int64_t expected_perblock_bucket = int64_t( double(initial_supply.get_amount()) * double(usecs_between_fills) * (0.25 * cont_rate/ 5.) / usecs_per_year )
                                               + initial_perblock_bucket;
      const int64_t expected_pervote_bucket  = int64_t( double(initial_supply.get_amount()) * double(usecs_between_fills) * (0.75 * cont_rate/ 5.) / usecs_per_year )
                                               + initial_pervote_bucket;
      const int64_t from_perblock_bucket = initial_unpaid_blocks * expected_perblock_bucket / initial_tot_unpaid_blocks ;
      const int64_t from_pervote_bucket  = int64_t( vote_shares[prod_index] * expected_pervote_bucket);

      BOOST_REQUIRE( 1 >= abs(int32_t(initial_tot_unpaid_blocks - tot_unpaid_blocks) - int32_t(initial_unpaid_blocks - unpaid_blocks)) );
      if (from_pervote_bucket >= 100 * 10000) {
         BOOST_REQUIRE( within_one( from_perblock_bucket + from_pervote_bucket, balance.get_amount() - initial_balance.get_amount() ) );
         BOOST_REQUIRE( within_one( expected_pervote_bucket - from_pervote_bucket, pervote_bucket ) );
         BOOST_REQUIRE( within_one( expected_pervote_bucket - from_pervote_bucket, vpay_balance.get_amount() ) );
         BOOST_REQUIRE( within_one( expected_perblock_bucket - from_perblock_bucket, perblock_bucket ) );
         BOOST_REQUIRE( within_one( expected_perblock_bucket - from_perblock_bucket, bpay_balance.get_amount() ) );
      } else {
         BOOST_REQUIRE( within_one( from_perblock_bucket, balance.get_amount() - initial_balance.get_amount() ) );
         BOOST_REQUIRE( within_one( expected_pervote_bucket, pervote_bucket ) );
      }

      produce_blocks(5);

      BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                          push_action(prod_name, "claimrewards"_n, mvo()("owner", prod_name)));
   }

   {
      const uint32_t prod_index = 24;
      const auto prod_name = producer_names[prod_index];
      BOOST_REQUIRE_EQUAL(success(),
                          push_action(prod_name, "claimrewards"_n, mvo()("owner", prod_name)));
      BOOST_REQUIRE(100 * 10000 <= get_balance(prod_name).get_amount());
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                          push_action(prod_name, "claimrewards"_n, mvo()("owner", prod_name)));
   }

   {
      const uint32_t rmv_index = 5;
      account_name prod_name = producer_names[rmv_index];

      auto info = get_producer_info(prod_name);
      BOOST_REQUIRE( info["is_active"].as<bool>() );
      BOOST_REQUIRE( fc::crypto::public_key() != fc::crypto::public_key(info["producer_key"].as_string()) );

      BOOST_REQUIRE_EQUAL( error("missing authority of eosio"),
                           push_action(prod_name, "rmvproducer"_n, mvo()("producer", prod_name)));
      BOOST_REQUIRE_EQUAL( error("missing authority of eosio"),
                           push_action(producer_names[rmv_index + 2], "rmvproducer"_n, mvo()("producer", prod_name) ) );
      BOOST_REQUIRE_EQUAL( success(),
                           push_action(config::system_account_name, "rmvproducer"_n, mvo()("producer", prod_name) ) );
      {
         bool rest_didnt_produce = true;
         for (uint32_t i = 21; i < producer_names.size(); ++i) {
            if (0 < get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
               rest_didnt_produce = false;
            }
         }
         BOOST_REQUIRE(rest_didnt_produce);
      }

      produce_blocks(3 * 21 * 12);
      info = get_producer_info(prod_name);
      const uint32_t init_unpaid_blocks = info["unpaid_blocks"].as<uint32_t>();
      BOOST_REQUIRE( !info["is_active"].as<bool>() );
      BOOST_REQUIRE( fc::crypto::public_key() == fc::crypto::public_key(info["producer_key"].as_string()) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("producer does not have an active key"),
                           push_action(prod_name, "claimrewards"_n, mvo()("owner", prod_name) ) );
      produce_blocks(3 * 21 * 12);
      BOOST_REQUIRE_EQUAL( init_unpaid_blocks, get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>() );
      {
         bool prod_was_replaced = false;
         for (uint32_t i = 21; i < producer_names.size(); ++i) {
            if (0 < get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
               prod_was_replaced = true;
            }
         }
         BOOST_REQUIRE(prod_was_replaced);
      }
   }

   {
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("producer not found"),
                           push_action( config::system_account_name, "rmvproducer"_n, mvo()("producer", "nonexistingp") ) );
   }

   produce_block(fc::hours(24));

   // switch to new producer pay metric
   {
      BOOST_REQUIRE_EQUAL( 0, get_global_state2()["revision"].as<uint8_t>() );
      BOOST_REQUIRE_EQUAL( error("missing authority of eosio"),
                           push_action(producer_names[1], "updtrevision"_n, mvo()("revision", 1) ) );
      BOOST_REQUIRE_EQUAL( success(),
                           push_action(config::system_account_name, "updtrevision"_n, mvo()("revision", 1) ) );
      BOOST_REQUIRE_EQUAL( 1, get_global_state2()["revision"].as<uint8_t>() );

      const uint32_t prod_index = 2;
      const auto prod_name = producer_names[prod_index];

      const auto     initial_prod_info         = get_producer_info(prod_name);
      const auto     initial_prod_info2        = get_producer_info2(prod_name);
      const auto     initial_global_state      = get_global_state();
      const double   initial_tot_votepay_share = get_global_state2()["total_producer_votepay_share"].as_double();
      const double   initial_tot_vpay_rate     = get_global_state3()["total_vpay_share_change_rate"].as_double();
      const uint64_t initial_vpay_state_update = microseconds_since_epoch_of_iso_string( get_global_state3()["last_vpay_state_update"] );
      const uint64_t initial_bucket_fill_time  = microseconds_since_epoch_of_iso_string( initial_global_state["last_pervote_bucket_fill"] );
      const int64_t  initial_pervote_bucket    = initial_global_state["pervote_bucket"].as<int64_t>();
      const int64_t  initial_perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();
      const asset    initial_supply            = get_token_supply();
      const asset    initial_balance           = get_balance(prod_name);
      const uint32_t initial_unpaid_blocks     = initial_prod_info["unpaid_blocks"].as<uint32_t>();
      [[maybe_unused]] const uint64_t initial_claim_time = microseconds_since_epoch_of_iso_string( initial_prod_info["last_claim_time"] );
      const uint64_t initial_prod_update_time  = microseconds_since_epoch_of_iso_string( initial_prod_info2["last_votepay_share_update"] );

      BOOST_TEST_REQUIRE( 0 == get_producer_info2(prod_name)["votepay_share"].as_double() );
      BOOST_REQUIRE_EQUAL( success(), push_action(prod_name, "claimrewards"_n, mvo()("owner", prod_name) ) );

      const auto     prod_info         = get_producer_info(prod_name);
      const auto     prod_info2        = get_producer_info2(prod_name);
      const auto     global_state      = get_global_state();
      const uint64_t vpay_state_update = microseconds_since_epoch_of_iso_string( get_global_state3()["last_vpay_state_update"] );
      const uint64_t bucket_fill_time  = microseconds_since_epoch_of_iso_string( global_state["last_pervote_bucket_fill"] );
      const int64_t  pervote_bucket    = global_state["pervote_bucket"].as<int64_t>();
      [[maybe_unused]] const int64_t  perblock_bucket   = global_state["perblock_bucket"].as<int64_t>();
      [[maybe_unused]] const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();
      const asset    supply            = get_token_supply();
      const asset    balance           = get_balance(prod_name);
      [[maybe_unused]] const uint32_t unpaid_blocks = prod_info["unpaid_blocks"].as<uint32_t>();
      const uint64_t claim_time        = microseconds_since_epoch_of_iso_string( prod_info["last_claim_time"] );
      const uint64_t prod_update_time  = microseconds_since_epoch_of_iso_string( prod_info2["last_votepay_share_update"] );

      const uint64_t usecs_between_fills         = bucket_fill_time - initial_bucket_fill_time;
      const double   secs_between_global_updates = (vpay_state_update - initial_vpay_state_update) / 1E6;
      const double   secs_between_prod_updates   = (prod_update_time - initial_prod_update_time) / 1E6;
      const double   votepay_share               = initial_prod_info2["votepay_share"].as_double() + secs_between_prod_updates * prod_info["total_votes"].as_double();
      const double   tot_votepay_share           = initial_tot_votepay_share + initial_tot_vpay_rate * secs_between_global_updates;

      const int64_t expected_perblock_bucket = int64_t( double(initial_supply.get_amount()) * double(usecs_between_fills) * (0.25 * cont_rate/ 5.) / usecs_per_year )
         + initial_perblock_bucket;
      const int64_t expected_pervote_bucket  = int64_t( double(initial_supply.get_amount()) * double(usecs_between_fills) * (0.75 * cont_rate/ 5.) / usecs_per_year )
         + initial_pervote_bucket;
      const int64_t from_perblock_bucket = initial_unpaid_blocks * expected_perblock_bucket / initial_tot_unpaid_blocks;
      const int64_t from_pervote_bucket  = int64_t( ( votepay_share * expected_pervote_bucket ) / tot_votepay_share );

      const double expected_supply_growth = initial_supply.get_amount() * double(usecs_between_fills) * cont_rate / usecs_per_year;
      BOOST_REQUIRE_EQUAL( int64_t(expected_supply_growth), supply.get_amount() - initial_supply.get_amount() );
      BOOST_REQUIRE_EQUAL( claim_time, vpay_state_update );
      BOOST_REQUIRE( 100 * 10000 < from_pervote_bucket );
      BOOST_CHECK_EQUAL( expected_pervote_bucket - from_pervote_bucket, pervote_bucket );
      BOOST_CHECK_EQUAL( from_perblock_bucket + from_pervote_bucket, balance.get_amount() - initial_balance.get_amount() );
      BOOST_TEST_REQUIRE( 0 == get_producer_info2(prod_name)["votepay_share"].as_double() );

      produce_block(fc::hours(2));

      BOOST_REQUIRE_EQUAL( wasm_assert_msg("already claimed rewards within past day"),
                           push_action(prod_name, "claimrewards"_n, mvo()("owner", prod_name) ) );
   }

} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE(eosio_system_votepay_tests)

BOOST_FIXTURE_TEST_CASE(multiple_producer_votepay_share, eosio_system_tester, * boost::unit_test::tolerance(1e-10)) try {

   const asset net = core_sym::from_string("80.0000");
   const asset cpu = core_sym::from_string("80.0000");
   const std::vector<account_name> voters = { "producvotera"_n, "producvoterb"_n, "producvoterc"_n, "producvoterd"_n };
   for (const auto& v: voters) {
      create_account_with_resources( v, config::system_account_name, core_sym::from_string("1.0000"), false, net, cpu );
      transfer( config::system_account_name, v, core_sym::from_string("100000000.0000"), config::system_account_name );
      BOOST_REQUIRE_EQUAL(success(), stake(v, core_sym::from_string("30000000.0000"), core_sym::from_string("30000000.0000")) );
   }

   // create accounts {defproducera, defproducerb, ..., defproducerz, abcproducera, ..., defproducern} and register as producers
   std::vector<account_name> producer_names;
   {
      producer_names.reserve('z' - 'a' + 1);
      {
         const std::string root("defproducer");
         for ( char c = 'a'; c <= 'z'; ++c ) {
            producer_names.emplace_back(root + std::string(1, c));
         }
      }
      {
         const std::string root("abcproducer");
         for ( char c = 'a'; c <= 'n'; ++c ) {
            producer_names.emplace_back(root + std::string(1, c));
         }
      }
      setup_producer_accounts(producer_names);
      for (const auto& p: producer_names) {
         BOOST_REQUIRE_EQUAL( success(), regproducer(p) );
         produce_blocks(1);
         ilog( "------ get pro----------" );
         wdump((p));
         BOOST_TEST_REQUIRE(0 == get_producer_info(p)["total_votes"].as_double());
         BOOST_TEST_REQUIRE(0 == get_producer_info2(p)["votepay_share"].as_double());
         BOOST_REQUIRE(0 < microseconds_since_epoch_of_iso_string( get_producer_info2(p)["last_votepay_share_update"] ));
      }
   }

   produce_block( fc::hours(24) );

   // producvotera votes for defproducera ... defproducerj
   // producvoterb votes for defproducera ... defproduceru
   // producvoterc votes for defproducera ... defproducerz
   // producvoterd votes for abcproducera ... abcproducern
   {
      BOOST_TEST_REQUIRE( 0 == get_global_state3()["total_vpay_share_change_rate"].as_double() );
      BOOST_REQUIRE_EQUAL( success(), vote("producvotera"_n, vector<account_name>(producer_names.begin(), producer_names.begin()+10)) );
      produce_block( fc::hours(10) );
      BOOST_TEST_REQUIRE( 0 == get_global_state2()["total_producer_votepay_share"].as_double() );
      const auto& init_info  = get_producer_info(producer_names[0]);
      const auto& init_info2 = get_producer_info2(producer_names[0]);
      uint64_t init_update = microseconds_since_epoch_of_iso_string( init_info2["last_votepay_share_update"] );
      double   init_votes  = init_info["total_votes"].as_double();
      BOOST_REQUIRE_EQUAL( success(), vote("producvoterb"_n, vector<account_name>(producer_names.begin(), producer_names.begin()+21)) );
      const auto& info  = get_producer_info(producer_names[0]);
      const auto& info2 = get_producer_info2(producer_names[0]);
      BOOST_TEST_REQUIRE( ((microseconds_since_epoch_of_iso_string( info2["last_votepay_share_update"] ) - init_update)/double(1E6)) * init_votes == info2["votepay_share"].as_double() );
      BOOST_TEST_REQUIRE( info2["votepay_share"].as_double() * 10 == get_global_state2()["total_producer_votepay_share"].as_double() );

      BOOST_TEST_REQUIRE( 0 == get_producer_info2(producer_names[11])["votepay_share"].as_double() );
      produce_block( fc::hours(13) );
      BOOST_REQUIRE_EQUAL( success(), vote("producvoterc"_n, vector<account_name>(producer_names.begin(), producer_names.begin()+26)) );
      BOOST_REQUIRE( 0 < get_producer_info2(producer_names[11])["votepay_share"].as_double() );
      produce_block( fc::hours(1) );
      BOOST_REQUIRE_EQUAL( success(), vote("producvoterd"_n, vector<account_name>(producer_names.begin()+26, producer_names.end())) );
      BOOST_TEST_REQUIRE( 0 == get_producer_info2(producer_names[26])["votepay_share"].as_double() );
   }

   {
      auto proda = get_producer_info( "defproducera"_n );
      auto prodj = get_producer_info( "defproducerj"_n );
      auto prodk = get_producer_info( "defproducerk"_n );
      auto produ = get_producer_info( "defproduceru"_n );
      auto prodv = get_producer_info( "defproducerv"_n );
      auto prodz = get_producer_info( "defproducerz"_n );

      BOOST_REQUIRE (0 == proda["unpaid_blocks"].as<uint32_t>() && 0 == prodz["unpaid_blocks"].as<uint32_t>());

      // check vote ratios
      BOOST_REQUIRE ( 0 < proda["total_votes"].as_double() && 0 < prodz["total_votes"].as_double() );
      BOOST_TEST_REQUIRE( proda["total_votes"].as_double() == prodj["total_votes"].as_double() );
      BOOST_TEST_REQUIRE( prodk["total_votes"].as_double() == produ["total_votes"].as_double() );
      BOOST_TEST_REQUIRE( prodv["total_votes"].as_double() == prodz["total_votes"].as_double() );
      BOOST_TEST_REQUIRE( 2 * proda["total_votes"].as_double() == 3 * produ["total_votes"].as_double() );
      BOOST_TEST_REQUIRE( proda["total_votes"].as_double() ==  3 * prodz["total_votes"].as_double() );
   }

   std::vector<double> vote_shares(producer_names.size());
   {
      double total_votes = 0;
      for (uint32_t i = 0; i < producer_names.size(); ++i) {
         vote_shares[i] = get_producer_info(producer_names[i])["total_votes"].as_double();
         total_votes += vote_shares[i];
      }
      BOOST_TEST_REQUIRE( total_votes == get_global_state()["total_producer_vote_weight"].as_double() );
      BOOST_TEST_REQUIRE( total_votes == get_global_state3()["total_vpay_share_change_rate"].as_double() );
      BOOST_REQUIRE_EQUAL( microseconds_since_epoch_of_iso_string( get_producer_info2(producer_names.back())["last_votepay_share_update"] ),
                           microseconds_since_epoch_of_iso_string( get_global_state3()["last_vpay_state_update"] ) );

      std::for_each( vote_shares.begin(), vote_shares.end(), [total_votes](double& x) { x /= total_votes; } );
      BOOST_TEST_REQUIRE( double(1) == std::accumulate(vote_shares.begin(), vote_shares.end(), double(0)) );
      BOOST_TEST_REQUIRE( double(3./71.) == vote_shares.front() );
      BOOST_TEST_REQUIRE( double(1./71.) == vote_shares.back() );
   }

   std::vector<double> votepay_shares(producer_names.size());
   {
      const auto& gs3 = get_global_state3();
      double total_votepay_shares          = 0;
      double expected_total_votepay_shares = 0;
      for (uint32_t i = 0; i < producer_names.size() ; ++i) {
         const auto& info  = get_producer_info(producer_names[i]);
         const auto& info2 = get_producer_info2(producer_names[i]);
         votepay_shares[i] = info2["votepay_share"].as_double();
         total_votepay_shares          += votepay_shares[i];
         expected_total_votepay_shares += votepay_shares[i];
         expected_total_votepay_shares += info["total_votes"].as_double()
                                           * double( ( microseconds_since_epoch_of_iso_string( gs3["last_vpay_state_update"] )
                                                        - microseconds_since_epoch_of_iso_string( info2["last_votepay_share_update"] )
                                                     ) / 1E6 );
      }
      BOOST_TEST( expected_total_votepay_shares > total_votepay_shares );
      BOOST_TEST_REQUIRE( expected_total_votepay_shares == get_global_state2()["total_producer_votepay_share"].as_double() );
   }

   {
      const uint32_t prod_index = 15;
      const account_name prod_name = producer_names[prod_index];
      const auto& init_info        = get_producer_info(prod_name);
      const auto& init_info2       = get_producer_info2(prod_name);
      BOOST_REQUIRE( 0 < init_info2["votepay_share"].as_double() );
      BOOST_REQUIRE( 0 < microseconds_since_epoch_of_iso_string( init_info2["last_votepay_share_update"] ) );

      BOOST_REQUIRE_EQUAL( success(), push_action(prod_name, "claimrewards"_n, mvo()("owner", prod_name)) );

      BOOST_TEST_REQUIRE( 0 == get_producer_info2(prod_name)["votepay_share"].as_double() );
      BOOST_REQUIRE_EQUAL( get_producer_info(prod_name)["last_claim_time"].as_string(),
                           get_producer_info2(prod_name)["last_votepay_share_update"].as_string() );
      BOOST_REQUIRE_EQUAL( get_producer_info(prod_name)["last_claim_time"].as_string(),
                           get_global_state3()["last_vpay_state_update"].as_string() );
      const auto& gs3 = get_global_state3();
      double expected_total_votepay_shares = 0;
      for (uint32_t i = 0; i < producer_names.size(); ++i) {
         const auto& info  = get_producer_info(producer_names[i]);
         const auto& info2 = get_producer_info2(producer_names[i]);
         expected_total_votepay_shares += info2["votepay_share"].as_double();
         expected_total_votepay_shares += info["total_votes"].as_double()
                                           * double( ( microseconds_since_epoch_of_iso_string( gs3["last_vpay_state_update"] )
                                                        - microseconds_since_epoch_of_iso_string( info2["last_votepay_share_update"] )
                                                     ) / 1E6 );
      }
      BOOST_TEST_REQUIRE( expected_total_votepay_shares == get_global_state2()["total_producer_votepay_share"].as_double() );
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(votepay_share_invariant, eosio_system_tester, * boost::unit_test::tolerance(1e-10)) try {

   cross_15_percent_threshold();

   const asset net = core_sym::from_string("80.0000");
   const asset cpu = core_sym::from_string("80.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n, "carolaccount"_n, "emilyaccount"_n };
   for (const auto& a: accounts) {
      create_account_with_resources( a, config::system_account_name, core_sym::from_string("1.0000"), false, net, cpu );
      transfer( config::system_account_name, a, core_sym::from_string("1000.0000"), config::system_account_name );
   }
   const auto vota  = accounts[0];
   const auto votb  = accounts[1];
   const auto proda = accounts[2];
   const auto prodb = accounts[3];

   BOOST_REQUIRE_EQUAL( success(), stake( vota, core_sym::from_string("100.0000"), core_sym::from_string("100.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), stake( votb, core_sym::from_string("100.0000"), core_sym::from_string("100.0000") ) );

   BOOST_REQUIRE_EQUAL( success(), regproducer( proda ) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( prodb ) );

   BOOST_REQUIRE_EQUAL( success(), vote( vota, { proda } ) );
   BOOST_REQUIRE_EQUAL( success(), vote( votb, { prodb } ) );

   produce_block( fc::hours(25) );

   BOOST_REQUIRE_EQUAL( success(), vote( vota, { proda } ) );
   BOOST_REQUIRE_EQUAL( success(), vote( votb, { prodb } ) );

   produce_block( fc::hours(1) );

   BOOST_REQUIRE_EQUAL( success(), push_action(proda, "claimrewards"_n, mvo()("owner", proda)) );
   BOOST_TEST_REQUIRE( 0 == get_producer_info2(proda)["votepay_share"].as_double() );

   produce_block( fc::hours(24) );

   BOOST_REQUIRE_EQUAL( success(), vote( vota, { proda } ) );

   produce_block( fc::hours(24) );

   BOOST_REQUIRE_EQUAL( success(), push_action(prodb, "claimrewards"_n, mvo()("owner", prodb)) );
   BOOST_TEST_REQUIRE( 0 == get_producer_info2(prodb)["votepay_share"].as_double() );

   produce_block( fc::hours(10) );

   BOOST_REQUIRE_EQUAL( success(), vote( votb, { prodb } ) );

   produce_block( fc::hours(16) );

   BOOST_REQUIRE_EQUAL( success(), vote( votb, { prodb } ) );
   produce_block( fc::hours(2) );
   BOOST_REQUIRE_EQUAL( success(), vote( vota, { proda } ) );

   const auto& info  = get_producer_info(prodb);
   const auto& info2 = get_producer_info2(prodb);
   const auto& gs2   = get_global_state2();
   const auto& gs3   = get_global_state3();

   double expected_total_vpay_share = info2["votepay_share"].as_double()
                                       + info["total_votes"].as_double()
                                          * ( microseconds_since_epoch_of_iso_string( gs3["last_vpay_state_update"] )
                                               - microseconds_since_epoch_of_iso_string( info2["last_votepay_share_update"] ) ) / 1E6;

   BOOST_TEST_REQUIRE( expected_total_vpay_share == gs2["total_producer_votepay_share"].as_double() );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(votepay_share_proxy, eosio_system_tester, * boost::unit_test::tolerance(1e-5)) try {

   cross_15_percent_threshold();

   const asset net = core_sym::from_string("80.0000");
   const asset cpu = core_sym::from_string("80.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n, "carolaccount"_n, "emilyaccount"_n };
   for (const auto& a: accounts) {
      create_account_with_resources( a, config::system_account_name, core_sym::from_string("1.0000"), false, net, cpu );
      transfer( config::system_account_name, a, core_sym::from_string("1000.0000"), config::system_account_name );
   }
   const auto alice = accounts[0];
   const auto bob   = accounts[1];
   const auto carol = accounts[2];
   const auto emily = accounts[3];

   // alice becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( alice, "regproxy"_n, mvo()("proxy", alice)("isproxy", true) ) );
   REQUIRE_MATCHING_OBJECT( proxy( alice ), get_voter_info( alice ) );

   // carol and emily become producers
   BOOST_REQUIRE_EQUAL( success(), regproducer( carol, 1) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( emily, 1) );

   // bob chooses alice as proxy
   BOOST_REQUIRE_EQUAL( success(), stake( bob, core_sym::from_string("100.0002"), core_sym::from_string("50.0001") ) );
   BOOST_REQUIRE_EQUAL( success(), stake( alice, core_sym::from_string("150.0000"), core_sym::from_string("150.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), vote( bob, { }, alice ) );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("150.0003")) == get_voter_info(alice)["proxied_vote_weight"].as_double() );

   // alice (proxy) votes for carol
   BOOST_REQUIRE_EQUAL( success(), vote( alice, { carol } ) );
   double total_votes = get_producer_info(carol)["total_votes"].as_double();
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("450.0003")) == total_votes );
   BOOST_TEST_REQUIRE( 0 == get_producer_info2(carol)["votepay_share"].as_double() );
   uint64_t last_update_time = microseconds_since_epoch_of_iso_string( get_producer_info2(carol)["last_votepay_share_update"] );

   produce_block( fc::hours(15) );

   // alice (proxy) votes again for carol
   BOOST_REQUIRE_EQUAL( success(), vote( alice, { carol } ) );
   auto cur_info2 = get_producer_info2(carol);
   double expected_votepay_share = double( (microseconds_since_epoch_of_iso_string( cur_info2["last_votepay_share_update"] ) - last_update_time) / 1E6 ) * total_votes;
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("450.0003")) == get_producer_info(carol)["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( expected_votepay_share == cur_info2["votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( expected_votepay_share == get_global_state2()["total_producer_votepay_share"].as_double() );
   last_update_time = microseconds_since_epoch_of_iso_string( cur_info2["last_votepay_share_update"] );
   total_votes      = get_producer_info(carol)["total_votes"].as_double();

   produce_block( fc::hours(40) );

   // bob unstakes
   BOOST_REQUIRE_EQUAL( success(), unstake( bob, core_sym::from_string("10.0002"), core_sym::from_string("10.0001") ) );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("430.0000")), get_producer_info(carol)["total_votes"].as_double() );

   cur_info2 = get_producer_info2(carol);
   expected_votepay_share += double( (microseconds_since_epoch_of_iso_string( cur_info2["last_votepay_share_update"] ) - last_update_time) / 1E6 ) * total_votes;
   BOOST_TEST_REQUIRE( expected_votepay_share == cur_info2["votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( expected_votepay_share == get_global_state2()["total_producer_votepay_share"].as_double() );
   last_update_time = microseconds_since_epoch_of_iso_string( cur_info2["last_votepay_share_update"] );
   total_votes      = get_producer_info(carol)["total_votes"].as_double();

   // carol claims rewards
   BOOST_REQUIRE_EQUAL( success(), push_action(carol, "claimrewards"_n, mvo()("owner", carol)) );

   produce_block( fc::hours(20) );

   // bob votes for carol
   BOOST_REQUIRE_EQUAL( success(), vote( bob, { carol } ) );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("430.0000")), get_producer_info(carol)["total_votes"].as_double() );
   cur_info2 = get_producer_info2(carol);
   expected_votepay_share = double( (microseconds_since_epoch_of_iso_string( cur_info2["last_votepay_share_update"] ) - last_update_time) / 1E6 ) * total_votes;
   BOOST_TEST_REQUIRE( expected_votepay_share == cur_info2["votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( expected_votepay_share == get_global_state2()["total_producer_votepay_share"].as_double() );

   produce_block( fc::hours(54) );

   // bob votes for carol again
   // carol hasn't claimed rewards in over 3 days
   total_votes = get_producer_info(carol)["total_votes"].as_double();
   BOOST_REQUIRE_EQUAL( success(), vote( bob, { carol } ) );
   BOOST_REQUIRE_EQUAL( get_producer_info2(carol)["last_votepay_share_update"].as_string(),
                        get_global_state3()["last_vpay_state_update"].as_string() );
   BOOST_TEST_REQUIRE( 0 == get_producer_info2(carol)["votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( 0 == get_global_state2()["total_producer_votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( 0 == get_global_state3()["total_vpay_share_change_rate"].as_double() );

   produce_block( fc::hours(20) );

   // bob votes for carol again
   // carol still hasn't claimed rewards
   BOOST_REQUIRE_EQUAL( success(), vote( bob, { carol } ) );
   BOOST_REQUIRE_EQUAL(get_producer_info2(carol)["last_votepay_share_update"].as_string(),
                       get_global_state3()["last_vpay_state_update"].as_string() );
   BOOST_TEST_REQUIRE( 0 == get_producer_info2(carol)["votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( 0 == get_global_state2()["total_producer_votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( 0 == get_global_state3()["total_vpay_share_change_rate"].as_double() );

   produce_block( fc::hours(24) );

   // carol finally claims rewards
   BOOST_REQUIRE_EQUAL( success(), push_action( carol, "claimrewards"_n, mvo()("owner", carol) ) );
   BOOST_TEST_REQUIRE( 0           == get_producer_info2(carol)["votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( 0           == get_global_state2()["total_producer_votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( total_votes == get_global_state3()["total_vpay_share_change_rate"].as_double() );

   produce_block( fc::hours(5) );

   // alice votes for carol and emily
   // emily hasn't claimed rewards in over 3 days
   last_update_time = microseconds_since_epoch_of_iso_string( get_producer_info2(carol)["last_votepay_share_update"] );
   BOOST_REQUIRE_EQUAL( success(), vote( alice, { carol, emily } ) );
   cur_info2 = get_producer_info2(carol);
   auto cur_info2_emily = get_producer_info2(emily);

   expected_votepay_share = double( (microseconds_since_epoch_of_iso_string( cur_info2["last_votepay_share_update"] ) - last_update_time) / 1E6 ) * total_votes;
   BOOST_TEST_REQUIRE( expected_votepay_share == cur_info2["votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( 0                      == cur_info2_emily["votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( expected_votepay_share == get_global_state2()["total_producer_votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( get_producer_info(carol)["total_votes"].as_double() ==
                       get_global_state3()["total_vpay_share_change_rate"].as_double() );
   BOOST_REQUIRE_EQUAL( cur_info2["last_votepay_share_update"].as_string(),
                        get_global_state3()["last_vpay_state_update"].as_string() );
   BOOST_REQUIRE_EQUAL( cur_info2_emily["last_votepay_share_update"].as_string(),
                        get_global_state3()["last_vpay_state_update"].as_string() );

   produce_block( fc::hours(10) );

   // bob chooses alice as proxy
   // emily still hasn't claimed rewards
   last_update_time = microseconds_since_epoch_of_iso_string( get_producer_info2(carol)["last_votepay_share_update"] );
   BOOST_REQUIRE_EQUAL( success(), vote( bob, { }, alice ) );
   cur_info2 = get_producer_info2(carol);
   cur_info2_emily = get_producer_info2(emily);

   expected_votepay_share += double( (microseconds_since_epoch_of_iso_string( cur_info2["last_votepay_share_update"] ) - last_update_time) / 1E6 ) * total_votes;
   BOOST_TEST_REQUIRE( expected_votepay_share == cur_info2["votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( 0                      == cur_info2_emily["votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( expected_votepay_share == get_global_state2()["total_producer_votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( get_producer_info(carol)["total_votes"].as_double() ==
                       get_global_state3()["total_vpay_share_change_rate"].as_double() );
   BOOST_REQUIRE_EQUAL( cur_info2["last_votepay_share_update"].as_string(),
                        get_global_state3()["last_vpay_state_update"].as_string() );
   BOOST_REQUIRE_EQUAL( cur_info2_emily["last_votepay_share_update"].as_string(),
                        get_global_state3()["last_vpay_state_update"].as_string() );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(votepay_share_update_order, eosio_system_tester, * boost::unit_test::tolerance(1e-10)) try {

   cross_15_percent_threshold();

   const asset net = core_sym::from_string("80.0000");
   const asset cpu = core_sym::from_string("80.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n, "carolaccount"_n, "emilyaccount"_n };
   for (const auto& a: accounts) {
      create_account_with_resources( a, config::system_account_name, core_sym::from_string("1.0000"), false, net, cpu );
      transfer( config::system_account_name, a, core_sym::from_string("1000.0000"), config::system_account_name );
   }
   const auto alice = accounts[0];
   const auto bob   = accounts[1];
   const auto carol = accounts[2];
   const auto emily = accounts[3];

   BOOST_REQUIRE_EQUAL( success(), regproducer( carol ) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( emily ) );

   produce_block( fc::hours(24) );

   BOOST_REQUIRE_EQUAL( success(), stake( alice, core_sym::from_string("100.0000"), core_sym::from_string("100.0000") ) );
   BOOST_REQUIRE_EQUAL( success(), stake( bob,   core_sym::from_string("100.0000"), core_sym::from_string("100.0000") ) );

   BOOST_REQUIRE_EQUAL( success(), vote( alice, { carol, emily } ) );


   BOOST_REQUIRE_EQUAL( success(), push_action( carol, "claimrewards"_n, mvo()("owner", carol) ) );
   produce_block( fc::hours(1) );
   BOOST_REQUIRE_EQUAL( success(), push_action( emily, "claimrewards"_n, mvo()("owner", emily) ) );

   produce_block( fc::hours(3 * 24 + 1) );

   {
      signed_transaction trx;
      set_transaction_headers(trx);

      trx.actions.emplace_back( get_action( vaulta_account_name, "claimrewards"_n, { {carol, config::active_name} },
                                            mvo()("owner", carol) ) );

      std::vector<account_name> prods = { carol, emily };
      trx.actions.emplace_back( get_action( config::system_account_name, "voteproducer"_n, { {alice, config::active_name} },
                                            mvo()("voter", alice)("proxy", name(0))("producers", prods) ) );

      trx.actions.emplace_back( get_action( vaulta_account_name, "claimrewards"_n, { {emily, config::active_name} },
                                            mvo()("owner", emily) ) );

      trx.sign( get_private_key( carol, "active" ), control->get_chain_id() );
      trx.sign( get_private_key( alice, "active" ), control->get_chain_id() );
      trx.sign( get_private_key( emily, "active" ), control->get_chain_id() );

      push_transaction( trx );
   }

   const auto& carol_info  = get_producer_info(carol);
   const auto& carol_info2 = get_producer_info2(carol);
   const auto& emily_info  = get_producer_info(emily);
   const auto& emily_info2 = get_producer_info2(emily);
   const auto& gs3         = get_global_state3();
   BOOST_REQUIRE_EQUAL( carol_info2["last_votepay_share_update"].as_string(), gs3["last_vpay_state_update"].as_string() );
   BOOST_REQUIRE_EQUAL( emily_info2["last_votepay_share_update"].as_string(), gs3["last_vpay_state_update"].as_string() );
   BOOST_TEST_REQUIRE( 0  == carol_info2["votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( 0  == emily_info2["votepay_share"].as_double() );
   BOOST_REQUIRE( 0 < carol_info["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( carol_info["total_votes"].as_double() == emily_info["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( gs3["total_vpay_share_change_rate"].as_double() == 2 * carol_info["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(votepay_transition, eosio_system_tester, * boost::unit_test::tolerance(1e-10)) try {

   const asset net = core_sym::from_string("80.0000");
   const asset cpu = core_sym::from_string("80.0000");
   const std::vector<account_name> voters = { "producvotera"_n, "producvoterb"_n, "producvoterc"_n, "producvoterd"_n };
   for (const auto& v: voters) {
      create_account_with_resources( v, config::system_account_name, core_sym::from_string("1.0000"), false, net, cpu );
      transfer( config::system_account_name, v, core_sym::from_string("100000000.0000"), config::system_account_name );
      BOOST_REQUIRE_EQUAL(success(), stake(v, core_sym::from_string("30000000.0000"), core_sym::from_string("30000000.0000")) );
   }

   // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
   std::vector<account_name> producer_names;
   {
      producer_names.reserve('z' - 'a' + 1);
      {
         const std::string root("defproducer");
         for ( char c = 'a'; c <= 'd'; ++c ) {
            producer_names.emplace_back(root + std::string(1, c));
         }
      }
      setup_producer_accounts(producer_names);
      for (const auto& p: producer_names) {
         BOOST_REQUIRE_EQUAL( success(), regproducer(p) );
         BOOST_TEST_REQUIRE(0 == get_producer_info(p)["total_votes"].as_double());
         BOOST_TEST_REQUIRE(0 == get_producer_info2(p)["votepay_share"].as_double());
         BOOST_REQUIRE(0 < microseconds_since_epoch_of_iso_string( get_producer_info2(p)["last_votepay_share_update"] ));
      }
   }

   BOOST_REQUIRE_EQUAL( success(), vote("producvotera"_n, vector<account_name>(producer_names.begin(), producer_names.end())) );
   auto* tbl = control->db().find<eosio::chain::table_id_object, eosio::chain::by_code_scope_table>(
                  boost::make_tuple( config::system_account_name,
                                     config::system_account_name,
                                     "producers2"_n ) );
   BOOST_REQUIRE( tbl );
   BOOST_REQUIRE( 0 < microseconds_since_epoch_of_iso_string( get_producer_info2("defproducera")["last_votepay_share_update"] ) );

   // const_cast hack for now
   const_cast<chainbase::database&>(control->db()).remove( *tbl );
   tbl = control->db().find<eosio::chain::table_id_object, eosio::chain::by_code_scope_table>(
                  boost::make_tuple( config::system_account_name,
                                     config::system_account_name,
                                     "producers2"_n ) );
   BOOST_REQUIRE( !tbl );

   BOOST_REQUIRE_EQUAL( success(), vote("producvoterb"_n, vector<account_name>(producer_names.begin(), producer_names.end())) );
   tbl = control->db().find<eosio::chain::table_id_object, eosio::chain::by_code_scope_table>(
            boost::make_tuple( config::system_account_name,
                               config::system_account_name,
                               "producers2"_n ) );
   BOOST_REQUIRE( !tbl );
   BOOST_REQUIRE_EQUAL( success(), regproducer("defproducera"_n) );
   BOOST_REQUIRE( microseconds_since_epoch_of_iso_string( get_producer_info("defproducera"_n)["last_claim_time"] ) < microseconds_since_epoch_of_iso_string( get_producer_info2("defproducera"_n)["last_votepay_share_update"] ) );

   create_account_with_resources( "defproducer1"_n, config::system_account_name, core_sym::from_string("1.0000"), false, net, cpu );
   BOOST_REQUIRE_EQUAL( success(), regproducer("defproducer1"_n) );
   BOOST_REQUIRE( 0 < microseconds_since_epoch_of_iso_string( get_producer_info("defproducer1"_n)["last_claim_time"] ) );
   BOOST_REQUIRE_EQUAL( get_producer_info("defproducer1"_n)["last_claim_time"].as_string(),
                        get_producer_info2("defproducer1"_n)["last_votepay_share_update"].as_string() );

} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_CASE(votepay_transition2, * boost::unit_test::tolerance(1e-10)) try {
   eosio_system_tester t(eosio_system_tester::setup_level::minimal);

   std::string old_contract_core_symbol_name = "SYS"; // Set to core symbol used in contracts::util::system_wasm_old()
   symbol old_contract_core_symbol{::eosio::chain::string_to_symbol_c( 4, old_contract_core_symbol_name.c_str() )};

   auto old_core_from_string = [&]( const std::string& s ) {
      return eosio::chain::asset::from_string(s + " " + old_contract_core_symbol_name);
   };

   t.create_core_token( old_contract_core_symbol );
   t.set_code( config::system_account_name, contracts::util::system_wasm_old() );
   t.set_abi(  config::system_account_name, contracts::util::system_abi_old().data() );
   {
      const auto& accnt = t.control->db().get<account_object,by_name>( config::system_account_name );
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      t.abi_ser.set_abi(abi, abi_serializer::create_yield_function(eosio_system_tester::abi_serializer_max_time));
   }
   const asset net = old_core_from_string("80.0000");
   const asset cpu = old_core_from_string("80.0000");
   const std::vector<account_name> voters = { "producvotera"_n, "producvoterb"_n, "producvoterc"_n, "producvoterd"_n };
   for (const auto& v: voters) {
      t.create_account_with_resources( v, config::system_account_name, old_core_from_string("1.0000"), false, net, cpu );
      t.transfer( config::system_account_name, v, old_core_from_string("100000000.0000"), config::system_account_name );
      BOOST_REQUIRE_EQUAL(t.success(), t.stake(v, old_core_from_string("30000000.0000"), old_core_from_string("30000000.0000")) );
   }

   // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
   std::vector<account_name> producer_names;
   {
      producer_names.reserve('z' - 'a' + 1);
      {
         const std::string root("defproducer");
         for ( char c = 'a'; c <= 'd'; ++c ) {
            producer_names.emplace_back(root + std::string(1, c));
         }
      }
     t.setup_producer_accounts( producer_names, old_core_from_string("1.0000"),
                     old_core_from_string("80.0000"), old_core_from_string("80.0000") );
      for (const auto& p: producer_names) {
         BOOST_REQUIRE_EQUAL( t.success(), t.regproducer(p) );
         BOOST_TEST_REQUIRE(0 == t.get_producer_info(p)["total_votes"].as_double());
      }
   }

   BOOST_REQUIRE_EQUAL( t.success(), t.vote("producvotera"_n, vector<account_name>(producer_names.begin(), producer_names.end())) );
   t.produce_block( fc::hours(20) );
   BOOST_REQUIRE_EQUAL( t.success(), t.vote("producvoterb"_n, vector<account_name>(producer_names.begin(), producer_names.end())) );
   t.produce_block( fc::hours(30) );
   BOOST_REQUIRE_EQUAL( t.success(), t.vote("producvoterc"_n, vector<account_name>(producer_names.begin(), producer_names.end())) );
   BOOST_REQUIRE_EQUAL( t.success(), t.push_action(producer_names[0], "claimrewards"_n, mvo()("owner", producer_names[0])) );
   BOOST_REQUIRE_EQUAL( t.success(), t.push_action(producer_names[1], "claimrewards"_n, mvo()("owner", producer_names[1])) );
   auto* tbl = t.control->db().find<eosio::chain::table_id_object, eosio::chain::by_code_scope_table>(
                                    boost::make_tuple( config::system_account_name,
                                                       config::system_account_name,
                                                       "producers2"_n ) );
   BOOST_REQUIRE( !tbl );

   t.produce_block( fc::hours(2*24) );

   t.deploy_contract( false );

   t.produce_blocks(2);
   t.produce_block( fc::hours(24 + 1) );

   BOOST_REQUIRE_EQUAL( t.success(), t.push_action(producer_names[0], "claimrewards"_n, mvo()("owner", producer_names[0])) );
   BOOST_TEST_REQUIRE( 0 == t.get_global_state2()["total_producer_votepay_share"].as_double() );
   BOOST_TEST_REQUIRE( t.get_producer_info(producer_names[0])["total_votes"].as_double() == t.get_global_state3()["total_vpay_share_change_rate"].as_double() );

   t.produce_block( fc::hours(5) );

   BOOST_REQUIRE_EQUAL( t.success(), t.regproducer(producer_names[1]) );
   BOOST_TEST_REQUIRE( t.get_producer_info(producer_names[0])["total_votes"].as_double() + t.get_producer_info(producer_names[1])["total_votes"].as_double() ==
                       t.get_global_state3()["total_vpay_share_change_rate"].as_double() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE(producers_upgrade_system_contract, eosio_system_tester) try {
   //install multisig contract
   abi_serializer msig_abi_ser = initialize_multisig();
   auto producer_names = active_and_vote_producers();

   //change `default_max_inline_action_size` to 512 KB
   eosio::chain::chain_config params = control->get_global_properties().configuration;
   params.max_inline_action_size = 512 * 1024;
   base_tester::push_action( config::system_account_name, "setparams"_n, config::system_account_name, mutable_variant_object()
                              ("params", params) );

   produce_blocks();

   //helper function
   auto push_action_msig = [&]( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) -> action_result {
         string action_type_name = msig_abi_ser.get_action_type(name);

         action act;
         act.account = "eosio.msig"_n;
         act.name = name;
         act.data = msig_abi_ser.variant_to_binary( action_type_name, data, abi_serializer::create_yield_function(abi_serializer_max_time) );

         return base_tester::push_action( std::move(act), (auth ? signer : signer == "bob111111111"_n ? "alice1111111"_n : "bob111111111"_n).to_uint64_t() );
   };
   // test begins
   vector<permission_level> prod_perms;
   for ( auto& x : producer_names ) {
      prod_perms.push_back( { name(x), config::active_name } );
   }

   transaction trx;
   {
      //prepare system contract with different hash (contract differs in one byte)
      auto code = contracts::system_wasm();
      string msg = "producer votes must be unique and sorted";
      auto it = std::search( code.begin(), code.end(), msg.begin(), msg.end() );
      BOOST_REQUIRE( it != code.end() );
      msg[0] = 'P';
      std::copy( msg.begin(), msg.end(), it );

      fc::variant pretty_trx = fc::mutable_variant_object()
         ("expiration", "2020-01-01T00:30")
         ("ref_block_num", 2)
         ("ref_block_prefix", 3)
         ("net_usage_words", 0)
         ("max_cpu_usage_ms", 0)
         ("delay_sec", 0)
         ("actions", fc::variants({
               fc::mutable_variant_object()
                  ("account", name(config::system_account_name))
                  ("name", "setcode")
                  ("authorization", vector<permission_level>{ { config::system_account_name, config::active_name } })
                  ("data", fc::mutable_variant_object() ("account", name(config::system_account_name))
                   ("vmtype", 0)
                   ("vmversion", "0")
                   ("code", bytes( code.begin(), code.end() ))
                  )
                  })
         );
      abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer::create_yield_function(abi_serializer_max_time));
   }

   BOOST_REQUIRE_EQUAL(success(), push_action_msig( "alice1111111"_n, "propose"_n, mvo()
                                                    ("proposer",      "alice1111111")
                                                    ("proposal_name", "upgrade1")
                                                    ("trx",           trx)
                                                    ("requested", prod_perms)
                       )
   );

   // get 15 approvals
   for ( size_t i = 0; i < 14; ++i ) {
      BOOST_REQUIRE_EQUAL(success(), push_action_msig( name(producer_names[i]), "approve"_n, mvo()
                                                       ("proposer",      "alice1111111")
                                                       ("proposal_name", "upgrade1")
                                                       ("level",         permission_level{ name(producer_names[i]), config::active_name })
                          )
      );
   }

   //should fail
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("transaction authorization failed"),
                       push_action_msig( "alice1111111"_n, "exec"_n, mvo()
                                         ("proposer",      "alice1111111")
                                         ("proposal_name", "upgrade1")
                                         ("executer",      "alice1111111")
                       )
   );

   // one more approval
   BOOST_REQUIRE_EQUAL(success(), push_action_msig( name(producer_names[14]), "approve"_n, mvo()
                                                    ("proposer",      "alice1111111")
                                                    ("proposal_name", "upgrade1")
                                                    ("level",         permission_level{ name(producer_names[14]), config::active_name })
                          )
   );

   transaction_trace_ptr trace;
   control->applied_transaction().connect(
   [&]( std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> p ) {
      trace = std::get<0>(p);
   } );

   BOOST_REQUIRE_EQUAL(success(), push_action_msig( "alice1111111"_n, "exec"_n, mvo()
                                                    ("proposer",      "alice1111111")
                                                    ("proposal_name", "upgrade1")
                                                    ("executer",      "alice1111111")
                       )
   );

   BOOST_REQUIRE( bool(trace) );
   BOOST_REQUIRE_EQUAL( 1, trace->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace->receipt->status );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(producer_onblock_check, eosio_system_tester) try {

   const asset large_asset = core_sym::from_string("80.0000");
   create_account_with_resources( "producvotera"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );
   create_account_with_resources( "producvoterb"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );
   create_account_with_resources( "producvoterc"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );

   // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
   std::vector<account_name> producer_names;
   producer_names.reserve('z' - 'a' + 1);
   const std::string root("defproducer");
   for ( char c = 'a'; c <= 'z'; ++c ) {
      producer_names.emplace_back(root + std::string(1, c));
   }
   setup_producer_accounts(producer_names);

   for (auto a:producer_names)
      regproducer(a);

   produce_block(fc::hours(24));

   BOOST_REQUIRE_EQUAL(0, get_producer_info( producer_names.front() )["total_votes"].as<double>());
   BOOST_REQUIRE_EQUAL(0, get_producer_info( producer_names.back() )["total_votes"].as<double>());


   transfer(config::system_account_name, "producvotera", core_sym::from_string("200000000.0000"), config::system_account_name);
   BOOST_REQUIRE_EQUAL(success(), stake("producvotera", core_sym::from_string("70000000.0000"), core_sym::from_string("70000000.0000") ));
   BOOST_REQUIRE_EQUAL(success(), vote( "producvotera"_n, vector<account_name>(producer_names.begin(), producer_names.begin()+10)));
   BOOST_CHECK_EQUAL( wasm_assert_msg( "cannot undelegate bandwidth until the chain is activated (at least 15% of all tokens participate in voting)" ),
                      unstake( "producvotera", core_sym::from_string("50.0000"), core_sym::from_string("50.0000") ) );

   // give a chance for everyone to produce blocks
   {
      produce_blocks(21 * 12);
      bool all_21_produced = true;
      for (uint32_t i = 0; i < 21; ++i) {
         if (0 == get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            all_21_produced= false;
         }
      }
      bool rest_didnt_produce = true;
      for (uint32_t i = 21; i < producer_names.size(); ++i) {
         if (0 < get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            rest_didnt_produce = false;
         }
      }
      BOOST_REQUIRE_EQUAL(false, all_21_produced);
      BOOST_REQUIRE_EQUAL(true, rest_didnt_produce);
   }

   {
      const char* claimrewards_activation_error_message = "cannot claim rewards until the chain is activated (at least 15% of all tokens participate in voting)";
      BOOST_CHECK_EQUAL(0, get_global_state()["total_unpaid_blocks"].as<uint32_t>());
      BOOST_REQUIRE_EQUAL(wasm_assert_msg( claimrewards_activation_error_message ),
                          push_action(producer_names.front(), "claimrewards"_n, mvo()("owner", producer_names.front())));
      BOOST_REQUIRE_EQUAL(0, get_balance(producer_names.front()).get_amount());
      BOOST_REQUIRE_EQUAL(wasm_assert_msg( claimrewards_activation_error_message ),
                          push_action(producer_names.back(), "claimrewards"_n, mvo()("owner", producer_names.back())));
      BOOST_REQUIRE_EQUAL(0, get_balance(producer_names.back()).get_amount());
   }

   // stake across 15% boundary
   transfer(config::system_account_name, "producvoterb", core_sym::from_string("100000000.0000"), config::system_account_name);
   BOOST_REQUIRE_EQUAL(success(), stake("producvoterb", core_sym::from_string("4000000.0000"), core_sym::from_string("4000000.0000")));
   transfer(config::system_account_name, "producvoterc", core_sym::from_string("100000000.0000"), config::system_account_name);
   BOOST_REQUIRE_EQUAL(success(), stake("producvoterc", core_sym::from_string("2000000.0000"), core_sym::from_string("2000000.0000")));

   BOOST_REQUIRE_EQUAL(success(), vote( "producvoterb"_n, vector<account_name>(producer_names.begin(), producer_names.begin()+21)));
   BOOST_REQUIRE_EQUAL(success(), vote( "producvoterc"_n, vector<account_name>(producer_names.begin(), producer_names.end())));

   int retries = 50;
   while (produce_block()->producer == config::system_account_name && --retries);
   BOOST_REQUIRE(retries > 0);

   // give a chance for everyone to produce blocks
   {
      produce_blocks(21 * 12);
      bool all_21_produced = true;
      for (uint32_t i = 0; i < 21; ++i) {
         if (0 == get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            all_21_produced= false;
         }
      }
      bool rest_didnt_produce = true;
      for (uint32_t i = 21; i < producer_names.size(); ++i) {
         if (0 < get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            rest_didnt_produce = false;
         }
      }
      BOOST_REQUIRE_EQUAL(true, all_21_produced);
      BOOST_REQUIRE_EQUAL(true, rest_didnt_produce);
      BOOST_REQUIRE_EQUAL(success(),
                          push_action(producer_names.front(), "claimrewards"_n, mvo()("owner", producer_names.front())));
      BOOST_REQUIRE(0 < get_balance(producer_names.front()).get_amount());
   }

   BOOST_CHECK_EQUAL( success(), unstake( "producvotera", core_sym::from_string("50.0000"), core_sym::from_string("50.0000") ) );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( voters_actions_affect_proxy_and_producers, eosio_system_tester, * boost::unit_test::tolerance(1e+6) ) try {
   cross_15_percent_threshold();

   create_accounts_with_resources( { "donald111111"_n, "defproducer1"_n, "defproducer2"_n, "defproducer3"_n } );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer1"_n, 1) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer2"_n, 2) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer3"_n, 3) );

   //alice1111111 becomes a producer
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproxy"_n, mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy", true)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111"_n ), get_voter_info( "alice1111111" ) );

   //alice1111111 makes stake and votes
   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", core_sym::from_string("30.0001"), core_sym::from_string("20.0001") ) );
   BOOST_REQUIRE_EQUAL( success(), vote( "alice1111111"_n, { "defproducer1"_n, "defproducer2"_n } ) );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("50.0002")) == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("50.0002")) == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   BOOST_REQUIRE_EQUAL( success(), push_action( "donald111111"_n, "regproxy"_n, mvo()
                                                ("proxy",  "donald111111")
                                                ("isproxy", true)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "donald111111"_n ), get_voter_info( "donald111111" ) );

   //bob111111111 chooses alice1111111 as a proxy
   issue_and_transfer( "bob111111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", core_sym::from_string("100.0002"), core_sym::from_string("50.0001") ) );
   BOOST_REQUIRE_EQUAL( success(), vote( "bob111111111"_n, vector<account_name>(), "alice1111111" ) );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("150.0003")) == get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("200.0005")) == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("200.0005")) == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //carol1111111 chooses alice1111111 as a proxy
   issue_and_transfer( "carol1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol1111111", core_sym::from_string("30.0001"), core_sym::from_string("20.0001") ) );
   BOOST_REQUIRE_EQUAL( success(), vote( "carol1111111"_n, vector<account_name>(), "alice1111111" ) );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("200.0005")) == get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("250.0007")) == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("250.0007")) == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //proxied voter carol1111111 increases stake
   BOOST_REQUIRE_EQUAL( success(), stake( "carol1111111", core_sym::from_string("50.0000"), core_sym::from_string("70.0000") ) );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("320.0005")) == get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("370.0007")) == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("370.0007")) == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //proxied voter bob111111111 decreases stake
   BOOST_REQUIRE_EQUAL( success(), unstake( "bob111111111", core_sym::from_string("50.0001"), core_sym::from_string("50.0001") ) );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("220.0003")) == get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("270.0005")) == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("270.0005")) == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //proxied voter carol1111111 chooses another proxy
   BOOST_REQUIRE_EQUAL( success(), vote( "carol1111111"_n, vector<account_name>(), "donald111111" ) );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("50.0001")), get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("170.0002")), get_voter_info( "donald111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("100.0003")), get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("100.0003")), get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //bob111111111 switches to direct voting and votes for one of the same producers, but not for another one
   BOOST_REQUIRE_EQUAL( success(), vote( "bob111111111"_n, { "defproducer2"_n } ) );
   BOOST_TEST_REQUIRE( 0.0 == get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE(  stake2votes(core_sym::from_string("50.0002")), get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes(core_sym::from_string("100.0003")), get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 0.0 == get_producer_info( "defproducer3" )["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE(eosio_system_part4_tests)

BOOST_FIXTURE_TEST_CASE( vote_both_proxy_and_producers, eosio_system_tester ) try {
   //alice1111111 becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproxy"_n, mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy", true)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111"_n ), get_voter_info( "alice1111111" ) );

   //carol1111111 becomes a producer
   BOOST_REQUIRE_EQUAL( success(), regproducer( "carol1111111"_n, 1) );

   //bob111111111 chooses alice1111111 as a proxy

   issue_and_transfer( "bob111111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", core_sym::from_string("100.0002"), core_sym::from_string("50.0001") ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("cannot vote for producers and proxy at same time"),
                        vote( "bob111111111"_n, { "carol1111111"_n }, "alice1111111" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( select_invalid_proxy, eosio_system_tester ) try {
   //accumulate proxied votes
   issue_and_transfer( "bob111111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", core_sym::from_string("100.0002"), core_sym::from_string("50.0001") ) );

   //selecting account not registered as a proxy
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "invalid proxy specified" ),
                        vote( "bob111111111"_n, vector<account_name>(), "alice1111111" ) );

   //selecting not existing account as a proxy
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "invalid proxy specified" ),
                        vote( "bob111111111"_n, vector<account_name>(), "notexist" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( double_register_unregister_proxy_keeps_votes, eosio_system_tester ) try {
   //alice1111111 becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproxy"_n, mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy",  1)
                        )
   );
   issue_and_transfer( "alice1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", core_sym::from_string("5.0000"), core_sym::from_string("5.0000") ) );
   edump((get_voter_info("alice1111111")));
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111"_n )( "staked", 100000 ), get_voter_info( "alice1111111" ) );

   //bob111111111 stakes and selects alice1111111 as a proxy
   issue_and_transfer( "bob111111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", core_sym::from_string("100.0002"), core_sym::from_string("50.0001") ) );
   BOOST_REQUIRE_EQUAL( success(), vote( "bob111111111"_n, vector<account_name>(), "alice1111111" ) );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111"_n )( "proxied_vote_weight", stake2votes( core_sym::from_string("150.0003") ))( "staked", 100000 ), get_voter_info( "alice1111111" ) );

   //double regestering should fail without affecting total votes and stake
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "action has no effect" ),
                        push_action( "alice1111111"_n, "regproxy"_n, mvo()
                                     ("proxy",  "alice1111111")
                                     ("isproxy",  1)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111"_n )( "proxied_vote_weight", stake2votes(core_sym::from_string("150.0003")) )( "staked", 100000 ), get_voter_info( "alice1111111" ) );

   //uregister
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproxy"_n, mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy",  0)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111" )( "proxied_vote_weight", stake2votes(core_sym::from_string("150.0003")) )( "staked", 100000 ), get_voter_info( "alice1111111" ) );

   //double unregistering should not affect proxied_votes and stake
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "action has no effect" ),
                        push_action( "alice1111111"_n, "regproxy"_n, mvo()
                                     ("proxy",  "alice1111111")
                                     ("isproxy",  0)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111" )( "proxied_vote_weight", stake2votes(core_sym::from_string("150.0003")))( "staked", 100000 ), get_voter_info( "alice1111111" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_cannot_use_another_proxy, eosio_system_tester ) try {
   //alice1111111 becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( "alice1111111"_n, "regproxy"_n, mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy",  1)
                        )
   );

   //bob111111111 becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( "bob111111111"_n, "regproxy"_n, mvo()
                                                ("proxy",  "bob111111111")
                                                ("isproxy",  1)
                        )
   );

   //proxy should not be able to use a proxy
   issue_and_transfer( "bob111111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", core_sym::from_string("100.0002"), core_sym::from_string("50.0001") ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "account registered as a proxy is not allowed to use a proxy" ),
                        vote( "bob111111111"_n, vector<account_name>(), "alice1111111" ) );

   //voter that uses a proxy should not be allowed to become a proxy
   issue_and_transfer( "carol1111111", core_sym::from_string("1000.0000"),  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol1111111", core_sym::from_string("100.0002"), core_sym::from_string("50.0001") ) );
   BOOST_REQUIRE_EQUAL( success(), vote( "carol1111111"_n, vector<account_name>(), "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "account that uses a proxy is not allowed to become a proxy" ),
                        push_action( "carol1111111"_n, "regproxy"_n, mvo()
                                     ("proxy",  "carol1111111")
                                     ("isproxy",  1)
                        )
   );

   //proxy should not be able to use itself as a proxy
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "cannot proxy to self" ),
                        vote( "bob111111111"_n, vector<account_name>(), "bob111111111" ) );

} FC_LOG_AND_RETHROW()

fc::mutable_variant_object config_to_variant( const eosio::chain::chain_config& config ) {
   return mutable_variant_object()
      ( "max_block_net_usage", config.max_block_net_usage )
      ( "target_block_net_usage_pct", config.target_block_net_usage_pct )
      ( "max_transaction_net_usage", config.max_transaction_net_usage )
      ( "base_per_transaction_net_usage", config.base_per_transaction_net_usage )
      ( "context_free_discount_net_usage_num", config.context_free_discount_net_usage_num )
      ( "context_free_discount_net_usage_den", config.context_free_discount_net_usage_den )
      ( "max_block_cpu_usage", config.max_block_cpu_usage )
      ( "target_block_cpu_usage_pct", config.target_block_cpu_usage_pct )
      ( "max_transaction_cpu_usage", config.max_transaction_cpu_usage )
      ( "min_transaction_cpu_usage", config.min_transaction_cpu_usage )
      ( "max_transaction_lifetime", config.max_transaction_lifetime )
      ( "deferred_trx_expiration_window", config.deferred_trx_expiration_window )
      ( "max_transaction_delay", config.max_transaction_delay )
      ( "max_inline_action_size", config.max_inline_action_size )
      ( "max_inline_action_depth", config.max_inline_action_depth )
      ( "max_authority_depth", config.max_authority_depth );
}

BOOST_FIXTURE_TEST_CASE( elect_producers /*_and_parameters*/, eosio_system_tester ) try {
   create_accounts_with_resources( {  "defproducer1"_n, "defproducer2"_n, "defproducer3"_n } );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer1"_n, 1) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer2"_n, 2) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer3"_n, 3) );

   //stake more than 15% of total SYS supply to activate chain
   transfer( "eosio", "alice1111111", core_sym::from_string("600000000.0000"), "eosio" );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "alice1111111", core_sym::from_string("300000000.0000"), core_sym::from_string("300000000.0000") ) );
   //vote for producers
   BOOST_REQUIRE_EQUAL( success(), vote( "alice1111111"_n, { "defproducer1"_n } ) );
   produce_blocks(250);
   auto producer_schedule = control->active_producers();
   BOOST_REQUIRE_EQUAL( 1, producer_schedule.producers.size() );
   BOOST_REQUIRE_EQUAL( name("defproducer1"), producer_schedule.producers[0].producer_name );

   //auto config = config_to_variant( control->get_global_properties().configuration );
   //auto prod1_config = testing::filter_fields( config, producer_parameters_example( 1 ) );
   //REQUIRE_EQUAL_OBJECTS(prod1_config, config);

   // elect 2 producers
   issue_and_transfer( "bob111111111", core_sym::from_string("80000.0000"),  config::system_account_name );
   ilog("stake");
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", core_sym::from_string("40000.0000"), core_sym::from_string("40000.0000") ) );
   ilog("start vote");
   BOOST_REQUIRE_EQUAL( success(), vote( "bob111111111"_n, { "defproducer2"_n } ) );
   ilog(".");
   produce_blocks(250);
   producer_schedule = control->active_producers();
   BOOST_REQUIRE_EQUAL( 2, producer_schedule.producers.size() );
   BOOST_REQUIRE_EQUAL( name("defproducer1"), producer_schedule.producers[0].producer_name );
   BOOST_REQUIRE_EQUAL( name("defproducer2"), producer_schedule.producers[1].producer_name );
   //config = config_to_variant( control->get_global_properties().configuration );
   //auto prod2_config = testing::filter_fields( config, producer_parameters_example( 2 ) );
   //REQUIRE_EQUAL_OBJECTS(prod2_config, config);

   // elect 3 producers
   BOOST_REQUIRE_EQUAL( success(), vote( "bob111111111"_n, { "defproducer2"_n, "defproducer3"_n } ) );
   produce_blocks(250);
   producer_schedule = control->active_producers();
   BOOST_REQUIRE_EQUAL( 3, producer_schedule.producers.size() );
   BOOST_REQUIRE_EQUAL( name("defproducer1"), producer_schedule.producers[0].producer_name );
   BOOST_REQUIRE_EQUAL( name("defproducer2"), producer_schedule.producers[1].producer_name );
   BOOST_REQUIRE_EQUAL( name("defproducer3"), producer_schedule.producers[2].producer_name );
   //config = config_to_variant( control->get_global_properties().configuration );
   //REQUIRE_EQUAL_OBJECTS(prod2_config, config);

   // try to go back to 2 producers and fail
   BOOST_REQUIRE_EQUAL( success(), vote( "bob111111111"_n, { "defproducer3"_n } ) );
   produce_blocks(250);
   producer_schedule = control->active_producers();
   BOOST_REQUIRE_EQUAL( 3, producer_schedule.producers.size() );

   // The test below is invalid now, producer schedule is not updated if there are
   // fewer producers in the new schedule
   /*
   BOOST_REQUIRE_EQUAL( 2, producer_schedule.size() );
   BOOST_REQUIRE_EQUAL( name("defproducer1"), producer_schedule[0].producer_name );
   BOOST_REQUIRE_EQUAL( name("defproducer3"), producer_schedule[1].producer_name );
   //config = config_to_variant( control->get_global_properties().configuration );
   //auto prod3_config = testing::filter_fields( config, producer_parameters_example( 3 ) );
   //REQUIRE_EQUAL_OBJECTS(prod3_config, config);
   */

} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE(eosio_system_name_tests)

BOOST_FIXTURE_TEST_CASE( buyname, eosio_system_tester ) try {
   create_accounts_with_resources( { "dan"_n, "sam"_n } );
   transfer( config::system_account_name, "dan", core_sym::from_string( "10000.0000" ) );
   transfer( config::system_account_name, "sam", core_sym::from_string( "10000.0000" ) );
   stake_with_transfer( config::system_account_name, "sam"_n, core_sym::from_string( "80000000.0000" ), core_sym::from_string( "80000000.0000" ) );
   stake_with_transfer( config::system_account_name, "dan"_n, core_sym::from_string( "80000000.0000" ), core_sym::from_string( "80000000.0000" ) );

   regproducer( config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), vote( "sam"_n, { config::system_account_name } ) );
   // wait 14 days after min required amount has been staked
   produce_block( fc::days(7) );
   BOOST_REQUIRE_EQUAL( success(), vote( "dan"_n, { config::system_account_name } ) );
   produce_block( fc::days(7) );
   produce_block();

   BOOST_REQUIRE_EXCEPTION( create_accounts_with_resources( { "fail"_n }, "dan"_n ), // dan shouldn't be able to create fail
                            eosio_assert_message_exception, eosio_assert_message_is( "no active bid for name: fail" ) );
   bidname( "dan", "nofail", core_sym::from_string( "1.0000" ) );
   BOOST_REQUIRE_EQUAL( "assertion failure with message: must increase bid by 10%", bidname( "sam", "nofail", core_sym::from_string( "1.0000" ) )); // didn't increase bid by 10%
   BOOST_REQUIRE_EQUAL( success(), bidname( "sam", "nofail", core_sym::from_string( "2.0000" ) )); // didn't increase bid by 10%
   produce_block( fc::days(1) );
   produce_block();

   BOOST_REQUIRE_EXCEPTION( create_accounts_with_resources( { "nofail"_n }, "dan"_n ), // dan shoudn't be able to do this, sam won
                            eosio_assert_message_exception, eosio_assert_message_is( "only highest bidder can claim" ) );
   //wlog( "verify sam can create nofail" );
   create_accounts_with_resources( { "nofail"_n }, "sam"_n ); // sam should be able to do this, he won the bid
   //wlog( "verify nofail can create test.nofail" );
   transfer( "eosio", "nofail", core_sym::from_string( "1000.0000" ) );
   create_accounts_with_resources( { "test.nofail"_n }, "nofail"_n ); // only nofail can create test.nofail
   //wlog( "verify dan cannot create test.fail" );
   BOOST_REQUIRE_EXCEPTION( create_accounts_with_resources( { "test.fail"_n }, "dan"_n ), // dan shouldn't be able to do this
                            eosio_assert_message_exception, eosio_assert_message_is( "only fail may create test.fail" ) );

   create_accounts_with_resources( { "goodgoodgood"_n }, "dan"_n ); /// 12 char names should succeed
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( bid_invalid_names, eosio_system_tester ) try {
   create_accounts_with_resources( { "dan"_n } );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "you can only bid on top-level suffix" ),
                        bidname( "dan", "abcdefg.12345", core_sym::from_string( "1.0000" ) ) );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "the empty name is not a valid account name to bid on" ),
                        bidname( "dan", "", core_sym::from_string( "1.0000" ) ) );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "13 character names are not valid account names to bid on" ),
                        bidname( "dan", "abcdefgh12345", core_sym::from_string( "1.0000" ) ) );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "accounts with 12 character names and no dots can be created without bidding required" ),
                        bidname( "dan", "abcdefg12345", core_sym::from_string( "1.0000" ) ) );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( multiple_namebids, eosio_system_tester ) try {

   const std::string not_closed_message("auction for name is not closed yet");

   std::vector<account_name> accounts = { "alice"_n, "bob"_n, "carl"_n, "david"_n, "eve"_n };
   create_accounts_with_resources( accounts );
   for ( const auto& a: accounts ) {
      transfer( config::system_account_name, a, core_sym::from_string( "10000.0000" ) );
      BOOST_REQUIRE_EQUAL( core_sym::from_string( "10000.0000" ), get_balance(a) );
   }
   create_accounts_with_resources( { "producer"_n } );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer"_n ) );

   produce_block();
   // stake but not enough to go live
   stake_with_transfer( config::system_account_name, "bob"_n,  core_sym::from_string( "35000000.0000" ), core_sym::from_string( "35000000.0000" ) );
   stake_with_transfer( config::system_account_name, "carl"_n, core_sym::from_string( "35000000.0000" ), core_sym::from_string( "35000000.0000" ) );
   BOOST_REQUIRE_EQUAL( success(), vote( "bob"_n, { "producer"_n } ) );
   BOOST_REQUIRE_EQUAL( success(), vote( "carl"_n, { "producer"_n } ) );

   // start bids
   bidname( "bob",  "prefa", core_sym::from_string("1.0003") );
   BOOST_REQUIRE_EQUAL( core_sym::from_string( "9998.9997" ), get_balance("bob") );
   bidname( "bob",  "prefb", core_sym::from_string("1.0000") );
   bidname( "bob",  "prefc", core_sym::from_string("1.0000") );
   BOOST_REQUIRE_EQUAL( core_sym::from_string( "9996.9997" ), get_balance("bob") );

   bidname( "carl", "prefd", core_sym::from_string("1.0000") );
   bidname( "carl", "prefe", core_sym::from_string("1.0000") );
   BOOST_REQUIRE_EQUAL( core_sym::from_string( "9998.0000" ), get_balance("carl") );

   BOOST_REQUIRE_EQUAL( error("assertion failure with message: account is already highest bidder"),
                        bidname( "bob", "prefb", core_sym::from_string("1.1001") ) );
   BOOST_REQUIRE_EQUAL( error("assertion failure with message: must increase bid by 10%"),
                        bidname( "alice", "prefb", core_sym::from_string("1.0999") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string( "9996.9997" ), get_balance("bob") );
   BOOST_REQUIRE_EQUAL( core_sym::from_string( "10000.0000" ), get_balance("alice") );


   // alice outbids bob on prefb
   {
      const asset initial_names_balance = get_balance("eosio.names"_n);
      BOOST_REQUIRE_EQUAL( success(),
                           bidname( "alice", "prefb", core_sym::from_string("1.1001") ) );
      // refund bob's failed bid on prefb
      BOOST_REQUIRE_EQUAL( success(), push_action( "bob"_n, "bidrefund"_n, mvo()("bidder","bob")("newname", "prefb") ) );
      BOOST_REQUIRE_EQUAL( core_sym::from_string( "9997.9997" ), get_balance("bob") );
      BOOST_REQUIRE_EQUAL( core_sym::from_string( "9998.8999" ), get_balance("alice") );
      BOOST_REQUIRE_EQUAL( initial_names_balance + core_sym::from_string("0.1001"), get_balance("eosio.names"_n) );
   }

   // david outbids carl on prefd
   {
      BOOST_REQUIRE_EQUAL( core_sym::from_string( "9998.0000" ), get_balance("carl") );
      BOOST_REQUIRE_EQUAL( core_sym::from_string( "10000.0000" ), get_balance("david") );
      BOOST_REQUIRE_EQUAL( success(),
                           bidname( "david", "prefd", core_sym::from_string("1.9900") ) );
      // refund carls's failed bid on prefd
      BOOST_REQUIRE_EQUAL( success(), push_action( "carl"_n, "bidrefund"_n, mvo()("bidder","carl")("newname", "prefd") ) );
      BOOST_REQUIRE_EQUAL( core_sym::from_string( "9999.0000" ), get_balance("carl") );
      BOOST_REQUIRE_EQUAL( core_sym::from_string( "9998.0100" ), get_balance("david") );
   }

   // eve outbids carl on prefe
   {
      BOOST_REQUIRE_EQUAL( success(),
                           bidname( "eve", "prefe", core_sym::from_string("1.7200") ) );
   }

   produce_block( fc::days(14) );
   produce_block();

   // highest bid is from david for prefd but no bids can be closed yet
   BOOST_REQUIRE_EXCEPTION( create_account_with_resources( "prefd"_n, "david"_n ),
                            fc::exception, fc_assert_exception_message_is( not_closed_message ) );

   // stake enough to go above the 15% threshold
   stake_with_transfer( config::system_account_name, "alice"_n, core_sym::from_string( "10000000.0000" ), core_sym::from_string( "10000000.0000" ) );
   BOOST_REQUIRE_EQUAL(0, get_producer_info("producer")["unpaid_blocks"].as<uint32_t>());
   BOOST_REQUIRE_EQUAL( success(), vote( "alice"_n, { "producer"_n } ) );

   // need to wait for 14 days after going live
   produce_blocks(10);
   produce_block( fc::days(2) );
   produce_blocks( 10 );
   BOOST_REQUIRE_EXCEPTION( create_account_with_resources( "prefd"_n, "david"_n ),
                            fc::exception, fc_assert_exception_message_is( not_closed_message ) );
   // it's been 14 days, auction for prefd has been closed
   produce_block( fc::days(12) );
   create_account_with_resources( "prefd"_n, "david"_n );
   produce_blocks(2);
   produce_block( fc::hours(23) );
   // auctions for prefa, prefb, prefc, prefe haven't been closed
   BOOST_REQUIRE_EXCEPTION( create_account_with_resources( "prefa"_n, "bob"_n ),
                            fc::exception, fc_assert_exception_message_is( not_closed_message ) );
   BOOST_REQUIRE_EXCEPTION( create_account_with_resources( "prefb"_n, "alice"_n ),
                            fc::exception, fc_assert_exception_message_is( not_closed_message ) );
   BOOST_REQUIRE_EXCEPTION( create_account_with_resources( "prefc"_n, "bob"_n ),
                            fc::exception, fc_assert_exception_message_is( not_closed_message ) );
   BOOST_REQUIRE_EXCEPTION( create_account_with_resources( "prefe"_n, "eve"_n ),
                            fc::exception, fc_assert_exception_message_is( not_closed_message ) );
   // attemp to create account with no bid
   BOOST_REQUIRE_EXCEPTION( create_account_with_resources( "prefg"_n, "alice"_n ),
                            fc::exception, fc_assert_exception_message_is( "no active bid for name: prefg" ) );
   // changing highest bid pushes auction closing time by 24 hours
   BOOST_REQUIRE_EQUAL( success(),
                        bidname( "eve",  "prefb", core_sym::from_string("2.1880") ) );

   produce_block( fc::hours(22) );
   produce_blocks(2);

   BOOST_REQUIRE_EXCEPTION( create_account_with_resources( "prefb"_n, "eve"_n ),
                            fc::exception, fc_assert_exception_message_is( not_closed_message ) );
   // but changing a bid that is not the highest does not push closing time
   BOOST_REQUIRE_EQUAL( success(),
                        bidname( "carl", "prefe", core_sym::from_string("2.0980") ) );
   produce_block( fc::hours(2) );
   produce_blocks(2);
   // bid for prefb has closed, only highest bidder can claim
   BOOST_REQUIRE_EXCEPTION( create_account_with_resources( "prefb"_n, "alice"_n ),
                            eosio_assert_message_exception, eosio_assert_message_is( "only highest bidder can claim" ) );
   BOOST_REQUIRE_EXCEPTION( create_account_with_resources( "prefb"_n, "carl"_n ),
                            eosio_assert_message_exception, eosio_assert_message_is( "only highest bidder can claim" ) );
   create_account_with_resources( "prefb"_n, "eve"_n );

   BOOST_REQUIRE_EXCEPTION( create_account_with_resources( "prefe"_n, "carl"_n ),
                            fc::exception, fc_assert_exception_message_is( not_closed_message ) );
   produce_block();
   produce_block( fc::hours(24) );
   // by now bid for prefe has closed
   create_account_with_resources( "prefe"_n, "carl"_n );
   // prefe can now create *.prefe
   BOOST_REQUIRE_EXCEPTION( create_account_with_resources( "xyz.prefe"_n, "carl"_n ),
                            fc::exception, fc_assert_exception_message_is("only prefe may create xyz.prefe") );
   transfer( config::system_account_name, "prefe"_n, core_sym::from_string("10000.0000") );
   create_account_with_resources( "xyz.prefe"_n, "prefe"_n );

   // other auctions haven't closed
   BOOST_REQUIRE_EXCEPTION( create_account_with_resources( "prefa"_n, "bob"_n ),
                            fc::exception, fc_assert_exception_message_is( not_closed_message ) );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( namebid_pending_winner, eosio_system_tester ) try {
   cross_15_percent_threshold();
   produce_block( fc::hours(14*24) );    //wait 14 day for name auction activation
   transfer( config::system_account_name, "alice1111111"_n, core_sym::from_string("10000.0000") );
   transfer( config::system_account_name, "bob111111111"_n, core_sym::from_string("10000.0000") );

   BOOST_REQUIRE_EQUAL( success(), bidname( "alice1111111", "prefa", core_sym::from_string( "50.0000" ) ));
   BOOST_REQUIRE_EQUAL( success(), bidname( "bob111111111", "prefb", core_sym::from_string( "30.0000" ) ));
   produce_block( fc::hours(100) ); //should close "perfa"
   produce_block( fc::hours(100) ); //should close "perfb"

   //despite "perfa" account hasn't been created, we should be able to create "perfb" account
   create_account_with_resources( "prefb"_n, "bob111111111"_n );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( vote_producers_in_and_out, eosio_system_tester ) try {

   const asset net = core_sym::from_string("80.0000");
   const asset cpu = core_sym::from_string("80.0000");
   std::vector<account_name> voters = { "producvotera"_n, "producvoterb"_n, "producvoterc"_n, "producvoterd"_n };
   for (const auto& v: voters) {
      create_account_with_resources(v, config::system_account_name, core_sym::from_string("1.0000"), false, net, cpu);
   }

   // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
   std::vector<account_name> producer_names;
   {
      producer_names.reserve('z' - 'a' + 1);
      const std::string root("defproducer");
      for ( char c = 'a'; c <= 'z'; ++c ) {
         producer_names.emplace_back(root + std::string(1, c));
      }
      setup_producer_accounts(producer_names);
      for (const auto& p: producer_names) {
         BOOST_REQUIRE_EQUAL( success(), regproducer(p) );
         produce_blocks(1);
         ilog( "------ get pro----------" );
         wdump((p));
         BOOST_TEST(0 == get_producer_info(p)["total_votes"].as<double>());
      }
   }

   for (const auto& v: voters) {
      transfer( config::system_account_name, v, core_sym::from_string("200000000.0000"), config::system_account_name );
      BOOST_REQUIRE_EQUAL(success(), stake(v, core_sym::from_string("30000000.0000"), core_sym::from_string("30000000.0000")) );
   }

   {
      BOOST_REQUIRE_EQUAL(success(), vote("producvotera"_n, vector<account_name>(producer_names.begin(), producer_names.begin()+20)));
      BOOST_REQUIRE_EQUAL(success(), vote("producvoterb"_n, vector<account_name>(producer_names.begin(), producer_names.begin()+21)));
      BOOST_REQUIRE_EQUAL(success(), vote("producvoterc"_n, vector<account_name>(producer_names.begin(), producer_names.end())));
   }

   // give a chance for everyone to produce blocks
   {
      produce_blocks(23 * 12 + 20);
      bool all_21_produced = true;
      for (uint32_t i = 0; i < 21; ++i) {
         if (0 == get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            all_21_produced = false;
         }
      }
      bool rest_didnt_produce = true;
      for (uint32_t i = 21; i < producer_names.size(); ++i) {
         if (0 < get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            rest_didnt_produce = false;
         }
      }
      BOOST_REQUIRE(all_21_produced && rest_didnt_produce);
   }

   {
      produce_block(fc::hours(7));
      const uint32_t voted_out_index = 20;
      const uint32_t new_prod_index  = 23;
      BOOST_REQUIRE_EQUAL(success(), stake("producvoterd", core_sym::from_string("40000000.0000"), core_sym::from_string("40000000.0000")));
      BOOST_REQUIRE_EQUAL(success(), vote("producvoterd"_n, { producer_names[new_prod_index] }));
      BOOST_REQUIRE_EQUAL(0, get_producer_info(producer_names[new_prod_index])["unpaid_blocks"].as<uint32_t>());
      produce_blocks(4 * 12 * 21);
      BOOST_REQUIRE(0 < get_producer_info(producer_names[new_prod_index])["unpaid_blocks"].as<uint32_t>());
      const uint32_t initial_unpaid_blocks = get_producer_info(producer_names[voted_out_index])["unpaid_blocks"].as<uint32_t>();
      produce_blocks(2 * 12 * 21);
      BOOST_REQUIRE_EQUAL(initial_unpaid_blocks, get_producer_info(producer_names[voted_out_index])["unpaid_blocks"].as<uint32_t>());
      produce_block(fc::hours(24));
      BOOST_REQUIRE_EQUAL(success(), vote("producvoterd"_n, { producer_names[voted_out_index] }));
      produce_blocks(2 * 12 * 21);
      BOOST_REQUIRE(fc::crypto::public_key() != fc::crypto::public_key(get_producer_info(producer_names[voted_out_index])["producer_key"].as_string()));
      BOOST_REQUIRE_EQUAL(success(), push_action(producer_names[voted_out_index], "claimrewards"_n, mvo()("owner", producer_names[voted_out_index])));
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( setparams, eosio_system_tester ) try {
   //install multisig contract
   abi_serializer msig_abi_ser = initialize_multisig();
   auto producer_names = active_and_vote_producers();

   //helper function
   auto push_action_msig = [&]( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) -> action_result {
         string action_type_name = msig_abi_ser.get_action_type(name);

         action act;
         act.account = "eosio.msig"_n;
         act.name = name;
         act.data = msig_abi_ser.variant_to_binary( action_type_name, data, abi_serializer::create_yield_function(abi_serializer_max_time) );

         return base_tester::push_action( std::move(act), (auth ? signer : signer == "bob111111111"_n ? "alice1111111"_n : "bob111111111"_n).to_uint64_t() );
   };

   // test begins
   vector<permission_level> prod_perms;
   for ( auto& x : producer_names ) {
      prod_perms.push_back( { name(x), config::active_name } );
   }

   eosio::chain::chain_config params;
   params = control->get_global_properties().configuration;
   //change some values
   params.max_block_net_usage += 10;
   params.max_transaction_lifetime += 1;

   transaction trx;
   {
      fc::variant pretty_trx = fc::mutable_variant_object()
         ("expiration", "2020-01-01T00:30")
         ("ref_block_num", 2)
         ("ref_block_prefix", 3)
         ("net_usage_words", 0)
         ("max_cpu_usage_ms", 0)
         ("delay_sec", 0)
         ("actions", fc::variants({
               fc::mutable_variant_object()
                  ("account", name(config::system_account_name))
                  ("name", "setparams")
                  ("authorization", vector<permission_level>{ { config::system_account_name, config::active_name } })
                  ("data", fc::mutable_variant_object()
                   ("params", params)
                  )
                  })
         );
      abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer::create_yield_function(abi_serializer_max_time));
   }

   BOOST_REQUIRE_EQUAL(success(), push_action_msig( "alice1111111"_n, "propose"_n, mvo()
                                                    ("proposer",      "alice1111111")
                                                    ("proposal_name", "setparams1")
                                                    ("trx",           trx)
                                                    ("requested", prod_perms)
                       )
   );

   // get 16 approvals
   for ( size_t i = 0; i < 15; ++i ) {
      BOOST_REQUIRE_EQUAL(success(), push_action_msig( name(producer_names[i]), "approve"_n, mvo()
                                                       ("proposer",      "alice1111111")
                                                       ("proposal_name", "setparams1")
                                                       ("level",         permission_level{ name(producer_names[i]), config::active_name })
                          )
      );
   }

   transaction_trace_ptr trace;
   control->applied_transaction().connect(
   [&]( std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> p ) {
      trace = std::get<0>(p);
   } );

   BOOST_REQUIRE_EQUAL(success(), push_action_msig( "alice1111111"_n, "exec"_n, mvo()
                                                    ("proposer",      "alice1111111")
                                                    ("proposal_name", "setparams1")
                                                    ("executer",      "alice1111111")
                       )
   );

   BOOST_REQUIRE( bool(trace) );
   BOOST_REQUIRE_EQUAL( 1, trace->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace->receipt->status );

   produce_blocks( 250 );

   // make sure that changed parameters were applied
   auto active_params = control->get_global_properties().configuration;
   BOOST_REQUIRE_EQUAL( params.max_block_net_usage, active_params.max_block_net_usage );
   BOOST_REQUIRE_EQUAL( params.max_transaction_lifetime, active_params.max_transaction_lifetime );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( wasmcfg, eosio_system_tester ) try {
   //install multisig contract
   abi_serializer msig_abi_ser = initialize_multisig();
   auto producer_names = active_and_vote_producers();

   //helper function
   auto push_action_msig = [&]( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) -> action_result {
         string action_type_name = msig_abi_ser.get_action_type(name);

         action act;
         act.account = "eosio.msig"_n;
         act.name = name;
         act.data = msig_abi_ser.variant_to_binary( action_type_name, data, abi_serializer::create_yield_function(abi_serializer_max_time) );

         return base_tester::push_action( std::move(act), (auth ? signer : signer == "bob111111111"_n ? "alice1111111"_n : "bob111111111"_n).to_uint64_t() );
   };

   // test begins
   vector<permission_level> prod_perms;
   for ( auto& x : producer_names ) {
      prod_perms.push_back( { name(x), config::active_name } );
   }

   transaction trx;
   {
      fc::variant pretty_trx = fc::mutable_variant_object()
         ("expiration", "2020-01-01T00:30")
         ("ref_block_num", 2)
         ("ref_block_prefix", 3)
         ("net_usage_words", 0)
         ("max_cpu_usage_ms", 0)
         ("delay_sec", 0)
         ("actions", fc::variants({
               fc::mutable_variant_object()
                  ("account", name(config::system_account_name))
                  ("name", "wasmcfg")
                  ("authorization", vector<permission_level>{ { config::system_account_name, config::active_name } })
                  ("data", fc::mutable_variant_object()
                   ("settings", "high"_n)
                  )
                  })
         );
      abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer::create_yield_function(abi_serializer_max_time));
   }

   BOOST_REQUIRE_EQUAL(success(), push_action_msig( "alice1111111"_n, "propose"_n, mvo()
                                                    ("proposer",      "alice1111111")
                                                    ("proposal_name", "setparams1")
                                                    ("trx",           trx)
                                                    ("requested", prod_perms)
                       )
   );

   // get 16 approvals
   for ( size_t i = 0; i < 15; ++i ) {
      BOOST_REQUIRE_EQUAL(success(), push_action_msig( name(producer_names[i]), "approve"_n, mvo()
                                                       ("proposer",      "alice1111111")
                                                       ("proposal_name", "setparams1")
                                                       ("level",         permission_level{ name(producer_names[i]), config::active_name })
                          )
      );
   }

   transaction_trace_ptr trace;
   control->applied_transaction().connect(
   [&]( std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> p ) {
      trace = std::get<0>(p);
   } );

   BOOST_REQUIRE_EQUAL(success(), push_action_msig( "alice1111111"_n, "exec"_n, mvo()
                                                    ("proposer",      "alice1111111")
                                                    ("proposal_name", "setparams1")
                                                    ("executer",      "alice1111111")
                       )
   );

   BOOST_REQUIRE( bool(trace) );
   BOOST_REQUIRE_EQUAL( 1, trace->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace->receipt->status );

   produce_blocks( 250 );

   // make sure that changed parameters were applied
   auto active_params = control->get_global_properties().wasm_configuration;
   BOOST_REQUIRE_EQUAL( active_params.max_table_elements, 8192 );
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE(eosio_system_part5_tests)

BOOST_FIXTURE_TEST_CASE( setram_effect, eosio_system_tester ) try {

   const asset net = core_sym::from_string("8.0000");
   const asset cpu = core_sym::from_string("8.0000");
   std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n };
   for (const auto& a: accounts) {
      create_account_with_resources(a, config::system_account_name, core_sym::from_string("1.0000"), false, net, cpu);
   }

   {
      const auto name_a = accounts[0];
      transfer( config::system_account_name, name_a, core_sym::from_string("1000.0000") );
      BOOST_REQUIRE_EQUAL( core_sym::from_string("1000.0000"), get_balance(name_a) );
      const uint64_t init_bytes_a = get_total_stake(name_a)["ram_bytes"].as_uint64();
      BOOST_REQUIRE_EQUAL( success(), buyram( name_a, name_a, core_sym::from_string("300.0000") ) );
      BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance(name_a) );
      const uint64_t bought_bytes_a = get_total_stake(name_a)["ram_bytes"].as_uint64() - init_bytes_a;

      // after buying and selling balance should be 700 + 300 * 0.995 * 0.995 = 997.0075 (actually 997.0074 due to rounding fees up)
      BOOST_REQUIRE_EQUAL( success(), sellram(name_a, bought_bytes_a ) );
      BOOST_REQUIRE_EQUAL( core_sym::from_string("997.0074"), get_balance(name_a) );
   }

   {
      const auto name_b = accounts[1];
      transfer( config::system_account_name, name_b, core_sym::from_string("1000.0000") );
      BOOST_REQUIRE_EQUAL( core_sym::from_string("1000.0000"), get_balance(name_b) );
      const uint64_t init_bytes_b = get_total_stake(name_b)["ram_bytes"].as_uint64();
      // name_b buys ram at current price
      BOOST_REQUIRE_EQUAL( success(), buyram( name_b, name_b, core_sym::from_string("300.0000") ) );
      BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance(name_b) );
      const uint64_t bought_bytes_b = get_total_stake(name_b)["ram_bytes"].as_uint64() - init_bytes_b;

      // increase max_ram_size, ram bought by name_b loses part of its value
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("ram may only be increased"),
                           push_action(config::system_account_name, "setram"_n, mvo()("max_ram_size", 64ll*1024 * 1024 * 1024)) );
      BOOST_REQUIRE_EQUAL( error("missing authority of eosio"),
                           push_action(name_b, "setram"_n, mvo()("max_ram_size", 80ll*1024 * 1024 * 1024)) );
      BOOST_REQUIRE_EQUAL( success(),
                           push_action(config::system_account_name, "setram"_n, mvo()("max_ram_size", 80ll*1024 * 1024 * 1024)) );

      BOOST_REQUIRE_EQUAL( success(), sellram(name_b, bought_bytes_b ) );
      BOOST_REQUIRE( core_sym::from_string("900.0000") < get_balance(name_b) );
      BOOST_REQUIRE( core_sym::from_string("950.0000") > get_balance(name_b) );
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( ram_inflation, eosio_system_tester ) try {

   const uint64_t init_max_ram_size = 64ll*1024 * 1024 * 1024;

   BOOST_REQUIRE_EQUAL( init_max_ram_size, get_global_state()["max_ram_size"].as_uint64() );
   produce_blocks(20);
   BOOST_REQUIRE_EQUAL( init_max_ram_size, get_global_state()["max_ram_size"].as_uint64() );
   transfer( config::system_account_name, "alice1111111", core_sym::from_string("1000.0000"), config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("100.0000") ) );
   produce_blocks(3);
   BOOST_REQUIRE_EQUAL( init_max_ram_size, get_global_state()["max_ram_size"].as_uint64() );
   uint16_t rate = 1000;
   BOOST_REQUIRE_EQUAL( success(), push_action( config::system_account_name, "setramrate"_n, mvo()("bytes_per_block", rate) ) );
   BOOST_REQUIRE_EQUAL( rate, get_global_state2()["new_ram_per_block"].as<uint16_t>() );
   // last time update_ram_supply called is in buyram, num of blocks since then to
   // the block that includes the setramrate action is 1 + 3 = 4.
   // However, those 4 blocks were accumulating at a rate of 0, so the max_ram_size should not have changed.
   BOOST_REQUIRE_EQUAL( init_max_ram_size, get_global_state()["max_ram_size"].as_uint64() );
   // But with additional blocks, it should start accumulating at the new rate.
   uint64_t cur_ram_size = get_global_state()["max_ram_size"].as_uint64();
   produce_blocks(10);
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("100.0000") ) );
   BOOST_REQUIRE_EQUAL( cur_ram_size + 11 * rate, get_global_state()["max_ram_size"].as_uint64() );
   cur_ram_size = get_global_state()["max_ram_size"].as_uint64();
   produce_blocks(5);
   BOOST_REQUIRE_EQUAL( cur_ram_size, get_global_state()["max_ram_size"].as_uint64() );
   BOOST_REQUIRE_EQUAL( success(), sellram( "alice1111111", 100 ) );
   BOOST_REQUIRE_EQUAL( cur_ram_size + 6 * rate, get_global_state()["max_ram_size"].as_uint64() );
   cur_ram_size = get_global_state()["max_ram_size"].as_uint64();
   produce_blocks();
   BOOST_REQUIRE_EQUAL( success(), buyrambytes( "alice1111111", "alice1111111", 100 ) );
   BOOST_REQUIRE_EQUAL( cur_ram_size + 2 * rate, get_global_state()["max_ram_size"].as_uint64() );

   BOOST_REQUIRE_EQUAL( error("missing authority of eosio"),
                        push_action( "alice1111111"_n, "setramrate"_n, mvo()("bytes_per_block", rate) ) );

   cur_ram_size = get_global_state()["max_ram_size"].as_uint64();
   produce_blocks(10);
   uint16_t old_rate = rate;
   rate = 5000;
   BOOST_REQUIRE_EQUAL( success(), push_action( config::system_account_name, "setramrate"_n, mvo()("bytes_per_block", rate) ) );
   BOOST_REQUIRE_EQUAL( cur_ram_size + 11 * old_rate, get_global_state()["max_ram_size"].as_uint64() );
   produce_blocks(5);
   BOOST_REQUIRE_EQUAL( success(), buyrambytes( "alice1111111", "alice1111111", 100 ) );
   BOOST_REQUIRE_EQUAL( cur_ram_size + 11 * old_rate + 6 * rate, get_global_state()["max_ram_size"].as_uint64() );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( eosioram_ramusage, eosio_system_tester ) try {
   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( "alice1111111" ) );
   transfer( "eosio", "alice1111111", core_sym::from_string("1000.0000"), "eosio" );
   BOOST_REQUIRE_EQUAL( success(), stake( "eosio", "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );

   [[maybe_unused]] const asset initial_ram_balance = get_balance("eosio.ram"_n);
   [[maybe_unused]] const asset initial_fees_balance = get_balance("eosio.fees"_n);
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("1000.0000") ) );

   BOOST_REQUIRE_EQUAL( false, get_row_by_account( "eosio.token"_n, "alice1111111"_n, "accounts"_n, account_name(symbol{CORE_SYM}.to_symbol_code()) ).empty() );

   //remove row
   base_tester::push_action( "eosio.token"_n, "close"_n, "alice1111111"_n, mvo()
                             ( "owner", "alice1111111" )
                             ( "symbol", symbol{CORE_SYM} )
   );
   BOOST_REQUIRE_EQUAL( true, get_row_by_account( "eosio.token"_n, "alice1111111"_n, "accounts"_n, account_name(symbol{CORE_SYM}.to_symbol_code()) ).empty() );

   auto rlm = control->get_resource_limits_manager();
   auto eosioram_ram_usage = rlm.get_account_ram_usage("eosio.ram"_n);
   auto alice_ram_usage = rlm.get_account_ram_usage("alice1111111"_n);

   BOOST_REQUIRE_EQUAL( success(), sellram( "alice1111111", 2048 ) );

   //make sure that ram was billed to alice, not to eosio.ram
   BOOST_REQUIRE_EQUAL( true, alice_ram_usage < rlm.get_account_ram_usage("alice1111111"_n) );
   BOOST_REQUIRE_EQUAL( eosioram_ram_usage, rlm.get_account_ram_usage("eosio.ram"_n) );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( ram_gift, eosio_system_tester ) try {
   active_and_vote_producers();

   auto rlm = control->get_resource_limits_manager();
   int64_t ram_bytes_orig, net_weight, cpu_weight;
   rlm.get_account_limits( "alice1111111"_n, ram_bytes_orig, net_weight, cpu_weight );

   /*
    * It seems impossible to write this test, because buyrambytes action doesn't give you exact amount of bytes requested
    *
   //check that it's possible to create account bying required_bytes(2724) + userres table(112) + userres row(160) - ram_gift_bytes(1400)
   create_account_with_resources( "abcdefghklmn"_n, "alice1111111"_n, 2724 + 112 + 160 - 1400 );

   //check that one byte less is not enough
   BOOST_REQUIRE_THROW( create_account_with_resources( "abcdefghklmn"_n, "alice1111111"_n, 2724 + 112 + 160 - 1400 - 1 ),
                        ram_usage_exceeded );
   */

   //check that stake/unstake keeps the gift
   transfer( "eosio", "alice1111111", core_sym::from_string("1000.0000"), "eosio" );
   BOOST_REQUIRE_EQUAL( success(), stake( "eosio", "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );
   int64_t ram_bytes_after_stake;
   rlm.get_account_limits( "alice1111111"_n, ram_bytes_after_stake, net_weight, cpu_weight );
   BOOST_REQUIRE_EQUAL( ram_bytes_orig, ram_bytes_after_stake );

   BOOST_REQUIRE_EQUAL( success(), unstake( "eosio", "alice1111111", core_sym::from_string("20.0000"), core_sym::from_string("10.0000") ) );
   int64_t ram_bytes_after_unstake;
   rlm.get_account_limits( "alice1111111"_n, ram_bytes_after_unstake, net_weight, cpu_weight );
   BOOST_REQUIRE_EQUAL( ram_bytes_orig, ram_bytes_after_unstake );

   uint64_t ram_gift = 1400;

   int64_t ram_bytes;
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("1000.0000") ) );
   rlm.get_account_limits( "alice1111111"_n, ram_bytes, net_weight, cpu_weight );
   auto userres = get_total_stake( "alice1111111"_n );
   BOOST_REQUIRE_EQUAL( userres["ram_bytes"].as_uint64() + ram_gift, ram_bytes );

   BOOST_REQUIRE_EQUAL( success(), sellram( "alice1111111", 1024 ) );
   rlm.get_account_limits( "alice1111111"_n, ram_bytes, net_weight, cpu_weight );
   userres = get_total_stake( "alice1111111"_n );
   BOOST_REQUIRE_EQUAL( userres["ram_bytes"].as_uint64() + ram_gift, ram_bytes );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE(eosio_system_origin_rex_tests)

BOOST_FIXTURE_TEST_CASE( rex_rounding_issue, eosio_system_tester ) try {
   const std::vector<name> whales { "whale1"_n, "whale2"_n, "whale3"_n, "whale4"_n , "whale5"_n  };
   const name bob{ "bob"_n }, alice{ "alice"_n };
   const std::vector<name> accounts = {bob, alice};
   const std::vector<name> rexborrowers = {"rex1"_n, "rex2"_n, "rex3"_n, "rex4"_n};
    const asset one_eos      = core_sym::from_string("1.0000");
   const asset one_rex      = asset::from_string("1.0000 REX");
   const asset init_balance = core_sym::from_string("1000.0000");
   [[maybe_unused]] const asset whale_balance = core_sym::from_string("1000000.0000");

   setup_rex_accounts( accounts, init_balance );
   // create_accounts_with_resources({ bob, whale });
   for (auto& w : whales) {
      create_accounts_with_resources({ w });
      stake_with_transfer(config::system_account_name, w, core_sym::from_string("1000.0000"), core_sym::from_string("1000.0000"));
      issue_and_transfer(w, core_sym::from_string("100000000.0000"));
      BOOST_REQUIRE_EQUAL( success(), vote( w, { }, "proxyaccount"_n ) );
      BOOST_REQUIRE_EQUAL( success(), push_action( w, "deposit"_n, mvo()("owner", w)("amount", core_sym::from_string("1000000.0000")) ) );
      BOOST_REQUIRE_EQUAL( success(), push_action( w, "buyrex"_n, mvo()("from", w)("amount", core_sym::from_string("1000000.0000")) ) );
   }

   issue_and_transfer(bob, init_balance);


   for(auto& rb : rexborrowers) {
      create_accounts_with_resources({ rb });
      buyram(config::system_account_name, rb, core_sym::from_string("10.0000"));
      issue_and_transfer(rb, core_sym::from_string("1000000.0000"));
      stake_with_transfer(config::system_account_name, rb, core_sym::from_string("1000.0000"), core_sym::from_string("1000.0000"));
      BOOST_REQUIRE_EQUAL( success(), vote( rb, { }, "proxyaccount"_n ) );
      BOOST_REQUIRE_EQUAL( success(), push_action( rb, "deposit"_n, mvo()("owner", rb)("amount", core_sym::from_string("1000000.0000")) ) );
   }
   // get accounts into rex
   for(auto& acct : accounts){
      stake_with_transfer(config::system_account_name, acct, core_sym::from_string("1000.0000"), core_sym::from_string("1000.0000"));
      issue_and_transfer(acct, core_sym::from_string("100.1239"));
      BOOST_REQUIRE_EQUAL( success(), vote( acct, { }, "proxyaccount"_n ) );
      BOOST_REQUIRE_EQUAL( success(), push_action( acct, "deposit"_n, mvo()("owner", acct)("amount", core_sym::from_string("100.1239")) ) );
      BOOST_REQUIRE_EQUAL( success(), push_action( acct, "buyrex"_n, mvo()("from", acct)("amount", core_sym::from_string("100.1239")) ) );
   }

   auto rent_and_go = [&] (int cnt) {
      for(auto& rb : rexborrowers) {
         BOOST_REQUIRE_EQUAL( success(),
                        push_action( rb, "rentcpu"_n,
                        mvo()
                        ("from", rb)
                        ("receiver", rb)
                        ("loan_payment", core_sym::from_string("100.0000"))
                        ("loan_fund", core_sym::from_string("0.0000")) ) );
      }
      // exec and update
      for(auto& acct : accounts) {
         if(cnt % 10 == 0) {
            BOOST_REQUIRE_EQUAL( success(), push_action( acct, "rexexec"_n, mvo()("user", acct)("max", 2) ) );
            BOOST_REQUIRE_EQUAL( success(), push_action( acct, "updaterex"_n, mvo()("owner", acct) ) );
         }
         BOOST_REQUIRE_EQUAL( success(), push_action( acct, "sellrex"_n, mvo()("from", acct)("rex",one_rex) ) );
         BOOST_REQUIRE_EQUAL( success(),
                            push_action( acct, "unstaketorex"_n, mvo()("owner", acct)("receiver", acct)("from_net", one_eos)("from_cpu", one_eos) ) );
      }

      produce_block( fc::days(1) );
   };
   // BOOST_REQUIRE_EQUAL( get_rex_vote_stake( alice ),                init_rex_stake - core_sym::from_string("0.0006") );
   auto check_tables = [&] (const name& acct, bool is_error=false) {
      auto rex_stake = get_rex_vote_stake(acct);
      auto vote_staked = get_voter_info(acct)["staked"];
      auto delband = get_dbw_obj(acct, acct);
      auto cpu_stake = delband["cpu_weight"].as<asset>();
      auto net_stake = delband["net_weight"].as<asset>();
      ilog( "voter:\t${voter}", ("voter", vote_staked));
      ilog( "calc_vote:\t${calc_vote}", ("calc_vote", (rex_stake + cpu_stake + net_stake).get_amount()));
      if(is_error){
         BOOST_REQUIRE_EQUAL( vote_staked - (rex_stake + cpu_stake + net_stake).get_amount(), 1 );
      }
      else{
         BOOST_REQUIRE_EQUAL( (rex_stake + cpu_stake + net_stake).get_amount(), vote_staked );
      }
   };
   // move ahead 5 days to unlock rex
   produce_block(fc::days(5));
   for(int i = 0; i < 159; ++i){
      rent_and_go(i);
      if(i % 10 == 0)
         for(auto& acct : accounts) {
            ilog("${i}", ("i",i));
            check_tables(acct);
         }
   }
   // day 160 there was a divergence of voter.staked and (rex_bal + delband.cpu + delband.net)
   rent_and_go(160);
   for(auto& acct : accounts) {
      check_tables(acct, false);
   }
   // update vote
   std::vector<name> delegates = {};
   for(auto& acct : accounts) {
      BOOST_REQUIRE_EQUAL( success(),
                            push_action( acct, "voteupdate"_n, mvo()("voter_name", acct) ) );
   }
   // check that values are equal
   for(auto& acct : accounts) {
      check_tables(acct, false);
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( rex_auth, eosio_system_tester ) try {

   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n };
   const account_name alice = accounts[0], bob = accounts[1];
   const asset init_balance = core_sym::from_string("1000.0000");
   const asset one_eos      = core_sym::from_string("1.0000");
   const asset one_rex      = asset::from_string("1.0000 REX");
   setup_rex_accounts( accounts, init_balance );

   const std::string error_msg("missing authority of aliceaccount");
   BOOST_REQUIRE_EQUAL( error(error_msg), push_action( bob, "deposit"_n, mvo()("owner", alice)("amount", one_eos) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg), push_action( bob, "withdraw"_n, mvo()("owner", alice)("amount", one_eos) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg), push_action( bob, "buyrex"_n, mvo()("from", alice)("amount", one_eos) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg),
                        push_action( bob, "unstaketorex"_n, mvo()("owner", alice)("receiver", alice)("from_net", one_eos)("from_cpu", one_eos) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg), push_action( bob, "sellrex"_n, mvo()("from", alice)("rex", one_rex) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg), push_action( bob, "cnclrexorder"_n, mvo()("owner", alice) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg),
                        push_action( bob, "rentcpu"_n, mvo()("from", alice)("receiver", alice)("loan_payment", one_eos)("loan_fund", one_eos) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg),
                        push_action( bob, "rentnet"_n, mvo()("from", alice)("receiver", alice)("loan_payment", one_eos)("loan_fund", one_eos) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg), push_action( bob, "fundcpuloan"_n, mvo()("from", alice)("loan_num", 1)("payment", one_eos) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg), push_action( bob, "fundnetloan"_n, mvo()("from", alice)("loan_num", 1)("payment", one_eos) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg), push_action( bob, "defcpuloan"_n, mvo()("from", alice)("loan_num", 1)("amount", one_eos) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg), push_action( bob, "defnetloan"_n, mvo()("from", alice)("loan_num", 1)("amount", one_eos) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg), push_action( bob, "updaterex"_n, mvo()("owner", alice) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg), push_action( bob, "rexexec"_n, mvo()("user", alice)("max", 1) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg), push_action( bob, "consolidate"_n, mvo()("owner", alice) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg), push_action( bob, "mvtosavings"_n, mvo()("owner", alice)("rex", one_rex) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg), push_action( bob, "mvfrsavings"_n, mvo()("owner", alice)("rex", one_rex) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg), push_action( bob, "closerex"_n, mvo()("owner", alice) ) );
   BOOST_REQUIRE_EQUAL( error(error_msg),
                        push_action( bob, "donatetorex"_n, mvo()("payer", alice)("quantity", one_eos)("memo", "") ) );

   BOOST_REQUIRE_EQUAL( error("missing authority of eosio"), push_action( alice, "setrex"_n, mvo()("balance", one_eos) ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( buy_sell_rex, eosio_system_tester ) try {

   const int64_t ratio        = 10000;
   const asset   init_rent    = core_sym::from_string("20000.0000");
   const asset   init_balance = core_sym::from_string("1000.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n, "carolaccount"_n, "emilyaccount"_n, "frankaccount"_n };
   account_name alice = accounts[0], bob = accounts[1], carol = accounts[2];
   setup_rex_accounts( accounts, init_balance );

   const asset one_unit = core_sym::from_string("0.0001");
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient funds"), buyrex( alice, init_balance + one_unit ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("25000.0000 REX"),  get_buyrex_result( alice, core_sym::from_string("2.5000") ) );
   produce_blocks(2);
   produce_block(fc::days(5));
   BOOST_REQUIRE_EQUAL( core_sym::from_string("2.5000"),     get_sellrex_result( alice, asset::from_string("25000.0000 REX") ) );

   BOOST_REQUIRE_EQUAL( success(),                           buyrex( alice, core_sym::from_string("13.0000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("13.0000"),    get_rex_vote_stake( alice ) );
   BOOST_REQUIRE_EQUAL( success(),                           buyrex( alice, core_sym::from_string("17.0000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("30.0000"),    get_rex_vote_stake( alice ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("970.0000"),   get_rex_fund(alice) );
   BOOST_REQUIRE_EQUAL( get_rex_balance(alice).get_amount(), ratio * asset::from_string("30.0000 REX").get_amount() );
   auto rex_pool = get_rex_pool();
   BOOST_REQUIRE_EQUAL( core_sym::from_string("30.0000"),  rex_pool["total_lendable"].as<asset>() );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("30.0000"),  rex_pool["total_unlent"].as<asset>() );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"),   rex_pool["total_lent"].as<asset>() );
   BOOST_REQUIRE_EQUAL( init_rent,                         rex_pool["total_rent"].as<asset>() );
   BOOST_REQUIRE_EQUAL( get_rex_balance(alice),            rex_pool["total_rex"].as<asset>() );

   BOOST_REQUIRE_EQUAL( success(), buyrex( bob, core_sym::from_string("75.0000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("925.0000"), get_rex_fund(bob) );
   BOOST_REQUIRE_EQUAL( ratio * asset::from_string("75.0000 REX").get_amount(), get_rex_balance(bob).get_amount() );
   rex_pool = get_rex_pool();
   BOOST_REQUIRE_EQUAL( core_sym::from_string("105.0000"), rex_pool["total_lendable"].as<asset>() );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("105.0000"), rex_pool["total_unlent"].as<asset>() );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"),   rex_pool["total_lent"].as<asset>() );
   BOOST_REQUIRE_EQUAL( init_rent,                         rex_pool["total_rent"].as<asset>() );
   BOOST_REQUIRE_EQUAL( get_rex_balance(alice) + get_rex_balance(bob), rex_pool["total_rex"].as<asset>() );

   produce_block( fc::days(6) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("user must first buyrex"),        sellrex( carol, asset::from_string("5.0000 REX") ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("asset must be a positive amount of (REX, 4)"),
                        sellrex( bob, core_sym::from_string("55.0000") ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("asset must be a positive amount of (REX, 4)"),
                        sellrex( bob, asset::from_string("-75.0030 REX") ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),    sellrex( bob, asset::from_string("750000.0030 REX") ) );

   auto init_total_rex      = rex_pool["total_rex"].as<asset>().get_amount();
   auto init_total_lendable = rex_pool["total_lendable"].as<asset>().get_amount();
   BOOST_REQUIRE_EQUAL( success(),                             sellrex( bob, asset::from_string("550001.0000 REX") ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("199999.0000 REX"), get_rex_balance(bob) );
   rex_pool = get_rex_pool();
   auto total_rex      = rex_pool["total_rex"].as<asset>().get_amount();
   auto total_lendable = rex_pool["total_lendable"].as<asset>().get_amount();
   BOOST_REQUIRE_EQUAL( init_total_rex / init_total_lendable,          total_rex / total_lendable );
   BOOST_REQUIRE_EQUAL( total_lendable,                                rex_pool["total_unlent"].as<asset>().get_amount() );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"),               rex_pool["total_lent"].as<asset>() );
   BOOST_REQUIRE_EQUAL( init_rent,                                     rex_pool["total_rent"].as<asset>() );
   BOOST_REQUIRE_EQUAL( get_rex_balance(alice) + get_rex_balance(bob), rex_pool["total_rex"].as<asset>() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( buy_sell_small_rex, eosio_system_tester ) try {

   const int64_t ratio        = 10000;
   const asset   init_balance = core_sym::from_string("50000.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n, "carolaccount"_n };
   account_name alice = accounts[0], bob = accounts[1], carol = accounts[2];
   setup_rex_accounts( accounts, init_balance );

   const asset payment = core_sym::from_string("40000.0000");
   BOOST_REQUIRE_EQUAL( ratio * payment.get_amount(),               get_buyrex_result( alice, payment ).get_amount() );

   produce_blocks(2);
   produce_block( fc::days(5) );
   produce_blocks(2);

   asset init_rex_stake = get_rex_vote_stake( alice );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("proceeds are negligible"), sellrex( alice, asset::from_string("0.0001 REX") ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("proceeds are negligible"), sellrex( alice, asset::from_string("0.9999 REX") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0001"),            get_sellrex_result( alice, asset::from_string("1.0000 REX") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0001"),            get_sellrex_result( alice, asset::from_string("1.9999 REX") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0002"),            get_sellrex_result( alice, asset::from_string("2.0000 REX") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0002"),            get_sellrex_result( alice, asset::from_string("2.9999 REX") ) );
   BOOST_REQUIRE_EQUAL( get_rex_vote_stake( alice ),                init_rex_stake - core_sym::from_string("0.0006") );

   BOOST_REQUIRE_EQUAL( success(),                                  rentcpu( carol, bob, core_sym::from_string("10.0000") ) );
   BOOST_REQUIRE_EQUAL( success(),                                  sellrex( alice, asset::from_string("1.0000 REX") ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("proceeds are negligible"), sellrex( alice, asset::from_string("0.4000 REX") ) );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( unstake_buy_rex, eosio_system_tester, * boost::unit_test::tolerance(1e-10) ) try {

   const int64_t ratio        = 10000;
   const asset   zero_asset   = core_sym::from_string("0.0000");
   const asset   neg_asset    = core_sym::from_string("-0.0001");
   const asset   one_token    = core_sym::from_string("1.0000");
   const asset   init_balance = core_sym::from_string("10000.0000");
   const asset   init_net     = core_sym::from_string("70.0000");
   const asset   init_cpu     = core_sym::from_string("90.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n, "carolaccount"_n, "emilyaccount"_n, "frankaccount"_n };
   account_name alice = accounts[0], bob = accounts[1], carol = accounts[2], emily = accounts[3], frank = accounts[4];
   setup_rex_accounts( accounts, init_balance, init_net, init_cpu, false );

   // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
   std::vector<account_name> producer_names;
   {
      producer_names.reserve('z' - 'a' + 1);
      const std::string root("defproducer");
      for ( char c = 'a'; c <= 'z'; ++c ) {
         producer_names.emplace_back(root + std::string(1, c));
      }

      setup_producer_accounts(producer_names);
      for ( const auto& p: producer_names ) {
         BOOST_REQUIRE_EQUAL( success(), regproducer(p) );
         BOOST_TEST_REQUIRE( 0 == get_producer_info(p)["total_votes"].as<double>() );
      }
   }

   const int64_t init_cpu_limit = get_cpu_limit( alice );
   const int64_t init_net_limit = get_net_limit( alice );

   {
      const asset net_stake = core_sym::from_string("25.5000");
      const asset cpu_stake = core_sym::from_string("12.4000");
      const asset tot_stake = net_stake + cpu_stake;
      BOOST_REQUIRE_EQUAL( init_balance,                            get_balance( alice ) );
      BOOST_REQUIRE_EQUAL( success(),                               stake( alice, alice, net_stake, cpu_stake ) );
      BOOST_REQUIRE_EQUAL( get_cpu_limit( alice ),                  init_cpu_limit + cpu_stake.get_amount() );
      BOOST_REQUIRE_EQUAL( get_net_limit( alice ),                  init_net_limit + net_stake.get_amount() );
      BOOST_REQUIRE_EQUAL( success(),
                           vote( alice, std::vector<account_name>(producer_names.begin(), producer_names.begin() + 20) ) );
      BOOST_REQUIRE_EQUAL( success(),
                           vote( alice, std::vector<account_name>(producer_names.begin(), producer_names.begin() + 21) ) );
      const asset init_eosio_stake_balance = get_balance( "eosio.stake"_n );
      const auto init_voter_info = get_voter_info( alice );
      const auto init_prod_info  = get_producer_info( producer_names[0] );
      BOOST_TEST_REQUIRE( init_prod_info["total_votes"].as_double() ==
                          stake2votes( asset( init_voter_info["staked"].as<int64_t>(), symbol{CORE_SYM} ) ) );
      produce_block( fc::days(4) );
      BOOST_REQUIRE_EQUAL( ratio * tot_stake.get_amount(),          get_unstaketorex_result( alice, alice, net_stake, cpu_stake ).get_amount() );
      BOOST_REQUIRE_EQUAL( get_cpu_limit( alice ),                  init_cpu_limit );
      BOOST_REQUIRE_EQUAL( get_net_limit( alice ),                  init_net_limit );
      BOOST_REQUIRE_EQUAL( ratio * tot_stake.get_amount(),          get_rex_balance( alice ).get_amount() );
      BOOST_REQUIRE_EQUAL( tot_stake,                               get_rex_balance_obj( alice )["vote_stake"].as<asset>() );
      BOOST_REQUIRE_EQUAL( tot_stake,                               get_balance( "eosio.rex"_n ) );
      BOOST_REQUIRE_EQUAL( tot_stake,                               init_eosio_stake_balance - get_balance( "eosio.stake"_n ) );
      auto current_voter_info = get_voter_info( alice );
      auto current_prod_info  = get_producer_info( producer_names[0] );
      BOOST_REQUIRE_EQUAL( init_voter_info["staked"].as<int64_t>(), current_voter_info["staked"].as<int64_t>() );
      BOOST_TEST_REQUIRE( current_prod_info["total_votes"].as_double() ==
                          stake2votes( asset( current_voter_info["staked"].as<int64_t>(), symbol{CORE_SYM} ) ) );
      BOOST_TEST_REQUIRE( init_prod_info["total_votes"].as_double() < current_prod_info["total_votes"].as_double() );
   }

   {
      const asset net_stake = core_sym::from_string("200.5000");
      const asset cpu_stake = core_sym::from_string("120.0000");
      const asset tot_stake = net_stake + cpu_stake;
      BOOST_REQUIRE_EQUAL( success(),                               stake( bob, carol, net_stake, cpu_stake ) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("amount exceeds tokens staked for net"),
                           unstaketorex( bob, carol, net_stake + one_token, zero_asset ) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("amount exceeds tokens staked for cpu"),
                           unstaketorex( bob, carol, zero_asset, cpu_stake + one_token ) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("delegated bandwidth record does not exist"),
                           unstaketorex( bob, emily, zero_asset, one_token ) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("must unstake a positive amount to buy rex"),
                           unstaketorex( bob, carol, zero_asset, zero_asset ) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("must unstake a positive amount to buy rex"),
                           unstaketorex( bob, carol, neg_asset, one_token ) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("must unstake a positive amount to buy rex"),
                           unstaketorex( bob, carol, one_token, neg_asset ) );
      BOOST_REQUIRE_EQUAL( init_net_limit + net_stake.get_amount(), get_net_limit( carol ) );
      BOOST_REQUIRE_EQUAL( init_cpu_limit + cpu_stake.get_amount(), get_cpu_limit( carol ) );
      BOOST_REQUIRE_EQUAL( false,                                   get_dbw_obj( bob, carol ).is_null() );
      BOOST_REQUIRE_EQUAL( success(),                               unstaketorex( bob, carol, net_stake, zero_asset ) );
      BOOST_REQUIRE_EQUAL( false,                                   get_dbw_obj( bob, carol ).is_null() );
      BOOST_REQUIRE_EQUAL( success(),                               unstaketorex( bob, carol, zero_asset, cpu_stake ) );
      BOOST_REQUIRE_EQUAL( true,                                    get_dbw_obj( bob, carol ).is_null() );
      BOOST_REQUIRE_EQUAL( 0,                                       get_rex_balance( carol ).get_amount() );
      BOOST_REQUIRE_EQUAL( ratio * tot_stake.get_amount(),          get_rex_balance( bob ).get_amount() );
      BOOST_REQUIRE_EQUAL( init_cpu_limit,                          get_cpu_limit( bob ) );
      BOOST_REQUIRE_EQUAL( init_net_limit,                          get_net_limit( bob ) );
      BOOST_REQUIRE_EQUAL( init_cpu_limit,                          get_cpu_limit( carol ) );
      BOOST_REQUIRE_EQUAL( init_net_limit,                          get_net_limit( carol ) );
   }

   {
      const asset net_stake = core_sym::from_string("130.5000");
      const asset cpu_stake = core_sym::from_string("220.0800");
      [[maybe_unused]] const asset tot_stake = net_stake + cpu_stake;
      BOOST_REQUIRE_EQUAL( success(),                               stake_with_transfer( emily, frank, net_stake, cpu_stake ) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("delegated bandwidth record does not exist"),
                           unstaketorex( emily, frank, net_stake, cpu_stake ) );
      BOOST_REQUIRE_EQUAL( success(),                               unstaketorex( frank, frank, net_stake, cpu_stake ) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),
                           sellrex( frank, asset::from_string("1.0000 REX") ) );
      produce_block( fc::days(5) );
      BOOST_REQUIRE_EQUAL( success(),                               sellrex( frank, asset::from_string("1.0000 REX") ) );
   }

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( buy_rent_rex, eosio_system_tester ) try {

   const int64_t ratio        = 10000;
   const asset   init_balance = core_sym::from_string("60000.0000");
   const asset   init_net     = core_sym::from_string("70.0000");
   const asset   init_cpu     = core_sym::from_string("90.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n, "carolaccount"_n, "emilyaccount"_n, "frankaccount"_n };
   account_name alice = accounts[0], bob = accounts[1], carol = accounts[2], emily = accounts[3];
   setup_rex_accounts( accounts, init_balance, init_net, init_cpu );

   const int64_t init_cpu_limit = get_cpu_limit( alice );
   const int64_t init_net_limit = get_net_limit( alice );

   // bob tries to rent rex
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("rex system not initialized yet"), rentcpu( bob, carol, core_sym::from_string("5.0000") ) );
   // alice lends rex
   BOOST_REQUIRE_EQUAL( success(), buyrex( alice, core_sym::from_string("50265.0000") ) );
   BOOST_REQUIRE_EQUAL( init_balance - core_sym::from_string("50265.0000"), get_rex_fund(alice) );
   auto rex_pool = get_rex_pool();
   const asset   init_tot_unlent   = rex_pool["total_unlent"].as<asset>();
   const asset   init_tot_lendable = rex_pool["total_lendable"].as<asset>();
   const asset   init_tot_rent     = rex_pool["total_rent"].as<asset>();
   [[maybe_unused]] const int64_t init_stake        = get_voter_info(alice)["staked"].as<int64_t>();
   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"),        rex_pool["total_lent"].as<asset>() );
   BOOST_REQUIRE_EQUAL( ratio * init_tot_lendable.get_amount(), rex_pool["total_rex"].as<asset>().get_amount() );
   BOOST_REQUIRE_EQUAL( rex_pool["total_rex"].as<asset>(),      get_rex_balance(alice) );

   BOOST_REQUIRE( get_rex_return_pool().is_null() );

   {
      // bob rents cpu for carol
      const asset fee = core_sym::from_string("17.0000");
      BOOST_REQUIRE_EQUAL( success(),          rentcpu( bob, carol, fee ) );
      BOOST_REQUIRE_EQUAL( init_balance - fee, get_rex_fund(bob) );
      rex_pool = get_rex_pool();
      BOOST_REQUIRE_EQUAL( init_tot_lendable,       rex_pool["total_lendable"].as<asset>() ); // 65
      BOOST_REQUIRE_EQUAL( init_tot_rent + fee,     rex_pool["total_rent"].as<asset>() );     // 100 + 17
      int64_t expected_total_lent = bancor_convert( init_tot_rent.get_amount(), init_tot_unlent.get_amount(), fee.get_amount() );
      BOOST_REQUIRE_EQUAL( expected_total_lent,
                           rex_pool["total_lent"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( rex_pool["total_lent"].as<asset>() + rex_pool["total_unlent"].as<asset>(),
                           rex_pool["total_lendable"].as<asset>() );

      auto rex_return_pool = get_rex_return_pool();
      BOOST_REQUIRE( !rex_return_pool.is_null() );
      BOOST_REQUIRE_EQUAL( 0,                   rex_return_pool["current_rate_of_increase"].as<int64_t>() );

      // test that carol's resource limits have been updated properly
      BOOST_REQUIRE_EQUAL( expected_total_lent, get_cpu_limit( carol ) - init_cpu_limit );
      BOOST_REQUIRE_EQUAL( 0,                   get_net_limit( carol ) - init_net_limit );

      // alice tries to sellrex, order gets scheduled then she cancels order
      BOOST_REQUIRE_EQUAL( cancelrexorder( alice ),           wasm_assert_msg("no sellrex order is scheduled") );
      produce_block( fc::days(6) );
      BOOST_REQUIRE_EQUAL( success(),                         sellrex( alice, get_rex_balance(alice) ) );
      BOOST_REQUIRE_EQUAL( success(),                         cancelrexorder( alice ) );
      BOOST_REQUIRE_EQUAL( rex_pool["total_rex"].as<asset>(), get_rex_balance(alice) );

      produce_block( fc::days(20) );
      BOOST_REQUIRE_EQUAL( success(), sellrex( alice, get_rex_balance(alice) ) );
      BOOST_REQUIRE_EQUAL( success(), cancelrexorder( alice ) );

      rex_return_pool = get_rex_return_pool();
      BOOST_REQUIRE( !rex_return_pool.is_null() );
      int64_t rate = fee.get_amount() / (30 * 144);
      BOOST_REQUIRE_EQUAL( rate,               rex_return_pool["current_rate_of_increase"].as<int64_t>() );

      produce_block( fc::days(10) );
      // alice is finally able to sellrex, she gains the fee paid by bob
      BOOST_REQUIRE_EQUAL( success(),          sellrex( alice, get_rex_balance(alice) ) );
      BOOST_REQUIRE_EQUAL( 0,                  get_rex_balance(alice).get_amount() );
      auto expected_rex_fund = (init_balance + fee).get_amount();
      auto actual_rex_fund   = get_rex_fund(alice).get_amount();
      BOOST_REQUIRE_EQUAL( expected_rex_fund,  actual_rex_fund );
      // test that carol's resource limits have been updated properly when loan expires
      BOOST_REQUIRE_EQUAL( init_cpu_limit,     get_cpu_limit( carol ) );
      BOOST_REQUIRE_EQUAL( init_net_limit,     get_net_limit( carol ) );

      rex_pool = get_rex_pool();
      BOOST_REQUIRE_EQUAL( 0, rex_pool["total_lendable"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( 0, rex_pool["total_unlent"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( 0, rex_pool["total_rex"].as<asset>().get_amount() );
   }

   {
      const int64_t init_net_limit = get_net_limit( emily );
      BOOST_REQUIRE_EQUAL( 0,         get_rex_balance(alice).get_amount() );
      BOOST_REQUIRE_EQUAL( success(), buyrex( alice, core_sym::from_string("20050.0000") ) );
      rex_pool = get_rex_pool();
      const asset fee = core_sym::from_string("0.4560");
      int64_t expected_net = bancor_convert( rex_pool["total_rent"].as<asset>().get_amount(),
                                             rex_pool["total_unlent"].as<asset>().get_amount(),
                                             fee.get_amount() );
      BOOST_REQUIRE_EQUAL( success(),    rentnet( emily, emily, fee ) );
      BOOST_REQUIRE_EQUAL( expected_net, get_net_limit( emily ) - init_net_limit );
   }

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( buy_sell_sell_rex, eosio_system_tester ) try {

   const int64_t ratio        = 10000;
   const asset   init_balance = core_sym::from_string("40000.0000");
   const asset   init_net     = core_sym::from_string("70.0000");
   const asset   init_cpu     = core_sym::from_string("90.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n, "carolaccount"_n };
   account_name alice = accounts[0], bob = accounts[1], carol = accounts[2];
   setup_rex_accounts( accounts, init_balance, init_net, init_cpu );

   [[maybe_unused]] const int64_t init_cpu_limit = get_cpu_limit( alice );
   [[maybe_unused]] const int64_t init_net_limit = get_net_limit( alice );

   // alice lends rex
   const asset payment = core_sym::from_string("30250.0000");
   BOOST_REQUIRE_EQUAL( success(),              buyrex( alice, payment ) );
   BOOST_REQUIRE_EQUAL( success(),              buyrex( bob, core_sym::from_string("0.0005") ) );
   BOOST_REQUIRE_EQUAL( init_balance - payment, get_rex_fund(alice) );
   auto rex_pool = get_rex_pool();
   [[maybe_unused]] const asset   init_tot_unlent   = rex_pool["total_unlent"].as<asset>();
   const asset   init_tot_lendable = rex_pool["total_lendable"].as<asset>();
   const asset   init_tot_rent     = rex_pool["total_rent"].as<asset>();
   [[maybe_unused]] const int64_t init_stake        = get_voter_info(alice)["staked"].as<int64_t>();
   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"),        rex_pool["total_lent"].as<asset>() );
   BOOST_REQUIRE_EQUAL( ratio * init_tot_lendable.get_amount(), rex_pool["total_rex"].as<asset>().get_amount() );
   BOOST_REQUIRE_EQUAL( rex_pool["total_rex"].as<asset>(),      get_rex_balance(alice) + get_rex_balance(bob) );

   // bob rents cpu for carol
   const asset fee = core_sym::from_string("7.0000");
   BOOST_REQUIRE_EQUAL( success(),               rentcpu( bob, carol, fee ) );
   rex_pool = get_rex_pool();
   BOOST_REQUIRE_EQUAL( init_tot_lendable,       rex_pool["total_lendable"].as<asset>() );
   BOOST_REQUIRE_EQUAL( init_tot_rent + fee,     rex_pool["total_rent"].as<asset>() );

   produce_block( fc::days(5) );
   produce_blocks(2);
   const asset rex_tok = asset::from_string("1.0000 REX");
   BOOST_REQUIRE_EQUAL( success(),                                           sellrex( alice, get_rex_balance(alice) - rex_tok ) );
   BOOST_REQUIRE_EQUAL( false,                                               get_rex_order_obj( alice ).is_null() );
   BOOST_REQUIRE_EQUAL( success(),                                           sellrex( alice, rex_tok ) );
   BOOST_REQUIRE_EQUAL( sellrex( alice, rex_tok ),                           wasm_assert_msg("insufficient funds for current and scheduled orders") );
   BOOST_REQUIRE_EQUAL( ratio * payment.get_amount() - rex_tok.get_amount(), get_rex_order( alice )["rex_requested"].as<asset>().get_amount() );
   BOOST_REQUIRE_EQUAL( success(),                                           consolidate( alice ) );
   BOOST_REQUIRE_EQUAL( 0,                                                   get_rex_balance_obj( alice )["rex_maturities"].get_array().size() );

   produce_block( fc::days(26) );
   produce_blocks(2);

   BOOST_REQUIRE_EQUAL( success(),  rexexec( alice, 2 ) );
   BOOST_REQUIRE_EQUAL( 0,          get_rex_balance( alice ).get_amount() );
   BOOST_REQUIRE_EQUAL( 0,          get_rex_balance_obj( alice )["matured_rex"].as<int64_t>() );
   const asset init_fund = get_rex_fund( alice );
   BOOST_REQUIRE_EQUAL( success(),  updaterex( alice ) );
   BOOST_REQUIRE_EQUAL( 0,          get_rex_balance( alice ).get_amount() );
   BOOST_REQUIRE_EQUAL( 0,          get_rex_balance_obj( alice )["matured_rex"].as<int64_t>() );
   BOOST_REQUIRE      ( init_fund < get_rex_fund( alice ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( buy_sell_claim_rex, eosio_system_tester ) try {

   const asset init_balance = core_sym::from_string("3000000.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n, "carolaccount"_n, "emilyaccount"_n, "frankaccount"_n };
   account_name alice = accounts[0], bob = accounts[1], carol = accounts[2], emily = accounts[3], frank = accounts[4];
   setup_rex_accounts( accounts, init_balance );

   const auto purchase1  = core_sym::from_string("50000.0000");
   const auto purchase2  = core_sym::from_string("105500.0000");
   const auto purchase3  = core_sym::from_string("104500.0000");
   const auto init_stake = get_voter_info(alice)["staked"].as<int64_t>();
   BOOST_REQUIRE_EQUAL( success(), buyrex( alice, purchase1) );
   BOOST_REQUIRE_EQUAL( success(), buyrex( bob,   purchase2) );
   BOOST_REQUIRE_EQUAL( success(), buyrex( carol, purchase3) );

   BOOST_REQUIRE_EQUAL( init_balance - purchase1, get_rex_fund(alice) );
   BOOST_REQUIRE_EQUAL( purchase1.get_amount(),   get_voter_info(alice)["staked"].as<int64_t>() - init_stake );

   BOOST_REQUIRE_EQUAL( init_balance - purchase2, get_rex_fund(bob) );
   BOOST_REQUIRE_EQUAL( init_balance - purchase3, get_rex_fund(carol) );

   auto init_alice_rex = get_rex_balance(alice);
   auto init_bob_rex   = get_rex_balance(bob);
   auto init_carol_rex = get_rex_balance(carol);

   BOOST_REQUIRE_EQUAL( core_sym::from_string("20000.0000"), get_rex_pool()["total_rent"].as<asset>() );

   for (uint8_t i = 0; i < 4; ++i) {
      BOOST_REQUIRE_EQUAL( success(), rentcpu( emily, emily, core_sym::from_string("12000.0000") ) );
   }

   produce_block( fc::days(25) );

   const asset rent_payment = core_sym::from_string("46000.0000");
   BOOST_REQUIRE_EQUAL( success(), rentcpu( frank, frank, rent_payment, rent_payment ) );

   produce_block( fc::days(4) );

   BOOST_REQUIRE_EQUAL( success(), rexexec( alice, 1 ) );

   const auto    init_rex_pool        = get_rex_pool();
   const int64_t total_lendable       = init_rex_pool["total_lendable"].as<asset>().get_amount();
   const int64_t total_rex            = init_rex_pool["total_rex"].as<asset>().get_amount();
   [[maybe_unused]] const int64_t init_alice_rex_stake = ( eosio::chain::uint128_t(init_alice_rex.get_amount()) * total_lendable ) / total_rex;

   BOOST_REQUIRE_EQUAL( success(), sellrex( alice, asset( 3 * get_rex_balance(alice).get_amount() / 4, symbol{SY(4,REX)} ) ) );

   BOOST_TEST_REQUIRE( within_one( init_alice_rex.get_amount() / 4, get_rex_balance(alice).get_amount() ) );

   init_alice_rex = get_rex_balance(alice);
   BOOST_REQUIRE_EQUAL( success(), sellrex( bob,   get_rex_balance(bob) ) );
   BOOST_REQUIRE_EQUAL( success(), sellrex( carol, get_rex_balance(carol) ) );
   BOOST_REQUIRE_EQUAL( success(), sellrex( alice, get_rex_balance(alice) ) );

   BOOST_REQUIRE_EQUAL( init_bob_rex,   get_rex_balance(bob) );
   BOOST_REQUIRE_EQUAL( init_carol_rex, get_rex_balance(carol) );
   BOOST_REQUIRE_EQUAL( init_alice_rex, get_rex_balance(alice) );

   // now bob's, carol's and alice's sellrex orders have been queued
   BOOST_REQUIRE_EQUAL( true,           get_rex_order(alice)["is_open"].as<bool>() );
   BOOST_REQUIRE_EQUAL( init_alice_rex, get_rex_order(alice)["rex_requested"].as<asset>() );
   BOOST_REQUIRE_EQUAL( 0,              get_rex_order(alice)["proceeds"].as<asset>().get_amount() );
   BOOST_REQUIRE_EQUAL( true,           get_rex_order(bob)["is_open"].as<bool>() );
   BOOST_REQUIRE_EQUAL( init_bob_rex,   get_rex_order(bob)["rex_requested"].as<asset>() );
   BOOST_REQUIRE_EQUAL( 0,              get_rex_order(bob)["proceeds"].as<asset>().get_amount() );
   BOOST_REQUIRE_EQUAL( true,           get_rex_order(carol)["is_open"].as<bool>() );
   BOOST_REQUIRE_EQUAL( init_carol_rex, get_rex_order(carol)["rex_requested"].as<asset>() );
   BOOST_REQUIRE_EQUAL( 0,              get_rex_order(carol)["proceeds"].as<asset>().get_amount() );

   // wait for a total of 30 days minus 1 hour
   produce_block( fc::hours(23) );
   BOOST_REQUIRE_EQUAL( success(),      updaterex( alice ) );
   BOOST_REQUIRE_EQUAL( true,           get_rex_order(alice)["is_open"].as<bool>() );
   BOOST_REQUIRE_EQUAL( true,           get_rex_order(bob)["is_open"].as<bool>() );
   BOOST_REQUIRE_EQUAL( true,           get_rex_order(carol)["is_open"].as<bool>() );

   // wait for 2 more hours, by now frank's loan has expired and there is enough balance in
   // total_unlent to close some sellrex orders. only two are processed, bob's and carol's.
   // alices's order is still open.
   // an action is needed to trigger queue processing
   produce_block( fc::hours(2) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("rex loans are currently not available"),
                        rentcpu( frank, frank, core_sym::from_string("0.0001") ) );
   {
      auto trace = base_tester::push_action( config::system_account_name, "rexexec"_n, frank,
                                             mvo()("user", frank)("max", 2) );
      auto output = get_rexorder_result( trace );
      BOOST_REQUIRE_EQUAL( output.size(),    1 );
      BOOST_REQUIRE_EQUAL( output[0].first,  bob );
      BOOST_REQUIRE_EQUAL( output[0].second, get_rex_order(bob)["proceeds"].as<asset>() );
   }

   {
      BOOST_REQUIRE_EQUAL( false,          get_rex_order(bob)["is_open"].as<bool>() );
      BOOST_REQUIRE_EQUAL( init_bob_rex,   get_rex_order(bob)["rex_requested"].as<asset>() );
      BOOST_TEST_REQUIRE ( 0 <             get_rex_order(bob)["proceeds"].as<asset>().get_amount() );

      BOOST_REQUIRE_EQUAL( true,           get_rex_order(alice)["is_open"].as<bool>() );
      BOOST_REQUIRE_EQUAL( init_alice_rex, get_rex_order(alice)["rex_requested"].as<asset>() );
      BOOST_REQUIRE_EQUAL( 0,              get_rex_order(alice)["proceeds"].as<asset>().get_amount() );

      BOOST_REQUIRE_EQUAL( true,           get_rex_order(carol)["is_open"].as<bool>() );
      BOOST_REQUIRE_EQUAL( init_carol_rex, get_rex_order(carol)["rex_requested"].as<asset>() );
      BOOST_REQUIRE_EQUAL( 0,              get_rex_order(carol)["proceeds"].as<asset>().get_amount() );

      BOOST_REQUIRE_EQUAL( wasm_assert_msg("rex loans are currently not available"),
                           rentcpu( frank, frank, core_sym::from_string("1.0000") ) );
   }

   produce_blocks(2);
   produce_block( fc::hours(13) );
   produce_blocks(2);
   produce_block( fc::days(30) );
   produce_blocks(2);

   {
      auto trace1 = base_tester::push_action( config::system_account_name, "updaterex"_n, bob, mvo()("owner", bob) );
      auto trace2 = base_tester::push_action( config::system_account_name, "updaterex"_n, carol, mvo()("owner", carol) );
      BOOST_REQUIRE_EQUAL( 0,              get_rex_vote_stake( bob ).get_amount() );
      BOOST_REQUIRE_EQUAL( init_stake,     get_voter_info( bob )["staked"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( 0,              get_rex_vote_stake( carol ).get_amount() );
      BOOST_REQUIRE_EQUAL( init_stake,     get_voter_info( carol )["staked"].as<int64_t>() );
      auto output1 = get_rexorder_result( trace1 );
      auto output2 = get_rexorder_result( trace2 );
      BOOST_REQUIRE_EQUAL( 2,              output1.size() + output2.size() );

      BOOST_REQUIRE_EQUAL( false,          get_rex_order_obj(alice).is_null() );
      BOOST_REQUIRE_EQUAL( true,           get_rex_order_obj(bob).is_null() );
      BOOST_REQUIRE_EQUAL( true,           get_rex_order_obj(carol).is_null() );
      BOOST_REQUIRE_EQUAL( false,          get_rex_order(alice)["is_open"].as<bool>() );

      const auto& rex_pool = get_rex_pool();
      BOOST_REQUIRE_EQUAL( 0,              rex_pool["total_lendable"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( 0,              rex_pool["total_rex"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("rex loans are currently not available"),
                           rentcpu( frank, frank, core_sym::from_string("1.0000") ) );

      BOOST_REQUIRE_EQUAL( success(),      buyrex( emily, core_sym::from_string("22000.0000") ) );
      BOOST_REQUIRE_EQUAL( false,          get_rex_order_obj(alice).is_null() );
      BOOST_REQUIRE_EQUAL( false,          get_rex_order(alice)["is_open"].as<bool>() );

      BOOST_REQUIRE_EQUAL( success(),      rentcpu( frank, frank, core_sym::from_string("1.0000") ) );
   }

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( rex_loans, eosio_system_tester ) try {

   const asset   init_balance = core_sym::from_string("40000.0000");
   const asset   one_unit     = core_sym::from_string("0.0001");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n, "carolaccount"_n, "emilyaccount"_n, "frankaccount"_n };
   account_name alice = accounts[0], bob = accounts[1], emily = accounts[3], frank = accounts[4];
   setup_rex_accounts( accounts, init_balance  );

   BOOST_REQUIRE_EQUAL( success(), buyrex( alice, core_sym::from_string("25000.0000") ) );

   auto rex_pool            = get_rex_pool();
   const asset payment      = core_sym::from_string("30.0000");
   const asset zero_asset   = core_sym::from_string("0.0000");
   const asset neg_asset    = core_sym::from_string("-1.0000");
   BOOST_TEST_REQUIRE( 0 > neg_asset.get_amount() );
   asset cur_frank_balance  = get_rex_fund( frank );
   int64_t expected_stake   = bancor_convert( rex_pool["total_rent"].as<asset>().get_amount(),
                                              rex_pool["total_unlent"].as<asset>().get_amount(),
                                              payment.get_amount() );
   const int64_t init_stake = get_cpu_limit( frank );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg("must use core token"),
                        rentcpu( frank, bob, asset::from_string("10.0000 RND") ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("must use core token"),
                        rentcpu( frank, bob, payment, asset::from_string("10.0000 RND") ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("must use positive asset amount"),
                        rentcpu( frank, bob, zero_asset ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("must use positive asset amount"),
                        rentcpu( frank, bob, payment, neg_asset ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("must use positive asset amount"),
                        rentcpu( frank, bob, neg_asset, payment ) );
   // create 2 cpu and 3 net loans
   const asset rented_tokens{ expected_stake, symbol{CORE_SYM} };
   BOOST_REQUIRE_EQUAL( rented_tokens,  get_rentcpu_result( frank, bob, payment ) ); // loan_num = 1
   BOOST_REQUIRE_EQUAL( success(),      rentcpu( alice, emily, payment ) );          // loan_num = 2
   BOOST_REQUIRE_EQUAL( 2,              get_last_cpu_loan()["loan_num"].as_uint64() );

   asset expected_rented_net;
   {
      const auto& pool = get_rex_pool();
      const int64_t r  = bancor_convert( pool["total_rent"].as<asset>().get_amount(),
                                         pool["total_unlent"].as<asset>().get_amount(),
                                         payment.get_amount() );
      expected_rented_net = asset{ r, symbol{CORE_SYM} };
   }
   BOOST_REQUIRE_EQUAL( expected_rented_net, get_rentnet_result( alice, emily, payment ) ); // loan_num = 3
   BOOST_REQUIRE_EQUAL( success(),           rentnet( alice, alice, payment ) );            // loan_num = 4
   BOOST_REQUIRE_EQUAL( success(),           rentnet( alice, frank, payment ) );            // loan_num = 5
   BOOST_REQUIRE_EQUAL( 5,                   get_last_net_loan()["loan_num"].as_uint64() );

   auto loan_info         = get_cpu_loan(1);
   auto old_frank_balance = cur_frank_balance;
   cur_frank_balance      = get_rex_fund( frank );
   BOOST_REQUIRE_EQUAL( old_frank_balance,           payment + cur_frank_balance );
   BOOST_REQUIRE_EQUAL( 1,                           loan_info["loan_num"].as_uint64() );
   BOOST_REQUIRE_EQUAL( payment,                     loan_info["payment"].as<asset>() );
   BOOST_REQUIRE_EQUAL( 0,                           loan_info["balance"].as<asset>().get_amount() );
   BOOST_REQUIRE_EQUAL( expected_stake,              loan_info["total_staked"].as<asset>().get_amount() );
   BOOST_REQUIRE_EQUAL( expected_stake + init_stake, get_cpu_limit( bob ) );

   // frank funds his loan enough to be renewed once
   const asset fund = core_sym::from_string("35.0000");
   BOOST_REQUIRE_EQUAL( fundcpuloan( frank, 1, cur_frank_balance + one_unit), wasm_assert_msg("insufficient funds") );
   BOOST_REQUIRE_EQUAL( fundnetloan( frank, 1, fund ), wasm_assert_msg("loan not found") );
   BOOST_REQUIRE_EQUAL( fundcpuloan( alice, 1, fund ), wasm_assert_msg("user must be loan creator") );
   BOOST_REQUIRE_EQUAL( success(),                     fundcpuloan( frank, 1, fund ) );
   old_frank_balance = cur_frank_balance;
   cur_frank_balance = get_rex_fund( frank );
   loan_info         = get_cpu_loan(1);
   BOOST_REQUIRE_EQUAL( old_frank_balance, fund + cur_frank_balance );
   BOOST_REQUIRE_EQUAL( fund,              loan_info["balance"].as<asset>() );
   BOOST_REQUIRE_EQUAL( payment,           loan_info["payment"].as<asset>() );

   // in the meantime, defund then fund the same amount and test the balances
   {
      const asset amount = core_sym::from_string("7.5000");
      BOOST_REQUIRE_EQUAL( defundnetloan( frank, 1, fund ),                             wasm_assert_msg("loan not found") );
      BOOST_REQUIRE_EQUAL( defundcpuloan( alice, 1, fund ),                             wasm_assert_msg("user must be loan creator") );
      BOOST_REQUIRE_EQUAL( defundcpuloan( frank, 1, core_sym::from_string("75.0000") ), wasm_assert_msg("insufficent loan balance") );
      old_frank_balance = cur_frank_balance;
      asset old_loan_balance = get_cpu_loan(1)["balance"].as<asset>();
      BOOST_REQUIRE_EQUAL( defundcpuloan( frank, 1, amount ), success() );
      BOOST_REQUIRE_EQUAL( old_loan_balance,                  get_cpu_loan(1)["balance"].as<asset>() + amount );
      cur_frank_balance = get_rex_fund( frank );
      old_loan_balance  = get_cpu_loan(1)["balance"].as<asset>();
      BOOST_REQUIRE_EQUAL( old_frank_balance + amount,        cur_frank_balance );
      old_frank_balance = cur_frank_balance;
      BOOST_REQUIRE_EQUAL( fundcpuloan( frank, 1, amount ),   success() );
      BOOST_REQUIRE_EQUAL( old_loan_balance + amount,         get_cpu_loan(1)["balance"].as<asset>() );
      cur_frank_balance = get_rex_fund( frank );
      BOOST_REQUIRE_EQUAL( old_frank_balance - amount,        cur_frank_balance );
   }

   // wait for 30 days, frank's loan will be renewed at the current price
   produce_block( fc::minutes(30*24*60 - 1) );
   BOOST_REQUIRE_EQUAL( success(), updaterex( alice ) );
   rex_pool = get_rex_pool();
   {
      int64_t unlent_tokens = bancor_convert( rex_pool["total_unlent"].as<asset>().get_amount(),
                                              rex_pool["total_rent"].as<asset>().get_amount(),
                                              expected_stake );

      expected_stake = bancor_convert( rex_pool["total_rent"].as<asset>().get_amount() - unlent_tokens,
                                       rex_pool["total_unlent"].as<asset>().get_amount() + expected_stake,
                                       payment.get_amount() );
   }

   produce_block( fc::minutes(2) );

   BOOST_REQUIRE_EQUAL( success(), sellrex( alice, asset::from_string("1.0000 REX") ) );

   loan_info = get_cpu_loan(1);
   BOOST_REQUIRE_EQUAL( payment,                     loan_info["payment"].as<asset>() );
   BOOST_REQUIRE_EQUAL( fund - payment,              loan_info["balance"].as<asset>() );
   BOOST_REQUIRE_EQUAL( expected_stake,              loan_info["total_staked"].as<asset>().get_amount() );
   BOOST_REQUIRE_EQUAL( expected_stake + init_stake, get_cpu_limit( bob ) );

   // check that loans have been processed in order
   BOOST_REQUIRE_EQUAL( false, get_cpu_loan(1).is_null() );
   BOOST_REQUIRE_EQUAL( true,  get_cpu_loan(2).is_null() );
   BOOST_REQUIRE_EQUAL( true,  get_net_loan(3).is_null() );
   BOOST_REQUIRE_EQUAL( true,  get_net_loan(4).is_null() );
   BOOST_REQUIRE_EQUAL( false, get_net_loan(5).is_null() );
   BOOST_REQUIRE_EQUAL( 1,     get_last_cpu_loan()["loan_num"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 5,     get_last_net_loan()["loan_num"].as_uint64() );

   // wait for another month, frank's loan doesn't have enough funds and will be closed
   produce_block( fc::hours(30*24) );
   BOOST_REQUIRE_EQUAL( success(),  buyrex( alice, core_sym::from_string("10.0000") ) );
   BOOST_REQUIRE_EQUAL( true,       get_cpu_loan(1).is_null() );
   BOOST_REQUIRE_EQUAL( init_stake, get_cpu_limit( bob ) );
   old_frank_balance = cur_frank_balance;
   cur_frank_balance = get_rex_fund( frank );
   BOOST_REQUIRE_EQUAL( fund - payment,     cur_frank_balance - old_frank_balance );
   BOOST_REQUIRE      ( old_frank_balance < cur_frank_balance );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( rex_loan_checks, eosio_system_tester ) try {

   const asset   init_balance = core_sym::from_string("40000.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n };
   account_name alice = accounts[0], bob = accounts[1];
   setup_rex_accounts( accounts, init_balance );

   const asset payment1 = core_sym::from_string("20000.0000");
   const asset payment2 = core_sym::from_string("4.0000");
   const asset fee      = core_sym::from_string("1.0000");
   BOOST_REQUIRE_EQUAL( success(), buyrex( alice, payment1 ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("loan price does not favor renting"),
                        rentcpu( bob, bob, fee ) );
   BOOST_REQUIRE_EQUAL( success(),            buyrex( alice, payment2 ) );
   BOOST_REQUIRE_EQUAL( success(),            rentcpu( bob, bob, fee, fee + fee + fee ) );
   BOOST_REQUIRE_EQUAL( true,                 !get_cpu_loan(1).is_null() );
   BOOST_REQUIRE_EQUAL( 3 * fee.get_amount(), get_last_cpu_loan()["balance"].as<asset>().get_amount() );

   produce_block( fc::days(31) );
   BOOST_REQUIRE_EQUAL( success(),            rexexec( alice, 3) );
   BOOST_REQUIRE_EQUAL( 2 * fee.get_amount(), get_last_cpu_loan()["balance"].as<asset>().get_amount() );

   BOOST_REQUIRE_EQUAL( success(),            sellrex( alice, asset::from_string("1000000.0000 REX") ) );
   produce_block( fc::days(31) );
   BOOST_REQUIRE_EQUAL( success(),            rexexec( alice, 3) );
   BOOST_REQUIRE_EQUAL( true,                 get_cpu_loan(1).is_null() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( ramfee_namebid_to_rex, eosio_system_tester ) try {

   const asset   init_balance = core_sym::from_string("10000.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n, "carolaccount"_n, "emilyaccount"_n, "frankaccount"_n };
   account_name alice = accounts[0], bob = accounts[1], carol = accounts[2], frank = accounts[4];
   setup_rex_accounts( accounts, init_balance, core_sym::from_string("80.0000"), core_sym::from_string("80.0000"), false );

   asset cur_fees_balance = get_balance( "eosio.fees"_n );
   BOOST_REQUIRE_EQUAL( success(),                      buyram( alice, alice, core_sym::from_string("20.0000") ) );
   BOOST_REQUIRE_EQUAL( get_balance( "eosio.fees"_n ), core_sym::from_string("0.1000") + cur_fees_balance );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("must deposit to REX fund first"),
                        buyrex( alice, core_sym::from_string("350.0000") ) );
   BOOST_REQUIRE_EQUAL( success(),                      deposit( alice, core_sym::from_string("350.0000") ) );
   BOOST_REQUIRE_EQUAL( success(),                      buyrex( alice, core_sym::from_string("350.0000") ) );
   cur_fees_balance = get_balance( "eosio.fees"_n );
   asset cur_rex_balance = get_balance( "eosio.rex"_n );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("350.0000"), cur_rex_balance );
   BOOST_REQUIRE_EQUAL( success(),                         buyram( bob, carol, core_sym::from_string("70.0000") ) );
   BOOST_REQUIRE_EQUAL( cur_fees_balance,                get_balance( "eosio.fees"_n ) );
   BOOST_REQUIRE_EQUAL( get_balance( "eosio.rex"_n ),       cur_rex_balance + core_sym::from_string("0.3500") );

   cur_rex_balance = get_balance( "eosio.rex"_n );

   produce_blocks( 1 );
   produce_block( fc::hours(30*24 + 12) );
   produce_blocks( 1 );

   BOOST_REQUIRE_EQUAL( success(),       rexexec( alice, 1 ) );
   auto cur_rex_pool = get_rex_pool();

   BOOST_TEST_REQUIRE(  within_one( cur_rex_balance.get_amount(), cur_rex_pool["total_unlent"].as<asset>().get_amount() ) );
   BOOST_TEST_REQUIRE(  cur_rex_balance >=                        cur_rex_pool["total_unlent"].as<asset>() );
   BOOST_REQUIRE_EQUAL( 0,                                        cur_rex_pool["total_lent"].as<asset>().get_amount() );
   BOOST_TEST_REQUIRE(  within_one( cur_rex_balance.get_amount(), cur_rex_pool["total_lendable"].as<asset>().get_amount() ) );
   BOOST_TEST_REQUIRE(  cur_rex_balance >=                        cur_rex_pool["total_lendable"].as<asset>() );
   BOOST_REQUIRE_EQUAL( 0,                                        cur_rex_pool["namebid_proceeds"].as<asset>().get_amount() );

   // required for closing namebids
   cross_15_percent_threshold();
   produce_block( fc::days(14) );

   cur_rex_balance = get_balance( "eosio.rex"_n );
   BOOST_REQUIRE_EQUAL( success(),                        bidname( carol, "rndmbid"_n, core_sym::from_string("23.7000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("23.7000"), get_balance( "eosio.names"_n ) );
   BOOST_REQUIRE_EQUAL( success(),                        bidname( alice, "rndmbid"_n, core_sym::from_string("29.3500") ) );
   // refund carol, the losing bid
   BOOST_REQUIRE_EQUAL( success(), push_action( carol, "bidrefund"_n, mvo()("bidder","carolaccount")("newname", "rndmbid") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("29.3500"), get_balance( "eosio.names"_n ));

   produce_block( fc::hours(24) );
   produce_blocks( 2 );

   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_rex_pool()["namebid_proceeds"].as<asset>() );
   BOOST_REQUIRE_EQUAL( success(),                        deposit( frank, core_sym::from_string("5.0000") ) );
   BOOST_REQUIRE_EQUAL( success(),                        buyrex( frank, core_sym::from_string("5.0000") ) );
   BOOST_REQUIRE_EQUAL( get_balance( "eosio.rex"_n ),      cur_rex_balance + core_sym::from_string("34.3500") );
   BOOST_REQUIRE_EQUAL( 0,                                get_balance( "eosio.names"_n ).get_amount() );

   cur_rex_balance = get_balance( "eosio.rex"_n );
   produce_block( fc::hours(30*24 + 13) );
   produce_blocks( 1 );

   BOOST_REQUIRE_EQUAL( success(),                        rexexec( alice, 1 ) );
   BOOST_REQUIRE_EQUAL( cur_rex_balance,                  get_rex_pool()["total_lendable"].as<asset>() );
   BOOST_REQUIRE_EQUAL( get_rex_pool()["total_lendable"].as<asset>(),
                        get_rex_pool()["total_unlent"].as<asset>() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( rex_maturity, eosio_system_tester ) try {

   const asset init_balance = core_sym::from_string("1000000.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n };
   account_name alice = accounts[0], bob = accounts[1];
   setup_rex_accounts( accounts, init_balance );

   const int64_t rex_ratio = 10000;
   const symbol  rex_sym( SY(4, REX) );

   {
      BOOST_REQUIRE_EQUAL( success(), buyrex( alice, core_sym::from_string("11.5000") ) );
      produce_block( fc::hours(3) );
      BOOST_REQUIRE_EQUAL( success(), buyrex( alice, core_sym::from_string("18.5000") ) );
      produce_block( fc::hours(25) );
      BOOST_REQUIRE_EQUAL( success(), buyrex( alice, core_sym::from_string("25.0000") ) );

      auto rex_balance = get_rex_balance_obj( alice );
      BOOST_REQUIRE_EQUAL( 550000 * rex_ratio, rex_balance["rex_balance"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( 0,                  rex_balance["matured_rex"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( 2,                  rex_balance["rex_maturities"].get_array().size() );

      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),
                           sellrex( alice, asset::from_string("115000.0000 REX") ) );
      produce_block( fc::hours( 3*24 + 20) );
      BOOST_REQUIRE_EQUAL( success(),          sellrex( alice, asset::from_string("300000.0000 REX") ) );
      rex_balance = get_rex_balance_obj( alice );
      BOOST_REQUIRE_EQUAL( 250000 * rex_ratio, rex_balance["rex_balance"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( 0,                  rex_balance["matured_rex"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( 1,                  rex_balance["rex_maturities"].get_array().size() );
      produce_block( fc::hours(23) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),
                           sellrex( alice, asset::from_string("250000.0000 REX") ) );
      produce_block( fc::days(1) );
      BOOST_REQUIRE_EQUAL( success(),          sellrex( alice, asset::from_string("130000.0000 REX") ) );
      rex_balance = get_rex_balance_obj( alice );
      BOOST_REQUIRE_EQUAL( 1200000000,         rex_balance["rex_balance"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( 1200000000,         rex_balance["matured_rex"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( 0,                  rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),
                           sellrex( alice, asset::from_string("130000.0000 REX") ) );
      BOOST_REQUIRE_EQUAL( success(),          sellrex( alice, asset::from_string("120000.0000 REX") ) );
      rex_balance = get_rex_balance_obj( alice );
      BOOST_REQUIRE_EQUAL( 0,                  rex_balance["rex_balance"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( 0,                  rex_balance["matured_rex"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( 0,                  rex_balance["rex_maturities"].get_array().size() );
   }

   {
      const asset payment1 = core_sym::from_string("14.8000");
      const asset payment2 = core_sym::from_string("15.2000");
      const asset payment  = payment1 + payment2;
      const asset rex_bucket( rex_ratio * payment.get_amount(), rex_sym );
      for ( uint8_t i = 0; i < 8; ++i ) {
         BOOST_REQUIRE_EQUAL( success(), buyrex( bob, payment1 ) );
         produce_block( fc::hours(2) );
         BOOST_REQUIRE_EQUAL( success(), buyrex( bob, payment2 ) );
         produce_block( fc::hours(24) );
      }

      auto rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 8 * rex_bucket.get_amount(), rex_balance["rex_balance"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( 5,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 3 * rex_bucket.get_amount(), rex_balance["matured_rex"].as<int64_t>() );

      BOOST_REQUIRE_EQUAL( success(),                   updaterex( bob ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 4,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 4 * rex_bucket.get_amount(), rex_balance["matured_rex"].as<int64_t>() );

      produce_block( fc::hours(2) );
      BOOST_REQUIRE_EQUAL( success(),                   updaterex( bob ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 4,                           rex_balance["rex_maturities"].get_array().size() );

      produce_block( fc::hours(1) );
      BOOST_REQUIRE_EQUAL( success(),                   sellrex( bob, asset( 3 * rex_bucket.get_amount(), rex_sym ) ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 4,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( rex_bucket.get_amount(),     rex_balance["matured_rex"].as<int64_t>() );

      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),
                           sellrex( bob, asset( 2 * rex_bucket.get_amount(), rex_sym ) ) );
      BOOST_REQUIRE_EQUAL( success(),                   sellrex( bob, asset( rex_bucket.get_amount(), rex_sym ) ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 4 * rex_bucket.get_amount(), rex_balance["rex_balance"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( 4,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 0,                           rex_balance["matured_rex"].as<int64_t>() );

      produce_block( fc::hours(23) );
      BOOST_REQUIRE_EQUAL( success(),                   updaterex( bob ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 3,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( rex_bucket.get_amount(),     rex_balance["matured_rex"].as<int64_t>() );

      BOOST_REQUIRE_EQUAL( success(),                   consolidate( bob ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 1,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 0,                           rex_balance["matured_rex"].as<int64_t>() );

      produce_block( fc::days(3) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),
                           sellrex( bob, asset( 4 * rex_bucket.get_amount(), rex_sym ) ) );
      produce_block( fc::hours(24 + 20) );
      BOOST_REQUIRE_EQUAL( success(),                   sellrex( bob, asset( 4 * rex_bucket.get_amount(), rex_sym ) ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 0,                           rex_balance["rex_balance"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( 0,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 0,                           rex_balance["matured_rex"].as<int64_t>() );
   }

   {
      const asset payment1 = core_sym::from_string("250000.0000");
      const asset payment2 = core_sym::from_string("10000.0000");
      const asset rex_bucket1( rex_ratio * payment1.get_amount(), rex_sym );
      const asset rex_bucket2( rex_ratio * payment2.get_amount(), rex_sym );
      const asset tot_rex = rex_bucket1 + rex_bucket2;

      BOOST_REQUIRE_EQUAL( success(), buyrex( bob, payment1 ) );
      produce_block( fc::days(3) );
      BOOST_REQUIRE_EQUAL( success(), buyrex( bob, payment2 ) );
      produce_block( fc::days(2) );
      BOOST_REQUIRE_EQUAL( success(), updaterex( bob ) );

      auto rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( tot_rex,                  rex_balance["rex_balance"].as<asset>() );
      BOOST_REQUIRE_EQUAL( rex_bucket1.get_amount(), rex_balance["matured_rex"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( success(),                rentcpu( alice, alice, core_sym::from_string("8000.0000") ) );
      BOOST_REQUIRE_EQUAL( success(),                sellrex( bob, asset( rex_bucket1.get_amount() - 20, rex_sym ) ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( rex_bucket1.get_amount(), get_rex_order( bob )["rex_requested"].as<asset>().get_amount() + 20 );
      BOOST_REQUIRE_EQUAL( tot_rex,                  rex_balance["rex_balance"].as<asset>() );
      BOOST_REQUIRE_EQUAL( rex_bucket1.get_amount(), rex_balance["matured_rex"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( success(),                consolidate( bob ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( rex_bucket1.get_amount(), rex_balance["matured_rex"].as<int64_t>() + 20 );
      BOOST_REQUIRE_EQUAL( success(),                cancelrexorder( bob ) );
      BOOST_REQUIRE_EQUAL( success(),                consolidate( bob ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 0,                        rex_balance["matured_rex"].as<int64_t>() );
   }

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( rex_savings, eosio_system_tester ) try {

   const asset init_balance = core_sym::from_string("100000.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n, "carolaccount"_n, "emilyaccount"_n, "frankaccount"_n };
   account_name alice = accounts[0], bob = accounts[1], carol = accounts[2];
   setup_rex_accounts( accounts, init_balance );

   const int64_t rex_ratio = 10000;
   const symbol  rex_sym( SY(4, REX) );

   {
      const asset payment1 = core_sym::from_string("14.8000");
      const asset payment2 = core_sym::from_string("15.2000");
      const asset payment  = payment1 + payment2;
      const asset rex_bucket( rex_ratio * payment.get_amount(), rex_sym );
      for ( uint8_t i = 0; i < 8; ++i ) {
         BOOST_REQUIRE_EQUAL( success(), buyrex( alice, payment1 ) );
         produce_block( fc::hours(12) );
         BOOST_REQUIRE_EQUAL( success(), buyrex( alice, payment2 ) );
         produce_block( fc::hours(14) );
      }

      auto rex_balance = get_rex_balance_obj( alice );
      BOOST_REQUIRE_EQUAL( 8 * rex_bucket.get_amount(), rex_balance["rex_balance"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( 5,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 4 * rex_bucket.get_amount(), rex_balance["matured_rex"].as<int64_t>() );

      BOOST_REQUIRE_EQUAL( success(),                   mvtosavings( alice, asset( 8 * rex_bucket.get_amount(), rex_sym ) ) );
      rex_balance = get_rex_balance_obj( alice );
      BOOST_REQUIRE_EQUAL( 1,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 0,                           rex_balance["matured_rex"].as<int64_t>() );
      produce_block( fc::days(1000) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),
                           sellrex( alice, asset::from_string( "1.0000 REX" ) ) );
      BOOST_REQUIRE_EQUAL( success(),                   mvfrsavings( alice, asset::from_string( "10.0000 REX" ) ) );
      rex_balance = get_rex_balance_obj( alice );
      BOOST_REQUIRE_EQUAL( 2,                           rex_balance["rex_maturities"].get_array().size() );
      produce_block( fc::days(3) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),
                           sellrex( alice, asset::from_string( "1.0000 REX" ) ) );
      produce_blocks( 2 );
      produce_block( fc::days(2) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),
                           sellrex( alice, asset::from_string( "10.0001 REX" ) ) );
      BOOST_REQUIRE_EQUAL( success(),                   sellrex( alice, asset::from_string( "10.0000 REX" ) ) );
      rex_balance = get_rex_balance_obj( alice );
      BOOST_REQUIRE_EQUAL( 1,                           rex_balance["rex_maturities"].get_array().size() );
      produce_block( fc::days(100) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),
                           sellrex( alice, asset::from_string( "0.0001 REX" ) ) );
      BOOST_REQUIRE_EQUAL( success(),                   mvfrsavings( alice, get_rex_balance( alice ) ) );
      produce_block( fc::days(5) );
      BOOST_REQUIRE_EQUAL( success(),                   sellrex( alice, get_rex_balance( alice ) ) );
   }

   {
      const asset payment = core_sym::from_string("20.0000");
      const asset rex_bucket( rex_ratio * payment.get_amount(), rex_sym );
      for ( uint8_t i = 0; i < 5; ++i ) {
         produce_block( fc::hours(24) );
         BOOST_REQUIRE_EQUAL( success(), buyrex( bob, payment ) );
      }

      auto rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 5 * rex_bucket.get_amount(), rex_balance["rex_balance"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( 5,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 0,                           rex_balance["matured_rex"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( success(),                   mvtosavings( bob, asset( rex_bucket.get_amount() / 2, rex_sym ) ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 6,                           rex_balance["rex_maturities"].get_array().size() );

      BOOST_REQUIRE_EQUAL( success(),                   mvtosavings( bob, asset( rex_bucket.get_amount() / 2, rex_sym ) ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 5,                           rex_balance["rex_maturities"].get_array().size() );
      produce_block( fc::days(1) );
      BOOST_REQUIRE_EQUAL( success(),                   sellrex( bob, rex_bucket ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 4,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 0,                           rex_balance["matured_rex"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( 4 * rex_bucket.get_amount(), rex_balance["rex_balance"].as<asset>().get_amount() );

      BOOST_REQUIRE_EQUAL( success(),                   mvtosavings( bob, asset( 3 * rex_bucket.get_amount() / 2, rex_sym ) ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 3,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),
                           sellrex( bob, rex_bucket ) );

      produce_block( fc::days(1) );
      BOOST_REQUIRE_EQUAL( success(),                   sellrex( bob, rex_bucket ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 2,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 0,                           rex_balance["matured_rex"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( 3 * rex_bucket.get_amount(), rex_balance["rex_balance"].as<asset>().get_amount() );

      produce_block( fc::days(1) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),
                           sellrex( bob, rex_bucket ) );
      BOOST_REQUIRE_EQUAL( success(),                   sellrex( bob, asset( rex_bucket.get_amount() / 2, rex_sym ) ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 1,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 0,                           rex_balance["matured_rex"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( 5 * rex_bucket.get_amount(), 2 * rex_balance["rex_balance"].as<asset>().get_amount() );

      produce_block( fc::days(20) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),
                           sellrex( bob, rex_bucket ) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient REX in savings"),
                           mvfrsavings( bob, asset( 3 * rex_bucket.get_amount(), rex_sym ) ) );
      BOOST_REQUIRE_EQUAL( success(),                   mvfrsavings( bob, rex_bucket ) );
      BOOST_REQUIRE_EQUAL( 2,                           get_rex_balance_obj( bob )["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient REX balance"),
                           mvtosavings( bob, asset( 3 * rex_bucket.get_amount() / 2, rex_sym ) ) );
      produce_block( fc::days(1) );
      BOOST_REQUIRE_EQUAL( success(),                   mvfrsavings( bob, rex_bucket ) );
      BOOST_REQUIRE_EQUAL( 3,                           get_rex_balance_obj( bob )["rex_maturities"].get_array().size() );
      produce_block( fc::days(4) );
      BOOST_REQUIRE_EQUAL( success(),                   sellrex( bob, rex_bucket ) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),
                           sellrex( bob, rex_bucket ) );
      produce_block( fc::days(1) );
      BOOST_REQUIRE_EQUAL( success(),                   sellrex( bob, rex_bucket ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 1,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( rex_bucket.get_amount() / 2, rex_balance["rex_balance"].as<asset>().get_amount() );

      BOOST_REQUIRE_EQUAL( success(),                   mvfrsavings( bob, asset( rex_bucket.get_amount() / 4, rex_sym ) ) );
      produce_block( fc::days(2) );
      BOOST_REQUIRE_EQUAL( success(),                   mvfrsavings( bob, asset( rex_bucket.get_amount() / 8, rex_sym ) ) );
      BOOST_REQUIRE_EQUAL( 3,                           get_rex_balance_obj( bob )["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( success(),                   consolidate( bob ) );
      BOOST_REQUIRE_EQUAL( 2,                           get_rex_balance_obj( bob )["rex_maturities"].get_array().size() );

      produce_block( fc::days(5) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient available rex"),
                           sellrex( bob, asset( rex_bucket.get_amount() / 2, rex_sym ) ) );
      BOOST_REQUIRE_EQUAL( success(),                   sellrex( bob, asset( 3 * rex_bucket.get_amount() / 8, rex_sym ) ) );
      rex_balance = get_rex_balance_obj( bob );
      BOOST_REQUIRE_EQUAL( 1,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 0,                           rex_balance["matured_rex"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( rex_bucket.get_amount() / 8, rex_balance["rex_balance"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( success(),                   mvfrsavings( bob, get_rex_balance( bob ) ) );
      produce_block( fc::days(5) );
      BOOST_REQUIRE_EQUAL( success(),                   sellrex( bob, get_rex_balance( bob ) ) );
   }

   {
      const asset payment = core_sym::from_string("40000.0000");
      const int64_t rex_bucket_amount = rex_ratio * payment.get_amount();
      const asset rex_bucket( rex_bucket_amount, rex_sym );
      BOOST_REQUIRE_EQUAL( success(),  buyrex( alice, payment ) );
      BOOST_REQUIRE_EQUAL( rex_bucket, get_rex_balance( alice ) );
      BOOST_REQUIRE_EQUAL( rex_bucket, get_rex_pool()["total_rex"].as<asset>() );

      produce_block( fc::days(5) );

      BOOST_REQUIRE_EQUAL( success(),                       rentcpu( bob, bob, core_sym::from_string("3000.0000") ) );
      BOOST_REQUIRE_EQUAL( success(),                       sellrex( alice, asset( 9 * rex_bucket_amount / 10, rex_sym ) ) );
      BOOST_REQUIRE_EQUAL( rex_bucket,                      get_rex_balance( alice ) );
      BOOST_REQUIRE_EQUAL( success(),                       mvtosavings( alice, asset( rex_bucket_amount / 10, rex_sym ) ) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient REX balance"),
                           mvtosavings( alice, asset( rex_bucket_amount / 10, rex_sym ) ) );
      BOOST_REQUIRE_EQUAL( success(),                       cancelrexorder( alice ) );
      BOOST_REQUIRE_EQUAL( success(),                       mvtosavings( alice, asset( rex_bucket_amount / 10, rex_sym ) ) );
      auto rb = get_rex_balance_obj( alice );
      BOOST_REQUIRE_EQUAL( rb["matured_rex"].as<int64_t>(), 8 * rex_bucket_amount / 10 );
      BOOST_REQUIRE_EQUAL( success(),                       mvfrsavings( alice, asset( 2 * rex_bucket_amount / 10, rex_sym ) ) );
      produce_block( fc::days(31) );
      BOOST_REQUIRE_EQUAL( success(),                       sellrex( alice, get_rex_balance( alice ) ) );
   }

   {
      const asset   payment                = core_sym::from_string("250.0000");
      const asset   half_payment           = core_sym::from_string("125.0000");
      const int64_t rex_bucket_amount      = rex_ratio * payment.get_amount();
      const int64_t half_rex_bucket_amount = rex_bucket_amount / 2;
      const asset   rex_bucket( rex_bucket_amount, rex_sym );
      const asset   half_rex_bucket( half_rex_bucket_amount, rex_sym );

      BOOST_REQUIRE_EQUAL( success(),                   buyrex( carol, payment ) );
      BOOST_REQUIRE_EQUAL( rex_bucket,                  get_rex_balance( carol ) );
      auto rex_balance = get_rex_balance_obj( carol );

      BOOST_REQUIRE_EQUAL( 1,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 0,                           rex_balance["matured_rex"].as<int64_t>() );
      produce_block( fc::days(1) );
      BOOST_REQUIRE_EQUAL( success(),                   buyrex( carol, payment ) );
      rex_balance = get_rex_balance_obj( carol );
      BOOST_REQUIRE_EQUAL( 2,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 0,                           rex_balance["matured_rex"].as<int64_t>() );

      BOOST_REQUIRE_EQUAL( success(),                   mvtosavings( carol, half_rex_bucket ) );
      rex_balance = get_rex_balance_obj( carol );
      BOOST_REQUIRE_EQUAL( 3,                           rex_balance["rex_maturities"].get_array().size() );

      BOOST_REQUIRE_EQUAL( success(),                   buyrex( carol, half_payment ) );
      rex_balance = get_rex_balance_obj( carol );
      BOOST_REQUIRE_EQUAL( 3,                           rex_balance["rex_maturities"].get_array().size() );

      produce_block( fc::days(5) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("asset must be a positive amount of (REX, 4)"),
                           mvfrsavings( carol, asset::from_string("0.0000 REX") ) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("asset must be a positive amount of (REX, 4)"),
                           mvfrsavings( carol, asset::from_string("1.0000 RND") ) );
      BOOST_REQUIRE_EQUAL( success(),                   mvfrsavings( carol, half_rex_bucket ) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient REX in savings"),
                           mvfrsavings( carol, asset::from_string("0.0001 REX") ) );
      rex_balance = get_rex_balance_obj( carol );
      BOOST_REQUIRE_EQUAL( 1,                           rex_balance["rex_maturities"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 5 * half_rex_bucket_amount,  rex_balance["rex_balance"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( 2 * rex_bucket_amount,       rex_balance["matured_rex"].as<int64_t>() );
      produce_block( fc::days(5) );
      BOOST_REQUIRE_EQUAL( success(),                   sellrex( carol, get_rex_balance( carol) ) );
   }

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( update_rex, eosio_system_tester, * boost::unit_test::tolerance(1e-10) ) try {

   const asset init_balance = core_sym::from_string("30000.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n, "carolaccount"_n, "emilyaccount"_n };
   account_name alice = accounts[0], bob = accounts[1], emily = accounts[3];
   setup_rex_accounts( accounts, init_balance );

   const int64_t init_stake = get_voter_info( alice )["staked"].as<int64_t>();

   // alice buys rex
   const asset payment = core_sym::from_string("25000.0000");
   BOOST_REQUIRE_EQUAL( success(),                              buyrex( alice, payment ) );
   BOOST_REQUIRE_EQUAL( payment,                                get_rex_vote_stake(alice) );
   BOOST_REQUIRE_EQUAL( get_rex_vote_stake(alice).get_amount(), get_voter_info(alice)["staked"].as<int64_t>() - init_stake );

   // emily rents cpu
   const asset fee = core_sym::from_string("50.0000");
   BOOST_REQUIRE_EQUAL( success(),                              rentcpu( emily, bob, fee ) );
   BOOST_REQUIRE_EQUAL( success(),                              updaterex( alice ) );
   BOOST_REQUIRE_EQUAL( payment,                                get_rex_vote_stake(alice) );
   BOOST_REQUIRE_EQUAL( get_rex_vote_stake(alice).get_amount(), get_voter_info( alice )["staked"].as<int64_t>() - init_stake );

   // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
   std::vector<account_name> producer_names;
   {
      producer_names.reserve('z' - 'a' + 1);
      const std::string root("defproducer");
      for ( char c = 'a'; c <= 'z'; ++c ) {
         producer_names.emplace_back(root + std::string(1, c));
      }

      setup_producer_accounts(producer_names);
      for ( const auto& p: producer_names ) {
         BOOST_REQUIRE_EQUAL( success(), regproducer(p) );
         BOOST_TEST_REQUIRE( 0 == get_producer_info(p)["total_votes"].as<double>() );
      }
   }

   BOOST_REQUIRE_EQUAL( success(),
                        vote( alice, std::vector<account_name>(producer_names.begin(), producer_names.begin() + 21) ) );

   BOOST_TEST_REQUIRE( stake2votes( asset( get_voter_info( alice )["staked"].as<int64_t>(), symbol{CORE_SYM} ) )
                       == get_producer_info(producer_names[0])["total_votes"].as<double>() );
   BOOST_TEST_REQUIRE( stake2votes( asset( get_voter_info( alice )["staked"].as<int64_t>(), symbol{CORE_SYM} ) )
                       == get_producer_info(producer_names[20])["total_votes"].as<double>() );

   BOOST_REQUIRE_EQUAL( success(), updaterex( alice ) );
   produce_block( fc::days(10) );
   BOOST_TEST_REQUIRE( get_producer_info(producer_names[20])["total_votes"].as<double>()
                       < stake2votes( asset( get_voter_info( alice )["staked"].as<int64_t>(), symbol{CORE_SYM} ) ) );

   BOOST_REQUIRE_EQUAL( success(), updaterex( alice ) );
   BOOST_TEST_REQUIRE( stake2votes( asset( get_voter_info( alice )["staked"].as<int64_t>(), symbol{CORE_SYM} ) )
                       == get_producer_info(producer_names[20])["total_votes"].as<double>() );

   produce_block( fc::hours(19 * 24 + 23) );
   BOOST_REQUIRE_EQUAL( success(),                                       rexexec( alice, 1 ) );
   const asset   init_rex             = get_rex_balance( alice );
   const auto    current_rex_pool     = get_rex_pool();
   const int64_t total_lendable       = current_rex_pool["total_lendable"].as<asset>().get_amount();
   const int64_t total_rex            = current_rex_pool["total_rex"].as<asset>().get_amount();
   const int64_t init_alice_rex_stake = ( eosio::chain::uint128_t(init_rex.get_amount()) * total_lendable ) / total_rex;
   const asset rex_sell_amount( get_rex_balance(alice).get_amount() / 4, symbol( SY(4,REX) ) );
   BOOST_REQUIRE_EQUAL( success(),                                       sellrex( alice, rex_sell_amount ) );
   BOOST_REQUIRE_EQUAL( init_rex,                                        get_rex_balance(alice) + rex_sell_amount );
   BOOST_REQUIRE_EQUAL( 3 * init_alice_rex_stake,                         4 * get_rex_vote_stake(alice).get_amount() );
   BOOST_REQUIRE_EQUAL( get_voter_info( alice )["staked"].as<int64_t>(), init_stake + get_rex_vote_stake(alice).get_amount() );
   BOOST_TEST_REQUIRE( stake2votes( asset( get_voter_info( alice )["staked"].as<int64_t>(), symbol{CORE_SYM} ) )
                       == get_producer_info(producer_names[0])["total_votes"].as<double>() );
   produce_block( fc::days(31) );
   BOOST_REQUIRE_EQUAL( success(), sellrex( alice, get_rex_balance( alice ) ) );
   BOOST_REQUIRE_EQUAL( 0,         get_rex_balance( alice ).get_amount() );
   BOOST_REQUIRE_EQUAL( success(), vote( alice, { producer_names[0], producer_names[4] } ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( update_rex_vote, eosio_system_tester, * boost::unit_test::tolerance(1e-8) ) try {

   cross_15_percent_threshold();

   // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
   std::vector<account_name> producer_names;
   {
      producer_names.reserve('z' - 'a' + 1);
      const std::string root("defproducer");
      for ( char c = 'a'; c <= 'z'; ++c ) {
         producer_names.emplace_back(root + std::string(1, c));
      }

      setup_producer_accounts(producer_names);
      for ( const auto& p: producer_names ) {
         BOOST_REQUIRE_EQUAL( success(), regproducer(p) );
         BOOST_TEST_REQUIRE( 0 == get_producer_info(p)["total_votes"].as<double>() );
      }
   }

   const asset init_balance = core_sym::from_string("30000.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n, "carolaccount"_n, "emilyaccount"_n };
   account_name alice = accounts[0], bob = accounts[1], emily = accounts[3];
   setup_rex_accounts( accounts, init_balance );

   const int64_t init_stake_amount = get_voter_info( alice )["staked"].as<int64_t>();
   const asset init_stake( init_stake_amount, symbol{CORE_SYM} );

   const asset purchase = core_sym::from_string("25000.0000");
   BOOST_REQUIRE_EQUAL( success(),                              buyrex( alice, purchase ) );
   BOOST_REQUIRE_EQUAL( purchase,                               get_rex_pool()["total_lendable"].as<asset>() );
   BOOST_REQUIRE_EQUAL( purchase,                               get_rex_vote_stake(alice) );
   BOOST_REQUIRE_EQUAL( get_rex_vote_stake(alice).get_amount(), get_voter_info(alice)["staked"].as<int64_t>() - init_stake_amount );
   BOOST_REQUIRE_EQUAL( purchase,                               get_rex_pool()["total_lendable"].as<asset>() );

   BOOST_REQUIRE_EQUAL( success(),                              vote( alice, std::vector<account_name>(producer_names.begin(), producer_names.begin() + 21) ) );
   BOOST_REQUIRE_EQUAL( purchase,                               get_rex_vote_stake(alice) );
   BOOST_REQUIRE_EQUAL( purchase.get_amount(),                  get_voter_info(alice)["staked"].as<int64_t>() - init_stake_amount );

   const auto init_rex_pool = get_rex_pool();
   const asset rent = core_sym::from_string("25.0000");
   BOOST_REQUIRE_EQUAL( success(),                              rentcpu( emily, bob, rent ) );

   produce_block( fc::days(31) );
   BOOST_REQUIRE_EQUAL( success(),                              rexexec( alice, 1 ) );
   const auto curr_rex_pool = get_rex_pool();
   BOOST_TEST_REQUIRE( within_one( curr_rex_pool["total_lendable"].as<asset>().get_amount(),
                                   init_rex_pool["total_lendable"].as<asset>().get_amount() + rent.get_amount() ) );

   BOOST_REQUIRE_EQUAL( success(),                              vote( alice, std::vector<account_name>(producer_names.begin(), producer_names.begin() + 21) ) );
   BOOST_TEST_REQUIRE( within_one( (purchase + rent).get_amount(), get_voter_info(alice)["staked"].as<int64_t>() - init_stake_amount ) );
   BOOST_TEST_REQUIRE( within_one( (purchase + rent).get_amount(), get_rex_vote_stake(alice).get_amount() ) );
   BOOST_TEST_REQUIRE ( stake2votes(purchase + rent + init_stake) ==
                        get_producer_info(producer_names[0])["total_votes"].as_double() );
   BOOST_TEST_REQUIRE ( stake2votes(purchase + rent + init_stake) ==
                        get_producer_info(producer_names[20])["total_votes"].as_double() );

   const asset to_net_stake = core_sym::from_string("60.0000");
   const asset to_cpu_stake = core_sym::from_string("40.0000");
   transfer( config::system_account_name, alice, to_net_stake + to_cpu_stake, config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(),                              rentcpu( emily, bob, rent ) );
   produce_block( fc::hours(30 * 24 + 13) );
   BOOST_REQUIRE_EQUAL( success(),                              rexexec( alice, 1 ) );
   BOOST_REQUIRE_EQUAL( success(),                              stake( alice, alice, to_net_stake, to_cpu_stake ) );
   BOOST_REQUIRE_EQUAL(  purchase + rent + rent,                get_rex_vote_stake(alice) );
   BOOST_TEST_REQUIRE ( stake2votes(init_stake + purchase + rent + rent + to_net_stake + to_cpu_stake) ==
                        get_producer_info(producer_names[0])["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( success(),                              rentcpu( emily, bob, rent ) );
   produce_block( fc::hours(30 * 24 + 13) );
   BOOST_REQUIRE_EQUAL( success(),                              rexexec( alice, 1 ) );
   BOOST_REQUIRE_EQUAL( success(),                              unstake( alice, alice, to_net_stake, to_cpu_stake ) );
   BOOST_REQUIRE_EQUAL( purchase + rent + rent + rent,          get_rex_vote_stake(alice) );
   BOOST_TEST_REQUIRE ( stake2votes(init_stake + get_rex_vote_stake(alice) ) ==
                        get_producer_info(producer_names[0])["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( deposit_rex_fund, eosio_system_tester ) try {

   const asset init_balance = core_sym::from_string("1000.0000");
   const asset init_net     = core_sym::from_string("70.0000");
   const asset init_cpu     = core_sym::from_string("90.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n };
   account_name alice = accounts[0];
   setup_rex_accounts( accounts, init_balance, init_net, init_cpu, false );

   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"),                   get_rex_fund( alice ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("must deposit to REX fund first"), withdraw( alice, core_sym::from_string("0.0001") ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("overdrawn balance"),              deposit( alice, init_balance + init_balance ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("must deposit core token"),        deposit( alice, asset::from_string("1.0000 RNDM") ) );

   asset deposit_quant( init_balance.get_amount() / 5, init_balance.get_symbol() );
   BOOST_REQUIRE_EQUAL( success(),                             deposit( alice, deposit_quant ) );
   BOOST_REQUIRE_EQUAL( get_balance( alice ),                  init_balance - deposit_quant );
   BOOST_REQUIRE_EQUAL( get_rex_fund( alice ),                 deposit_quant );
   BOOST_REQUIRE_EQUAL( success(),                             deposit( alice, deposit_quant ) );
   BOOST_REQUIRE_EQUAL( get_rex_fund( alice ),                 deposit_quant + deposit_quant );
   BOOST_REQUIRE_EQUAL( get_balance( alice ),                  init_balance - deposit_quant - deposit_quant );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient funds"), withdraw( alice, get_rex_fund( alice ) + core_sym::from_string("0.0001")) );
   BOOST_REQUIRE_EQUAL( success(),                             withdraw( alice, deposit_quant ) );
   BOOST_REQUIRE_EQUAL( get_rex_fund( alice ),                 deposit_quant );
   BOOST_REQUIRE_EQUAL( get_balance( alice ),                  init_balance - deposit_quant );
   BOOST_REQUIRE_EQUAL( success(),                             withdraw( alice, get_rex_fund( alice ) ) );
   BOOST_REQUIRE_EQUAL( get_rex_fund( alice ).get_amount(),    0 );
   BOOST_REQUIRE_EQUAL( get_balance( alice ),                  init_balance );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( rex_lower_bound, eosio_system_tester ) try {

   const asset init_balance = core_sym::from_string("25000.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n };
   account_name alice = accounts[0], bob = accounts[1];
   setup_rex_accounts( accounts, init_balance );
   const symbol rex_sym( SY(4, REX) );

   const asset payment = core_sym::from_string("25000.0000");
   BOOST_REQUIRE_EQUAL( success(), buyrex( alice, payment ) );
   const asset fee = core_sym::from_string("25.0000");
   BOOST_REQUIRE_EQUAL( success(), rentcpu( bob, bob, fee ) );

   const auto rex_pool = get_rex_pool();
   const int64_t tot_rex      = rex_pool["total_rex"].as<asset>().get_amount();
   const int64_t tot_unlent   = rex_pool["total_unlent"].as<asset>().get_amount();
   const int64_t tot_lent     = rex_pool["total_lent"].as<asset>().get_amount();
   const int64_t tot_lendable = rex_pool["total_lendable"].as<asset>().get_amount();
   double rex_per_eos = double(tot_rex) / double(tot_lendable);
   int64_t sell_amount = rex_per_eos * ( tot_unlent - 0.09 * tot_lent );
   produce_block( fc::days(5) );
   BOOST_REQUIRE_EQUAL( success(), sellrex( alice, asset( sell_amount, rex_sym ) ) );
   BOOST_REQUIRE_EQUAL( success(), cancelrexorder( alice ) );
   sell_amount = rex_per_eos * ( tot_unlent - 0.1 * tot_lent );
   BOOST_REQUIRE_EQUAL( success(), sellrex( alice, asset( sell_amount, rex_sym ) ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("no sellrex order is scheduled"),
                        cancelrexorder( alice ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( close_rex, eosio_system_tester ) try {

   const asset init_balance = core_sym::from_string("25000.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n, "carolaccount"_n, "emilyaccount"_n };
   account_name alice = accounts[0], bob = accounts[1], carol = accounts[2], emily = accounts[3];
   setup_rex_accounts( accounts, init_balance );

   BOOST_REQUIRE_EQUAL( true,              !get_rex_fund_obj( alice ).is_null() );
   BOOST_REQUIRE_EQUAL( init_balance,      get_rex_fund( alice ) );
   BOOST_REQUIRE_EQUAL( success(),         closerex( alice ) );
   BOOST_REQUIRE_EQUAL( success(),         withdraw( alice, init_balance ) );
   BOOST_REQUIRE_EQUAL( success(),         closerex( alice ) );
   BOOST_REQUIRE_EQUAL( true,              get_rex_fund_obj( alice ).is_null() );
   BOOST_REQUIRE_EQUAL( success(),         deposit( alice, init_balance ) );
   BOOST_REQUIRE_EQUAL( true,              !get_rex_fund_obj( alice ).is_null() );

   BOOST_REQUIRE_EQUAL( true,              get_rex_balance_obj( bob ).is_null() );
   BOOST_REQUIRE_EQUAL( success(),         buyrex( bob, init_balance ) );
   BOOST_REQUIRE_EQUAL( true,              !get_rex_balance_obj( bob ).is_null() );
   BOOST_REQUIRE_EQUAL( true,              !get_rex_fund_obj( bob ).is_null() );
   BOOST_REQUIRE_EQUAL( 0,                 get_rex_fund( bob ).get_amount() );
   BOOST_REQUIRE_EQUAL( closerex( bob ),   wasm_assert_msg("account has remaining REX balance, must sell first") );
   produce_block( fc::days(5) );
   BOOST_REQUIRE_EQUAL( success(),         sellrex( bob, get_rex_balance( bob ) ) );
   BOOST_REQUIRE_EQUAL( success(),         closerex( bob ) );
   BOOST_REQUIRE_EQUAL( success(),         withdraw( bob, get_rex_fund( bob ) ) );
   BOOST_REQUIRE_EQUAL( success(),         closerex( bob ) );
   BOOST_REQUIRE_EQUAL( true,              get_rex_balance_obj( bob ).is_null() );
   BOOST_REQUIRE_EQUAL( true,              get_rex_fund_obj( bob ).is_null() );

   BOOST_REQUIRE_EQUAL( success(),         deposit( bob, init_balance ) );
   BOOST_REQUIRE_EQUAL( success(),         buyrex( bob, init_balance ) );

   const asset fee = core_sym::from_string("1.0000");
   BOOST_REQUIRE_EQUAL( success(),         rentcpu( carol, emily, fee ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("insufficient funds"),
                        withdraw( carol, init_balance ) );
   BOOST_REQUIRE_EQUAL( success(),         withdraw( carol, init_balance - fee ) );

   produce_block( fc::days(20) );

   BOOST_REQUIRE_EQUAL( success(),         closerex( carol ) );
   BOOST_REQUIRE_EQUAL( true,              !get_rex_fund_obj( carol ).is_null() );

   produce_block( fc::days(10) );

   BOOST_REQUIRE_EQUAL( success(),         closerex( carol ) );
   BOOST_REQUIRE_EQUAL( true,              get_rex_balance_obj( carol ).is_null() );
   BOOST_REQUIRE_EQUAL( true,              get_rex_fund_obj( carol ).is_null() );

   BOOST_REQUIRE_EQUAL( success(),         rentnet( emily, emily, fee ) );
   BOOST_REQUIRE_EQUAL( true,              !get_rex_fund_obj( emily ).is_null() );
   BOOST_REQUIRE_EQUAL( success(),         closerex( emily ) );
   BOOST_REQUIRE_EQUAL( true,              !get_rex_fund_obj( emily ).is_null() );

   BOOST_REQUIRE_EQUAL( success(),         sellrex( bob, get_rex_balance( bob ) ) );
   BOOST_REQUIRE_EQUAL( closerex( bob ),   wasm_assert_msg("account has remaining REX balance, must sell first") );

   produce_block( fc::days(30) );

   BOOST_REQUIRE_EQUAL( closerex( bob ),   success() );
   BOOST_REQUIRE      ( 0 <                get_rex_fund( bob ).get_amount() );
   BOOST_REQUIRE_EQUAL( success(),         withdraw( bob, get_rex_fund( bob ) ) );
   BOOST_REQUIRE_EQUAL( success(),         closerex( bob ) );
   BOOST_REQUIRE_EQUAL( true,              get_rex_balance_obj( bob ).is_null() );
   BOOST_REQUIRE_EQUAL( true,              get_rex_fund_obj( bob ).is_null() );

   BOOST_REQUIRE_EQUAL( 0,                 get_rex_pool()["total_rex"].as<asset>().get_amount() );
   BOOST_REQUIRE_EQUAL( 0,                 get_rex_pool()["total_lendable"].as<asset>().get_amount() );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("rex loans are currently not available"),
                        rentcpu( emily, emily, core_sym::from_string("1.0000") ) );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( donate_to_rex, eosio_system_tester ) try {

   const asset   init_balance = core_sym::from_string("10000.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n };
   account_name alice = accounts[0], bob = accounts[1];
   setup_rex_accounts( accounts, init_balance );
   issue_and_transfer( bob, core_sym::from_string("1000.0000"), config::system_account_name );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg("rex system is not initialized"),
                        donatetorex( bob, core_sym::from_string("500.0000"), "") );
   BOOST_REQUIRE_EQUAL( success(), buyrex( alice, init_balance ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("quantity must be core token"),
                        donatetorex( bob, asset::from_string("100 TKN"), "") );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "quantity must be positive" ),
                        donatetorex( bob, core_sym::from_string("-100.0000"), "") );

   BOOST_REQUIRE_EQUAL( success(), donatetorex( bob, core_sym::from_string("100.0000"), "") );


   for (int i = 0; i < 4; ++i) {
      const asset rex_balance = get_balance("eosio.rex"_n);
      const int64_t rex_proceeds = get_rex_return_pool()["proceeds"].as<int64_t>();
      BOOST_REQUIRE_EQUAL( success(), donatetorex( bob, core_sym::from_string("100.0000"), "") );
      BOOST_REQUIRE_EQUAL( rex_balance + core_sym::from_string("100.0000"), get_balance("eosio.rex"_n) );
      BOOST_REQUIRE_EQUAL( rex_proceeds + 1000000, get_rex_return_pool()["proceeds"].as<int64_t>() );
   }
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( set_rex, eosio_system_tester ) try {

   const asset init_balance = core_sym::from_string("25000.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n };
   account_name alice = accounts[0], bob = accounts[1];
   setup_rex_accounts( accounts, init_balance );

   const name act_name{ "setrex"_n };
   const asset init_total_rent  = core_sym::from_string("20000.0000");
   const asset set_total_rent   = core_sym::from_string("10000.0000");
   const asset negative_balance = core_sym::from_string("-10000.0000");
   const asset different_symbol = asset::from_string("10000.0000 RND");
   BOOST_REQUIRE_EQUAL( error("missing authority of eosio"),
                        push_action( alice, act_name, mvo()("balance", set_total_rent) ) );
   BOOST_REQUIRE_EQUAL( error("missing authority of eosio"),
                        push_action( bob, act_name, mvo()("balance", set_total_rent) ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("rex system is not initialized"),
                        push_action( config::system_account_name, act_name, mvo()("balance", set_total_rent) ) );
   BOOST_REQUIRE_EQUAL( success(), buyrex( alice, init_balance ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("balance must be set to have a positive amount"),
                        push_action( config::system_account_name, act_name, mvo()("balance", negative_balance) ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("balance symbol must be core symbol"),
                        push_action( config::system_account_name, act_name, mvo()("balance", different_symbol) ) );
   const asset fee = core_sym::from_string("100.0000");
   BOOST_REQUIRE_EQUAL( success(),             rentcpu( bob, bob, fee ) );
   const auto& init_rex_pool = get_rex_pool();
   BOOST_REQUIRE_EQUAL( init_total_rent + fee, init_rex_pool["total_rent"].as<asset>() );
   BOOST_TEST_REQUIRE( set_total_rent != init_rex_pool["total_rent"].as<asset>() );
   BOOST_REQUIRE_EQUAL( success(),
                        push_action( config::system_account_name, act_name, mvo()("balance", set_total_rent) ) );
   const auto& curr_rex_pool = get_rex_pool();
   BOOST_REQUIRE_EQUAL( init_rex_pool["total_lendable"].as<asset>(),   curr_rex_pool["total_lendable"].as<asset>() );
   BOOST_REQUIRE_EQUAL( init_rex_pool["total_lent"].as<asset>(),       curr_rex_pool["total_lent"].as<asset>() );
   BOOST_REQUIRE_EQUAL( init_rex_pool["total_unlent"].as<asset>(),     curr_rex_pool["total_unlent"].as<asset>() );
   BOOST_REQUIRE_EQUAL( init_rex_pool["namebid_proceeds"].as<asset>(), curr_rex_pool["namebid_proceeds"].as<asset>() );
   BOOST_REQUIRE_EQUAL( init_rex_pool["loan_num"].as_uint64(),         curr_rex_pool["loan_num"].as_uint64() );
   BOOST_REQUIRE_EQUAL( set_total_rent,                                curr_rex_pool["total_rent"].as<asset>() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( b1_vesting, eosio_system_tester ) try {

   cross_15_percent_threshold();

   produce_block( fc::days(14) );

   const asset init_balance = core_sym::from_string("25000.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n };
   account_name alice = accounts[0], bob = accounts[1];
   setup_rex_accounts( accounts, init_balance );

   const name b1{ "b1"_n };

   issue_and_transfer( alice, core_sym::from_string("20000.0000"), config::system_account_name );
   issue_and_transfer( bob,   core_sym::from_string("20000.0000"), config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), bidname( bob,   b1, core_sym::from_string( "0.5000" ) ) );
   BOOST_REQUIRE_EQUAL( success(), bidname( alice, b1, core_sym::from_string( "1.0000" ) ) );

   produce_block( fc::days(1) );

   create_accounts_with_resources( { b1 }, alice );

   const asset stake_amount = core_sym::from_string("50000000.0000");
   issue_and_transfer( b1, stake_amount + stake_amount + stake_amount, config::system_account_name );

   stake( b1, b1, stake_amount, stake_amount );

   BOOST_REQUIRE_EQUAL( 2 * stake_amount.get_amount(), get_voter_info( b1 )["staked"].as<int64_t>() );

   // The code has changed since the tests were originally written, and B1's vesting is no longer based
   // on the time of the block, but is a fixed amount instead.
   // The total amount of possible vested is 35329651.2515, meaning there is 64670348.7485
   // left which will not be vested.
   // These tests now reflect the new behavior.

   const asset vested = core_sym::from_string("35329651.2515");
   const asset unvestable = core_sym::from_string("64670348.7485");
   const asset oneToken = core_sym::from_string("1.0000");
   const asset zero = core_sym::from_string("0.0000");

   BOOST_REQUIRE_EQUAL( error("missing authority of eosio"), vote( b1, { }, "proxyaccount"_n ) );

   // Can't take what isn't vested
   BOOST_REQUIRE_EQUAL(
      wasm_assert_msg("b1 can only claim what has already vested"),
      unstake( b1, b1, stake_amount, stake_amount )
   );
   BOOST_REQUIRE_EQUAL(
      wasm_assert_msg("b1 can only claim what has already vested"),
      unstake( b1, b1, stake_amount, zero )
   );

   // Taking the vested amount - 1 token
   BOOST_REQUIRE_EQUAL( success(), unstake( b1, b1, vested-oneToken, zero ) );
   produce_block( fc::days(4) );
   BOOST_REQUIRE_EQUAL( success(), push_action( b1, "refund"_n, mvo()("owner", b1) ) );
   BOOST_REQUIRE_EQUAL(unvestable.get_amount() + oneToken.get_amount(),
                        get_voter_info( b1 )["staked"].as<int64_t>() );

   // Can't take 2 tokens, only 1 is vested
   BOOST_REQUIRE_EQUAL(
      wasm_assert_msg("b1 can only claim what has already vested"),
      unstake( b1, b1, oneToken, oneToken )
   );

   // Can't unvest the 1 token, as it's already unvested
   BOOST_REQUIRE_EQUAL(
      wasm_assert_msg("can only unvest what is not already vested"),
      unvest( b1, (stake_amount - vested) + oneToken, stake_amount )
   );

   auto supply_before = get_token_supply();

   // Unvesting the remaining unvested tokens
   BOOST_REQUIRE_EQUAL( success(), unvest( b1, stake_amount - vested, stake_amount ) );
   BOOST_REQUIRE_EQUAL(oneToken.get_amount(), get_voter_info( b1 )["staked"].as<int64_t>() );

   // Should have retired the unvestable tokens
   BOOST_REQUIRE_EQUAL(
      get_token_supply(),
      supply_before-unvestable
   );

   // B1 can take the last token, even after unvesting has occurred
   BOOST_REQUIRE_EQUAL( success(), unstake( b1, b1, oneToken, zero )  );
   produce_block( fc::days(4) );
   BOOST_REQUIRE_EQUAL( success(), push_action( b1, "refund"_n, mvo()("owner", b1) ) );
   BOOST_REQUIRE_EQUAL(0, get_voter_info( b1 )["staked"].as<int64_t>() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( rex_return, eosio_system_tester ) try {

   constexpr uint32_t total_intervals = 30 * 144;
   constexpr uint32_t dist_interval   = 10 * 60;
   BOOST_REQUIRE_EQUAL( true,                get_rex_return_pool().is_null() );

   const asset init_balance = core_sym::from_string("100000.0000");
   const std::vector<account_name> accounts = { "aliceaccount"_n, "bobbyaccount"_n };
   account_name alice = accounts[0], bob = accounts[1];
   setup_rex_accounts( accounts, init_balance );

   const asset payment = core_sym::from_string("100000.0000");
   {
      BOOST_REQUIRE_EQUAL( success(),        buyrex( alice, payment ) );
      auto rex_pool = get_rex_pool();
      BOOST_REQUIRE_EQUAL( payment,          rex_pool["total_lendable"].as<asset>() );
      BOOST_REQUIRE_EQUAL( payment,          rex_pool["total_unlent"].as<asset>() );

      BOOST_REQUIRE_EQUAL( true,             get_rex_return_pool().is_null() );
   }

   {
      const asset    fee                 = core_sym::from_string("30.0000");
      const uint32_t bucket_interval_sec = fc::hours(12).to_seconds();
      const uint32_t current_time_sec    = control->pending_block_time().sec_since_epoch();
      const time_point_sec expected_pending_bucket_time{current_time_sec - current_time_sec % bucket_interval_sec + bucket_interval_sec};
      BOOST_REQUIRE_EQUAL( success(),        rentcpu( bob, bob, fee ) );
      auto rex_return_pool = get_rex_return_pool();
      BOOST_REQUIRE_EQUAL( false,            rex_return_pool.is_null() );
      BOOST_REQUIRE_EQUAL( 0,                rex_return_pool["current_rate_of_increase"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( 0,                get_rex_return_buckets()["return_buckets"].get_array().size() );
      BOOST_REQUIRE_EQUAL( expected_pending_bucket_time.sec_since_epoch(),
                           rex_return_pool["pending_bucket_time"].as<time_point_sec>().sec_since_epoch() );
      int32_t t0 = rex_return_pool["pending_bucket_time"].as<time_point_sec>().sec_since_epoch();

      produce_block( fc::hours(13) );
      BOOST_REQUIRE_EQUAL( success(),        rexexec( bob, 1 ) );
      rex_return_pool = get_rex_return_pool();
      int64_t rate    = fee.get_amount() / total_intervals;
      BOOST_REQUIRE_EQUAL( rate,             rex_return_pool["current_rate_of_increase"].as<int64_t>() );

      int32_t t1 = rex_return_pool["last_dist_time"].as<time_point_sec>().sec_since_epoch();
      int64_t  change   = rate * ((t1-t0) / dist_interval) + fee.get_amount() % total_intervals;
      int64_t  expected = payment.get_amount() + change;

      auto rex_pool = get_rex_pool();
      BOOST_REQUIRE_EQUAL( expected,         rex_pool["total_lendable"].as<asset>().get_amount() );

      produce_blocks( 1 );
      produce_block( fc::days(25) );
      BOOST_REQUIRE_EQUAL( success(),        rexexec( bob, 1 ) );
      rex_return_pool = get_rex_return_pool();
      BOOST_REQUIRE_EQUAL( rate,             rex_return_pool["current_rate_of_increase"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( 1,                get_rex_return_buckets()["return_buckets"].get_array().size() );
      int64_t t2 = rex_return_pool["last_dist_time"].as<time_point_sec>().sec_since_epoch();
      change      = rate * ((t2-t0) / dist_interval) + fee.get_amount() % total_intervals;
      expected    = payment.get_amount() + change;

      rex_pool = get_rex_pool();
      BOOST_REQUIRE_EQUAL( expected,         rex_pool["total_lendable"].as<asset>().get_amount() );

      produce_blocks( 1 );
      produce_block( fc::days(5) );
      BOOST_REQUIRE_EQUAL( success(),        rexexec( bob, 1 ) );

      rex_return_pool = get_rex_return_pool();
      BOOST_REQUIRE_EQUAL( 0,                rex_return_pool["current_rate_of_increase"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( 0,                get_rex_return_buckets()["return_buckets"].get_array().size() );

      rex_pool = get_rex_pool();
      expected = payment.get_amount() + fee.get_amount();
      BOOST_REQUIRE_EQUAL( expected,         rex_pool["total_lendable"].as<asset>().get_amount() );
      BOOST_REQUIRE_EQUAL( rex_pool["total_lendable"].as<asset>(),
                           rex_pool["total_unlent"].as<asset>() );
   }

   produce_block( fc::hours(1) );

   {
      const asset init_lendable = get_rex_pool()["total_lendable"].as<asset>();
      const asset fee = core_sym::from_string("15.0000");
      BOOST_REQUIRE_EQUAL( success(),        rentcpu( bob, bob, fee ) );
      auto rex_return_pool = get_rex_return_pool();
      uint32_t t0 = rex_return_pool["last_dist_time"].as<time_point_sec>().sec_since_epoch();
      produce_block( fc::hours(1) );
      BOOST_REQUIRE_EQUAL( success(),        rentnet( bob, bob, fee ) );
      rex_return_pool = get_rex_return_pool();
      BOOST_REQUIRE_EQUAL( 0,                rex_return_pool["current_rate_of_increase"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( 0,                get_rex_return_buckets()["return_buckets"].get_array().size() );
      uint32_t t1 = rex_return_pool["last_dist_time"].as<time_point_sec>().sec_since_epoch();
      BOOST_REQUIRE_EQUAL( t1,               t0 + 6 * dist_interval );

      produce_block( fc::hours(12) );
      BOOST_REQUIRE_EQUAL( success(),        rentnet( bob, bob, fee ) );
      rex_return_pool = get_rex_return_pool();
      BOOST_REQUIRE_EQUAL( 1,                get_rex_return_buckets()["return_buckets"].get_array().size() );
      int64_t rate = 2 * fee.get_amount() / total_intervals;
      BOOST_REQUIRE_EQUAL( rate,             rex_return_pool["current_rate_of_increase"].as<int64_t>() );
      produce_block( fc::hours(8) );
      BOOST_REQUIRE_EQUAL( success(),        rexexec( bob, 1 ) );
      rex_return_pool = get_rex_return_pool();
      BOOST_REQUIRE_EQUAL( rate,             rex_return_pool["current_rate_of_increase"].as<int64_t>() );

      produce_block( fc::days(30) );
      BOOST_REQUIRE_EQUAL( success(),        rexexec( bob, 1 ) );
      rex_return_pool = get_rex_return_pool();
      BOOST_REQUIRE_EQUAL( fee.get_amount() / total_intervals, rex_return_pool["current_rate_of_increase"].as<int64_t>() );
      BOOST_TEST_REQUIRE( (init_lendable + fee + fee).get_amount() < get_rex_pool()["total_lendable"].as<asset>().get_amount() );

      produce_block( fc::days(1) );
      BOOST_REQUIRE_EQUAL( success(),        rexexec( bob, 1 ) );
      rex_return_pool = get_rex_return_pool();
      BOOST_REQUIRE_EQUAL( 0,                rex_return_pool["current_rate_of_increase"].as<int64_t>() );
      BOOST_REQUIRE_EQUAL( 0,                get_rex_return_buckets()["return_buckets"].get_array().size() );
      BOOST_REQUIRE_EQUAL( init_lendable.get_amount() + 3 * fee.get_amount(),
                           get_rex_pool()["total_lendable"].as<asset>().get_amount() );
   }

   {
      const asset fee = core_sym::from_string("25.0000");
      BOOST_REQUIRE_EQUAL( success(),        rentcpu( bob, bob, fee ) );
      produce_block( fc::hours(13) );
      BOOST_REQUIRE_EQUAL( success(),        rexexec( bob, 1 ) );
      auto rex_pool_0        = get_rex_pool();
      auto rex_return_pool_0 = get_rex_return_pool();
      produce_block( fc::minutes(2) );
      BOOST_REQUIRE_EQUAL( success(),        rexexec( bob, 1 ) );
      auto rex_pool_1        = get_rex_pool();
      auto rex_return_pool_1 = get_rex_return_pool();
      BOOST_REQUIRE_EQUAL( rex_return_pool_0["last_dist_time"].as<time_point_sec>().sec_since_epoch(),
                           rex_return_pool_1["last_dist_time"].as<time_point_sec>().sec_since_epoch() );
      BOOST_REQUIRE_EQUAL( rex_pool_0["total_lendable"].as<asset>(),
                           rex_pool_1["total_lendable"].as<asset>() );
      produce_block( fc::minutes(9) );
      BOOST_REQUIRE_EQUAL( success(),        rexexec( bob, 1 ) );
      auto rex_pool_2        = get_rex_pool();
      auto rex_return_pool_2 = get_rex_return_pool();
      BOOST_TEST_REQUIRE( rex_return_pool_1["last_dist_time"].as<time_point_sec>().sec_since_epoch() <
                          rex_return_pool_2["last_dist_time"].as<time_point_sec>().sec_since_epoch() );
      BOOST_TEST_REQUIRE( rex_pool_1["total_lendable"].as<asset>().get_amount() <
                          rex_pool_2["total_lendable"].as<asset>().get_amount() );
      produce_block( fc::days(31) );
      produce_blocks( 1 );
      BOOST_REQUIRE_EQUAL( success(),        rexexec( bob, 1 ) );
      BOOST_REQUIRE_EQUAL( 0,                get_rex_return_buckets()["return_buckets"].get_array().size() );
      BOOST_REQUIRE_EQUAL( 0,                get_rex_return_pool()["current_rate_of_increase"].as<int64_t>() );
   }

   {
      const asset fee = core_sym::from_string("30.0000");
      for ( uint8_t i = 0; i < 5; ++i ) {
         BOOST_REQUIRE_EQUAL( success(),        rentcpu( bob, bob, fee ) );
         produce_block( fc::days(1) );
      }
      BOOST_REQUIRE_EQUAL( success(),        rexexec( bob, 1 ) );
      BOOST_REQUIRE_EQUAL( 5,                get_rex_return_buckets()["return_buckets"].get_array().size() );
      produce_block( fc::days(30) );
      BOOST_REQUIRE_EQUAL( success(),        rexexec( bob, 1 ) );
      BOOST_REQUIRE_EQUAL( 0,                get_rex_return_buckets()["return_buckets"].get_array().size() );
   }

} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_CASE( setabi_bios ) try {
   fc::temp_directory tempdir;
   validating_tester t( tempdir, true );
   t.execute_setup_policy( setup_policy::full );

   abi_serializer abi_ser(fc::json::from_string( (const char*)contracts::bios_abi().data()).template as<abi_def>(), abi_serializer::create_yield_function(base_tester::abi_serializer_max_time));
   t.set_code( config::system_account_name, contracts::bios_wasm() );
   t.set_abi( config::system_account_name, contracts::bios_abi().data() );
   t.create_account("eosio.token"_n);
   t.set_abi( "eosio.token"_n, contracts::token_abi().data() );
   {
      auto res = t.get_row_by_account( config::system_account_name, config::system_account_name, "abihash"_n, "eosio.token"_n );
      _abi_hash abi_hash;
      auto abi_hash_var = abi_ser.binary_to_variant( "abi_hash", res, abi_serializer::create_yield_function(base_tester::abi_serializer_max_time) );
      abi_serializer::from_variant( abi_hash_var, abi_hash, t.get_resolver(), abi_serializer::create_yield_function(base_tester::abi_serializer_max_time));
      auto abi = fc::raw::pack(fc::json::from_string( (const char*)contracts::token_abi().data()).template as<abi_def>());
      auto result = fc::sha256::hash( (const char*)abi.data(), abi.size() );

      BOOST_REQUIRE( abi_hash.hash == result );
   }

   t.set_abi( "eosio.token"_n, contracts::system_abi().data() );
   {
      auto res = t.get_row_by_account( config::system_account_name, config::system_account_name, "abihash"_n, "eosio.token"_n );
      _abi_hash abi_hash;
      auto abi_hash_var = abi_ser.binary_to_variant( "abi_hash", res, abi_serializer::create_yield_function(base_tester::abi_serializer_max_time) );
      abi_serializer::from_variant( abi_hash_var, abi_hash, t.get_resolver(), abi_serializer::create_yield_function(base_tester::abi_serializer_max_time));
      auto abi = fc::raw::pack(fc::json::from_string( (const char*)contracts::system_abi().data()).template as<abi_def>());
      auto result = fc::sha256::hash( (const char*)abi.data(), abi.size() );

      BOOST_REQUIRE( abi_hash.hash == result );
   }
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( setabi, eosio_system_tester ) try {
   set_abi( "eosio.token"_n, contracts::token_abi().data() );
   {
      auto res = get_row_by_account( config::system_account_name, config::system_account_name, "abihash"_n, "eosio.token"_n );
      _abi_hash abi_hash;
      auto abi_hash_var = abi_ser.binary_to_variant( "abi_hash", res, abi_serializer::create_yield_function(abi_serializer_max_time) );
      abi_serializer::from_variant( abi_hash_var, abi_hash, get_resolver(), abi_serializer::create_yield_function(abi_serializer_max_time));
      auto abi = fc::raw::pack(fc::json::from_string( (const char*)contracts::token_abi().data()).template as<abi_def>());
      auto result = fc::sha256::hash( (const char*)abi.data(), abi.size() );

      BOOST_REQUIRE( abi_hash.hash == result );
   }

   set_abi( "eosio.token"_n, contracts::system_abi().data() );
   {
      auto res = get_row_by_account( config::system_account_name, config::system_account_name, "abihash"_n, "eosio.token"_n );
      _abi_hash abi_hash;
      auto abi_hash_var = abi_ser.binary_to_variant( "abi_hash", res, abi_serializer::create_yield_function(abi_serializer_max_time) );
      abi_serializer::from_variant( abi_hash_var, abi_hash, get_resolver(), abi_serializer::create_yield_function(abi_serializer_max_time));
      auto abi = fc::raw::pack(fc::json::from_string( (const char*)contracts::system_abi().data()).template as<abi_def>());
      auto result = fc::sha256::hash( (const char*)abi.data(), abi.size() );

      BOOST_REQUIRE( abi_hash.hash == result );
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( change_limited_account_back_to_unlimited, eosio_system_tester ) try {
   BOOST_REQUIRE( get_total_stake( "eosio" ).is_null() );

   transfer( "eosio"_n, "alice1111111"_n, core_sym::from_string("1.0000") );

   auto error_msg = stake( "alice1111111"_n, "eosio"_n, core_sym::from_string("0.0000"), core_sym::from_string("1.0000") );
   auto semicolon_pos = error_msg.find(';');

   BOOST_REQUIRE_EQUAL( error("account eosio has insufficient ram"),
                        error_msg.substr(0, semicolon_pos) );

   int64_t ram_bytes_needed = 0;
   {
      std::istringstream s( error_msg );
      s.seekg( semicolon_pos + 7, std::ios_base::beg );
      s >> ram_bytes_needed;
      ram_bytes_needed += 256; // enough room to cover total_resources_table
   }

   push_action( "eosio"_n, "setalimits"_n, mvo()
                                          ("account", "eosio")
                                          ("ram_bytes", ram_bytes_needed)
                                          ("net_weight", -1)
                                          ("cpu_weight", -1)
              );

   stake( "alice1111111"_n, "eosio"_n, core_sym::from_string("0.0000"), core_sym::from_string("1.0000") );

   REQUIRE_MATCHING_OBJECT( get_total_stake( "eosio" ), mvo()
      ("owner", "eosio")
      ("net_weight", core_sym::from_string("0.0000"))
      ("cpu_weight", core_sym::from_string("1.0000"))
      ("ram_bytes",  0)
   );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "only supports unlimited accounts" ),
                        push_action( "eosio"_n, "setalimits"_n, mvo()
                                          ("account", "eosio")
                                          ("ram_bytes", ram_bytes_needed)
                                          ("net_weight", -1)
                                          ("cpu_weight", -1)
                        )
   );

   BOOST_REQUIRE_EQUAL( error( "transaction net usage is too high: 128 > 0" ),
                        push_action( "eosio"_n, "setalimits"_n, mvo()
                           ("account", "eosio.saving")
                           ("ram_bytes", -1)
                           ("net_weight", -1)
                           ("cpu_weight", -1)
                        )
   );

   BOOST_REQUIRE_EQUAL( success(),
                        push_action( "eosio"_n, "setacctnet"_n, mvo()
                           ("account", "eosio")
                           ("net_weight", -1)
                        )
   );

   BOOST_REQUIRE_EQUAL( success(),
                        push_action( "eosio"_n, "setacctcpu"_n, mvo()
                           ("account", "eosio")
                           ("cpu_weight", -1)

                        )
   );

   BOOST_REQUIRE_EQUAL( success(),
                        push_action( "eosio"_n, "setalimits"_n, mvo()
                                          ("account", "eosio.saving")
                                          ("ram_bytes", ram_bytes_needed)
                                          ("net_weight", -1)
                                          ("cpu_weight", -1)
                        )
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( buy_pin_sell_ram, eosio_system_tester ) try {
   BOOST_REQUIRE( get_total_stake( "eosio" ).is_null() );

   transfer( "eosio"_n, "alice1111111"_n, core_sym::from_string("1020.0000") );

   auto error_msg = stake( "alice1111111"_n, "eosio"_n, core_sym::from_string("10.0000"), core_sym::from_string("10.0000") );
   auto semicolon_pos = error_msg.find(';');

   BOOST_REQUIRE_EQUAL( error("account eosio has insufficient ram"),
                        error_msg.substr(0, semicolon_pos) );

   int64_t ram_bytes_needed = 0;
   {
      std::istringstream s( error_msg );
      s.seekg( semicolon_pos + 7, std::ios_base::beg );
      s >> ram_bytes_needed;
      ram_bytes_needed += ram_bytes_needed/10; // enough buffer to make up for buyrambytes estimation errors
   }

   auto alice_original_balance = get_balance( "alice1111111"_n );

   BOOST_REQUIRE_EQUAL( success(), buyrambytes( "alice1111111"_n, "eosio"_n, static_cast<uint32_t>(ram_bytes_needed) ) );

   auto tokens_paid_for_ram = alice_original_balance - get_balance( "alice1111111"_n );

   auto total_res = get_total_stake( "eosio" );

   REQUIRE_MATCHING_OBJECT( total_res, mvo()
      ("owner", "eosio")
      ("net_weight", core_sym::from_string("0.0000"))
      ("cpu_weight", core_sym::from_string("0.0000"))
      ("ram_bytes",  total_res["ram_bytes"].as_int64() )
   );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "only supports unlimited accounts" ),
                        push_action( "eosio"_n, "setalimits"_n, mvo()
                                          ("account", "eosio")
                                          ("ram_bytes", ram_bytes_needed)
                                          ("net_weight", -1)
                                          ("cpu_weight", -1)
                        )
   );

   BOOST_REQUIRE_EQUAL( success(),
                        push_action( "eosio"_n, "setacctram"_n, mvo()
                           ("account", "eosio")
                           ("ram_bytes", total_res["ram_bytes"].as_int64() )
                        )
   );

   auto eosio_original_balance = get_balance( "eosio"_n );

   BOOST_REQUIRE_EQUAL( success(), sellram( "eosio"_n, total_res["ram_bytes"].as_int64() ) );

   auto tokens_received_by_selling_ram = get_balance( "eosio"_n ) - eosio_original_balance;

   BOOST_REQUIRE( double(tokens_paid_for_ram.get_amount() - tokens_received_by_selling_ram.get_amount()) / tokens_paid_for_ram.get_amount() < 0.01 );

} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(eosio_system_account_name_restriction)

// -----------------------------------------------------------------------------------------
//             tests for account name restrictions (blacklist patterns)
// -----------------------------------------------------------------------------------------

template<class T>
concept VectorLike = requires(T v) { v.begin(); v.end(); (void)v[0]; };

template<VectorLike... Vs>  // returns a new vector which is the concatenation of the vectors passed as arguments
auto cat(Vs&&... vs) {
    std::common_type_t<Vs...> res;
    res.reserve((0 + ... + vs.size()));
    (..., (res.insert(res.end(), std::begin(std::forward<Vs>(vs)), std::end(std::forward<Vs>(vs)))));
    return res;
}

BOOST_FIXTURE_TEST_CASE( restrictions_update, eosio_system_tester ) try {
   const std::vector<account_name> accounts = { "alice"_n };
   create_accounts_with_resources( accounts );
   const account_name alice = accounts[0];

   std::vector<name> add1 {"bob"_n, "bob.yxz"_n};

   auto hash = denyhashcalc(alice, add1);
   BOOST_REQUIRE(hash);                                                     // no auth required to calculate a hash

   BOOST_REQUIRE(!denyhashcalc(alice, {}));                                 // it fails on empty vector
   BOOST_REQUIRE(!!denyhashcalc(alice, {""_n}));                            // but passes on empty or
   BOOST_REQUIRE(!!denyhashcalc(alice, {"alice1alice2a"_n}));               // 13 characters long names

   BOOST_REQUIRE_EQUAL(denyhashadd(alice, *hash),
                       error("missing authority of eosio"));                // alice cannot add the hash
   BOOST_REQUIRE_EQUAL(denynames(alice, add1),
                       error("assertion failure with message: Verification hash not found in denyhash table"));
   BOOST_REQUIRE_EQUAL(denyhashrm("eosio"_n, *hash),
                       error("assertion failure with message: Trying to remove a deny hash which is not present"));  
   BOOST_REQUIRE(get_blacklisted_names().empty());                          // `denynames` failed so the blacklist is still empty
   
   BOOST_REQUIRE_EQUAL(denyhashadd("eosio"_n, *hash), success());           // "eosio"_n can add a hash
   BOOST_REQUIRE_EQUAL(denynames(alice, add1), success());                  // and then anyone (alice here) can deny the name patterns
   BOOST_REQUIRE(get_blacklisted_names() == add1);                          // newly added names are present in blacklist

   BOOST_REQUIRE_EQUAL(denynames(alice, {}),
                       error("assertion failure with message: No patterns provided"));
   
   BOOST_REQUIRE_EQUAL(denynames(alice, add1),                              // hash was removed when we added the `add1` names above
                       error("assertion failure with message: Verification hash not found in denyhash table"));

   std::vector<name> add2 {"alice.xyz.x"_n, "alice"_n};
   auto add2_hash = *denyhashcalc(alice, add2);
   BOOST_REQUIRE_EQUAL(denyhashadd("eosio"_n, add2_hash), success());       
   BOOST_REQUIRE_EQUAL(denyhashrm("eosio"_n, add2_hash), success());        // check that hash can be removed
   BOOST_REQUIRE_EQUAL(denynames(alice, add2),                              // and then appending fails since hash was removes
                       error("assertion failure with message: Verification hash not found in denyhash table"));
   BOOST_REQUIRE_EQUAL(denyhashadd("eosio"_n, add2_hash), success());       // add the hash again
   BOOST_REQUIRE_EQUAL(denynames(alice, add2), success());                  // appending works
   BOOST_REQUIRE(get_blacklisted_names() == cat(add1, add2));

   // add two hashes in a row to make sure the table supports multiple rows.
   std::vector<name> add3 {"bob.yxz"_n, "alice"_n};
   BOOST_REQUIRE_EQUAL(denyhashadd("eosio"_n, *denyhashcalc(alice, add3)),
                       success());
   std::vector<name> add4 {"fred.xyz.x"_n, "fred"_n};
   BOOST_REQUIRE_EQUAL(denyhashadd("eosio"_n, *denyhashcalc(alice, cat(add4, add4, add4))),
                       success());
   
   BOOST_REQUIRE_EQUAL(denynames(alice, add3), success());                  // duplicates are ignored
   BOOST_REQUIRE(get_blacklisted_names() == cat(add1, add2));

   BOOST_REQUIRE_EQUAL(denynames(alice, add3),                              // but the hash was removed
                       error("assertion failure with message: Verification hash not found in denyhash table"));

   BOOST_REQUIRE_EQUAL(denynames(alice, cat(add4, add4, add4)), success()); // duplicates are ignored even within one call
   BOOST_REQUIRE(get_blacklisted_names() == cat(add1, add2, add4));

   BOOST_REQUIRE_EQUAL(undenynames("eosio"_n, {}), success());             // empty list is silently ignored.

   BOOST_REQUIRE_EQUAL(undenynames("eosio"_n, cat(add1, add4)), success());
   BOOST_REQUIRE(get_blacklisted_names() == add2);                         // removing names work

   BOOST_REQUIRE_EQUAL(undenynames("eosio"_n, add1), success());           // `undenynames` silently ignores names not present

   BOOST_REQUIRE_EQUAL(undenynames("eosio"_n, add2), success());           // removing all remaining names work
   BOOST_REQUIRE(get_blacklisted_names() == std::vector<name>{});

   BOOST_REQUIRE_EQUAL(denyhashadd("eosio"_n, *denyhashcalc(alice, add2)),
                       success());
   BOOST_REQUIRE_EQUAL(denynames(alice, add2), success());                 // and adding some names again for good measure
   BOOST_REQUIRE(get_blacklisted_names() == add2);

} FC_LOG_AND_RETHROW()


// check restrictions on names
// ---------------------------
struct name_restrictions_checker : public eosio_system_tester {
   name_restrictions_checker(name creator_account) : creator(creator_account) {
      if (creator_account != "eosio"_n) {
         create_account_with_resources(creator, config::system_account_name, 10'000'000u);
         transfer(config::system_account_name, creator, core_sym::from_string("10000.0000"));

         stake_with_transfer(config::system_account_name, creator,  core_sym::from_string("80000000.0000"),
                             core_sym::from_string("80000000.0000"));

         regproducer(config::system_account_name);
         BOOST_REQUIRE_EQUAL(success(), vote(creator, {config::system_account_name}));

         produce_block(fc::days(14)); // wait 14 days after min required amount has been staked
         produce_block();
         produce_block();
      }
   }

   void create_accounts(std::vector<name> v, bool create) {
      for (auto n : v) {
         bidname(creator, n, core_sym::from_string("2.0000"));

         produce_block(fc::days(1));
         produce_block();

         if (create) {
            create_account_with_resources(n, creator, 1'000'000u);
            transfer("eosio"_n, n, core_sym::from_string("1000.0000"));
            stake_with_transfer("eosio"_n, n, net, cpu);
         }
      }
   }

   std::pair<bool, base_tester::action_result> check_allowed(name n) {
      auto suffix = n.suffix();
      bool toplevel_account = (suffix == n);
      auto actual_creator = (toplevel_account || creator == "eosio"_n) ? creator : suffix;
      try {
         create_account_with_resources(n, actual_creator, 100'000u);
         if (toplevel_account) {
            transfer("eosio"_n, n, core_sym::from_string("1000.0000"));
            stake_with_transfer("eosio"_n, n, core_sym::from_string("100.0000"), core_sym::from_string("100.0000"));
         }
         return {true, success()};
      } catch (const fc::exception& ex) {
         return {false, error(ex.top_message())};
      }
   }

   void check_allowed(const std::initializer_list<name>& l) {
      for (auto n : l) {
         auto [allowed, action_res] = check_allowed(n);
         BOOST_CHECK_MESSAGE(allowed, action_res);
      }
   }

   void check_disallowed(const std::initializer_list<name>& l) {
      for (auto n : l) {
         auto [allowed, action_res] = check_allowed(n);
         BOOST_CHECK_MESSAGE(!allowed, n.to_string() << " should be disallowed");
      }
   }

   name creator;
   const asset net = core_sym::from_string("100.0000");
   const asset cpu = core_sym::from_string("100.0000");
};

// check restrictions when creating accounts using a non-privileged account, here "fred"
// -------------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( restrictions_checking ) try {
   name_restrictions_checker r{"fred"_n};

   // first get some name suffixes used in tests
   // ------------------------------------------
   r.create_accounts({"xyz"_n, "e12"_n, "fe"_n, "safe"_n}, true);
   r.create_accounts({"thereal3safe"_n, "esafe"_n}, false); // false means just bid for the name but don't create the account

   std::vector<name> add { "esafe"_n, "e.safe"_n };
   BOOST_REQUIRE_EQUAL(r.denyhashadd("eosio"_n, *r.denyhashcalc("fred"_n, add)), r.success());
   BOOST_REQUIRE_EQUAL(r.denynames("fred"_n, add), r.success());

   r.check_disallowed(
      {
           "therealesafe"_n
        ,  "esafe.xyz"_n
        ,  "e.safe.abc"_n
        ,  "12e.safe.abc"_n
        ,  "esafe.e.safe"_n
      });

   r.check_allowed(
      {
           "thereal3safe"_n
        ,  "esafe"_n
        ,  "esave.xyz"_n
        ,  "esaf.e12"_n
        ,  "esa.fe"_n
        ,  "esafe.esafe"_n
        ,  "e.esafe"_n
        ,  "e.safe"_n
        ,  "a.e.safe"_n
      });
   
} FC_LOG_AND_RETHROW()

// check that "eosio"_n is not subject to account name restrictions and does not need to add a hash
// ------------------------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( eosio_restrictions_checking ) try {
   name_restrictions_checker r{"eosio"_n};

   // first get some name suffixes used in tests
   // ------------------------------------------
   r.create_accounts({"xyz"_n, "e12"_n, "fe"_n, "safe"_n}, true);
   r.create_accounts({"thereal3safe"_n, "esafe"_n}, false);  // false means just bid for the name but don't create the account

   // "eosio"_n can add a deny list by adding a hash first
   // ----------------------------------------------------
   std::vector<name> add1 { "esafe"_n };
   auto add1_hash = *r.denyhashcalc("eosio"_n, add1);
   BOOST_REQUIRE_EQUAL(r.denyhashadd("eosio"_n, add1_hash), r.success());
   BOOST_REQUIRE_EQUAL(r.denyhashadd("eosio"_n, add1_hash),
                       r.error("assertion failure with message: Trying to add a deny hash which is already present"));

   BOOST_REQUIRE_EQUAL(r.denynames("eosio"_n, add1), r.success());        // `denynames` will remove add1_hash
   BOOST_REQUIRE_EQUAL(r.denyhashrm("eosio"_n, add1_hash),
                       r.error("assertion failure with message: Trying to remove a deny hash which is not present"));  

   // but also can do it without adding a hash first
   // ----------------------------------------------
   std::vector<name> add2 { "e.safe"_n };
   BOOST_REQUIRE_EQUAL(r.denynames("eosio"_n, add2), r.success());

   r.check_allowed(   // these are allowed for "eosio"_n because "eosio"_n is not restricted.
      {
           "therealesafe"_n
        ,  "esafe.xyz"_n
        ,  "e.safe.abc"_n
        ,  "12e.safe.abc"_n
        ,  "esafe.e.safe"_n
      });

   r.check_allowed(
      {
           "thereal3safe"_n
        ,  "esafe"_n
        ,  "esave.xyz"_n
        ,  "esaf.e12"_n
        ,  "esa.fe"_n
        ,  "esafe.esafe"_n
        ,  "e.esafe"_n
        ,  "e.safe"_n
        ,  "a.e.safe"_n
      });

} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_SUITE_END()
