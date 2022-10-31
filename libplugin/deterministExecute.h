#pragma once

namespace dev{
    namespace plugin
    {
        class deterministExecute:public std::enable_shared_from_this<deterministExecute>
        {
            public:
                static void deterministExecuteTx();
                void start();
        };
    }
}