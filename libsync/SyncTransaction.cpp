/*
 * @CopyRight:
 * FISCO-BCOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FISCO-BCOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>
 * (c) 2016-2019 fisco-dev contributors.
 */
/**
 * @brief : implementation of sync transaction
 * @author: yujiechen
 * @date: 2019-09-16
 */

#include "SyncTransaction.h"
#include "SyncMsgPacket.h"
#include "libconsensus/Common.h"
#include "libdevcore/Log.h"
#include "libsync/Common.h"
#include <json/json.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::sync;
using namespace dev::p2p;
using namespace dev::txpool;

static unsigned const c_maxSendTransactions = 1000;

void SyncTransaction::start()
{
    startWorking();
    m_running.store(true);
    SYNC_LOG(DEBUG) << LOG_DESC("start SyncTransaction") << LOG_KV("groupId", m_groupId);
}

void SyncTransaction::stop()
{
    if (m_running.load())
    {
        m_running.store(false);
        doneWorking();
        stopWorking();
        // will not restart worker, so terminate it
        terminate();
        SYNC_LOG(DEBUG) << LOG_DESC("stop SyncTransaction") << LOG_KV("groupId", m_groupId);
    }
    else
    {
        SYNC_LOG(DEBUG) << LOG_DESC("SyncTransaction already stopped")
                        << LOG_KV("groupId", m_groupId);
    }
}

void SyncTransaction::doWork()
{
    maintainDownloadingTransactions();

    // only maintain transactions for the nodes inner the group
    if (m_needMaintainTransactions && m_newTransactions && m_txQueue->bufferSize() == 0)
    {
        // SYNC_LOG(INFO) << LOG_DESC("m_needMaintainTransactions && m_newTransactions && m_txQueue->bufferSize() == 0,开始调用maintainTransactions()");
        maintainTransactions4nl();
        // maintainTransactions4nl();
    }

    if (m_needForwardRemainTxs)
    {
        SYNC_LOG(INFO) << LOG_DESC("m_needForwardRemainTxs,开始调用forwardRemainingTxs()");
        forwardRemainingTxs();
    }
}

void SyncTransaction::workLoop()
{
    while (workerState() == WorkerState::Started)
    {
        doWork();
        // no new transactions and the size of transactions need to be broadcasted is zero
        if (idleWaitMs() && !m_newTransactions && m_txQueue->bufferSize() == 0)
        {
            boost::unique_lock<boost::mutex> l(x_signalled);
            m_signalled.wait_for(l, boost::chrono::milliseconds(idleWaitMs()));
        }
    }
}



void SyncTransaction::maintainTransactions()
{
    auto ts = m_txPool->topTransactionsCondition(c_maxSendTransactions, m_nodeId);
    auto txSize = ts->size();
    if (txSize == 0)
    {
        m_newTransactions = false;
        return;
    }
    sendTransactions(ts, false, 0);
}

void SyncTransaction::sendTransactions(std::shared_ptr<Transactions> _ts,
    bool const& _fastForwardRemainTxs, int64_t const& _startIndex)
{
    std::shared_ptr<NodeIDs> selectedPeers;
    std::shared_ptr<std::set<dev::h512>> peers = m_syncStatus->peersSet();
    // fastforward remaining transactions
    if (_fastForwardRemainTxs)
    {
        SYNC_LOG(INFO) <<  LOG_DESC("fastforward remaining transactions");
        // copy m_fastForwardedNodes to selectedPeers in case of m_fastForwardedNodes changed
        selectedPeers = std::make_shared<NodeIDs>();
        *selectedPeers = *m_fastForwardedNodes;
    }
    else
    {
        // only broadcastTransactions to the consensus nodes
        if (fp_txsReceiversFilter)
        {
            selectedPeers = fp_txsReceiversFilter(peers);
        }
        else
        {
            selectedPeers = m_syncStatus->peers();
        }
    }
    SYNC_LOG(INFO) <<  LOG_DESC("打印转发目标节点");
    for(auto peer : *selectedPeers){
         SYNC_LOG(INFO) <<  LOG_KV("peer",peer.abridged());
    }

    // send the transactions from RPC
    broadcastTransactions(selectedPeers, _ts, _fastForwardRemainTxs, _startIndex);
    if (!_fastForwardRemainTxs && m_running.load())
    {
        // Added sleep to prevent excessive redundant transaction message packets caused by
        // transaction status spreading too fast
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        sendTxsStatus(_ts, selectedPeers);
    }
}

