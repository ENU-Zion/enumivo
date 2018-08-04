/**
 *  @file
 *  @copyright defined in enumivo/LICENSE
 */
#pragma once

#include <enumivo/chain/block_header_state.hpp>
#include <enumivo/chain/block.hpp>
#include <enumivo/chain/transaction_metadata.hpp>
#include <enumivo/chain/action_receipt.hpp>

namespace enumivo { namespace chain {

   struct block_state : public block_header_state {
      block_state( const block_header_state& cur ):block_header_state(cur){}
      block_state( const block_header_state& prev, signed_block_ptr b, bool trust = false );
      block_state( const block_header_state& prev, block_timestamp_type when );
      block_state() = default;

      /// weak_ptr prev_block_state....
      signed_block_ptr                                    block;
      bool                                                validated = false;
      bool                                                in_current_chain = false;

      /// this data is redundant with the data stored in block, but facilitates
      /// recapturing transactions when we pop a block
      vector<transaction_metadata_ptr>                    trxs;
   };

   using block_state_ptr = std::shared_ptr<block_state>;

} } /// namespace enumivo::chain

FC_REFLECT_DERIVED( enumivo::chain::block_state, (enumivo::chain::block_header_state), (block)(validated)(in_current_chain) )
