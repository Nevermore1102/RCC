#include <cmath>
#include <leveldb/db.h>
#include <libblockchain/BlockChainImp.h>
#include <libblockverifier/BlockVerifier.h>
#include <libblockverifier/Common.h>
#include <libblockverifier/ExecutiveContextFactory.h>
#include <libdevcore/BasicLevelDB.h>
#include <libdevcore/CommonData.h>
#include <libdevcore/CommonJS.h>
#include <libdevcore/TopicInfo.h>
#include <libdevcrypto/Common.h>
#include <libethcore/ABI.h>
#include <libethcore/Block.h>
#include <libethcore/PrecompiledContract.h>
#include <libethcore/Protocol.h>
#include <libethcore/TransactionReceipt.h>
#include <libinitializer/Initializer.h>
#include <libinitializer/P2PInitializer.h>
#include <libmptstate/MPTStateFactory.h>
#include <librpc/Rpc.h>
#include <libstorage/LevelDBStorage.h>
#include <libstorage/MemoryTableFactory.h>
#include <libstorage/Storage.h>
#include <libstoragestate/StorageStateFactory.h>
#include <libplugin/PluginMsgManager.h>
#include <libplugin/SyncThreadMaster.h>
#include <libplugin/PluginMsgBase.h>
#include <libplugin/Common.h>
#include <stdlib.h>
#include <sys/time.h>
#include <tbb/concurrent_queue.h>
#include <cassert>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
#include <thread>
//#include <libplugin/Benchmark.h>
#include "libdevcore/Log.h"
#include "libplugin/benchmark.h"
#include <libconsensus/pbft/Common.h>
#include <libethcore/Block.h>

using namespace std;
using namespace dev;
using namespace dev::crypto;
using namespace dev::eth;
using namespace dev::rpc;
using namespace dev::ledger;
using namespace dev::initializer;
using namespace dev::txpool;
using namespace dev::blockverifier;
using namespace dev::blockchain;
using namespace dev::storage;
using namespace dev::mptstate;
using namespace dev::executive;
using namespace dev::plugin;

namespace dev {
    namespace plugin {
        shared_ptr<ExecuteVM> executiveContext;
        string nodeIdHex;
        shared_ptr<map<string, bool>> m_validStateKey = make_shared<map<string, bool>>();
        shared_ptr<map<string, bool>> m_masterStateKey = make_shared<map<string, bool>>();
        shared_ptr<map<string, int>> m_masterRequestVotes = make_shared<map<string, int>>();
        shared_ptr<map<string, int>> m_masterChangedKey = make_shared<map<string, int>>();
        shared_ptr<map<h256, string>> intrashardtxhash2rwkeys = make_shared<map<h256, string>>(); // txhash - > readwriteset
        shared_ptr<set<string>> m_lockedStateKey = make_shared<set<string>>();
        Address depositAddress;
    }
}

namespace dev {
    namespace consensus {
        int global_node_idx = -1;
        int internal_groupId; // 当前分片所在的groupID
        int hiera_shard_number; // 分片总数
        vector<h512>forwardNodeId;
        vector<h512>shardNodeId;
        bool isShardLeader=false;

        bool  isNoneLeaderConsensus=true;
    }
}

namespace dev {
    namespace blockverifier{
    }
}

namespace dev{
    namespace rpc{
    }
}

void putGroupPubKeyIntoshardNodeId(boost::property_tree::ptree const& _pt)
{
    size_t index = 0;
    for (auto it : _pt.get_child("group"))
    {
        if (it.first.find("groups.") == 0)
        {
            std::vector<std::string> s;
            try
            {
                boost::split(s, it.second.data(), boost::is_any_of(":"), boost::token_compress_on);

                // 对分片中的所有节点id进行遍历，加入到列表中
                size_t s_size = s.size();
                for(size_t i = 0; i < s_size - 1; i++)
                {
                    h512 node;
                    node = h512(s[i]);
                    dev::consensus::shardNodeId.push_back(node);

                    if(index % 4 == 0)
                    {
                        dev::consensus::forwardNodeId.push_back(node);
                    }
                    index++;
                }
            }
            catch (std::exception& e)
            {
                exit(1);
                
            }
        }
    }
    // PLUGIN_LOG(INFO) << LOG_KV("dev::consensus::forwardNodeId", dev::consensus::forwardNodeId);
}