void SyncTransaction::broadcastTransactions(std::shared_ptr<NodeIDs> _selectedPeers,
    std::shared_ptr<Transactions> _ts, bool const& _fastForwardRemainTxs,
    int64_t const& _startIndex)
{
    //peerTransactions 将存储每个节点（NodeID）需要接收的交易索引列表。
    unordered_map<NodeID, std::vector<size_t>> peerTransactions;
    //计算 _ts（交易列表）中要发送的交易的结束索引。
    auto endIndex =
        std::min((int64_t)(_startIndex + c_maxSendTransactions - 1), (int64_t)(_ts->size() - 1));
    //randomSelectedPeers 是节点列表的副本
    auto randomSelectedPeers = _selectedPeers;
    bool randomSelectedPeersInited = false;
    int64_t consIndex = 0;
    if (m_treeRouter)
    {
        SYNC_LOG(INFO) <<  LOG_DESC("使用treeRouter");
        consIndex = m_treeRouter->consIndex();
    }
    //对于每个节点
    for (ssize_t i = _startIndex; i <= endIndex; ++i)
    {
        auto t = (*_ts)[i];
        NodeIDs peers;

        int64_t selectSize = _selectedPeers->size();
        // add redundancy when receive transactions from P2P
        if ((!t->rpcTx() || t->isKnownBySomeone()) && !_fastForwardRemainTxs)
        {
            continue;
        }
        if (m_treeRouter && !randomSelectedPeersInited && !_fastForwardRemainTxs)
        {
            randomSelectedPeers =
                m_treeRouter->selectNodes(m_syncStatus->peersSet(), consIndex, true);
            randomSelectedPeersInited = true;
        }
        // the randomSelectedPeers is empty
        if (randomSelectedPeers->size() == 0)
        {
            randomSelectedPeersInited = false;
            continue;
        }
        peers = m_syncStatus->filterPeers(
            selectSize, randomSelectedPeers, [&](std::shared_ptr<SyncPeerStatus> _p) {
                bool unsent = !t->isTheNodeContainsTransaction(m_nodeId) || _fastForwardRemainTxs;
                bool isSealer = _p->isSealer;
                return isSealer && unsent && !t->isTheNodeContainsTransaction(_p->nodeId);
            });

        t->appendNodeContainsTransaction(m_nodeId);
        if (0 == peers.size())
            continue;
        for (auto const& p : peers)
        {
            peerTransactions[p].push_back(i);
            t->appendNodeContainsTransaction(p);
        }
    }

    m_syncStatus->foreachPeerRandom([&](shared_ptr<SyncPeerStatus> _p) {
        std::vector<bytes> txRLPs;
        unsigned txsSize = peerTransactions[_p->nodeId].size();
        if (0 == txsSize)
            return true;  // No need to send

        for (auto const& i : peerTransactions[_p->nodeId])
        {
            txRLPs.emplace_back((*_ts)[i]->rlp(WithSignature));
        }


        std::shared_ptr<SyncTransactionsPacket> packet = std::make_shared<SyncTransactionsPacket>();
        if (m_treeRouter)
        {
            packet->encode(txRLPs, true, consIndex);
        }
        else
        {
            packet->encode(txRLPs);
        }
        auto msg = packet->toMessage(m_protocolId, (!_fastForwardRemainTxs));
        m_service->asyncSendMessageByNodeID(_p->nodeId, msg, CallbackFuncWithSession(), Options());
        SYNC_LOG(DEBUG) << LOG_BADGE("Tx") << LOG_DESC("Send transaction to peer")
                        << LOG_KV("txNum", int(txsSize))
                        << LOG_KV("fastForwardRemainTxs", _fastForwardRemainTxs)
                        << LOG_KV("startIndex", _startIndex)
                        << LOG_KV("toNodeId", _p->nodeId.abridged())
                        << LOG_KV("messageSize(B)", msg->buffer()->size());
        return true;
    });
}

