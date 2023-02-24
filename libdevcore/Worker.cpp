/*
    This file is part of cpp-ethereum.

    cpp-ethereum is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    cpp-ethereum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file Worker.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */
#include "Worker.h"
#include "Common.h"
#include <pthread.h>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <chrono>
#include <thread>
using namespace std;
using namespace dev;

void dev::setThreadName(std::string const& _n)
{
#if defined(__GLIBC__)
    pthread_setname_np(pthread_self(), _n.c_str());
#elif defined(__APPLE__)
    pthread_setname_np(_n.c_str());
#endif
}

void Worker::startWorking()
{
    /*
     * compare_exchange_strong 入参是3个，expect，desire，memoryorder
     当前值与期望值(expect)相等时，修改当前值为设定值(desired)，返回true
     当前值与期望值(expect)不等时，将期望值(expect)修改为当前值，返回false
     * */
    //对work上锁
    std::unique_lock<std::mutex> l(x_work);
    if (m_work)//如果线程指针存在
    {
        //如果当前 state = Stopped,就把其改为Starting,如果当前值为Starting,则把ex改为Starting
        WorkerState ex = WorkerState::Stopped;
        m_state.compare_exchange_strong(ex, WorkerState::Starting);
        m_state_notifier.notify_all();
    }
    else//如果线程不存在,就要初始化一个线程,给线程命名
    {
        m_state = WorkerState::Starting;
        m_state_notifier.notify_all();
        m_work.reset(new thread([&]() {
            // set threadName for log
            if (boost::log::core::get())
            {
                std::vector<std::string> fields;
                boost::split(fields, m_name, boost::is_any_of("-"), boost::token_compress_on);
                if (fields.size() > 1)
                {
                    boost::log::core::get()->add_thread_attribute(
                        "GroupId", boost::log::attributes::constant<std::string>(fields[1]));
                }
            }
            setThreadName(m_name.c_str());
            //当线程状态不为killing的时候
            while (m_state != WorkerState::Killing)
            {
                WorkerState ex = WorkerState::Starting;
                {
                    // the condition variable-related lock
                    unique_lock<mutex> l(x_work);
                    m_state = WorkerState::Started;
                }

                m_state_notifier.notify_all();
                //让线程开始工作
                try
                {
                    startedWorking();
                    workLoop();
                    doneWorking();
                }
                catch (std::exception const& e)
                {
                    LOG(ERROR) << LOG_DESC("Exception thrown in Worker thread")
                               << LOG_KV("threadName", m_name)
                               << LOG_KV("errorMsg", boost::diagnostic_information(e));
                }

                {
                    //对x_work上锁
                    // the condition variable-related lock
                    unique_lock<mutex> l(x_work);
                    ex = m_state.exchange(WorkerState::Stopped);
                    //如果ex为killing或者ex为Starting,就
                    if (ex == WorkerState::Killing || ex == WorkerState::Starting)
                        m_state.exchange(ex);
                }
                m_state_notifier.notify_all();

                {
                    unique_lock<mutex> l(x_work);
                    DEV_TIMED_ABOVE("Worker stopping", 100)
                    while (m_state == WorkerState::Stopped)
                        m_state_notifier.wait(l);
                }
            }
        }));
    }

    DEV_TIMED_ABOVE("Start worker", 100)
    while (m_state == WorkerState::Starting)
        m_state_notifier.wait(l);
}

void Worker::stopWorking()
{
    //对互斥量x_work上锁
    std::unique_lock<Mutex> l(x_work);
    //如果线程存在
    if (m_work)
    {
        WorkerState ex = WorkerState::Started;
        //返回值为True,代表当前状态和期望值Started相等,那么就改当前指为Stopping,并返回
        if (!m_state.compare_exchange_strong(ex, WorkerState::Stopping))
            return;
        m_state_notifier.notify_all();

        DEV_TIMED_ABOVE("Stop worker", 100)
        //当前状态不为Stopped的时候,就持续等待线程结束
        while (m_state != WorkerState::Stopped)
        {
            m_state_notifier.wait(l);
        }
    }
}

void Worker::terminate()
{
    std::unique_lock<Mutex> l(x_work);
    if (m_work)
    {
        if (m_state.exchange(WorkerState::Killing) == WorkerState::Killing)
            return;  // Somebody else is doing this
        l.unlock();
        m_state_notifier.notify_all();
        DEV_TIMED_ABOVE("Terminate worker", 100)
        m_work->join();

        l.lock();
        m_work.reset();
    }
}

void Worker::workLoop()
{
    //当状态为Started的时候,就开始调用 doWork()
    while (m_state == WorkerState::Started)
    {
        if (m_idleWaitMs)
            this_thread::sleep_for(chrono::milliseconds(m_idleWaitMs));
        doWork();
    }
}
