#include "Common.h"
#include <libplugin/SyncThreadMaster.h>
#include <libprotobasic/shard.pb.h>
#include <libsync/SyncMsgPacket.h>
#include <libconsensus/ConsensusEngineBase.h>

using namespace std;
using namespace dev;
using namespace dev::p2p;
using namespace dev::sync;
using namespace dev::plugin;


void SyncThreadMaster::receiveWorker()
{
    protos::SubCrossShardTx msg_subCrossShardTx;
    protos::BatchDistributedTxMsg msg_batchsubCrossShardTxs;
    protos::csTxRWset msg_csTxRWset;
    protos::ResponseToForward msg_responMsg;
    protos::CommitResponseToCoordinator msg_commitResponseToCoordinatorMsg;
    protos::RequestForMasterShardMsg msg_requestForMasterShardMsg;
    protos::MasterShardPrePrepareMsg msg_masterShardPrePrepareMsg;
    protos::MasterShardPrepareMsg msg_masterShardPrepareMsg;
    protos::MasterShardCommitMsg msg_masterShardCommitMsg;
    protos::IntraShardTxMsg msg_intraShardTxMsg;
    protos::ShuffleStateValue msg_shuffleStateValueMsg;
    protos::ShuffleTxRlps msg_shuffleTxMsg;

    bool got_message;

    while(true)
    {
        // 参与者轮循从队列中获取消息
        // 样例代码
        bool got_message = m_pluginManager->batchdistxs->try_pop(msg_batchsubCrossShardTxs);
        if(got_message == true)
        {
            // 加上消息处理逻辑代码...
        }

        got_message = m_pluginManager->distxs->try_pop(msg_subCrossShardTx);
        if(got_message == true) 
        {

        }

        got_message = m_pluginManager->commitResponseToCoordinatorMsg->try_pop(msg_commitResponseToCoordinatorMsg);
        if(got_message == true)
        {

        }

        got_message = m_pluginManager->requestForMasterShardMsg->try_pop(msg_requestForMasterShardMsg);
        if(got_message == true)
        {

        }
        
        got_message = m_pluginManager->masterShardPrePrepareMsg->try_pop(msg_masterShardPrePrepareMsg);
        if(got_message == true)
        {

        }
        
        got_message = m_pluginManager->masterShardPrepareMsg->try_pop(msg_masterShardPrepareMsg);
        if(got_message == true)
        {

        }

        got_message = m_pluginManager->masterShardCommitMsg->try_pop(msg_masterShardCommitMsg);
        if(got_message == true)
        {

        }

        got_message = m_pluginManager->intraShardTxMsg->try_pop(msg_intraShardTxMsg);
        if(got_message == true)
        {

        }

        got_message = m_pluginManager->shuffleStateValueMsg->try_pop(msg_shuffleStateValueMsg);
        if(got_message == true)
        {

        }

        got_message = m_pluginManager->shuffleTxMsg->try_pop(msg_shuffleTxMsg);
        if(got_message == true)
        {

        }

        if(got_message == false)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

bool SyncThreadMaster::startP2PThread()
{
    typedef void* (*FUNC)(void*);
    FUNC receiveWorkerCallback = (FUNC)&SyncThreadMaster::receiveWorker;

    int ret = pthread_create(&receivethread, NULL, receiveWorkerCallback, this);

    if (ret != 0)
    {
        PLUGIN_LOG(INFO) << LOG_DESC("跨片通信P2P消息处理线程启动成功");
        return false;
    }
}

std::shared_ptr<dev::plugin::TransactionExecuter> SyncThreadMaster::getdeterministExecute()
{
    return m_transactionExecuter;
}

void SyncThreadMaster::startExecuteThreads()
{
    PLUGIN_LOG(INFO) << LOG_DESC("startExecuteThreads...");

    std::thread{[this]()  {
        m_transactionExecuter->processConsensusBlock();
    }}.detach();
}

void SyncThreadMaster::dowork(dev::sync::SyncPacketType const& packettype, byte const& data)
{
    
}

void SyncThreadMaster::dowork(dev::sync::SyncPacketType const& packettype, byte const& data, dev::network::NodeID const& destnodeId)
{
    
}

void SyncThreadMaster::sendMessage(bytes const& _blockRLP, dev::sync::SyncPacketType const& packetreadytype)
{

}

void SyncThreadMaster::sendMessage(bytes const& _blockRLP, dev::sync::SyncPacketType const& packettype, dev::network::NodeID const& destnodeId)
{

}

void SyncThreadMaster::start(byte const &pt, byte const& data)
{
    SyncPacketType packettype;
    switch (pt)
    {
        case 0x06:
            packettype = ParamRequestPacket;
            break;
        case 0x07:
            packettype = ParamResponsePacket;
            break;
        case 0x08:
            packettype = CheckpointPacket;
            break;
        case 0x09:
            packettype = ReadSetPacket;
            break;
        case 0x10:
            packettype = BlockForExecutePacket;
            break;
        case 0x11:
            packettype = BlockForStoragePacket;
            break;
        case 0x12:
            packettype = CommitStatePacket;
            break;
        default:
            std::cout << "unknown byte type!" << std::endl;
            break;
    }
    dowork(packettype, data);
}

void SyncThreadMaster::start(byte const &pt, byte const& data, dev::network::NodeID const& destnodeId)
{
    SyncPacketType packettype;
    switch (pt)
    {
        case 0x06:
            packettype = ParamRequestPacket;
            break;
        case 0x07:
            packettype = ParamResponsePacket;
            break;
        case 0x08:
            packettype = CheckpointPacket;
            break;
        case 0x09:
            packettype = ReadSetPacket;
            break;
        case 0x10:
            packettype = BlockForExecutePacket;
            break;
        case 0x11:
            packettype = BlockForStoragePacket;
            break;
        case 0x12:
            packettype = CommitStatePacket;
            break;
        default:
            std::cout << "unknown byte type!" << std::endl;
            break;
    }
    dowork(packettype, data, destnodeId);
}


void SyncThreadMaster::setAttribute(std::shared_ptr<dev::blockchain::BlockChainInterface> _blockchainManager)
{
    m_blockchainManager = _blockchainManager;
    m_msgEngine->setAttribute(_blockchainManager);
    m_transactionExecuter->setAttribute(_blockchainManager);
}


void SyncThreadMaster::setAttribute(std::shared_ptr<PluginMsgManager> _pluginManager)
{
    m_pluginManager = _pluginManager;
    m_msgEngine->setAttribute(_pluginManager);
}

// void SyncThreadMaster::cacheCrossShardTx(std::string _rlpstr, protos::SubCrossShardTx _subcrossshardtx)
// {
//     std::lock_guard<std::mutex> lock(x_map_Mutex);
//     m_cacheCrossShardTxMap.insert(std::make_pair( _rlpstr, _subcrossshardtx ));
// }

void SyncThreadMaster::submitTransactionToTxPool(Transaction::Ptr tx)
{
    auto txPool = m_ledgerManager->txPool(dev::consensus::internal_groupId);
    txPool->submitTransactions(tx);
}