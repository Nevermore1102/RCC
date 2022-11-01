#include "Common.h"
#include <libplugin/deterministExecute.h>
#include <libplugin/executeVM.h>
#include <libconsensus/pbft/Common.h>
#include <libethcore/Transaction.h>
#include <thread>

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
            dev::plugin::executiveContext->executeTransaction(exec, tx);
            dev::plugin::executiveContext->m_vminstance_pool.push(vm);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void deterministExecute::start()
{
    PLUGIN_LOG(INFO) << LOG_DESC("Start DeterministExecute...");
    std::thread executetxsThread(deterministExecute::deterministExecuteTx);
    executetxsThread.detach();
}