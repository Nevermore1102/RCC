//#include <grpcpp/grpcpp.h>
// #include <json/json.h>
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

#include <libplugin/ConsensusPluginManager.h>
#include <libplugin/SyncThreadMaster.h>
#include <libplugin/ex_SyncMsgEngine.h>
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
#include <libplugin/benchmark.h>
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
        std::map<std::string, std::string> txRWSet;
        std::map<int, std::vector<std::string>> processingTxD;
        std::map<std::string, int> subTxRlp2ID;
        tbb::concurrent_queue<std::vector<std::string>> receCommTxRlps;
        std::map<std::string, std::vector<std::string>> conAddress2txrlps;
        std::vector<std::string> disTxDepositAddrs;
        std::map<std::string, int> subTxNum;
        std::map<std::string, std::vector<std::string>> resendTxs;
        std::vector<std::string> committedDisTxRlp;
        std::vector<std::string> preCommittedDisTxRlp;
        std::map<std::string, std::string>txRlp2ConAddress;
        std::vector<std::string> coordinatorRlp;
    }
}

namespace dev{
    namespace consensus{
        int internal_groupId; // 当前分片所在的groupID
        int SHARDNUM; // 分片总数
        int NODENUM; // 所有节点数目
        std::vector<dev::h512>forwardNodeId;
        std::vector<dev::h512>shardNodeId;
        std::map<int, int> messageIDs;
        std::set<std::string> sendedcrossshardtxhash; //记录已经发送的跨片子交易
        std::queue<std::shared_ptr<dev::eth::Block>> cachedBlocks;
    }
}

namespace dev{
    namespace blockverifier{
        std::vector<int>latest_commit_cs_tx;
        std::map<std::string, std::shared_ptr<dev::eth::Transaction>> blocked_txs;
        std::map<std::string, std::shared_ptr<dev::eth::Block>> blocked_blocks;
        blocked_tx_pool _blocked_tx_pool;
        std::map<int, blockExecuteContent> cached_executeContents; // 缓存的区块执行变量
    }
}

namespace dev{
    namespace rpc{
        std::vector<dev::h256> subcrosstxhash; // 记录所有待处理的跨片子交易hash
        std::map<dev::h256, int> txhash2sourceshardid; // txhash - > sourceshardid
        std::map<dev::h256, int> txhash2messageid; // txhash - > messageid
        std::map<dev::h256, std::string> txhash2readwriteset; // txhash - > readwriteset
        std::map<dev::h256, std::string> innertxhash2readwriteset; // txhash - > readwriteset
        std::map<dev::h256, transaction_info> corsstxhash2transaction_info; // txhash - > readwriteset

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
}

void putGroupPubKeyIntoService(std::shared_ptr<Service> service, boost::property_tree::ptree const& _pt)
{
    std::map<GROUP_ID, h512s> groupID2NodeList;
    h512s nodelist;
    int groupid;
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
                groupid = (int)((s[s_size - 1])[0] - '0');
            }
            catch (std::exception& e)
            {
                exit(1);
            }
        }
    }
    groupID2NodeList.insert(std::make_pair(groupid, nodelist)); // 都是同一个groupid，所以插入一次就好了
    std::cout << groupID2NodeList[groupid] << std::endl;
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

