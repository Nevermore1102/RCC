#include "benchmark.h"
#include "libdevcore/Log.h"
#include "libethcore/ABI.h"
//#include <libplugin/executeVM.h>
#include <libplugin/Common.h>
#include <librpc/Rpc.h>
#include <libdevcore/Address.h>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

using namespace std;
using namespace dev::plugin;
using namespace dev::consensus;
//9分片注入交易主逻辑
void transactionInjectionTest::injectionTxin9shards()
{
    //类型: 1，片内交易 ; 2，域内跨篇 （1，2，3）（2，4，5，6）（3，7，8，9） ; 3，跨域交易
    //vec_rlp1 :片内交易 : introtx 
    //vec_rlp2 :域内交易 shards1:12,13  ||shard2: 23 _ 24 25 26 _ ||shard3: _ 37 38 39 ||shard4:_ 45 46_  ||shard5:_ 56 _
    //                  shards6:       || shard7: __78 79         ||shard8: __89    ||shard9 :__
    //vec_rlp3 :跨域交易 
    //需要的参数
    
    //@ 1, 分片id; 2, 
    LOG(INFO)<<LOG_DESC("开始进行试验")<<LOG_KV("分片总数",m_sum_shard)<<LOG_KV("所属分片", m_internal_groupId)
            <<LOG_KV("is leader:", m_isleader);
    intiTopo4nineshards();
    for(int i=1;i<=9;i++)
    {
        LOG(INFO)<<LOG_KV("*********i=", i)<<LOG_KV("size1:", topo_area[i].size())<<LOG_KV("size2:", topo_cross_area[i].size());
    }
    //9分片片内交易发送测试
    if(m_internal_groupId>=4) return ;
    if(!m_isleader) return ;

    LOG(INFO)<<LOG_KV("*********进行4分片实验,分片号为",m_internal_groupId);
    //if(m_internal_groupId>2) return ;
    //片内比例
    if(m_internal_groupId==1)
    {
        m_rpcService->sendRandomRawTransaction4sharperWithMixedTxs(0.6,m_internal_groupId,getRlps4fourShards(),12500);
    }
    else if(m_internal_groupId==2)
    {
        m_rpcService->sendRandomRawTransaction4sharperWithMixedTxs(1,m_internal_groupId,getRlps4fourShards(),7500);
    }
    else if(m_internal_groupId==3)
    {
        m_rpcService->sendRandomRawTransaction4sharperWithMixedTxs(0.6,m_internal_groupId,getRlps4fourShards(),12500);
    }
    else if(m_internal_groupId==4)
    {
        m_rpcService->sendRandomRawTransaction4sharperWithMixedTxs(1,m_internal_groupId,getRlps4fourShards(),7500);
    }
    //if(m_internal_groupId==9) return;
    if(!m_isleader) return ;
    // std::string signedTransaction =createIntraTx4sharper();
    // m_rpcService->sendRandomRawTransaction4sharper(m_internal_groupId,signedTransaction,2000);
    //send
    //injectionCorssTxin9shards();
}
vector<std::string> transactionInjectionTest::getRlps4fourShards()
{
    vector<std::string> res;
    res.push_back(createIntraTx4sharper());
    if(m_internal_groupId==1)
    {
        res.push_back(createCrossTx4sharper(1,1,2));
        //res.push_back(createCrossTx4sharper(1,1,3));
        //res.push_back(createCrossTx4sharper(1,1,4));
    }
    else if(m_internal_groupId==2)
    {
        res.push_back(createCrossTx4sharper(2,2,3));
        //res.push_back(createCrossTx4sharper(2,2,4));
    }
    else if(m_internal_groupId==3)
    {
        res.push_back(createCrossTx4sharper(3,3,4));
    }
    else if(m_internal_groupId==4)
    {
        res.push_back(createCrossTx4sharper(4,4,1));
    }
    return res;
}
void transactionInjectionTest::intiTopo4nineshards()
{
    //域内
    topo_area[1]=vector<int>{2,3};
    topo_area[2]=vector<int>{3,4,5,6};
    topo_area[3]=vector<int>{7,8,9};
    topo_area[4]=vector<int>{5,6};
    topo_area[5]=vector<int>{6};
    topo_area[6]=vector<int>();
    topo_area[7]=vector<int>{8,9};
    topo_area[8]=vector<int>{9};
    topo_area[9]=vector<int>();
    //跨域
    topo_cross_area[1]=vector<int>{4,5,6,7,8,9};
    topo_cross_area[2]=vector<int>{7,8,9};
    topo_cross_area[3]=vector<int>{4,5,6};
    topo_cross_area[4]=vector<int>{7,8,9};
    topo_cross_area[5]=vector<int>{7,8,9};
    topo_cross_area[6]=vector<int>{7,8,9};
    topo_cross_area[7]=vector<int>();
    topo_cross_area[8]=vector<int>();
    topo_cross_area[9]=vector<int>();


}
void transactionInjectionTest::injectionCorssTxin9shards()
{
    std::string signedTransaction=createCrossTx4sharper(m_internal_groupId, m_internal_groupId, m_internal_groupId-1);
    m_rpcService->sendRandomRawTransaction4sharper(m_internal_groupId,signedTransaction,2000);
    return;
}
//!!!
void transactionInjectionTest::injectionIntraTxin1shards(int num_tx) 
{
    std::string signedTransaction=createIntraTx4sharper();
    m_rpcService->sendRandomRawTransaction4sharper(m_internal_groupId,signedTransaction,2000);
    return; 
}
//产生一笔片内交易
std::string transactionInjectionTest::createIntraTx4sharper()
{
    std::string requestLabel = "0x444555666";
    std::string flag = "|";
    std::string stateAddress = "state" + to_string((rand() % 100) + 1)
                            + "_state" + to_string((rand() % 100) + 1);
    
    PLUGIN_LOG(INFO) << LOG_DESC("createInnerTransactions...")
                     << LOG_KV("stateAddress", stateAddress);

    // std::string hex_m_testdata_str = requestLabel + flag + std::to_string(sourceshardid) + flag + std::to_string(destinshardid)
    //                                     + flag + readwritekey + flag + requestmessageid + flag + std::to_string(coordinatorshardid);

    std::string hex_m_data_str = requestLabel + flag + stateAddress + flag;

    // 自己构造交易
    // 虚空合约地址
    std::string str_address=innerContact_1;
    // if (_groupId == 1) {
    //     // PLUGIN_LOG(INFO) << LOG_DESC("GroupID为1...");
    //     str_address = innerContact_1;
    // } else if (_groupId == 2) {
    //     // PLUGIN_LOG(INFO) << LOG_DESC("GroupID为2...");
    //     str_address = innerContact_2;
    // } else if (_groupId == 3) {
    //     // PLUGIN_LOG(INFO) << LOG_DESC("GroupID为3...");
    //     str_address = innerContact_3;
    // }
    dev::Address contactAddress(str_address);
    dev::eth::ContractABI abi;
    bytes data = abi.abiIn("add(string)", hex_m_data_str);  // add
    // bytes data = [];

    Transaction tx(0, 1000, 0, contactAddress, data);
    tx.setNonce(tx.nonce() + u256(utcTime()));
    tx.setGroupId(m_internal_groupId);
    tx.setBlockLimit(u256(m_ledgerManager->blockChain(m_internal_groupId)->number()) + 500);
    
    auto keyPair = KeyPair::create();
    auto sig = dev::crypto::Sign(keyPair, tx.hash(WithoutSignature));
    tx.updateSignature(sig);

    auto rlp = tx.rlp();
    PLUGIN_LOG(INFO) << LOG_DESC("交易生成完毕...")
                     << LOG_KV("rlp", toHex(rlp));

    // m_rpcService->sendRawTransaction(_groupId, toHex(rlp)); // 通过调用本地的RPC接口发起新的共识
    // PLUGIN_LOG(INFO) << LOG_DESC("发送完毕...");

    return toHex(rlp);
}
//产生一笔跨片交易
//@para: 1,分片1(一般指自己分片) 2,分片2 (另一分片)
std::string transactionInjectionTest::createCrossTx4sharper(int coorGroupId,int subGroupId1,int  subGroupId2)
{
    std::string requestLabel = "0x111222333";
    std::string flag = "|";
    // std::string stateAddress = "state1";
    // srand((unsigned)time(0));

    std::string stateAddress1 = "state" + to_string((rand() % 100) + 1);
    std::string stateAddress2 = "state" + to_string((rand() % 100) + 1);


    PLUGIN_LOG(INFO) << LOG_DESC("createCrossTransactions...")
                     << LOG_KV("stateAddress", stateAddress1)
                     << LOG_KV("stateAddress", stateAddress2);

    auto keyPair = KeyPair::create();

    // 生成子交易1
    std::string str_address;
    if (subGroupId1 == 1) {
        str_address = innerContact_1;
    } else if (subGroupId1 == 2) {
        str_address = innerContact_2;
    } else if (subGroupId1 == 3) {
        str_address = innerContact_3;
    }
    dev::Address subAddress1(str_address);
    dev::eth::ContractABI abi;
    bytes data = abi.abiIn("add(string)");  // add
    // bytes data = [];

    Transaction subTx1(0, 1000, 0, subAddress1, data);
    subTx1.setNonce(subTx1.nonce() + u256(utcTime()));
    subTx1.setGroupId(subGroupId1);
    subTx1.setBlockLimit(u256(m_ledgerManager->blockChain(m_internal_groupId)->number()) + 500);

    
    auto subSig1 = dev::crypto::Sign(keyPair, subTx1.hash(WithoutSignature));
    subTx1.updateSignature(subSig1);

    auto subrlp1 = subTx1.rlp();
    std::string signTx1 = toHex(subrlp1);

    // 生成子交易2
    if (subGroupId2 == 1) {
        str_address = innerContact_1;
    } else if (subGroupId2 == 2) {
        str_address = innerContact_2;
    } else if (subGroupId2 == 3) {
        str_address = innerContact_3;
    }
    dev::Address subAddress2(str_address);
    // dev::eth::ContractABI abi;
    data = abi.abiIn("add(string)");  // sub
    // bytes data = [];

    Transaction subTx2(0, 1000, 0, subAddress2, data);
    subTx2.setNonce(subTx2.nonce() + u256(utcTime()));
    subTx2.setGroupId(subGroupId2);
    subTx2.setBlockLimit(u256(m_ledgerManager->blockChain(m_internal_groupId)->number()) + 500);

    
    // auto keyPair = KeyPair::create();
    auto subSig2 = dev::crypto::Sign(keyPair, subTx2.hash(WithoutSignature));
    subTx2.updateSignature(subSig2);

    auto subrlp = subTx2.rlp();
    std::string signTx2 = toHex(subrlp);

    // 生成跨片交易
    std::string hex_m_data_str = requestLabel
                                + flag + std::to_string(subGroupId1) + flag + signTx1 + flag + stateAddress1 
                                + flag + std::to_string(subGroupId2) + flag + signTx2 + flag + stateAddress2
                                + flag;

    str_address = crossContact_3;
    dev::Address crossAddress(str_address);
    // dev::eth::ContractABI abi;
    data = abi.abiIn("set(string)", hex_m_data_str);  // set
    // bytes data = [];

    Transaction tx(0, 1000, 0, crossAddress, data);
    tx.setNonce(tx.nonce() + u256(utcTime()));
    tx.setGroupId(coorGroupId);
    tx.setBlockLimit(u256(m_ledgerManager->blockChain(coorGroupId)->number()) + 500);

    
    // auto keyPair = KeyPair::create();
    auto sig = dev::crypto::Sign(keyPair, tx.hash(WithoutSignature));
    tx.updateSignature(sig);

    auto rlp = tx.rlp();
    PLUGIN_LOG(INFO) << LOG_DESC("跨片交易生成完毕...")
                     << LOG_KV("rlp", toHex(rlp));

    // m_rpcService->sendRawTransaction(coorGroupId, toHex(rlp)); // 通过调用本地的RPC接口发起新的共识
    // PLUGIN_LOG(INFO) << LOG_DESC("发送完毕...");

    return toHex(rlp);
}
void transactionInjectionTest::deployContractTransaction(std::string filename, int32_t groupId)
{
    ifstream infile(filename, ios::binary); //deploy.json
    Json::Reader reader;
    Json::Value root;
    std::string deploytx_str = "";

    if(reader.parse(infile, root))
    {
        for(int i = 0; i < root.size(); i++)
        {
            std::string deployTx = root[i].asString();
            m_rpcService->sendRawTransaction(groupId, deployTx);
        }
    }

    //         deploytx_str = root[i].asString();

    //         Transaction::Ptr tx = std::make_shared<Transaction>(
    //             jsToBytes(deploytx_str, OnFailed::Throw), CheckTransaction::Everything);

    //         auto exec = executiveContext->getExecutive();
    //         auto vm = executiveContext->getExecutiveInstance();
    //         exec->setVM(vm);
    //         executiveContext->executeTransaction(exec, tx);

    //         Address newAddress = exec->newAddress();
    //         PLUGIN_LOG(INFO) << LOG_KV("Contract created at ", newAddress);

    //         u256 value = 0;
    //         u256 gasPrice = 0;
    //         u256 gas = 100000000;
    //         auto keyPair = KeyPair::create();
    //         Address caller = Address("1000000000000000000000000000000000000000");

    //         // add()
    //         bytes callDataToSet =
    //             fromHex(string("0xb0c8f9dc") +  // set(0xaa)
    //                     string("444555666") +
    //                     string("7c") +
    //                     string("9b2a577e6de3dc20cbaf1d04311fe23411c34ba20"));
    //         Transaction::Ptr setTx =
    //             std::make_shared<Transaction>(value, gasPrice, gas, newAddress, callDataToSet);
    //         auto sig = dev::crypto::Sign(keyPair, setTx->hash(WithoutSignature));
    //         setTx->updateSignature(sig);
    //         setTx->forceSender(caller);
    //         PLUGIN_LOG(INFO) << LOG_KV("setTx RLP", toHex(setTx->rlp()));
    //         m_rpcService->sendRawTransaction(groupId, toHex(setTx->rlp()));

    //         executiveContext->m_vminstance_pool.push(vm);
    //     }
    // }


    // /*
    // // if(reader.parse(infile, root)) { deploytx_str = root[0].asString(); }
    
    // // // m_rpcService->sendRawTransaction(groupId, deploytx_str);

    // // std::string path = "./" + to_string(groupId);
    // // auto executiveContext = std::make_shared<ExecuteVMTestFixture>(path);

    // // auto vm = executiveContext->getExecutiveInstance();
    // // auto exec1 = executiveContext->getExecutive();
    // // exec1->setVM(vm);

    // // Transaction::Ptr tx1 = std::make_shared<Transaction>(
    // //         jsToBytes(deploytx_str, OnFailed::Throw), CheckTransaction::Everything);

    // // PLUGIN_LOG(INFO) << LOG_KV("deploytx_str", deploytx_str);


    // // executiveContext->executeTransaction(exec1, tx1);

    // // Address newAddress = exec1->newAddress();
    // // PLUGIN_LOG(INFO) << LOG_KV("Contract created at: ", newAddress);

    // // if(reader.parse(infile, root)) { deploytx_str = root[1].asString(); }
    // // auto exec2 = executiveContext->getExecutive();

    // // Transaction::Ptr tx2 = std::make_shared<Transaction>(
    // //         jsToBytes(deploytx_str, OnFailed::Throw), CheckTransaction::Everything);

    // // PLUGIN_LOG(INFO) << LOG_KV("deploytx_str", deploytx_str);


    // // executiveContext->executeTransaction(exec2, tx2);

    // // if(reader.parse(infile, root)) { deploytx_str = root[2].asString(); }
    // // auto exec3 = executiveContext->getExecutive();

    // // PLUGIN_LOG(INFO) << LOG_KV("deploytx_str", deploytx_str);


    // // Transaction::Ptr tx3 = std::make_shared<Transaction>(
    // //         jsToBytes(deploytx_str, OnFailed::Throw), CheckTransaction::Everything);

    // // executiveContext->executeTransaction(exec3, tx3);

    // */

   
    // // Deploy a contract
    // u256 value = 0;
    // u256 gasPrice = 0;
    // u256 gas = 100000000;
    // Address caller = Address("1000000000000000000000000000000000000000");
    // bytes code = fromHex(
    //     string("60806040526000805534801561001457600080fd5b50610120806100246000396000f3006080604052600436106049576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680636d4ce63c14604e578063b0c8f9dc146076575b600080fd5b348015605957600080fd5b50606060dc565b6040518082815260200191505060405180910390f35b348015608157600080fd5b5060da600480360381019080803590602001908201803590602001908080601f016020809104026020016040519081016040528093929190818152602001838380828437820191505050505050919291929050505060e5565b005b60008054905090565b600160005401600081905550505600a165627a7a723058201dcc0875527adadfb1650a147c914536d7279256a6f7bd1bc100f26a54c1c7ec0029") +
    //     string(""));

    // Transaction::Ptr tx = std::make_shared<Transaction>(
    //     value, gasPrice, gas, code);  // Use contract creation constructor
    // auto keyPair = KeyPair::create();
    // auto sig = dev::crypto::Sign(keyPair, tx->hash(WithoutSignature));
    // tx->updateSignature(sig);
    // tx->forceSender(caller);

    // PLUGIN_LOG(INFO) << LOG_KV("DeployTx RLP", toHex(tx->rlp()));

    // auto exec = executiveContext->getExecutive();
    // auto vm = executiveContext->getExecutiveInstance();
    // exec->setVM(vm);

    // executiveContext->executeTransaction(exec, tx);
    // Address newAddress = exec->newAddress();
    // cout << "Contract created at: " << newAddress << endl;

    // // set()
    // bytes callDataToSet =
    //     fromHex(string("0x60fe47b1") +  // set(0xaa)
    //             string("00000000000000000000000000000000000000000000000000000000000000aa"));
    // Transaction::Ptr setTx =
    //     std::make_shared<Transaction>(value, gasPrice, gas, newAddress, callDataToSet);
    // sig = dev::crypto::Sign(keyPair, setTx->hash(WithoutSignature));
    // setTx->updateSignature(sig);
    // setTx->forceSender(caller);

    // auto exec1 = executiveContext->getExecutive();

    // executiveContext->executeTransaction(exec1, setTx);

    // PLUGIN_LOG(INFO) << LOG_KV("setTx RLP", toHex(setTx->rlp()));


    // // get()
    // bytes callDataToGet = fromHex(string("6d4ce63c") +  // get()
    //                               string(""));

    // Transaction::Ptr getTx =
    //     std::make_shared<Transaction>(value, gasPrice, gas, newAddress, callDataToGet);
    // sig = dev::crypto::Sign(keyPair, getTx->hash(WithoutSignature));
    // getTx->updateSignature(sig);
    // getTx->forceSender(caller);

    // auto exec2 = executiveContext->getExecutive();

    // executiveContext->executeTransaction(exec2, getTx);
    // PLUGIN_LOG(INFO) << LOG_KV("getTx RLP", toHex(getTx->rlp()));

    infile.close();
    PLUGIN_LOG(INFO) << LOG_DESC("部署合约交易完成...");
}