void putGroupPubKeyIntoService(std::shared_ptr<Service> service, boost::property_tree::ptree const& _pt)
{
    std::map<GROUP_ID, h512s> groupID2NodeList;
    h512s nodelist;
    int groupid;
    size_t index = 0;
    for (auto it : _pt.get_child("group"))
    {
        if (it.first.find("groups.") == 0)
        {
            std::vector<std::string> s;
            try
            {
                boost::split(s, it.second.data(), boost::is_any_of(":"), boost::token_compress_on);
                // 对分片中的所有节点id进行遍历，加入到列表中
                int s_size = s.size();
                for(int i = 0; i < s_size - 1; i++)
                {
                    h512 node;
                    node = h512(s[i]);
                    nodelist.push_back(node);
                }
                //groupid = (int)((s[s_size - 1])[0] - '0');
                // groupid = (int)((s[s_size - 1]));

                groupid = atoi(s.at(s_size - 1).c_str());

                // index++;
                // if(index == 4)
                // {
                //     groupid = (int)((s[s_size - 1])[0] - '0');
                //     PLUGIN_LOG(INFO) << LOG_KV("groupID2NodeList[groupid]", groupID2NodeList[groupid]);
                //     std::cout << "groupID2NodeList[groupid] = " << groupID2NodeList[groupid];
                //     index = 0;
                //     nodelist.clear();
                // }
            }
            catch (std::exception& e)
            {
                exit(1);
            }
        }
    }
    groupID2NodeList.insert(std::make_pair(groupid, nodelist)); // 都是同一个groupid，所以插入一次就好了
//    std::cout << "groupID2NodeList " << groupid << " " << groupID2NodeList[groupid];

    // std::cout << groupID2NodeList[groupid] << std::endl;
    service->setGroupID2NodeList(groupID2NodeList);
}

class GroupP2PService
{
public:
    GroupP2PService(std::string const& _path)
    {
        boost::property_tree::ptree pt;
        boost::property_tree::read_ini(_path, pt);
        m_secureInitializer = std::make_shared<SecureInitializer>();
        m_secureInitializer->initConfig(pt);
        m_p2pInitializer = std::make_shared<P2PInitializer>();
        m_p2pInitializer->setSSLContext(m_secureInitializer->SSLContext(SecureInitializer::Usage::ForP2P));
        m_p2pInitializer->setKeyPair(m_secureInitializer->keyPair());
        m_p2pInitializer->initConfig(pt);
    }
    P2PInitializer::Ptr p2pInitializer() { return m_p2pInitializer; }
    ~GroupP2PService()
    {
        if (m_p2pInitializer)
        {
            m_p2pInitializer->stop();
        }
    }

private:
    P2PInitializer::Ptr m_p2pInitializer;
    SecureInitializer::Ptr m_secureInitializer;
};

void loadHieraInfo(boost::property_tree::ptree& pt, std::string& nearest_upper_groupId, std::string& lower_groupIds)
{
                                    //     
                                    //                              分片9
                                    //            

                                    //                 分片4                        分片8
                                    //       

                                    //         分片1    分片2   分片3          分片5   分片6   分片7
                                    //       
                                    //     

    /*
    // 九分片架构
    if(shardid == 1) {
        nearest_upper_groupId = "4";
        nearest_lower_groupId = "N/A";
    }

    if(shardid == 2) {
        nearest_upper_groupId = "4";
        nearest_lower_groupId = "N/A";
    }

    if(shardid == 3) {
        nearest_upper_groupId = "4";
        nearest_lower_groupId = "N/A";
    }

    if(shardid == 4) {
        nearest_upper_groupId = "9";
        nearest_lower_groupId = "1,2,3";
    }

    if(shardid == 5) {
        nearest_upper_groupId = "8";
        nearest_lower_groupId = "N/A";
    }

    if(shardid == 6) {
        nearest_upper_groupId = "8";
        nearest_lower_groupId = "N/A";
    }

    if(shardid == 7) {
        nearest_upper_groupId = "8";
        nearest_lower_groupId = "N/A";
    }

    if(shardid == 8) {
        nearest_upper_groupId = "9";
        nearest_lower_groupId = "5,6,7";
    }

    if(shardid == 9) {
        nearest_upper_groupId = "N/A";
        nearest_lower_groupId = "1,2,3,4,5,6,7,8";
    }
    */

    nearest_upper_groupId = pt.get<std::string>("layer.nearest_upper_groupId"); // 
    lower_groupIds = pt.get<std::string>("layer.lower_groupIds");
}