int main(){

    std::cout<< "* * * * * * * * * HieraChain v0.0.3 * * * * * * * * *" <<std::endl;
    dev::consensus::SHARDNUM = 3; // 初始化分片数目
    std::cout << "SHARDNUM = " << dev::consensus::SHARDNUM << std::endl;

    // 开始增加组间通信同步组
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./configgroup.ini", pt);

    std::string jsonrpc_listen_ip = pt.get<std::string>("rpc.jsonrpc_listen_ip");
    std::string jsonrpc_listen_port = pt.get<std::string>("rpc.jsonrpc_listen_port");
    std::string nearest_upper_groupId = pt.get<std::string>("layer.nearest_upper_groupId");
    std::string nearest_lower_groupId = pt.get<std::string>("layer.nearest_lower_groupId");

    // 对dev::consensus::messageIDs进行初始化
    for(int i = 0; i < dev::consensus::SHARDNUM; i++)
    {
        dev::consensus::messageIDs.insert(std::make_pair(i, 0));
    }

    // 对dev::consensus::latest_commit_cs_tx进行初始化
    for(int i = 0; i < dev::consensus::SHARDNUM; i++)
    {
        dev::blockverifier::latest_commit_cs_tx.push_back(0);
    }

    GroupP2PService groupP2Pservice("./configgroup.ini");
    auto p2pService = groupP2Pservice.p2pInitializer()->p2pService();
    putGroupPubKeyIntoService(p2pService, pt);
    putGroupPubKeyIntoshardNodeId(pt); // 读取全网所有节点Ï
    p2pService->start();

    GROUP_ID groupId = std::stoi(pt.get<std::string>("group.global_group_id")); // 全局通信使用的groupid

    auto nodeIdstr = asString(contents("conf/node.nodeid"));
    NodeID nodeId = NodeID(nodeIdstr.substr(0, 128));
    std::string nodeIdHex = toHex(nodeId);

    PROTOCOL_ID syncId = getGroupProtoclID(groupId, ProtocolID::InterGroup);

    std::shared_ptr<dev::initializer::Initializer> initialize = std::make_shared<dev::initializer::Initializer>();
    // initialize->init_with_groupP2PService("./config.ini", p2pService);  // 启动3个群组
    initialize->init_with_groupP2PService("./config.ini", p2pService, syncId);  // 启动3个群组
    // initialize->init("./config.ini");  // 启动3个群组

    dev::consensus::internal_groupId = std::stoi(pt.get<std::string>("group.internal_group_id")); // 片内使通信使用的groupID
    auto secureInitializer = initialize->secureInitializer();
    auto ledgerManager = initialize->ledgerInitializer()->ledgerManager();
    auto consensusP2Pservice = initialize->p2pInitializer()->p2pService();
    auto rpcService = std::make_shared<dev::rpc::Rpc>(initialize->ledgerInitializer(), consensusP2Pservice);
    auto blockchainManager = ledgerManager->blockChain(dev::consensus::internal_groupId);

    shared_ptr<dev::plugin::SyncThreadMaster> syncs = std::make_shared<dev::plugin::SyncThreadMaster>(p2pService, syncId, nodeId, dev::consensus::internal_groupId, rpcService);
    std::shared_ptr<ConsensusPluginManager> consensusPluginManager = std::make_shared<ConsensusPluginManager>(rpcService);
    syncs->setAttribute(blockchainManager);
    syncs->setAttribute(consensusPluginManager);

    // // 测试发送交易（分片1的node1向本分片1发送一笔片内交易
    // if(dev::consensus::internal_groupId == 1 && nodeIdHex == toHex(dev::consensus::forwardNodeId.at(0)))
    // {
    //     PLUGIN_LOG(INFO) << LOG_DESC("准备发送交易...")<< LOG_KV("nodeIdHex", nodeIdHex);
    //     transactionInjectionTest _injectionTest(rpcService, 1);
    //     _injectionTest.deployContractTransaction("./deploy.json", 1);
    //     // _injectionTest.injectionTransactions("./signedtxs.json", 1);
    // }

    std::cout << "node " + jsonrpc_listen_ip + ":" + jsonrpc_listen_port + " start success." << std::endl;

    if(nearest_upper_groupId != "N/A")
    {
        std::cout << "nearest_upper_groupId = " << nearest_upper_groupId << std::endl;
    }
    else
    {
        std::cout<<"it's a root group" << std::endl;
    }

    if(nearest_lower_groupId != "N/A")
    {
        std::cout << "nearest_lower_groupId = " << nearest_lower_groupId << std::endl;
    }
    else
    {
        std::cout<<"it's a leaf group" << std::endl;
    }

    size_t duration = 0;
    while (true)
    {
        duration ++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}