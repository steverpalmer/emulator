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
 * A BlockingQueue provides a similar interface to the std::queue
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

template < typename T, class Container=std::deque<T> >
class SynchronizationQueue
{
    // Types
public:
    typedef Container container_type;
    typedef T         value_type;
    typedef T         &reference;
    typedef const T   &const_reference;
private:
    std::mutex              m_mutex;
    std::condition_variable m_queue_not_empty;
protected:
    Container               c;
public:
    // front is not thread-safe, so not provided
    // back is not thread-safe, so not provided
    // empty is not thread-safe, so not provided
    // size is not thread-safe, so not provided

    void nonblocking_push(const value_type &p_item)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            c.push_back(p_item);
            lock.unlock();
            m_queue_not_empty.notify_one();
        }

    void nonblocking_push(const value_type&&p_item)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            c.push_back(p_item);
            lock.unlock();
            m_queue_not_empty.notify_one();
        }

    bool nonblocking_pull(reference p_item)
        {
            bool result;
            std::unique_lock<std::mutex> lock(m_mutex);
            if (result = !c.empty)
            {
                p_item = c.front();
                c.pop_front();
            }
            return result;
        }

    value_type blocking_pull()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_queue_not_empty.wait(lock, [this]{ return !this->c.empty(); });
            value_type result = c.front();
            c.pop_front();
            return result;
        }

    void blocking_pull(reference p_item)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_queue_not_empty.wait_for(lock, [this]{ return !this->c.empty(); });
            p_item = c.front();
            c.pop_front();
        }

    template<class Rep, class Period>
    bool blocking_pull(reference p_item, std::chrono::duration<Rep, Period>&p_duration)
        {
            bool result;
            std::unique_lock<std::mutex> lock(m_mutex);
            if (result = m_queue_not_empty.wait_for(lock, p_duration, [this]{ return !this->c.empty(); }))
            {
                p_item = c.front();
                c.pop_front();
            }
            return result;
        }

    template<class Clock, class Duration>
    bool blocking_pull(reference p_item, std::chrono::time_point<Clock, Duration>& p_timeout_time)
        {
            bool result;
            std::unique_lock<std::mutex> lock(m_mutex);
            if (result = m_queue_not_empty.wait_until(lock, p_timeout_time, [this]{ return !this->c.empty(); }))
            {
                p_item = c.front();
                c.pop_front();
            }
            return result;
        }

    void nonblocking_clear()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            c.clear();
        }
};

#endif