void transactionInjectionTest::injectionTransactions(std::string filename, int32_t groupId)
{
    PLUGIN_LOG(INFO) << LOG_DESC("进入injectionTransactions");

    ifstream infile(filename, ios::binary); // signedtxs.json

    Json::Reader reader;
    Json::Value root;
    int64_t number = 0;

    if(reader.parse(infile, root))
    {
        number = root.size();
        for(int i = 0; i < number; i++)
        {
            std::string signedTransaction = root[i].asString();
            m_rpcService->sendRawTransaction(groupId, signedTransaction);
        }
    }
    infile.close();
    PLUGIN_LOG(INFO) << LOG_DESC("普通交易发送完成...");
}
std::string transactionInjectionTest::createInnerTransactions4sharper(int32_t _groupId, std::shared_ptr<dev::ledger::LedgerManager> ledgerManager) {
    //
    std::string requestLabel = "0x444555666";
    std::string flag = "|";
    std::string stateAddress = "state" + to_string((rand() % 100) + 1)
                            + "_state" + to_string((rand() % 100) + 1);
    
    PLUGIN_LOG(INFO) << LOG_DESC("createInnerTransactions...")
                     << LOG_KV("stateAddress", stateAddress);

    // std::string hex_m_testdata_str = requestLabel + flag + std::to_string(sourceshardid) + flag + std::to_string(destinshardid)
    //                                     + flag + readwritekey + flag + requestmessageid + flag + std::to_string(coordinatorshardid);

    std::string hex_m_data_str = requestLabel + flag + stateAddress + flag;

    // 自己构造交易
    std::string str_address;
    if (_groupId == 1) {
        // PLUGIN_LOG(INFO) << LOG_DESC("GroupID为1...");
        str_address = innerContact_1;
    } else if (_groupId == 2) {
        // PLUGIN_LOG(INFO) << LOG_DESC("GroupID为2...");
        str_address = innerContact_2;
    } else if (_groupId == 3) {
        // PLUGIN_LOG(INFO) << LOG_DESC("GroupID为3...");
        str_address = innerContact_3;
    }
    dev::Address contactAddress(str_address);
    dev::eth::ContractABI abi;
    bytes data = abi.abiIn("add(string)", hex_m_data_str);  // add
    // bytes data = [];

    Transaction tx(0, 1000, 0, contactAddress, data);
    tx.setNonce(tx.nonce() + u256(utcTime()));
    tx.setGroupId(_groupId);
    tx.setBlockLimit(u256(ledgerManager->blockChain(_groupId)->number()) + 500);
    
    auto keyPair = KeyPair::create();
    auto sig = dev::crypto::Sign(keyPair, tx.hash(WithoutSignature));
    tx.updateSignature(sig);

    auto rlp = tx.rlp();
    PLUGIN_LOG(INFO) << LOG_DESC("交易生成完毕...")
                     << LOG_KV("rlp", toHex(rlp));

    // m_rpcService->sendRawTransaction(_groupId, toHex(rlp)); // 通过调用本地的RPC接口发起新的共识
    // PLUGIN_LOG(INFO) << LOG_DESC("发送完毕...");

    return toHex(rlp);
}

