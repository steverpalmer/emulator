// safe_queue.hpp
// Copyright 2016 Steve Palmer

#ifndef SYNCHRONIZATION_QUEUE
#define SYNCHRONIZATION_QUEUE

#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>

/** A thread-safe, blocking std::queue-like object.
 *
 * A SynchronizationQueue provides a similar interface to the std::queue
 * adaptor, but only allows thread-safe interface functions.
 * For example, push is allowed, but front isn't because
 * its results may be wrong immediately the function returns.
 * Similarly for functions like empty and size.  Also, retrieving
 * values from the object is done by a pull function which combines
 * front and pop.
 *
 * It can support multiple producers and multiple consumers.
 * It allows single access to the object as a time.  A second client
 * requesting concurrent access will be blocked.
 *
 * Like std::queue, the container must satisfy the requirements of
 * SequenceContainer.  Additionally, it must provide the following functions with the usual semantics:
 * - push_back()
 * - empty()
 * - front()
 * - pop_front()
 * - clear()
 * Unlike std::queue, T must be copyable.
 */

#include <ratio>

template < typename T, class Container=std::deque<T> >
class SynchronizationQueue
{
    static log4cxx::LoggerPtr cpptrace_log()
        {
            static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".synchronization_queue.hpp"));
            return result;
        }

    // Types
public:
    typedef Container container_type;
    typedef T         value_type;
    typedef T         &reference;
    typedef const T   &const_reference;
private:
    std::mutex                               mutex;
    std::condition_variable                  condition_variable;
    enum class State {Blocking, NonBlocking} state;
    int                                      waiting_count;
protected:
    Container                                c;
private:
    const std::chrono::duration<int,std::milli> delay;

public:
    explicit SynchronizationQueue(int p_delay=100)
        : state(State::Blocking)
        , waiting_count(0)
        , delay(p_delay)
        {
            LOG4CXX_INFO(cpptrace_log(), "SynchronizationQueue::SynchronizationQueue(" << p_delay << ")");
        }

public:

    // front is not thread-safe, so not provided
    // back is not thread-safe, so not provided
    // empty is not thread-safe, so not provided
    // size is not thread-safe, so not provided

    void nonblocking_push(const value_type &p_item)
        {
            LOG4CXX_INFO(cpptrace_log(), "SynchronizationQueue::nonblocking_push(" << p_item << ")");
            std::unique_lock<std::mutex> lock(mutex);
            c.push_back(p_item);
            lock.unlock();
            condition_variable.notify_one();
        }

    void nonblocking_push(const value_type&&p_item)
        {
            LOG4CXX_INFO(cpptrace_log(), "SynchronizationQueue::nonblocking_push(" << p_item << ")");
            std::unique_lock<std::mutex> lock(mutex);
            c.push_back(p_item);
            lock.unlock();
            condition_variable.notify_one();
        }

    bool nonblocking_pull(reference p_item)
        {
            LOG4CXX_INFO(cpptrace_log(), "SynchronizationQueue::nonblocking_pull(...)");
            bool result;
            std::unique_lock<std::mutex> lock(mutex);
            if ((result = !c.empty))
            {
                p_item = c.front();
                c.pop_front();
            }
            return result;
        }

    bool blocking_pull(reference p_item)
        {
            LOG4CXX_INFO(cpptrace_log(), "SynchronizationQueue::blocking_pull(...)");
            std::unique_lock<std::mutex> lock(mutex);
            if (state == State::Blocking && c.empty())
            {
                waiting_count += 1;
                do
                {
                    (void) condition_variable.wait_for(lock, delay);
                }
                while (state == State::Blocking && c.empty());
                waiting_count -= 1;
                if (state == State::NonBlocking && waiting_count == 0)
                    state = State::Blocking;
            }
            bool result;
            if ((result = !c.empty()))
            {
                p_item = c.front();
                c.pop_front();
            }
            LOG4CXX_DEBUG(cpptrace_log(), "SynchronizationQueue::blocking_pull(...) => " << result);
            return result;
        }

    template<class Rep, class Period>
    bool blocking_pull(reference p_item, std::chrono::duration<Rep, Period>&p_duration)
        {
            LOG4CXX_INFO(cpptrace_log(), "SynchronizationQueue::blocking_pull(..., duration)");
            std::unique_lock<std::mutex> lock(mutex);
            if (state == State::Blocking && c.empty())
            {
                waiting_count += 1;
                std::cv_status wait_rv;
                do
                {
                    wait_rv = condition_variable.wait_for(lock, p_duration);
                }
                while (state == State::Blocking && wait_rv == std::cv_status::no_timeout && c.empty());
                waiting_count -= 1;
                if (state == State::NonBlocking && waiting_count == 0)
                    state = State::Blocking;
            }
            bool result;
            if ((result = !c.empty()))
            {
                p_item = c.front();
                c.pop_front();
            }
            return result;
        }

    template<class Clock, class Duration>
    bool blocking_pull(reference p_item, std::chrono::time_point<Clock, Duration>& p_timeout_time)
        {
            LOG4CXX_INFO(cpptrace_log(), "SynchronizationQueue::blocking_pull(..., time_out)");
            std::unique_lock<std::mutex> lock(mutex);
            if (state == State::Blocking && c.empty())
            {
                waiting_count += 1;
                std::cv_status wait_rv;
                do
                {
                    wait_rv = condition_variable.wait_until(lock, p_timeout_time);
                }
                while (state == State::Blocking && wait_rv == std::cv_status::no_timeout && c.empty());
                waiting_count -= 1;
                if (state == State::NonBlocking && waiting_count == 0)
                    state = State::Blocking;
            }
            bool result;
            if ((result = !c.empty()))
            {
                p_item = c.front();
                c.pop_front();
            }
            return result;
        }

    void unblock()
        {
            LOG4CXX_INFO(cpptrace_log(), "SynchronizationQueue::unblock()");
            std::unique_lock<std::mutex> lock(mutex);
            state = State::NonBlocking;
            condition_variable.notify_all();
            LOG4CXX_DEBUG(cpptrace_log(), "SynchronizationQueue::unblock() done");
        }

    void unblock(T p_filler)
        {
            LOG4CXX_INFO(cpptrace_log(), "SynchronizationQueue::unblock(" << p_filler << ")");
            std::unique_lock<std::mutex> lock(mutex);
            state = State::NonBlocking;
            condition_variable.notify_all();
            LOG4CXX_DEBUG(cpptrace_log(), "SynchronizationQueue::unblock(" << p_filler << ") done");
        }

    void unblocking_clear()
        {
            LOG4CXX_INFO(cpptrace_log(), "SynchronizationQueue::unblocking_clear()");
            std::unique_lock<std::mutex> lock(mutex);
            c.clear();
            if (waiting_count > 0)
            {
            	state = State::NonBlocking;
            	condition_variable.notify_all();
            }
        }

    virtual ~SynchronizationQueue() = default;

};

#endif
