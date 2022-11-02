#pragma once

#include <libconsensus/pbft/Common.h>
#include <libplugin/executeVM.h>
#include <libplugin/executeTxPool.h>

using namespace std;
using namespace dev::plugin;

namespace dev{
    namespace plugin
    {
        class deterministExecute:public std::enable_shared_from_this<deterministExecute>
        {
            public:
                deterministExecute()
                {
                    std::string path = "./" + to_string(dev::consensus::internal_groupId);
                    dev::plugin::executiveContext = std::make_shared<ExecuteVMTestFixture>(path);
                    m_executeTxPool = std::make_shared<executeTxPool>();
                }
                void deterministExecuteTx();
                void start();

            public:
                std::shared_ptr<executeTxPool> m_executeTxPool;  // 每笔跨片交易应当收到的读写集个数(crossshardTxId-->num)
        };
    }
}