//Jason 在获取顶部交易时，将交易分配给不同的节点
void SyncTransaction::maintainTransactions4nl()
{

    auto ts = m_txPool->topTransactionsCondition(c_maxSendTransactions, m_nodeId);
    auto txSize = ts->size();
    if (txSize == 0)
    {
        m_newTransactions = false;
        return;
    }
     // 分配交易给不同节点
    std::unordered_map<NodeID, std::shared_ptr<Transactions>> transactionsByNode;
    SYNC_LOG(INFO) << LOG_KV("consensus::globalSealingNodes",consensus::globalSealingNodes)
                    << LOG_KV("tx size",ts->size());
    for (const auto& tx : *ts)
    {
        auto targetNodeIndex = tx->whichBeSended(consensus::globalSealingNodes);
        SYNC_LOG(INFO) <<  LOG_DESC("在maintainTransactions4nl确定发送给哪个节点")
                        << LOG_KV("tx hash",tx->hash().abridged())
                        << LOG_KV("targetNodeIndex",targetNodeIndex);

        const NodeID targetNodeId = m_blockChain->sealerList().at(targetNodeIndex);
        SYNC_LOG(INFO) <<  LOG_DESC("targetNodeId获取成功");
        if (transactionsByNode.find(targetNodeId) == transactionsByNode.end())
        {
            transactionsByNode[targetNodeId] = std::make_shared<Transactions>();
        }
        transactionsByNode[targetNodeId]->emplace_back(tx);
         SYNC_LOG(INFO) <<  LOG_DESC("tx加入成功");
    }

    // TODO 向每个节点发送分配给它们的交易
    // In SyncTransaction::maintainTransactions4nl 
    SYNC_LOG(INFO) <<  LOG_DESC("向每个节点发送分配给它们的交易");

    for (const auto& entry : transactionsByNode)
    {

        const NodeID& nodeId = entry.first;
        const std::shared_ptr<Transactions>& transactions = entry.second;
        SYNC_LOG(INFO) <<  LOG_KV("nodeId",nodeId.abridged())
                        << LOG_KV("tx size ", transactions->size());
        if (nodeId == m_nodeId) // 如果交易分配给自己
        {
            // 将交易插入交易池
            for (const auto& tx : *transactions)
            {
                m_txPool->import(tx);
            }
            SYNC_LOG(INFO) <<  LOG_DESC("交易分配给自己,插入交易池成功");
        }
        else
        {
            // 向其他节点发送交易
            sendTransactions4nl(transactions, false, 0,nodeId);
            SYNC_LOG(INFO) <<  LOG_DESC("向其他节点发送交易成功,在本节点交易池中删除交易");
            for (const auto& tx : *transactions)
            {
                m_txPool->drop(tx->hash());
            }
            SYNC_LOG(INFO) <<  LOG_DESC("已从交易池中删除发送给其他节点的交易");
        }
        
    }
}


//Jason
void SyncTransaction::sendTransactions4nl(std::shared_ptr<Transactions> _ts,
    bool const& _fastForwardRemainTxs, int64_t const& _startIndex,
    const NodeID& _targetNodeId  = dev::h512())
{
    std::shared_ptr<NodeIDs> selectedPeers;
    std::shared_ptr<std::set<dev::h512>> peers = m_syncStatus->peersSet();
    // fastforward remaining transactions
    if (_fastForwardRemainTxs)//默认为false
    {

        // copy m_fastForwardedNodes to selectedPeers in case of m_fastForwardedNodes changed
        selectedPeers = std::make_shared<NodeIDs>();
        *selectedPeers = *m_fastForwardedNodes;
    }
    else
    {
        // only broadcastTransactions to the consensus nodes
        if (fp_txsReceiversFilter)
        {


            // SYNC_LOG(INFO) <<  LOG_DESC("fp_txsReceiversFilter 不为null");
            // selectedPeers = fp_txsReceiversFilter(peers);
        }
        else
        {
            // selectedPeers = m_syncStatus->peers();
            //  selectedPeers = std::make_shared<NodeIDs>();

        }

    }
    selectedPeers = std::make_shared<NodeIDs>();
    // SYNC_LOG(INFO) <<  LOG_DESC("打印未添加转发目标节点");
    // for(auto peer : *selectedPeers){
    //      SYNC_LOG(INFO) <<  LOG_KV("peer",peer.abridged());
    // }
    //TODO 传递目标节点ID以及要发送的交易。

    if (_targetNodeId != dev::h512())
    {

        selectedPeers->emplace_back(_targetNodeId);
    }
    SYNC_LOG(INFO) <<  LOG_DESC("打印需要添加的目标节点之后")
                    <<  LOG_KV("peer",_targetNodeId.abridged());

    broadcastTransactions4nl(_targetNodeId, _ts, _fastForwardRemainTxs, _startIndex);
    // if (!_fastForwardRemainTxs && m_running.load())
    // {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //     sendTxsStatus(_ts, selectedPeers);
    // }
}




