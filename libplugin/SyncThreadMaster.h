#pragma once

#include <libplugin/PluginMsgBase.h>
#include <librpc/Rpc.h>
#include <libprotobasic/shard.pb.h>

using namespace dev::rpc;
using namespace dev::plugin;

namespace dev
{
    namespace eth
    {
        class Transaction;
    }
}

namespace dev{
    namespace plugin {

        struct cachedDistributedTxPacket
        {
            int sendedCrossshardTxId = 0;
            std::map<int, protos::SubCrossShardTx> cachedDistributedTxPacket;
        };

        /*
        * m_service: 网络传输接口
        * m_protocolId: 同一个group内，m_protocolId相同
        * m_shaId:同一个company内传输消息的m_shaId相同
        * m_nodeId:本节点的nodeId
        * m_sharedId:本节点所属的company
        * m_group:本节点所属的group
        * m_rpc_service: 本节点所属的group的rpc服务
        */
       class SyncThreadMaster{
           public:
                SyncThreadMaster(std::shared_ptr<dev::rpc::Rpc> _rpc_service, std::shared_ptr<dev::p2p::P2PInterface> group_p2p_service, std::shared_ptr <dev::p2p::P2PInterface> p2p_service, 
                    PROTOCOL_ID const & group_protocolID, dev::PROTOCOL_ID protocolID, dev::network::NodeID const& _nodeId, std::shared_ptr<dev::ledger::LedgerManager> ledgerManager, 
                        std::string& nearest_upper_groupId, std::string& lower_shardids)
                        :m_service(group_p2p_service), m_protocolId(group_protocolID), m_nodeId(_nodeId)
                {
                    m_rpc_service = _rpc_service;
                    m_transactionExecuter = std::make_shared<dev::plugin::TransactionExecuter>(group_p2p_service, p2p_service, group_protocolID, protocolID, nearest_upper_groupId, lower_shardids);
                    m_msgEngine = std::make_shared<PluginMsgBase>(group_p2p_service, group_protocolID, _nodeId);
                    m_ledgerManager = ledgerManager;
                    m_receivedResponseNum = std::make_shared<std::map<std::string, int>>();
                    m_receivedCommitResponseNum = std::make_shared<std::map<std::string, int>>();
                }

                void dowork(dev::sync::SyncPacketType const& packettype, byte const& data);

                void dowork(dev::sync::SyncPacketType const& packettype, byte const& data, dev::network::NodeID const& destnodeId);

                void receiveWorker();

                void receiveRemoteReadWriteSet();

                void receiveRemoteBatchIntershardTxs();

                bool startP2PThread();

                void sendMessage(bytes const& _blockRLP, dev::sync::SyncPacketType const& packetreadytype);

                void sendMessage(bytes const& _blockRLP, dev::sync::SyncPacketType const& packettype, dev::network::NodeID const& destnodeId);

                void start(byte const &pt, byte const& data);

                void start(byte const &pt, byte const& data, dev::network::NodeID const& destnodeId);

                void setAttribute(std::shared_ptr <dev::blockchain::BlockChainInterface> m_blockchainManager);

                void setAttribute(std::shared_ptr <PluginMsgManager> _plugin);

                std::map <u256, std::string> contractMap;

                void submitTransactionToTxPool(Transaction::Ptr tx);

                std::shared_ptr<dev::plugin::TransactionExecuter> getdeterministExecute();

                void startExecuteThreads();

                std::shared_ptr<dev::ledger::LedgerManager> m_ledgerManager;

            private:
                shared_ptr<queue<string>> m_remoteAccessedKeys; // 最近100个访问的其他分片的状态key
                std::string m_name;
                std::unique_ptr <std::thread> m_work;
                std::shared_ptr <dev::p2p::P2PInterface> m_service;
                std::shared_ptr <PluginMsgBase> m_msgEngine;
                dev::PROTOCOL_ID m_protocolId;
                // dev::GROUP_ID m_groupId;
                dev::GROUP_ID interal_groupId;
                dev::h512 m_nodeId;
                int counter = 0;

                pthread_t listenthread;
                pthread_t receivethread;

                std::shared_ptr <dev::blockchain::BlockChainInterface> m_blockchainManager;
                std::shared_ptr <PluginMsgManager> m_pluginManager;
                std::shared_ptr<dev::rpc::Rpc> m_rpc_service;
                std::shared_ptr<dev::plugin::TransactionExecuter> m_transactionExecuter;
                set<string> m_sendedStateChangeCommit;
                
                // ADD BY THB cacheCrossShardTxMap
                std::shared_ptr<std::map<std::string, int>> m_receivedResponseNum;
                std::shared_ptr<std::map<std::string, int>> m_receivedCommitResponseNum; 
                cachedDistributedTxPacket _cachedDistributedTxPacket;   
                std::mutex x_map_Mutex;
       };
    }
}