std::string transactionInjectionTest::createCrossTransactions4sharper(int32_t coorGroupId, int32_t subGroupId1, int32_t subGroupId2, 
                        std::shared_ptr<dev::ledger::LedgerManager> ledgerManager) 
{
    std::string requestLabel = "0x111222333";
    std::string flag = "|";
    // std::string stateAddress = "state1";
    // srand((unsigned)time(0));

    std::string stateAddress1 = "state" + to_string((rand() % 100) + 1);
    std::string stateAddress2 = "state" + to_string((rand() % 100) + 1);


    PLUGIN_LOG(INFO) << LOG_DESC("createCrossTransactions...")
                     << LOG_KV("stateAddress", stateAddress1)
                     << LOG_KV("stateAddress", stateAddress2);

    auto keyPair = KeyPair::create();

    // 生成子交易1
    std::string str_address;
    if (subGroupId1 == 1) {
        str_address = innerContact_1;
    } else if (subGroupId1 == 2) {
        str_address = innerContact_2;
    } else if (subGroupId1 == 3) {
        str_address = innerContact_3;
    }
    dev::Address subAddress1(str_address);
    dev::eth::ContractABI abi;
    bytes data = abi.abiIn("add(string)");  // add
    // bytes data = [];

    Transaction subTx1(0, 1000, 0, subAddress1, data);
    subTx1.setNonce(subTx1.nonce() + u256(utcTime()));
    subTx1.setGroupId(subGroupId1);
    subTx1.setBlockLimit(u256(ledgerManager->blockChain(m_internal_groupId)->number()) + 500);

    
    auto subSig1 = dev::crypto::Sign(keyPair, subTx1.hash(WithoutSignature));
    subTx1.updateSignature(subSig1);

    auto subrlp1 = subTx1.rlp();
    std::string signTx1 = toHex(subrlp1);

    // 生成子交易2
    if (subGroupId2 == 1) {
        str_address = innerContact_1;
    } else if (subGroupId2 == 2) {
        str_address = innerContact_2;
    } else if (subGroupId2 == 3) {
        str_address = innerContact_3;
    }
    dev::Address subAddress2(str_address);
    // dev::eth::ContractABI abi;
    data = abi.abiIn("add(string)");  // sub
    // bytes data = [];

    Transaction subTx2(0, 1000, 0, subAddress2, data);
    subTx2.setNonce(subTx2.nonce() + u256(utcTime()));
    subTx2.setGroupId(subGroupId2);
    subTx2.setBlockLimit(u256(ledgerManager->blockChain(m_internal_groupId)->number()) + 500);

    
    // auto keyPair = KeyPair::create();
    auto subSig2 = dev::crypto::Sign(keyPair, subTx2.hash(WithoutSignature));
    subTx2.updateSignature(subSig2);

    auto subrlp = subTx2.rlp();
    std::string signTx2 = toHex(subrlp);

    // 生成跨片交易
    std::string hex_m_data_str = requestLabel
                                + flag + std::to_string(subGroupId1) + flag + signTx1 + flag + stateAddress1 
                                + flag + std::to_string(subGroupId2) + flag + signTx2 + flag + stateAddress2
                                + flag;

    str_address = crossContact_3;
    dev::Address crossAddress(str_address);
    // dev::eth::ContractABI abi;
    data = abi.abiIn("set(string)", hex_m_data_str);  // set
    // bytes data = [];

    Transaction tx(0, 1000, 0, crossAddress, data);
    tx.setNonce(tx.nonce() + u256(utcTime()));
    tx.setGroupId(coorGroupId);
    tx.setBlockLimit(u256(ledgerManager->blockChain(coorGroupId)->number()) + 500);

    
    // auto keyPair = KeyPair::create();
    auto sig = dev::crypto::Sign(keyPair, tx.hash(WithoutSignature));
    tx.updateSignature(sig);

    auto rlp = tx.rlp();
    PLUGIN_LOG(INFO) << LOG_DESC("跨片交易生成完毕...")
                     << LOG_KV("rlp", toHex(rlp));

    // m_rpcService->sendRawTransaction(coorGroupId, toHex(rlp)); // 通过调用本地的RPC接口发起新的共识
    // PLUGIN_LOG(INFO) << LOG_DESC("发送完毕...");

    return toHex(rlp);
}