//Jason
void SyncTransaction::broadcastTransactions4nl
(const NodeID& _targetNodeId,
    std::shared_ptr<Transactions> _ts, bool const& _fastForwardRemainTxs,
    int64_t const& _startIndex)
    {   
        //peerTransactions 将存储每个节点（NodeID）需要接收的交易索引列表。
        unordered_map<NodeID, std::vector<size_t>> peerTransactions;
        auto endIndex =
        std::min((int64_t)(_startIndex + c_maxSendTransactions - 1), (int64_t)(_ts->size() - 1));
        for (ssize_t i = _startIndex; i <= endIndex; ++i)
        {
            auto t = (*_ts)[i];
            NodeIDs peers;
            
            // add redundancy when receive transactions from P2P
            if ((!t->rpcTx() || t->isKnownBySomeone()) && !_fastForwardRemainTxs)
            {
                continue;
            }
           
          
            //对方将获得当前交易的hash
            t->appendNodeContainsTransaction(_targetNodeId);
            peerTransactions[_targetNodeId].push_back(i);
        }


        m_syncStatus->foreachPeerRandom([&](shared_ptr<SyncPeerStatus> _p) {
        std::vector<bytes> txRLPs;
        unsigned txsSize = peerTransactions[_p->nodeId].size();
         SYNC_LOG(INFO) << LOG_DESC("broadcastTransactions4nl中的foreachPeerRandom")
                        << LOG_KV("_p.nodeId",_p->nodeId.abridged())
                        << LOG_KV("txSize",txsSize); 
        if (0 == txsSize)
            return true;  // No need to send

        for (auto const& i : peerTransactions[_p->nodeId])
        {
            txRLPs.emplace_back((*_ts)[i]->rlp(WithSignature));
        }

        std::shared_ptr<SyncTransactionsPacket> packet = std::make_shared<SyncTransactionsPacket>();
        int64_t consIndex = 0;
        if (m_treeRouter)
        {
            SYNC_LOG(INFO) <<  LOG_DESC("使用treeRouter");
            consIndex = m_treeRouter->consIndex();
        }
        packet->encode(txRLPs, true, consIndex);
        auto msg = packet->toMessage(m_protocolId, (!_fastForwardRemainTxs));
        m_service->asyncSendMessageByNodeID(_p->nodeId, msg, CallbackFuncWithSession(), Options());
        SYNC_LOG(INFO) << LOG_BADGE("Tx") << LOG_DESC("Send transaction to peer")
                        << LOG_KV("txNum", int(txsSize))
                        << LOG_KV("fastForwardRemainTxs", _fastForwardRemainTxs)
                        << LOG_KV("startIndex", _startIndex)
                        << LOG_KV("toNodeId", _p->nodeId.abridged())
                        << LOG_KV("messageSize(B)", msg->buffer()->size());
        return true;
        });
}


