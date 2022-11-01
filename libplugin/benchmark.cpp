#include "benchmark.h"
#include <libplugin/executeVM.h>
#include <libplugin/Common.h>
#include <librpc/Rpc.h>
#include <libdevcore/Address.h>

using namespace std;
using namespace dev::plugin;

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