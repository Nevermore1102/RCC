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
 * (c) 2016-2018 fisco-dev contributors.
 */

/**
 * @brief : implementation of Consensus
 * @file: Consensus.cpp
 * @author: yujiechen
 * @date: 2018-09-27
 *
 * @ author: yujiechen
 * @ date: 2018-10-26
 * @ file : Sealer.cpp
 * @ modification: rename Consensus.cpp to Sealer.cpp
 */
#include "Sealer.h"
#include <libethcore/LogEntry.h>
#include <libsync/SyncStatus.h>
using namespace std;
using namespace dev::sync;
using namespace dev::blockverifier;
using namespace dev::eth;
using namespace dev::p2p;
using namespace dev::consensus;

/// start the Sealer module
/*
 * 交易打包线程，负责从交易池取交易，并基于节点最高块打包交易，产生新区块，产生的新区块交给Engine线程处理
 * PBFT和Raft的交易打包线程分别为PBFTSealer和RaftSealer。
 * */
void Sealer::start()
{
    if (m_startConsensus)
    {
        SEAL_LOG(WARNING) << "[Sealer module has already been started]";
        return;
    }
    SEAL_LOG(INFO) << "[Start sealer module]";
    //开始以后初始化SealingBlock,默认h256Hash const& filter = h256Hash(), bool resetNextLeader = false
    resetSealingBlock();
    //报告
    m_consensusEngine->reportBlock(*(m_blockChain->getBlockByNumber(m_blockChain->number())));
    //得到最多可以封装的区块数
    m_maxBlockCanSeal = m_consensusEngine->maxBlockTransactions();
    m_syncBlock = false;
    /// start  a thread to execute doWork()&&workLoop()
    startWorking();
    m_startConsensus = true;
}
//
bool Sealer::shouldSeal()
{
    bool sealed = false;
    {
        // ReadGuard是共享锁
        ReadGuard l(x_sealing);
        sealed = m_sealing.block->isSealed();
    }
    //判断条件:
    /*
     * 1. sealed 为 false
     * 2. m_startConsensus为true,代表开始共识
     * 3. 共识引擎的中的节点为Sealer
     * 4. isBlockSyncing为false,即尚未同步
     * */
    return (!sealed && m_startConsensus &&
            m_consensusEngine->accountType() == NodeAccountType::SealerAccount &&
            !isBlockSyncing());
}

void Sealer::reportNewBlock()
{
    //声明一个flag
    bool t = true;
    if (m_syncBlock.compare_exchange_strong(t, false))
    {
        //获block number
        shared_ptr<dev::eth::Block> p_block =
            m_blockChain->getBlockByNumber(m_blockChain->number());
        //如果是空块,打印log日志
        if (!p_block)
        {
            LOG(ERROR) << "[reportNewBlock] empty block";
            return;
        }
        //不是空块就报告(???)
        m_consensusEngine->reportBlock(*p_block);
        //unique_lock 类型, Guards是什么
        WriteGuard l(x_sealing);
        {

            if (shouldResetSealing())
            {
                SEAL_LOG(DEBUG) << "[reportNewBlock] Reset sealing: [number]:  "
                                << m_blockChain->number()
                                << ", sealing number:" << m_sealing.block->blockHeader().number();
                resetSealingBlock();
            }
        }
    }
}
// 是否需要等待?
bool Sealer::shouldWait(bool const& wait) const
{
    return !m_syncBlock && wait;
}

void Sealer::doWork(bool wait)
{
    reportNewBlock();

    if (shouldSeal() && m_startConsensus.load())
    {
        //WriteGuard 是unique_lock,简化了 x_sealing 对象的上锁和解锁操作，方便线程对互斥量上锁
//        SEAL_LOG(INFO) << LOG_DESC("当前节点通过shouldSeal(),可以生成块")<<LOG_KV("blkNum", m_sealing.block->header().number());

        WriteGuard l(x_sealing);
        {
            /// get current transaction num
            uint64_t tx_num = m_sealing.block->getTransactionSize();

            /// add this to in case of unlimited-loop
            if (m_txPool->status().current == 0)
            {
                m_syncTxPool = false;
            }
            else
            {
                m_syncTxPool = true;
            }
            // 默认1000,是可变类型
            auto maxTxsPerBlock = maxBlockCanSeal();
            /// load transaction from transaction queue
            if (maxTxsPerBlock > tx_num && m_syncTxPool == true && !reachBlockIntervalTime())
            {
                // SEAL_LOG(INFO) << LOG_DESC("交易数不够,loading...");
                //从交易池中装载交易,装载数量为 maxTxsPerBlock - tx_num
                loadTransactions(maxTxsPerBlock - tx_num);
            }
                
            /// check enough or reach block interval
            if (!checkTxsEnough(maxTxsPerBlock))
            {
                ///< 10 milliseconds to next loop
                boost::unique_lock<boost::mutex> l(x_signalled);
                m_signalled.wait_for(l, boost::chrono::milliseconds(1));
                return;
            }
//            SEAL_LOG(INFO) << LOG_DESC("通过checkTxsEnough(maxTxsPerBlock)")<<LOG_KV("blkNum", m_sealing.block->header().number());
            //TODO:Jason修改的第二个点, 将shouldHandleBlock注释,让所有区块都能打包块
//            if (shouldHandleBlock())
            if (true)
            {
//                SEAL_LOG(INFO) << LOG_DESC("通过shouldHandleBlock(),可以生成块")<<LOG_KV("blkNum", m_sealing.block->header().number());
                // transactionNum += m_sealing.block->getTransactionSize();
                // SEAL_LOG(INFO) << LOG_KV("Seal_transactionNum", transactionNum); 
                m_txPool->dropBlockTrans(m_sealing.block);
                handleBlock();
            }
        }
    }
    if (shouldWait(wait))
    {
        boost::unique_lock<boost::mutex> l(x_blocksignalled);
        m_blockSignalled.wait_for(l, boost::chrono::milliseconds(10));
    }
}

