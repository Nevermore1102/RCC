#include <libplugin/executeTxPool.h>

using namespace dev::plugin;

void executeTxPool::insertIntraShardTx(dev::eth::Transaction::Ptr tx, std::string& readwrite_key)
{
    PLUGIN_LOG(INFO) << LOG_DESC("开始缓存被阻塞的片内交易...");

    // 检查candidateTxQueues中是否有相应读写集的队列
    if(candidateTxQueues->count(readwrite_key) == 0)
    {
        std::shared_ptr<std::queue<std::shared_ptr<dev::eth::Transaction>>> txQueue;
        candidateTxQueue caQueue{readwrite_key, txQueue};
        caQueue.queue->push(tx);
        candidateTxQueues->insert(std::make_pair(readwrite_key, caQueue));
        lockingRWKey->insert(std::make_pair(readwrite_key, 1)); // 更新 locking_key
    }
    else
    {
        candidateTxQueues->at(readwrite_key).queue->push(tx);
        int holdingTxNum = lockingRWKey->at(readwrite_key);
        lockingRWKey->at(readwrite_key) = holdingTxNum + 1;
    }
}

void executeTxPool::insertInterShardTx(dev::eth::Transaction::Ptr tx, std::string& readwrite_key)
{
    PLUGIN_LOG(INFO) << LOG_DESC("开始缓存被阻塞的跨片交易...");
    // 检查candidateTxQueues中是否有相应读写集的队列
    if(candidateTxQueues->count(readwrite_key) == 0)
    {
        auto txQueue = std::make_shared<std::queue<std::shared_ptr<dev::eth::Transaction>>>();
        candidateTxQueue caQueue{readwrite_key, txQueue};
        caQueue.queue->push(tx);
        candidateTxQueues->insert(std::make_pair(readwrite_key, caQueue));
        lockingRWKey->insert(std::make_pair(readwrite_key, 1)); // 更新 locking_key
    }
    else
    {
        candidateTxQueues->at(readwrite_key).queue->push(tx);
        int holdingTxNum = lockingRWKey->at(readwrite_key);
        lockingRWKey->at(readwrite_key) = holdingTxNum + 1;
    }
    PLUGIN_LOG(INFO) << LOG_DESC("交易缓存结束...");

}