void transactionInjectionTest::injectionTransactions4sharper(std::string filename, int32_t groupId)
{
    PLUGIN_LOG(INFO) << LOG_DESC("进入injectionTransactions");

    ifstream infile(filename, ios::binary); // signedtxs.json

    Json::Reader reader;
    Json::Value root;
    int64_t number = 0;

    if(reader.parse(infile, root))
    {
        number = root.size();
        for(int i = 0; i < number; i++)
        {
            std::string signedTransaction = root[i].asString();
            m_rpcService->sendRawTransaction4sharper(groupId, signedTransaction);
        }
    }
    infile.close();
    PLUGIN_LOG(INFO) << LOG_DESC("普通交易发送完成...");
}
void transactionInjectionTest::injectionRandomTransactions4sharper(std::string filename, int32_t groupId,int32_t num_randomtx)
{
    PLUGIN_LOG(INFO) << LOG_DESC("开始injectionRandomTransactions4sharper");

    ifstream infile(filename, ios::binary); // signedtxs.json

    Json::Reader reader;
    Json::Value root;
    int64_t number = 0;

    if(reader.parse(infile, root))
    {
        number = root.size();
        for(int i = 0; i < number; i++)
        {
            std::string signedTransaction = root[i].asString();
            m_rpcService->sendRandomRawTransaction4sharper(groupId,signedTransaction,num_randomtx);
        }
    }
    infile.close();
    PLUGIN_LOG(INFO) << LOG_DESC("随机交易发送完成...");
}
// @ratio_intra_cross :片内交易比例 
// @num_randomtx : 交易总数
void transactionInjectionTest::injectionRandomTransactions4sharperWithMixedTxs(std::string filename, int32_t groupId,int32_t num_randomtx,double ratio_intra_cross)
{
    PLUGIN_LOG(INFO) << LOG_DESC("开始injectionRandomTransactions4sharper");

    ifstream infile(filename, ios::binary); // signedtxs.json

    Json::Reader reader;
    Json::Value root;
    int64_t number = 0;
     std::vector<std::string> rlps;
    if(reader.parse(infile, root))
    {
        number = root.size();
        for(int i = 0; i < number; i++)
        {
            std::string signedTransaction = root[i].asString();
            rlps.push_back(signedTransaction);
        }
        m_rpcService->sendRandomRawTransaction4sharperWithMixedTxs(ratio_intra_cross,groupId,rlps,num_randomtx);
    }
    infile.close();
    PLUGIN_LOG(INFO) << LOG_DESC("随机交易发送完成...");
}




