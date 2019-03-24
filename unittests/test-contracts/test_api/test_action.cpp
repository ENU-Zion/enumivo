/**
 * @file action_test.cpp
 * @copyright defined in enumivo/LICENSE
 */
#include <enumivolib/action.hpp>
#include <enumivolib/chain.h>
#include <enumivolib/crypto.h>
#include <enumivolib/datastream.hpp>
#include <enumivolib/db.h>
#include <enumivolib/enumivo.hpp>
#include <enumivolib/print.hpp>
#include <enumivolib/privileged.h>
#include <enumivolib/transaction.hpp>

#include "test_api.hpp"

using namespace enumivo;

void test_action::read_action_normal() {

   char buffer[100];
   uint32_t total = 0;

   enumivo_assert( action_data_size() == sizeof(dummy_action), "action_size() == sizeof(dummy_action)" );

   total = read_action_data( buffer, 30 );
   enumivo_assert( total == sizeof(dummy_action) , "read_action(30)" );

   total = read_action_data( buffer, 100 );
   enumivo_assert(total == sizeof(dummy_action) , "read_action(100)" );

   total = read_action_data(buffer, 5);
   enumivo_assert( total == 5 , "read_action(5)" );

   total = read_action_data(buffer, sizeof(dummy_action) );
   enumivo_assert( total == sizeof(dummy_action), "read_action(sizeof(dummy_action))" );

   dummy_action *dummy13 = reinterpret_cast<dummy_action *>(buffer);

   enumivo_assert( dummy13->a == DUMMY_ACTION_DEFAULT_A, "dummy13->a == DUMMY_ACTION_DEFAULT_A" );
   enumivo_assert( dummy13->b == DUMMY_ACTION_DEFAULT_B, "dummy13->b == DUMMY_ACTION_DEFAULT_B" );
   enumivo_assert( dummy13->c == DUMMY_ACTION_DEFAULT_C, "dummy13->c == DUMMY_ACTION_DEFAULT_C" );
}

void test_action::test_dummy_action() {
   char buffer[100];
   int total = 0;

   // get_action
   total = get_action( 1, 0, buffer, 0 );
   total = get_action( 1, 0, buffer, static_cast<size_t>(total) );
   enumivo_assert( total > 0, "get_action failed" );
   enumivo::action act = enumivo::get_action( 1, 0 );
   enumivo_assert( act.authorization.back().actor == "testapi"_n, "incorrect permission actor" );
   enumivo_assert( act.authorization.back().permission == "active"_n, "incorrect permission name" );
   enumivo_assert( enumivo::pack_size(act) == static_cast<size_t>(total), "pack_size does not match get_action size" );
   enumivo_assert( act.account == "testapi"_n, "expected testapi account" );

   dummy_action dum13 = act.data_as<dummy_action>();

   if ( dum13.b == 200 ) {
      // attempt to access context free only api
      get_context_free_data( 0, nullptr, 0 );
      enumivo_assert( false, "get_context_free_data() not allowed in non-context free action" );
   } else {
      enumivo_assert( dum13.a == DUMMY_ACTION_DEFAULT_A, "dum13.a == DUMMY_ACTION_DEFAULT_A" );
      enumivo_assert( dum13.b == DUMMY_ACTION_DEFAULT_B, "dum13.b == DUMMY_ACTION_DEFAULT_B" );
      enumivo_assert( dum13.c == DUMMY_ACTION_DEFAULT_C, "dum13.c == DUMMY_ACTION_DEFAULT_C" );
   }
}

void test_action::read_action_to_0() {
   read_action_data( (void *)0, action_data_size() );
}

void test_action::read_action_to_64k() {
   read_action_data( (void *)((1<<16)-2), action_data_size());
}

