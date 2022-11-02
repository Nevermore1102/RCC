#include "Common.h"
#include <libplugin/deterministExecute.h>
#include <libplugin/executeVM.h>
#include <libconsensus/pbft/Common.h>
#include <libethcore/Transaction.h>
#include <thread>
#include <librpc/Common.h>
#include <libplugin/executeTxPool.h>

using namespace dev::plugin;

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
            auto exec = dev::plugin::executiveContext->getExecutive();
            auto vm = dev::plugin::executiveContext->getExecutiveInstance();
            exec->setVM(vm);

            /*开始补充跨片交易处理逻辑*/
            // 1. 判断交易类型
            if(dev::rpc::innertxhash2readwriteset.count(tx_hash) != 0)
            {
                auto rwkey = dev::rpc::innertxhash2readwriteset.at(tx_hash);
                PLUGIN_LOG(INFO) << LOG_DESC("该交易为片内交易,读写集为")
                                 << LOG_KV("rwkey", rwkey);

                if(m_executeTxPool->lockingRWKey->count(rwkey) == 0) // 片内交易的读写集未被阻塞
                {
                    PLUGIN_LOG(INFO) << LOG_DESC("片内交易的读写集未被阻塞, 立即执行...");
                    dev::plugin::executiveContext->executeTransaction(exec, tx);
                    dev::plugin::executiveContext->m_vminstance_pool.push(vm);
                }
                else // 片内交易已经被阻塞
                {
                    m_executeTxPool->insertIntraTx(tx, rwkey);
                }
            }
            else if(dev::rpc::corsstxhash2transaction_info.count(tx_hash) != 0)
            {
                auto transaction_info = dev::rpc::corsstxhash2transaction_info.at(tx_hash);
                int readwritesetnum = transaction_info.readwritesetnum;
                std::string crossshardtxid = transaction_info.crossshardtxid;
                std::string participants = transaction_info.participants;
                std::string readwrite_key = transaction_info.readwrite_key;

                PLUGIN_LOG(INFO) << LOG_DESC("该交易为跨片子交易")
                                 << LOG_KV("readwritesetnum", readwritesetnum)
                                 << LOG_KV("crossshardtxid", crossshardtxid)
                                 << LOG_KV("participants", participants)
                                 << LOG_KV("readwrite_key", readwrite_key);
                
                






            }
            else
            {
                PLUGIN_LOG(INFO) << LOG_DESC("该交易为部署合约交易"); // 普通部署合约交易能够立即执行
                dev::plugin::executiveContext->executeTransaction(exec, tx);
                dev::plugin::executiveContext->m_vminstance_pool.push(vm);
            }


            dev::plugin::executiveContext->executeTransaction(exec, tx);
            dev::plugin::executiveContext->m_vminstance_pool.push(vm);
        }
        else
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