std::string transactionInjectionTest::createInnerTransactions(int32_t _groupId, std::shared_ptr<dev::ledger::LedgerManager> ledgerManager) {
    
    std::string requestLabel = "0x444555666";
    std::string flag = "|";
    std::string stateAddress = "state333";

    // std::string hex_m_testdata_str = requestLabel + flag + std::to_string(sourceshardid) + flag + std::to_string(destinshardid)
    //                                     + flag + readwritekey + flag + requestmessageid + flag + std::to_string(coordinatorshardid);

    std::string hex_m_data_str = requestLabel + flag + stateAddress + flag;

    /*
    auto data_str_bytes = hex_m_data_str.c_str();
    int bytelen = strlen(data_str_bytes);
    bytes hex_m_data;
    for(int i = 0; i < bytelen; i++)
    {
        hex_m_data.push_back((uint8_t)data_str_bytes[i]);
    }
    */

    // 自己构造交易
    std::string str_address;
    if (_groupId == 1) {
        PLUGIN_LOG(INFO) << LOG_DESC("GroupID为1...");
        str_address = innerContact_1;
    } else if (_groupId == 2) {
        PLUGIN_LOG(INFO) << LOG_DESC("GroupID为2...");
        str_address = innerContact_2;
    } else if (_groupId == 3) {
        PLUGIN_LOG(INFO) << LOG_DESC("GroupID为3...");
        str_address = innerContact_3;
    }
    dev::Address contactAddress(str_address);
    dev::eth::ContractABI abi;
    bytes data = abi.abiIn("add(string)", hex_m_data_str);  // add
    // bytes data = [];

    Transaction tx(0, 1000, 0, contactAddress, data);
    tx.setNonce(tx.nonce() + u256(utcTime()));
    tx.setGroupId(_groupId);
    tx.setBlockLimit(u256(ledgerManager->blockChain(_groupId)->number()) + 500);
    
    auto keyPair = KeyPair::create();
    auto sig = dev::crypto::Sign(keyPair, tx.hash(WithoutSignature));
    tx.updateSignature(sig);

    auto rlp = tx.rlp();
    PLUGIN_LOG(INFO) << LOG_DESC("交易生成完毕...")
                     << LOG_KV("rlp", toHex(rlp));

    m_rpcService->sendRawTransaction(_groupId, toHex(rlp)); // 通过调用本地的RPC接口发起新的共识

    PLUGIN_LOG(INFO) << LOG_DESC("发送完毕...");

    return toHex(rlp);
}

