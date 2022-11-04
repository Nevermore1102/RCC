#include "Common.h"
#include <libplugin/deterministExecute.h>
#include <libplugin/executeVM.h>
#include <libconsensus/pbft/Common.h>
#include <libconsensus/pbft/PBFTEngine.h>
#include <libethcore/Transaction.h>
#include <thread>
#include <librpc/Common.h>
#include <libplugin/executeTxPool.h>
#include <libsync/Common.h>

using namespace dev::plugin;
using namespace dev::p2p;
using namespace std;

void deterministExecute::deterministExecuteTx()
{
    std::shared_ptr<dev::eth::Transaction> tx;
    while (true)
    {
        bool gettx = dev::consensus::toExecute_transactions.try_pop(tx);
        if(gettx == true)
        {
            auto tx_hash = tx->hash();
            PLUGIN_LOG(INFO) << LOG_DESC("缓存交易的hash") << LOG_KV("tx_hash", tx_hash);
            // 1. 判断交易类型
            if(dev::rpc::innertxhash2readwriteset.count(tx_hash) != 0)
            {
                auto rwkey = dev::rpc::innertxhash2readwriteset.at(tx_hash);
                PLUGIN_LOG(INFO) << LOG_DESC("该交易为片内交易,读写集为")
                                 << LOG_KV("rwkey", rwkey);

                if(m_executeTxPool->lockingRWKey->count(rwkey) == 0) // 片内交易的读写集未被阻塞
                {
                    PLUGIN_LOG(INFO) << LOG_DESC("片内交易的读写集未被阻塞, 立即执行...");
                    auto exec = dev::plugin::executiveContext->getExecutive();
                    auto vm = dev::plugin::executiveContext->getExecutiveInstance();
                    exec->setVM(vm);
                    dev::plugin::executiveContext->executeTransaction(exec, tx);
                    dev::plugin::executiveContext->m_vminstance_pool.push(vm);
                }
                else // 片内交易已经被阻塞
                {
                    m_executeTxPool->insertIntraShardTx(tx, rwkey);
                }
            }
            else if(dev::rpc::corsstxhash2transaction_info.count(tx_hash) != 0)
            {
                auto transaction_info = dev::rpc::corsstxhash2transaction_info.at(tx_hash);
                int readwritesetnum = transaction_info.readwritesetnum;
                std::string crossshardtxid = transaction_info.crossshardtxid;
                std::string participants = transaction_info.participants;
                std::string readwritekeys = transaction_info.readwrite_key;

                PLUGIN_LOG(INFO) << LOG_DESC("该交易为跨片子交易")
                                 << LOG_KV("readwritesetnum", readwritesetnum)
                                 << LOG_KV("crossshardtxid", crossshardtxid)
                                 << LOG_KV("participants", participants)
                                 << LOG_KV("readwritekeys", readwritekeys);
                
                // 将跨片交易缓存起来，当收齐读写集之后再执行 insertInterShardTx
                int itemIndex = 0;
                std::vector<std::string> dataItems;
                std::vector<int> participantGroupIds;

                boost::split(dataItems, participants, boost::is_any_of("|"), boost::token_compress_on); // 对分片中的所有节点id进行遍历, 加入到列表中
                int itemNum = dataItems.size();
                for(size_t i = 0; i < itemNum; i++)
                {
                    if(dataItems.at(i) == to_string(dev::consensus::internal_groupId))
                    {
                        itemIndex = i;
                        PLUGIN_LOG(INFO) << LOG_KV("itemIndex", itemIndex);
                    }
                    participantGroupIds.push_back(atoi(dataItems.at(i).c_str()));
                }

                boost::split(dataItems, readwritekeys, boost::is_any_of("_"), boost::token_compress_on); // 对分片中的所有节点id进行遍历, 加入到列表中
                auto rwkey = dataItems.at(itemIndex);
                PLUGIN_LOG(INFO) << LOG_KV("该笔跨片交易阻塞的分片读写key", rwkey);

                m_executeTxPool->csTxRWsetNum->insert(std::make_pair(crossshardtxid, itemNum)); // 缓存该笔交易应当收到的读写集个数
                // if(m_executeTxPool->lockingRWKey->count(rwkey) == 0)// 检查能否获得本地相应的状态
                // {
                    // PLUGIN_LOG(INFO) << LOG_DESC("能够立即获得本地相应的状态...");
                    // std::string key = crossshardtxid + "_" + rwkey;
                    // m_executeTxPool->receivedTxRWsetNum->insert(std::make_pair(key, 4)); // 缓存该笔交易已经实际收到的读写集个数(本地状态直接赋值4，代表已经收齐读写集)
                    // 这里其实并不需要，执行的时候是从缓存队列的队首中拿交易，队首一定没有与其冲突的交易，即只需要盘算是否收齐来自其他分片的读写集即可

                    // 向其他参与者分片所有节点广播读写集消息
                std::string rwKeyContent = "sss"; // 先假设每个节点发送的内容
                for(size_t i = 0; i < participantGroupIds.size(); i++)
                {
                    if(participantGroupIds.at(i) != dev::consensus::internal_groupId)
                    {
                        int groupId = participantGroupIds.at(i);
                        PLUGIN_LOG(INFO) << LOG_KV("其他参与者分片groupId", groupId);

                        protos::csTxRWset rwset;
                        rwset.set_crossshardtxid(crossshardtxid);
                        rwset.set_readwritekey(rwkey);
                        rwset.set_value(rwKeyContent);

                        string serializedrwset;
                        rwset.SerializeToString(&serializedrwset);
                        auto msgBytes = asBytes(serializedrwset);

                        dev::sync::SyncReadWriteSetMsg retPacket;
                        retPacket.encode(msgBytes);
                        auto msg = retPacket.toMessage(m_group_protocolID);

                        // 准备向对方分片的所有节点广播读写集消息
                        for(size_t k = 0; k < 4; k++)
                        {
                            PLUGIN_LOG(INFO) << LOG_KV("正在发送给", dev::consensus::shardNodeId.at((groupId - 1) * 4 + k));
                            m_p2p_service->asyncSendMessageByNodeID(dev::consensus::shardNodeId.at((groupId - 1) * 4 + k), msg, nullptr);
                        }
                    }
                }
                // }
                m_executeTxPool->insertInterShardTx(tx, rwkey); // 缓存该笔跨片交易
                m_executeTxPool->txhashTocrossshardtxid->insert(std::make_pair(tx_hash, crossshardtxid));
            }
            else
            {
                PLUGIN_LOG(INFO) << LOG_DESC("该交易为部署合约交易"); // 普通部署合约交易能够立即执行
                auto exec = dev::plugin::executiveContext->getExecutive();
                auto vm = dev::plugin::executiveContext->getExecutiveInstance();
                exec->setVM(vm);
                dev::plugin::executiveContext->executeTransaction(exec, tx);
                dev::plugin::executiveContext->m_vminstance_pool.push(vm);
            }
        }

        
        if(m_executeTxPool->lockingRWKey->size() != 0) // 检查是否有跨片交易达到执行要求
        {
            PLUGIN_LOG(INFO) << LOG_DESC("开始检查是否有跨片交易达到执行要求..."); // 普通部署合约交易能够立即执行

            // 检查缓存的交易能否被执行，片内状态已经获得，跨片交易状态收齐
            for(auto iter = m_executeTxPool->candidateTxQueues->cbegin(); iter != m_executeTxPool->candidateTxQueues->end();)
            {
                    auto txQueue = (*iter).second;
                    bool executeSucc = true;

                    while (txQueue.queue->size() > 0 && executeSucc == true)
                    {

                        PLUGIN_LOG(INFO) << LOG_KV("txQueue.queue.size()", txQueue.queue->size())
                                         << LOG_KV("executeSucc", executeSucc);

                        auto frontTx = txQueue.queue->front(); // 深度优先检查每个队列
                        auto tx_hash = frontTx->hash();

                        auto transaction_info = dev::rpc::corsstxhash2transaction_info.at(tx_hash);
                        std::string crossshardtxid = transaction_info.crossshardtxid;
                        std::string participants = transaction_info.participants;
                        std::string readwritekeys = transaction_info.readwrite_key;

                        std::vector<std::string> participantItems;
                        boost::split(participantItems, participants, boost::is_any_of("|"), boost::token_compress_on); // 对分片中的所有节点id进行遍历, 加入到列表中
                        std::vector<std::string> rwKeyItems;
                        boost::split(rwKeyItems, readwritekeys, boost::is_any_of("_"), boost::token_compress_on); // 对分片中的所有节点id进行遍历, 加入到列表中

                        int itemNum = participantItems.size();
                        for(size_t i = 0; i < itemNum; i++)
                        {
                            PLUGIN_LOG(INFO) << LOG_KV("i", i);
                            if(participantItems.at(i) == to_string(dev::consensus::internal_groupId)) // 跳过检查自己分片
                            {
                                continue;
                            }
                            std::string key = crossshardtxid + "_" + rwKeyItems.at(i); // 目前对方分片只有一个，因此只要判断一个过了就行
                            PLUGIN_LOG(INFO) << LOG_KV("key", key);

                            if(m_executeTxPool->receivedTxRWsetNum->count(key) > 0)
                            {
                                if( m_executeTxPool->receivedTxRWsetNum->at(key) > 2) // 收到了2f+1个读写集消息 
                                {
                                    PLUGIN_LOG(INFO) << LOG_DESC("所有读写集准备完毕, 开始执行跨片交易...")
                                                    << LOG_KV("m_executeTxPool->receivedTxRWsetNum->at(key)", m_executeTxPool->receivedTxRWsetNum->at(key)); // 普通部署合约交易能够立即执行

                                    auto exec = dev::plugin::executiveContext->getExecutive(); // 执行交易
                                    auto vm = dev::plugin::executiveContext->getExecutiveInstance();
                                    exec->setVM(vm);
                                    dev::plugin::executiveContext->executeTransaction(exec, tx);
                                    dev::plugin::executiveContext->m_vminstance_pool.push(vm);
                                    txQueue.queue->pop(); // 执行完毕，交易出队列
                                    executeSucc = true;
                                }
                            }
                            else
                            {
                                executeSucc = false;
                            }
                        }
                    }
                    iter++;
            }
        }

        //PLUGIN_LOG(INFO) << LOG_DESC("跳出..."); // 普通部署合约交易能够立即执行

        if(gettx == false && (m_executeTxPool->lockingRWKey->size() == 0)) // 无交易处理，让出CPU处理其他资源
        {
            std::this_thread::yield();
        }
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// void deterministExecute::start()
// {
//     PLUGIN_LOG(INFO) << LOG_DESC("Start DeterministExecute...");
//     std::thread executetxsThread(deterministExecute::deterministExecuteTx);
//     executetxsThread.detach();
// }