int main() {

//    dev::consensus::hiera_shard_number = 9; // 初始化分片数目
    dev::consensus::hiera_shard_number = 1; // 初始化分片数目, 单分片

    // 开始增加组间通信同步组
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./configgroup.ini", pt);

    std::string nearest_upper_groupId = "";
    std::string lower_groupIds = "";
    loadHieraInfo(pt, nearest_upper_groupId, lower_groupIds);

    // cout << "nearest_upper_groupId = " << nearest_upper_groupId << std::endl;
    // cout << "lower_groupIds = " << lower_groupIds << std::endl;

    GroupP2PService groupP2Pservice("./configgroup.ini");
    auto p2pService = groupP2Pservice.p2pInitializer()->p2pService();

    putGroupPubKeyIntoService(p2pService, pt);
    putGroupPubKeyIntoshardNodeId(pt); // 读取全网所有节点
    p2pService->start();

    GROUP_ID groupId = std::stoi(pt.get<std::string>("group.global_group_id")); // 全局通信使用的groupid
    auto nodeIdstr = asString(contents("conf/node.nodeid"));
    NodeID nodeId = NodeID(nodeIdstr.substr(0, 128));
    dev::plugin::nodeIdHex = toHex(nodeId);

    PROTOCOL_ID group_protocol_id = getGroupProtoclID(groupId, ProtocolID::InterGroup);

    std::shared_ptr<dev::initializer::Initializer> initialize = std::make_shared<dev::initializer::Initializer>();
    // initialize->init_with_groupP2PService("./config.ini", p2pService);  // 启动3个群组

    std::shared_ptr<Service> intra_p2pService; // 片内P2P通信指针
    
    initialize->init_with_groupP2PService("./config.ini", p2pService, intra_p2pService, group_protocol_id);  // 启动3个群组
    // initialize->init("./config.ini");  // 启动3个群组

    dev::consensus::internal_groupId = std::stoi(pt.get<std::string>("group.internal_group_id")); // 片内使通信使用的groupID
    PROTOCOL_ID protocol_id = getGroupProtoclID(dev::consensus::internal_groupId, ProtocolID::PBFT);

    auto secureInitializer = initialize->secureInitializer();
    auto ledgerManager = initialize->ledgerInitializer()->ledgerManager();
    auto consensusP2Pservice = initialize->p2pInitializer()->p2pService();
    auto rpcService = std::make_shared<dev::rpc::Rpc>(initialize->ledgerInitializer(), consensusP2Pservice);
    auto blockchainManager = ledgerManager->blockChain(dev::consensus::internal_groupId);

    shared_ptr<dev::plugin::SyncThreadMaster> syncs = 
                std::make_shared<dev::plugin::SyncThreadMaster>(rpcService, p2pService, intra_p2pService, group_protocol_id, protocol_id, nodeId, ledgerManager, nearest_upper_groupId, lower_groupIds);
    std::shared_ptr<PluginMsgManager> pluginManager = 
                std::make_shared<PluginMsgManager>(rpcService, p2pService, intra_p2pService, group_protocol_id, protocol_id);
    syncs->setAttribute(blockchainManager);
    syncs->setAttribute(pluginManager);
    
    syncs->startP2PThread(); // 启动跨片P2P通信线程
    syncs->startExecuteThreads(); // 启动执行线程

    std::this_thread::sleep_for(std::chrono::milliseconds(20000)); // 暂停20秒，等所有服务启动完毕

//     PLUGIN_LOG(INFO) << LOG_DESC("开始注入交易...");
    // injectTransactions(rpcService, ledgerManager);
    //!!! 发送片内交易
    transactionInjectionTest test(rpcService, dev::consensus::internal_groupId
        ,dev::consensus::hiera_shard_number,ledgerManager,consensus::isShardLeader);
    test.injectionIntraTxin1shards(6);
    while (true) {
        //节点主线程三秒一跳，不跳就宕机了
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        LOG(INFO)<<LOG_DESC("***************main heart beat**************");
    }
    return 0;
}