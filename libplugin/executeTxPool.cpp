#include <libplugin/executeTxPool.h>

using namespace dev::plugin;

void executeTxPool::insertIntraTx(dev::eth::Transaction::Ptr tx, std::string& readwrite_key)
{
    PLUGIN_LOG(INFO) << LOG_DESC("开始缓存被阻塞的片内交易...");

    // 检查candidateTxQueues中是否有相应读写集的队列
    if(candidateTxQueues->count(readwrite_key) == 0)
    {
        tbb::concurrent_queue<std::shared_ptr<dev::eth::Transaction>> txQueue;
        candidateTxQueue caQueue{readwrite_key, txQueue};
        caQueue.queue.push(tx);
        candidateTxQueues->insert(std::make_pair(readwrite_key, caQueue));
    }
    else
    {
        candidateTxQueues->at(readwrite_key).queue.push(tx);
    }
}