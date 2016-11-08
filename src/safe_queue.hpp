// safe_queue.hpp
// Copyright 2016 Steve Palmer

#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>

/** A thread-safe, blocking Queue-like object
 *
 * The Blocking Queue provides a similar interface to the std::queue
 * adaptor, but only allows for thread-safe interface functions.
 * It can support multiple producers and multiple consumers.
 */

template <typename T>
class BlockingQueue
{
    // Types
public:
    typedef std::deque<T> container_type;
    typedef T             value_type;
private:
    std::mutex              m_mutex;
    std::deque<T>           m_queue;
    std::condition_variable m_queue_not_empty;
public:
    // front is not thread-safe, so not provided
    // back is not thread-safe, so not provided
    // empty is not thread-safe, so not provided
    // size is not thread-safe, so not provided
    
    void push(const T& p_item)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_queue.push_back(p_item);
            lock.unlock();
            m_queue_not_empty.notify_one();
        }
    
    void push(T&& p_item)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_queue.push_back(p_item);
            lock.unlock();
            m_queue_not_empty.notify_one();
        }

    // pop is modified to be atomic, potentially blocking
    T pop()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (!m_queue.empty())
                m_queue_not_empty.wait(lock);
            auto result = m_queue.front();
            m_queue.pop_front();
            return result;
        }

    void pop(T& p_item)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (!m_queue.empty())
                m_queue_not_empty.wait(lock);
            p_item = m_queue.front();
            m_queue.pop_front();
        }

    template<class Rep, class Period>
    bool pop(T& p_item, std::chrono::duration<Rep, Period>& p_dur)
        {
            enum { Waiting, Timeout, NotEmpty } state (Waiting);
            std::unique_lock<std::mutex> lock(m_mutex);
            do
            {
                if (!m_queue.empty())
                    state = NotEmpty;
                else if (m_queue_not_empty.wait_for(lock, p_dur) == std::cv_status::timeout)
                    state = Timeout;
            }
            while (state == Waiting);
            if (state == NotEmpty)
            {
                p_item = m_queue.front();
                m_queue.pop_front();
            }
            return state == NotEmpty;
        }

    template<class Clock, class Duration>
    bool pop(T& p_item, std::chrono::time_point<Clock, Duration>& p_timeout_time)
        {
            enum { Waiting, Timeout, NotEmpty } state (Waiting);
            std::unique_lock<std::mutex> lock(m_mutex);
            do
            {
                if (!m_queue.empty())
                    state = NotEmpty;
                else if (m_queue_not_empty.wait_until(lock, p_timeout_time) == std::cv_status::timeout)
                    state = Timeout;
            }
            while (state == Waiting);
            if (state == NotEmpty)
            {
                p_item = m_queue.front();
                m_queue.pop_front();
            }
            return state == NotEmpty;
        }
};

#endif
