#pragma once

#include <libdevcore/CommonData.h>
#include <tbb/concurrent_queue.h>
#include <libdevcore/FixedHash.h>

namespace dev {
namespace plugin {

class ExecuteVMTestFixture;

#define PLUGIN_LOG(LEVEL) LOG(LEVEL) << LOG_BADGE("PLUGIN") << LOG_BADGE("PLUGIN")

    /// peer nodeid info just hardcode
    /// delete later
    const dev::h512 exNodeId[1] = {
            dev::h512("a1218f83771287ec2638faf3abea073a64e37bede0ce5dc670e261dc98de08fd2865d2966c3d24c37d636f4b7822f792ec50797c434db5c69016d0d9b904c142")
    };

    extern std::map<std::string, std::string> txRWSet;
    extern std::map<int, std::vector<std::string>> processingTxD;
    extern std::map<std::string, int> subTxRlp2ID;
    extern std::map<std::string, std::vector<std::string>> resendTxs;
    extern tbb::concurrent_queue<std::vector<std::string>> receCommTxRlps;
    extern std::map<std::string, std::vector<std::string>> conAddress2txrlps;
    extern std::vector<std::string> disTxDepositAddrs;
    extern std::map<std::string, int> subTxNum;
    extern std::vector<std::string> committedDisTxRlp;
    extern std::vector<std::string> preCommittedDisTxRlp;
    extern std::map<std::string, std::string> txRlp2ConAddress;
    extern std::vector<std::string> coordinatorRlp;
    extern int global_internal_groupId;
    extern std::shared_ptr<ExecuteVMTestFixture> executiveContext;

    }  // namespace plugin
}  // namespace dev