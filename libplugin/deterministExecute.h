#pragma once

#include <libconsensus/pbft/Common.h>
#include <libplugin/executeVM.h>

using namespace std;

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
                }
                static void deterministExecuteTx();
                void start();
        };
    }
}