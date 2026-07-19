#ifndef PROTOCOL_ATOMIC_LOCK_HPP
#define PROTOCOL_ATOMIC_LOCK_HPP

#include <atomic>

namespace protocol
{

class Lock
{
public:
    Lock() {}

    [[nodiscard]] 
    bool tryLock() { return !m_lock.exchange(true); }

    void unlock() { m_lock.exchange(false); }

    bool isLocked() const { return m_lock; }

    ~Lock() { unlock(); }

private:
    std::atomic_bool m_lock{false};
};

class LockGuard
{
public:
    explicit LockGuard(Lock &lock) : m_lock(lock) {}

    [[nodiscard]] 
    bool tryLock() { return m_lock.tryLock(); }

    void unlock() { m_lock.unlock(); }

    ~LockGuard() { unlock(); }

private:
    Lock &m_lock;
};

} // namespace protocol

#endif // PROTOCOL_ATOMIC_LOCK_HPP