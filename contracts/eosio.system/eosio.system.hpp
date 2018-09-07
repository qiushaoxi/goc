/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio.system/native.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <eosio.system/exchange_state.hpp>

#include <string>

namespace eosiosystem {

   using eosio::asset;
   using eosio::indexed_by;
   using eosio::const_mem_fun;
   using eosio::block_timestamp;

   struct name_bid {
     account_name            newname;
     account_name            high_bidder;
     int64_t                 high_bid = 0; ///< negative high_bid == closed auction waiting to be claimed
     uint64_t                last_bid_time = 0;

     auto     primary_key()const { return newname;                          }
     uint64_t by_high_bid()const { return static_cast<uint64_t>(-high_bid); }
   };

   typedef eosio::multi_index< N(namebids), name_bid,
                               indexed_by<N(highbid), const_mem_fun<name_bid, uint64_t, &name_bid::by_high_bid>  >
                               >  name_bid_table;


   struct eosio_global_state : eosio::blockchain_parameters {
      uint64_t free_ram()const { return max_ram_size - total_ram_bytes_reserved; }

      uint64_t             max_ram_size = 64ll*1024 * 1024 * 1024;
      uint64_t             total_ram_bytes_reserved = 0;
      int64_t              total_ram_stake = 0;

      block_timestamp      last_producer_schedule_update;
      uint64_t             last_pervote_bucket_fill = 0;
      int64_t              pervote_bucket = 0;
      int64_t              perblock_bucket = 0;
      uint32_t             total_unpaid_blocks = 0; /// all blocks which have been produced but not paid
      int64_t              total_activated_stake = 0;
      uint64_t             thresh_activated_stake_time = 0;
      uint16_t             last_producer_schedule_size = 0;
      double               total_producer_vote_weight = 0; /// the sum of all producer votes
      block_timestamp      last_name_close;

      //GOC parameters
      int64_t              goc_proposal_fee_limit=  10000000;
      int64_t              goc_stake_limit = 1000000000;
      int64_t              goc_action_fee = 10000;
      int64_t              goc_max_proposal_reward = 1000000;
      uint32_t             goc_governance_vote_period = 24 * 3600 * 7;  // 7 days
      uint32_t             goc_bp_vote_period = 24 * 3600 * 7;  // 7 days
      uint32_t             goc_vote_start_time = 24 * 3600 * 3;  // vote start after 3 Days
      
      int64_t              goc_voter_bucket = 0;
      int64_t              goc_gn_bucket = 0;
      uint32_t             last_gn_bucket_empty = 0;
      



      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE_DERIVED( eosio_global_state, eosio::blockchain_parameters,
                                (max_ram_size)(total_ram_bytes_reserved)(total_ram_stake)
                                (last_producer_schedule_update)(last_pervote_bucket_fill)
                                (pervote_bucket)(perblock_bucket)(total_unpaid_blocks)(total_activated_stake)(thresh_activated_stake_time)
                                (last_producer_schedule_size)(total_producer_vote_weight)(last_name_close) 
                                (goc_proposal_fee_limit)(goc_stake_limit)(goc_action_fee)(goc_max_proposal_reward)
                                (goc_governance_vote_period)(goc_bp_vote_period)(goc_vote_start_time)
                                (goc_voter_bucket)(goc_gn_bucket)(last_gn_bucket_empty) )
   };

   struct producer_info {
      account_name          owner;
      double                total_votes = 0;
      eosio::public_key     producer_key; /// a packed public key object
      bool                  is_active = true;
      std::string           url;
      uint32_t              unpaid_blocks = 0;
      uint64_t              last_claim_time = 0;
      uint16_t              location = 0;

      uint64_t primary_key()const { return owner;                                   }
      double   by_votes()const    { return is_active ? -total_votes : total_votes;  }
      bool     active()const      { return is_active;                               }
      void     deactivate()       { producer_key = public_key(); is_active = false; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( producer_info, (owner)(total_votes)(producer_key)(is_active)(url)
                        (unpaid_blocks)(last_claim_time)(location) )
   };