/**
 * @brief: load transactions from the transaction pool
 * @param transToFetch: max transactions to fetch
 * 从交易池加载交易信息
 */
void Sealer::loadTransactions(uint64_t const& transToFetch)
{
    /// fetch transactions and update m_transactionSet
    m_sealing.block->appendTransactions(
        m_txPool->topTransactions(transToFetch, m_sealing.m_transactionSet, true));
}

/// check whether the blocksync module is syncing
bool Sealer::isBlockSyncing()
{
    SyncStatus state = m_blockSync->status();
    //只要状态不是空闲.就返回true
    return (state.state != SyncState::Idle);
}

/**
 * @brief : reset specified sealing block by generating an empty block
 * 通过生成一个空块来重置指定的密封块
 * @param sealing :  the block should be resetted
 * @param filter : the tx hashes of transactions that shouldn't be packed into sealing block when
loadTransactions(used to set m_transactionSet)
 * @param resetNextLeader : reset realing for the next leader or not ? default is false.
 *                          true: reset sealing for the next leader; the block number of the sealing header should be reset to the current block number add 2
 *                          false: reset sealing for the current leader; the sealing header should be populated from the current block
 */
void Sealer::resetSealingBlock(Sealing& sealing, h256Hash const& filter, bool resetNextLeader)
{
    resetBlock(sealing.block, resetNextLeader);
    sealing.m_transactionSet = filter;
    sealing.p_execContext = nullptr;
}

/**
 * @brief : reset specified block according to 'resetNextLeader' option
 *
 * @param block : the block that should be resetted
 * @param resetNextLeader: reset the block for the next leader or not ? default is false.
 *                         true: reset block for the next leader; the block number of the block header should be reset to the current block number add 2
 *                         false: reset block for the current leader; the block header should be populated from the current block
 */
void Sealer::resetBlock(std::shared_ptr<dev::eth::Block> block, bool resetNextLeader)
{
    /// reset block for the next leader:
    /// 1. clear the block; 2. set the block number to current block number add 2
    if (resetNextLeader)
    {
        SEAL_LOG(DEBUG) << "reset next leader number to:" << (m_blockChain->number() + 2);
        block->resetCurrentBlock();
        block->header().setNumber(m_blockChain->number() + 2);
    }
    /// reset block for current leader:
    /// 1. clear the block; 2. populate header from the highest block
    else
    {
        auto highestBlock = m_blockChain->getBlockByNumber(m_blockChain->number());
        if (!highestBlock)
        {  // impossible so exit
            SEAL_LOG(FATAL) << LOG_DESC("exit because can't get highest block")
                            << LOG_KV("number", m_blockChain->number());
        }
        block->resetCurrentBlock(highestBlock->blockHeader());
        SEAL_LOG(DEBUG) << "resetCurrentBlock to"
                        << LOG_KV("sealingNum", block->blockHeader().number());
    }
}

/**
 * @brief : set some important fields for specified block header (called by PBFTSealer after load
 * transactions finished)
 *
 * @param header : the block header should be setted
 * the resetted fields including to:
 * 1. block import time;
 * 2. sealer list: reset to current leader list
 * 3. sealer: reset to the idx of the block generator
 */
void Sealer::resetSealingHeader(BlockHeader& header)
{
    /// import block
    resetCurrentTime();//重新设置时间
    //设置封装节点列表
    header.setSealerList(m_consensusEngine->consensusList());
    //设置封装节点,nodeIdx返回sealer节点的编号
    header.setSealer(m_consensusEngine->nodeIdx());
    header.setLogBloom(LogBloom());
    header.setGasUsed(u256(0));
    header.setExtraData(m_extraData);
}

/// stop the Sealer module
void Sealer::stop()
{
    if (m_startConsensus == false)
    {
        return;
    }
    SEAL_LOG(INFO) << "Stop sealer module...";
    m_startConsensus = false;
    doneWorking();
    if (isWorking())
    {
        stopWorking();
        // will not restart worker, so terminate it
        terminate();
    }
}