void test_action::test_cf_action() {

   enumivo::action act = enumivo::get_action( 0, 0 );
   cf_action cfa = act.data_as<cf_action>();
   if ( cfa.payload == 100 ) {
      // verify read of get_context_free_data, also verifies system api access
      int size = get_context_free_data( cfa.cfd_idx, nullptr, 0 );
      enumivo_assert( size > 0, "size determination failed" );
      std::vector<char> cfd( static_cast<size_t>(size) );
      size = get_context_free_data( cfa.cfd_idx, &cfd[0], static_cast<size_t>(size) );
      enumivo_assert(static_cast<size_t>(size) == cfd.size(), "get_context_free_data failed" );
      uint32_t v = enumivo::unpack<uint32_t>( &cfd[0], cfd.size() );
      enumivo_assert( v == cfa.payload, "invalid value" );

      // verify crypto api access
      capi_checksum256 hash;
      char test[] = "test";
      sha256( test, sizeof(test), &hash );
      assert_sha256( test, sizeof(test), &hash );
      // verify action api access
      action_data_size();
      // verify console api access
      enumivo::print("test\n");
      // verify memory api access
      uint32_t i = 42;
      memccpy( &v, &i, sizeof(i), sizeof(i) );
      // verify transaction api access
      enumivo_assert(transaction_size() > 0, "transaction_size failed");
      // verify softfloat api access
      float f1 = 1.0f, f2 = 2.0f;
      float f3 = f1 + f2;
      enumivo_assert( f3 >  2.0f, "Unable to add float.");
      // verify context_free_system_api
      enumivo_assert( true, "verify enumivo_assert can be called" );


   } else if ( cfa.payload == 200 ) {
      // attempt to access non context free api, privileged_api
      is_privileged(act.name.value);
      enumivo_assert( false, "privileged_api should not be allowed" );
   } else if ( cfa.payload == 201 ) {
      // attempt to access non context free api, producer_api
      get_active_producers( nullptr, 0 );
      enumivo_assert( false, "producer_api should not be allowed" );
   } else if ( cfa.payload == 202 ) {
      // attempt to access non context free api, db_api
      db_store_i64( "testapi"_n.value, "testapi"_n.value, "testapi"_n.value, 0, "test", 4 );
      enumivo_assert( false, "db_api should not be allowed" );
   } else if ( cfa.payload == 203 ) {
      // attempt to access non context free api, db_api
      uint64_t i = 0;
      db_idx64_store( "testapi"_n.value, "testapi"_n.value, "testapi"_n.value, 0, &i );
      enumivo_assert( false, "db_api should not be allowed" );
   } else if ( cfa.payload == 204 ) {
      db_find_i64( "testapi"_n.value, "testapi"_n.value, "testapi"_n.value, 1 );
      enumivo_assert( false, "db_api should not be allowed" );
   } else if ( cfa.payload == 205 ) {
      // attempt to access non context free api, send action
      enumivo::action dum_act;
      dum_act.send();
      enumivo_assert( false, "action send should not be allowed" );
   } else if ( cfa.payload == 206 ) {
      enumivo::require_auth("test"_n);
      enumivo_assert( false, "authorization_api should not be allowed" );
   } else if ( cfa.payload == 207 ) {
      now();
      enumivo_assert( false, "system_api should not be allowed" );
   } else if ( cfa.payload == 208 ) {
      current_time();
      enumivo_assert( false, "system_api should not be allowed" );
   } else if ( cfa.payload == 209 ) {
      publication_time();
      enumivo_assert( false, "system_api should not be allowed" );
   } else if ( cfa.payload == 210 ) {
      send_inline( (char*)"hello", 6 );
      enumivo_assert( false, "transaction_api should not be allowed" );
   } else if ( cfa.payload == 211 ) {
      send_deferred( "testapi"_n.value, "testapi"_n.value, "hello", 6, 0 );
      enumivo_assert( false, "transaction_api should not be allowed" );
   }

}

