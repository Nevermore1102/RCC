#pragma once


#include "Common.h"
#include <libconsensus/pbft/Common.h>
// #include <libplugin/BlockingTxQueue.h>
#include <libp2p/P2PInterface.h>
#include <queue>
#include <libexecutive/Executive.h>
#include <librpc/Common.h>
#include <libethcore/Transaction.h>
#include <libplugin/ExecuteVM.h>
#include <libconsensus/pbft/Common.h>
#include <libconsensus/pbft/PBFTEngine.h>
#include <thread>
#include <libsync/Common.h>
#include <test/unittests/libevm/FakeExtVMFace.h>
#include <evmc/evmc.h>
#include <chrono>
#include <libdevcore/Common.h>
#include <libethcore/Transaction.h>
#include <fstream>
#include <sys/time.h>
#include <stdlib.h>

using namespace std;
using namespace dev;
using namespace dev::p2p;
using namespace dev::plugin;
using namespace dev::consensus;

namespace dev{
    namespace plugin
    {
        class TransactionExecuter:public enable_shared_from_this<TransactionExecuter>
        {
            public:
                TransactionExecuter(shared_ptr <P2PInterface> group_p2p_service, shared_ptr <P2PInterface> p2p_service, 
                    PROTOCOL_ID group_protocolID, PROTOCOL_ID protocolID, string& nearest_upper_shardid, string& lower_shardids)
                {
                    string path = "./" + to_string(internal_groupId);
                    dev::plugin::executiveContext = make_shared<ExecuteVM>(path);

                    exec = dev::plugin::executiveContext->getExecutive();
                    vm = dev::plugin::executiveContext->getExecutiveInstance();
                    
                    m_nearest_upper_shardid = nearest_upper_shardid;
                    m_lower_shardids = lower_shardids;
                    m_group_p2p_service = group_p2p_service;
                    m_group_protocolID = group_protocolID;
                    m_p2p_service = p2p_service;
                    m_protocolID = protocolID;
                    processedTxNum = 0;
                }

                void processConsensusBlock();

                shared_ptr<dev::executive::Executive> getExecutive() { return exec; }

                void executeTx(shared_ptr<Transaction> tx);

                void setAttribute(shared_ptr<dev::blockchain::BlockChainInterface> _blockchainManager);

            public:
                shared_ptr<dev::executive::Executive> exec;
                shared_ptr<dev::eth::EVMInterface> vm;
                shared_ptr<P2PInterface> m_p2p_service;
                shared_ptr<P2PInterface> m_group_p2p_service;
                shared_ptr<dev::blockchain::BlockChainInterface> m_blockchainManager;

                string m_nearest_upper_shardid; // 最近祖先
                string m_lower_shardids; // 所有下层分片
                int processedTxNum = 0; // 统计处理的交易数目
                PROTOCOL_ID m_group_protocolID;
                PROTOCOL_ID m_protocolID;
        };
    }
}