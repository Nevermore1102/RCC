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

    std::string path = "./" + to_string(groupId);
    dev::plugin::executiveContext = std::make_shared<ExecuteVMTestFixture>(path);

    if(reader.parse(infile, root))
    {
        for(int i = 0; i < root.size(); i++)
        {
            std::string deployTx = root[i].asString();
            // m_rpcService->sendRawTransaction(groupId, deployTx);

            deploytx_str = root[i].asString();

            Transaction::Ptr tx = std::make_shared<Transaction>(
                jsToBytes(deploytx_str, OnFailed::Throw), CheckTransaction::Everything);

            auto exec = executiveContext->getExecutive();
            auto vm = executiveContext->getExecutiveInstance();
            exec->setVM(vm);
            executiveContext->executeTransaction(exec, tx);

            Address newAddress = exec->newAddress();
            PLUGIN_LOG(INFO) << LOG_KV("Contract created at ", newAddress);

            u256 value = 0;
            u256 gasPrice = 0;
            u256 gas = 100000000;
            auto keyPair = KeyPair::create();
            Address caller = Address("1000000000000000000000000000000000000000");

            // set()
            bytes callDataToSet =
                fromHex(string("0x60fe47b1") +  // set(0xaa)
                        string("00000000000000000000000000000000000000000000000000000000000000aa"));
            Transaction::Ptr setTx =
                std::make_shared<Transaction>(value, gasPrice, gas, newAddress, callDataToSet);
            auto sig = dev::crypto::Sign(keyPair, setTx->hash(WithoutSignature));
            setTx->updateSignature(sig);
            setTx->forceSender(caller);
            PLUGIN_LOG(INFO) << LOG_KV("setTx RLP", toHex(setTx->rlp()));
            m_rpcService->sendRawTransaction(groupId, toHex(setTx->rlp()));

            executiveContext->m_vminstance_pool.push(vm);
        }
    }



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
    //     string("60606040525b607b6000600050819055505b61018a8061001f6000396000f360606040526000357c01000000000000000000000000000000000000000000000000000000009004806360fe47b11461005d5780636d4ce63c1461007a57806383197ef0146100a2578063b7379733146100b657610058565b610002565b346100025761007860048080359060200190919050506100de565b005b346100025761008c60048050506100ec565b6040518082815260200191505060405180910390f35b34610002576100b460048050506100fe565b005b34610002576100c86004805050610104565b6040518082815260200191505060405180910390f35b806000600050819055505b50565b600060006000505490506100fb565b90565b6000ff5b565b60003073ffffffffffffffffffffffffffffffffffffffff16636d4ce63c600060405160200152604051817c0100000000000000000000000000000000000000000000000000000000028152600401809050602060405180830381600087803b156100025760325a03f11561000257505050604051805190602001509050610187565b9056") +
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