   struct voter_info {
      account_name                owner = 0; /// the voter
      account_name                proxy = 0; /// the proxy set by the voter, if any
      std::vector<account_name>   producers; /// the producers approved by this voter if no proxy set
      int64_t                     staked = 0;

      /**
       *  Every time a vote is cast we must first "undo" the last vote weight, before casting the
       *  new vote weight.  Vote weight is calculated as:
       *
       *  stated.amount * 2 ^ ( weeks_since_launch/weeks_per_year)
       */
      double                      last_vote_weight = 0; /// the vote weight cast the last time the vote was updated

      /**
       * Total vote weight delegated to this voter.
       */
      double                      proxied_vote_weight= 0; /// the total vote weight delegated to this voter as a proxy
      bool                        is_proxy = 0; /// whether the voter is a proxy for others


      uint32_t                    reserved1 = 0;
      time                        reserved2 = 0;
      eosio::asset                reserved3;

      uint64_t primary_key()const { return owner; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( voter_info, (owner)(proxy)(producers)(staked)(last_vote_weight)(proxied_vote_weight)(is_proxy)(reserved1)(reserved2)(reserved3) )
   };

   struct goc_proposal_info {
      uint64_t              id;
      account_name          owner;
      asset                 fee;
      std::string           proposal_name;
      std::string           proposal_content;
      std::string           url;

      time                  create_time;
      time                  vote_starttime;
      time                  bp_vote_starttime;
      time                  bp_vote_endtime;

      time                  settle_time = 0;
      asset                 reward;

      double                total_yeas;
      double                total_nays;
      uint64_t              total_voter = 0;
      double                bp_nays;
      uint16_t              total_bp = 0;

      uint64_t  primary_key()const     { return id; }
      uint64_t  by_endtime()const      { return bp_vote_endtime; }
      bool      vote_pass()const       { return total_yeas > total_nays;  }
      //need change to bp count
      bool      bp_pass()const         { return bp_nays < -7.0;  }

      EOSLIB_SERIALIZE( goc_proposal_info, (id)(owner)(fee)(proposal_name)(proposal_content)(url)
                            (create_time)(vote_starttime)(bp_vote_starttime)(bp_vote_endtime)(settle_time)
                            (total_yeas)(total_nays)
                            (bp_nays)
                            )
   };

   struct goc_vote_info {
     account_name           owner;
     bool                   vote;
     time                   vote_time;
     time                   vote_update_time;
     time                   settle_time = 0;

     uint64_t primary_key()const { return owner; }

     EOSLIB_SERIALIZE(goc_vote_info, (owner)(vote)(vote_time)(vote_update_time))     
   };

   struct goc_reward_info {
      account_name  owner;
      time          reward_time;
      uint64_t      proposal_id;
      eosio::asset  rewards = asset(0);

      uint64_t  primary_key()const { return owner; }

      EOSLIB_SERIALIZE( goc_reward_info, (owner)(reward_time)(rewards) )
   };

   typedef eosio::multi_index< N(voters), voter_info>  voters_table;



   typedef eosio::multi_index< N(producers), producer_info,
                               indexed_by<N(prototalvote), const_mem_fun<producer_info, double, &producer_info::by_votes>  >
                               >  producers_table;

   typedef eosio::multi_index< N(proposals), goc_proposal_info,
                               indexed_by<N(byendtime), const_mem_fun<goc_proposal_info, uint64_t, &goc_proposal_info::by_endtime>  >
                               //indexed_by<N(by_yea), const_mem_fun<goc_proposal_info, double, &goc_proposal_info::by_yea_votes>  >,
                               //indexed_by<N(by_nay), const_mem_fun<goc_proposal_info, double, &goc_proposal_info::by_nay_votes>  >
                               > goc_proposals_table;

   typedef eosio::multi_index< N(votes), goc_vote_info> goc_votes_table;
   typedef eosio::multi_index< N(bpvotes), goc_vote_info> goc_bp_votes_table;
   typedef eosio::multi_index< N(gocreward), goc_reward_info>      goc_rewards_table;

   typedef eosio::singleton<N(global), eosio_global_state> global_state_singleton;