std::string transactionInjectionTest::createCrossTransactions(int32_t coorGroupId, int32_t subGroupId1, int32_t subGroupId2, 
                        std::shared_ptr<dev::ledger::LedgerManager> ledgerManager) {
    std::string requestLabel = "0x111222333";
    std::string flag = "|";
    std::string stateAddress = "state1";
    auto keyPair = KeyPair::create();

    // 生成子交易1
    std::string str_address;
    if (subGroupId1 == 1) {
        str_address = innerContact_1;
    } else if (subGroupId1 == 2) {
        str_address = innerContact_2;
    } else if (subGroupId1 == 3) {
        str_address = innerContact_3;
    }
    dev::Address subAddress1(str_address);
    dev::eth::ContractABI abi;
    bytes data = abi.abiIn("add(string)");  // add
    // bytes data = [];

    Transaction subTx1(0, 1000, 0, subAddress1, data);
    subTx1.setNonce(subTx1.nonce() + u256(utcTime()));
    subTx1.setGroupId(subGroupId1);
    subTx1.setBlockLimit(u256(1000) + 500);

    
    auto subSig1 = dev::crypto::Sign(keyPair, subTx1.hash(WithoutSignature));
    subTx1.updateSignature(subSig1);

    auto subrlp1 = subTx1.rlp();
    std::string signTx1 = toHex(subrlp1);

    // 生成子交易2
    if (subGroupId2 == 1) {
        str_address = innerContact_1;
    } else if (subGroupId2 == 2) {
        str_address = innerContact_2;
    } else if (subGroupId2 == 3) {
        str_address = innerContact_3;
    }
    dev::Address subAddress2(str_address);
    // dev::eth::ContractABI abi;
    data = abi.abiIn("add(string)");  // sub
    // bytes data = [];

    Transaction subTx2(0, 1000, 0, subAddress2, data);
    subTx2.setNonce(subTx2.nonce() + u256(utcTime()));
    subTx2.setGroupId(subGroupId2);
    //subTx2.setBlockLimit(u256(ledgerManager->blockChain(dev::consensus::internal_groupId)->number()) + 500);
    subTx1.setBlockLimit(u256(1000) + 500);

    
    // auto keyPair = KeyPair::create();
    auto subSig2 = dev::crypto::Sign(keyPair, subTx2.hash(WithoutSignature));
    subTx2.updateSignature(subSig2);

    auto subrlp = subTx2.rlp();
    std::string signTx2 = toHex(subrlp);


    // 生成跨片交易
    std::string hex_m_data_str = requestLabel
                                + flag + std::to_string(subGroupId1) + flag + signTx1 + flag + stateAddress 
                                + flag + std::to_string(subGroupId2) + flag + signTx2 + flag + stateAddress
                                + flag;

    
    str_address = crossContact_3;
    dev::Address crossAddress(str_address);
    // dev::eth::ContractABI abi;
    data = abi.abiIn("set(string)", hex_m_data_str);  // set
    // bytes data = [];

    Transaction tx(0, 1000, 0, crossAddress, data);
    tx.setNonce(tx.nonce() + u256(utcTime()));
    tx.setGroupId(coorGroupId);
    tx.setBlockLimit(u256(ledgerManager->blockChain(coorGroupId)->number()) + 500);

    
    // auto keyPair = KeyPair::create();
    auto sig = dev::crypto::Sign(keyPair, tx.hash(WithoutSignature));
    tx.updateSignature(sig);

    auto rlp = tx.rlp();
    PLUGIN_LOG(INFO) << LOG_DESC("跨片交易生成完毕...")
                     << LOG_KV("rlp", toHex(rlp));

    // m_rpcService->sendRawTransaction(coorGroupId, toHex(rlp)); // 通过调用本地的RPC接口发起新的共识
    // PLUGIN_LOG(INFO) << LOG_DESC("发送完毕...");

    return toHex(rlp);
}

