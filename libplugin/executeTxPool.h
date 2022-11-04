#pragma once

#include "Common.h"
#include <libethcore/Transaction.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_queue.h>
#include <libdevcore/FixedHash.h>

namespace dev{
    namespace plugin{

        struct candidateTxQueue
        {
            std::string rwkey; // 队列所阻塞的读写集
            std::shared_ptr<std::queue<std::shared_ptr<dev::eth::Transaction>>> queue; // 必须用指针类型，防止深拷贝
            // std::queue<std::shared_ptr<dev::eth::Transaction>> queue; // 队列中缓存交易
        };

        class executeTxPool:public std::enable_shared_from_this<executeTxPool>
        {
            public:
                executeTxPool()
                {
                    csTxRWsetNum = std::make_shared<tbb::concurrent_unordered_map<std::string, int>>();
                    receivedTxRWsetNum = std::make_shared<tbb::concurrent_unordered_map<std::string, int>>();
                    candidateTxQueues = std::make_shared<tbb::concurrent_unordered_map<std::string, candidateTxQueue>>();
			        lockingRWKey = std::make_shared<tbb::concurrent_unordered_map<std::string, int>>();
                    txhashTocrossshardtxid = std::make_shared<std::map<dev::h256, std::string>>();
                }
                ~executeTxPool() {};

                void insertIntraShardTx(dev::eth::Transaction::Ptr tx, std::string& readwrite_key);
                void insertInterShardTx(dev::eth::Transaction::Ptr tx, std::string& readwrite_key);

            public:
                std::shared_ptr<tbb::concurrent_unordered_map<std::string, int>> csTxRWsetNum;  // 每笔跨片交易应当收到的读写集个数(crossshardTxId-->num)
                std::shared_ptr<tbb::concurrent_unordered_map<std::string, int>> receivedTxRWsetNum;  // 跨片交易已经收到的不同key的读写集数目 <crossshardTxId_rwkey1, num1>
                std::shared_ptr<tbb::concurrent_unordered_map<std::string, candidateTxQueue>> candidateTxQueues; //  交易缓存队列池 readwriteset --> candidate_tx_queue
                std::shared_ptr<std::map<dev::h256, std::string>> txhashTocrossshardtxid; // 交易hash->相应跨片交易id
                //（注：一笔交易可能要占有片内的多个读写集，如果是这样，需要同时插入到多个队列中，操作之间应该是原子的，否则可能发生死锁，为了实验方便，这里假设每个跨片交易仅占用自己分片的一个key，因此不存在该问题）
                // 于此同时，执行部分先设置1个线程，
		        std::shared_ptr<tbb::concurrent_unordered_map<std::string, int>> lockingRWKey; // 交易池交易因等待收齐状态而正在锁定的状态key
        };
    }
}