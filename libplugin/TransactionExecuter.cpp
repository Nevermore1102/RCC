#include "Common.h"
#include <libplugin/TransactionExecuter.h>

using namespace dev;
using namespace std;
using namespace dev::eth;
using namespace dev::p2p;
using namespace dev::plugin;

namespace dev
{
namespace eth
{
class Transaction;
}
}

namespace dev
{
namespace plugin {

/** 
 * NOTES:逐个处理共识结束后区块中的交易
 * */
void TransactionExecuter::processConsensusBlock()
{
    int blockid = 0;
    int currentblockid = 0;
    int transactions_size = 0;
    shared_ptr<Block> currentBlock;
    int scannedTxNum = 0;

    while(true) {
        currentblockid = m_blockchainManager->number(); // 当前块高
        if(currentblockid > blockid) {
            blockid++;
            currentBlock = m_blockchainManager->getBlockByNumber(blockid);
            transactions_size = currentBlock->getTransactionSize();
            scannedTxNum += transactions_size;
            PLUGIN_LOG(INFO) << LOG_KV("blockTx_size", transactions_size)
                             << LOG_KV("totalScannedTxNum", scannedTxNum);
            auto transactions = currentBlock->transactions(); // 逐个处理交易
            for(int i = 0; i < transactions_size; i++) {
                auto tx = transactions->at(i);
                executeTx(tx); // 执行交易

                // 跨分片P2P通信接口，msg为序列化后的消息包，NodeID为配置文件中的节点ID
                // m_group_p2p_service->asyncSendMessageByNodeID(NodeID, msg, nullptr);

                // 使用示例
                // // 开始将消息转发给下层分片节点
                // protos::BatchDistributedTxMsg txs;
                // txs.set_id(epochId);
                // txs.set_txcontents(subTxs);
                // txs.set_intrashard_txcontents(intrashardTxs);

                // string serializedTxs;
                // txs.SerializeToString(&serializedTxs);
                // auto txBytes = asBytes(serializedTxs);

                // dev::sync::SyncBatchDistributedTxMsg retPacket;
                // retPacket.encode(txBytes);
                // auto msg = retPacket.toMessage(m_group_protocolID);
                // m_group_p2p_service->asyncSendMessageByNodeID(NodeID, msg, nullptr);
            }
        }
        else {
            this_thread::sleep_for(chrono::milliseconds(10));
        }
}}

/** 
 * NOTES: 使用EVMInstance执行交易 shared_ptr<Transaction> tx
 * */
void TransactionExecuter::executeTx(shared_ptr<Transaction> tx)
{
    vm = dev::plugin::executiveContext->getExecutiveInstance();
    exec->setVM(vm);
    dev::plugin::executiveContext->executeTransaction(exec, tx);
    dev::plugin::executiveContext->m_vminstance_pool.push(vm);
}

/**
 * NOTES: 设置_blockchainManager指针
 * */
void TransactionExecuter::setAttribute(shared_ptr<dev::blockchain::BlockChainInterface> _blockchainManager)
{
    m_blockchainManager = _blockchainManager;
}
    }
}