std::string transactionInjectionTest::createCrossTransactions_HB(int32_t coorGroupId, int32_t subGroupId1, int32_t subGroupId2, int32_t squId) {
    std::string requestLabel = "0x111222333";
    std::string flag = "|";
    std::string stateAddress = "0x362de179294eb3070a36d13ed00c61f59bcfb542_0x728a02ac510f6802813fece0ed12e7f774dab69d";
    auto keyPair = KeyPair::create();

    // 生成子交易1
    std::string str_address = "0x362de179294eb3070a36d13ed00c61f59bcfb542";
    dev::Address subAddress1(str_address);
    dev::eth::ContractABI abi;
    bytes data = abi.abiIn("add(string)");  // add

    Transaction subTx1(0, 1000, 0, subAddress1, data);
    subTx1.setNonce(subTx1.nonce() + u256(utcTime()));
    subTx1.setGroupId(subGroupId1);
    
    auto subSig1 = dev::crypto::Sign(keyPair, subTx1.hash(WithoutSignature));
    subTx1.updateSignature(subSig1);

    auto subrlp1 = subTx1.rlp();
    std::string signTx1 = toHex(subrlp1);

    // 生成子交易2
    str_address = "0x728a02ac510f6802813fece0ed12e7f774dab69d";
    dev::Address subAddress2(str_address);
    data = abi.abiIn("add(string)");  // add

    Transaction subTx2(0, 1000, 0, subAddress2, data);
    subTx2.setNonce(subTx2.nonce() + u256(utcTime()));
    subTx2.setGroupId(subGroupId2);
    
    auto subSig2 = dev::crypto::Sign(keyPair, subTx2.hash(WithoutSignature));
    subTx2.updateSignature(subSig2);

    auto subrlp = subTx2.rlp();
    std::string signTx2 = toHex(subrlp);


    // 生成跨片交易
    std::string hex_m_data_str = requestLabel + flag + std::to_string(squId)
                                + flag + std::to_string(subGroupId1) + flag + signTx1 + flag + stateAddress 
                                + flag + std::to_string(subGroupId2) + flag + signTx2 + flag + stateAddress;
    
    PLUGIN_LOG(INFO) << LOG_DESC("in createCrossTransactions_HB...")
                     << LOG_KV("dataStr", hex_m_data_str);
    
    str_address = "0xf9cd680d54778346cc0b018fb45fdaff031c0125";
    dev::Address crossAddress(str_address);
    data = abi.abiIn("set(string)", hex_m_data_str);  // set

    Transaction tx(0, 1000, 0, crossAddress, data);
    tx.setNonce(tx.nonce() + u256(utcTime()));
    tx.setGroupId(coorGroupId);
    
    auto sig = dev::crypto::Sign(keyPair, tx.hash(WithoutSignature));
    tx.updateSignature(sig);

    auto rlp = tx.rlp();
    PLUGIN_LOG(INFO) << LOG_DESC("跨片交易生成完毕...")
                     << LOG_KV("rlp", toHex(rlp));
    
    return toHex(rlp);
}