   //   static constexpr uint32_t     max_inflation_rate = 5;  // 5% annual inflation
   static constexpr uint32_t     seconds_per_day = 24 * 3600;
   static constexpr uint64_t     system_token_symbol = CORE_SYMBOL;

   class system_contract : public native {
      private:
         voters_table           _voters;
         producers_table        _producers;
         goc_proposals_table    _gocproposals;
         global_state_singleton _global;

         eosio_global_state     _gstate;
         rammarket              _rammarket;

      public:
         system_contract( account_name s );
         ~system_contract();

         // Actions:
         void onblock( block_timestamp timestamp, account_name producer );
                      // const block_header& header ); /// only parse first 3 fields of block header

         // functions defined in delegate_bandwidth.cpp

         /**
          *  Stakes SYS from the balance of 'from' for the benfit of 'receiver'.
          *  If transfer == true, then 'receiver' can unstake to their account
          *  Else 'from' can unstake at any time.
          */
         void delegatebw( account_name from, account_name receiver,
                          asset stake_net_quantity, asset stake_cpu_quantity, bool transfer );


         /**
          *  Decreases the total tokens delegated by from to receiver and/or
          *  frees the memory associated with the delegation if there is nothing
          *  left to delegate.
          *
          *  This will cause an immediate reduction in net/cpu bandwidth of the
          *  receiver.
          *
          *  A transaction is scheduled to send the tokens back to 'from' after
          *  the staking period has passed. If existing transaction is scheduled, it
          *  will be canceled and a new transaction issued that has the combined
          *  undelegated amount.
          *
          *  The 'from' account loses voting power as a result of this call and
          *  all producer tallies are updated.
          */
         void undelegatebw( account_name from, account_name receiver,
                            asset unstake_net_quantity, asset unstake_cpu_quantity );


         /**
          * Increases receiver's ram quota based upon current price and quantity of
          * tokens provided. An inline transfer from receiver to system contract of
          * tokens will be executed.
          */
         void buyram( account_name buyer, account_name receiver, asset tokens );
         void buyrambytes( account_name buyer, account_name receiver, uint32_t bytes );

         /**
          *  Reduces quota my bytes and then performs an inline transfer of tokens
          *  to receiver based upon the average purchase price of the original quota.
          */
         void sellram( account_name receiver, int64_t bytes );


         void gocstake( account_name payer);
         void gocunstake( account_name receiver);
        // start_type: 0:normal, 1:skip waiting, 2:skip user vote, only for debug
         void gocnewprop( const account_name owner, asset fee, const std::string& pname, const std::string& pcontent, const std::string& url, uint16_t start_type);
         void gocupprop( const account_name owner, uint64_t id, const std::string& pname, const std::string& pcontent, const std::string& url );

         void gocvote( account_name voter, uint64_t id, bool yea );
         void gocbpvote( account_name bp, uint64_t id, bool yea );

         



         /**
          *  This action is called after the delegation-period to claim all pending
          *  unstaked tokens belonging to owner
          */
         void refund( account_name owner );

         // functions defined in voting.cpp

         void regproducer( const account_name producer, const public_key& producer_key, const std::string& url, uint16_t location );

         void unregprod( const account_name producer );

         void setram( uint64_t max_ram_size );

         void voteproducer( const account_name voter, const account_name proxy, const std::vector<account_name>& producers );

         void regproxy( const account_name proxy, bool isproxy );

         void setparams( const eosio::blockchain_parameters& params );

         // functions defined in producer_pay.cpp
         void claimrewards( const account_name& owner );

         void setpriv( account_name account, uint8_t ispriv );

         void rmvproducer( account_name producer );

         void bidname( account_name bidder, account_name newname, asset bid );
      private:
         void update_elected_producers( block_timestamp timestamp );

         // Implementation details:

         //defind in delegate_bandwidth.cpp
         void changebw( account_name from, account_name receiver,
                        asset stake_net_quantity, asset stake_cpu_quantity, bool transfer );

         //defined in voting.hpp
         static eosio_global_state get_default_parameters();

         void update_votes( const account_name voter, const account_name proxy, const std::vector<account_name>& producers, bool voting );

         // defined in voting.cpp
         void propagate_weight_change( const voter_info& voter );
   };

} /// eosiosystem
