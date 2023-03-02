#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <json/json.h>
#include <librpc/Rpc.h>

using namespace std;

namespace dev
{
    namespace plugin
    {
        #define PLUGIN_LOG(LEVEL) LOG(LEVEL) << LOG_BADGE("PLUGIN") << LOG_BADGE("PLUGIN")
        class transactionInjectionTest
        {
            private:
                std::vector<std::string> signedTransactions;
                std::string signedDeployContractTransaction;
                std::shared_ptr<dev::rpc::Rpc> m_rpcService;
                int32_t m_internal_groupId;
                std::string innerContact_1 = "0x93911693669c9a4b83f702838bc3294e95951438";
                std::string innerContact_2 = "0x1d89f9c61addceff5d8cae494c3439667b657deb";
                std::string innerContact_3 = "0x4bb13eaebeed711234f0bc2c455d1e74d0cef0c8";
                std::string crossContact_3 = "0x2fa6307e464428209f02702f65180ad663aa4fd9";
                unordered_map<int, vector<int>> topo_area;
                unordered_map<int, vector<int>> topo_cross_area;

                int m_sum_shard=9;
                std::shared_ptr<ledger::LedgerManager> m_ledgerManager;
                bool m_isleader=false;
            public:
                transactionInjectionTest(std::shared_ptr<dev::rpc::Rpc> _rpcService, int32_t _groupId)
                {
                    m_rpcService = _rpcService;
                    m_internal_groupId = _groupId;
                }
                transactionInjectionTest(std::shared_ptr<dev::rpc::Rpc> _rpcService, int32_t _groupId,int sum_shards)
                {
                    m_rpcService = _rpcService;
                    m_internal_groupId = _groupId;
                    m_sum_shard=sum_shards;
                }
                transactionInjectionTest(std::shared_ptr<dev::rpc::Rpc> _rpcService, int32_t _groupId,int sum_shards
                            ,std::shared_ptr<ledger::LedgerManager> ledgerManager)
                {
                    m_rpcService = _rpcService;
                    m_internal_groupId = _groupId;
                    m_sum_shard=sum_shards;
                    m_ledgerManager=ledgerManager;
                }
                transactionInjectionTest(std::shared_ptr<dev::rpc::Rpc> _rpcService, int32_t _groupId,int sum_shards
                            ,std::shared_ptr<ledger::LedgerManager> ledgerManager,bool isleader)
                {
                    m_rpcService = _rpcService;
                    m_internal_groupId = _groupId;
                    m_sum_shard=sum_shards;
                    m_ledgerManager=ledgerManager;
                    m_isleader=isleader;
                }
                ~transactionInjectionTest(){};
                
                vector<std::string> getRlps4fourShards();

                void deployContractTransaction(std::string filename, int32_t _groupId);
                void injectionTransactions(std::string filename, int32_t _groupId);
                void injectionTransactions4sharper(std::string filename, int32_t groupId);
                void injectionRandomTransactions4sharper(std::string filename, int32_t groupId,int32_t num_randomtx);
                void injectionRandomTransactions4sharperWithMixedTxs(std::string filename, int32_t groupId,int32_t num_randomtx,double ratio_intra_cross);
                void intiTopo4nineshards();
                ///!!!
                void injectionIntraTxin1shards(int num_tx);
                
                
                void injectionTxin9shards();
                void injectionCorssTxin9shards();
                
                std::string createIntraTx4sharper();
                std::string createCrossTx4sharper(int coorGroupId,int subGroupId1,int  subGroupId2);     
                
                std::string createInnerTransactions4sharper(int32_t _groupId, std::shared_ptr<dev::ledger::LedgerManager> ledgerManager) ;
                std::string createCrossTransactions4sharper(int32_t coorGroupId, int32_t subGroupId1, int32_t subGroupId2, 
                        std::shared_ptr<dev::ledger::LedgerManager> ledgerManager) ;
                std::string createCrossTransactions(int32_t coorGroupId, int32_t subGroupId1, int32_t subGroupId2, 
                        std::shared_ptr<dev::ledger::LedgerManager> ledgerManager);
                std::string createCrossTransactions_HB(int32_t coorGroupId, int32_t subGroupId1, int32_t subGroupId2, int32_t squId);
                std::string createInnerTransactions(int32_t _groupId, std::shared_ptr<dev::ledger::LedgerManager> ledgerManager);

                
        };
    }
}