void SyncTransaction::forwardRemainingTxs()
{
    Guard l(m_fastForwardMutex);
    int64_t currentTxsSize = m_txPool->pendingSize();
    // no need to forward remaining transactions if the txpool is empty
    if (currentTxsSize == 0)
    {
        return;
    }
    auto ts = m_txPool->topTransactions(currentTxsSize);
    int64_t startIndex = 0;
    while (startIndex < currentTxsSize)
    {
        sendTransactions(ts, m_needForwardRemainTxs, startIndex);
        startIndex += c_maxSendTransactions;
    }
    for (auto const& targetNode : *m_fastForwardedNodes)
    {
        SYNC_LOG(DEBUG) << LOG_DESC("forwardRemainingTxs") << LOG_KV("txsSize", currentTxsSize)
                        << LOG_KV("targetNode", targetNode.abridged());
    }
    m_needForwardRemainTxs = false;
    m_fastForwardedNodes->clear();
}

void SyncTransaction::maintainDownloadingTransactions()
{
    m_txQueue->pop2TxPool(m_txPool);
}

// send transaction hash
void SyncTransaction::sendTxsStatus(
    std::shared_ptr<dev::eth::Transactions> _txs, std::shared_ptr<NodeIDs> _selectedPeers)
{
    unsigned percent = 25;
    unsigned expectedSelectSize = (_selectedPeers->size() * percent + 99) / 100;
    int64_t selectSize = std::min(expectedSelectSize, m_txsStatusGossipMaxPeers);
    std::map<dev::h512, std::shared_ptr<std::set<dev::h256>>> txsHash;
    {
        for (auto tx : *_txs)
        {
            auto peers = m_syncStatus->filterPeers(
                selectSize, _selectedPeers, [&](std::shared_ptr<SyncPeerStatus> _p) {
                    bool unsent = !tx->isTheNodeContainsTransaction(m_nodeId);
                    bool isSealer = _p->isSealer;
                    return isSealer && unsent && !tx->isTheNodeContainsTransaction(_p->nodeId);
                });
            if (peers.size() == 0)
            {
                continue;
            }
            tx->appendNodeListContainTransaction(peers);
            tx->appendNodeContainsTransaction(m_nodeId);
            for (auto peer : peers)
            {
                if (!txsHash.count(peer))
                {
                    txsHash[peer] = std::make_shared<std::set<dev::h256>>();
                }
                txsHash[peer]->insert(tx->hash());
            }
        }
    }
    auto blockNumber = m_blockChain->number();
    for (auto const& it : txsHash)
    {
        std::shared_ptr<SyncTxsStatusPacket> txsStatusPacket =
            std::make_shared<SyncTxsStatusPacket>();
        if (it.second->size() == 0)
        {
            continue;
        }
        txsStatusPacket->encode(blockNumber, it.second);
        auto p2pMsg = txsStatusPacket->toMessage(m_protocolId);
        m_service->asyncSendMessageByNodeID(it.first, p2pMsg, CallbackFuncWithSession(), Options());
        SYNC_LOG(DEBUG) << LOG_BADGE("Tx") << LOG_DESC("Send transaction status to peer")
                        << LOG_KV("txNum", it.second->size())
                        << LOG_KV("toNode", it.first.abridged())
                        << LOG_KV("messageSize(B)", p2pMsg->length());
    }
}

void SyncTransaction::updateNeedMaintainTransactions(bool const& _needMaintainTxs)
{
    if (_needMaintainTxs != m_needMaintainTransactions)
    {
        // changed from sealer/observer to free-node
        if (m_needMaintainTransactions)
        {
            SYNC_LOG(DEBUG) << LOG_DESC(
                                   "updateNeedMaintainTransactions: node changed from "
                                   "sealer/observer to free-node, freshTxsStatus")
                            << LOG_KV("isSealerOrObserver", _needMaintainTxs)
                            << LOG_KV("isSealerOrObserverBeforeUpdate", m_needMaintainTransactions);
            m_txPool->freshTxsStatus();
        }
        else
        {
            SYNC_LOG(DEBUG) << LOG_DESC(
                                   "updateNeedMaintainTransactions: node changed from free-node to "
                                   "sealer/observer, noteNewTransactions")
                            << LOG_KV("isSealerOrObserver", _needMaintainTxs)
                            << LOG_KV("isSealerOrObserverBeforeUpdate", m_needMaintainTransactions);
            // changed from free-node into sealer/observer
            noteNewTransactions();
        }
        m_needMaintainTransactions = _needMaintainTxs;
    }
}