void test_action::require_notice( uint64_t receiver, uint64_t code, uint64_t action ) {
   (void)code;(void)action;
   if( receiver == "testapi"_n.value ) {
      enumivo::require_recipient( "acc1"_n );
      enumivo::require_recipient( "acc2"_n );
      enumivo::require_recipient( "acc1"_n, "acc2"_n );
      enumivo_assert( false, "Should've failed" );
   } else if ( receiver == "acc1"_n.value || receiver == "acc2"_n.value ) {
      return;
   }
   enumivo_assert( false, "Should've failed" );
}

void test_action::require_notice_tests( uint64_t receiver, uint64_t code, uint64_t action ) {
   enumivo::print( "require_notice_tests" );
   if( receiver == "testapi"_n.value ) {
      enumivo::print("require_recipient( \"acc5\"_n )");
      enumivo::require_recipient("acc5"_n);
   } else if( receiver == "acc5"_n.value ) {
      enumivo::print("require_recipient( \"testapi\"_n )");
      enumivo::require_recipient("testapi"_n);
   }
}

void test_action::require_auth() {
   prints("require_auth");
   enumivo::require_auth("acc3"_n);
   enumivo::require_auth("acc4"_n);
}

void test_action::assert_false() {
   enumivo_assert( false, "test_action::assert_false" );
}

void test_action::assert_true() {
   enumivo_assert( true, "test_action::assert_true" );
}

void test_action::assert_true_cf() {
   enumivo_assert( true, "test_action::assert_true" );
}

void test_action::test_abort() {
   abort();
   enumivo_assert( false, "should've aborted" );
}

void test_action::test_publication_time() {
   uint64_t pub_time = 0;
   uint32_t total = read_action_data( &pub_time, sizeof(uint64_t) );
   enumivo_assert( total == sizeof(uint64_t), "total == sizeof(uint64_t)" );
   enumivo_assert( pub_time == publication_time(), "pub_time == publication_time()" );
}

void test_action::test_current_receiver( uint64_t receiver, uint64_t code, uint64_t action ) {
   (void)code;(void)action;
   name cur_rec;
   read_action_data( &cur_rec, sizeof(name) );

   enumivo_assert( receiver == cur_rec.value, "the current receiver does not match" );
}

void test_action::test_current_time() {
   uint64_t tmp = 0;
   uint32_t total = read_action_data( &tmp, sizeof(uint64_t) );
   enumivo_assert( total == sizeof(uint64_t), "total == sizeof(uint64_t)" );
   enumivo_assert( tmp == current_time(), "tmp == current_time()" );
}

void test_action::test_assert_code() {
   uint64_t code = 0;
   uint32_t total = read_action_data(&code, sizeof(uint64_t));
   enumivo_assert( total == sizeof(uint64_t), "total == sizeof(uint64_t)" );
   enumivo_assert_code( false, code );
}

void test_action::test_ram_billing_in_notify( uint64_t receiver, uint64_t code, uint64_t action ) {
   uint128_t tmp = 0;
   uint32_t total = read_action_data( &tmp, sizeof(uint128_t) );
   enumivo_assert( total == sizeof(uint128_t), "total == sizeof(uint128_t)" );

   uint64_t to_notify = tmp >> 64;
   uint64_t payer = tmp & 0xFFFFFFFFFFFFFFFFULL;

   if( code == receiver ) {
      enumivo::require_recipient( name{to_notify} );
   } else {
      enumivo_assert( to_notify == receiver, "notified recipient other than the one specified in to_notify" );

      // Remove main table row if it already exists.
      int itr = db_find_i64( receiver, "notifytest"_n.value, "notifytest"_n.value, "notifytest"_n.value );
      if( itr >= 0 )
         db_remove_i64( itr );

      // Create the main table row simply for the purpose of charging code more RAM.
      if( payer != 0 )
         db_store_i64( "notifytest"_n.value, "notifytest"_n.value, payer, "notifytest"_n.value, &to_notify, sizeof(to_notify) );
   }
}
