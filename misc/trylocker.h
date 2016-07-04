#ifndef THREADING_UTILS_TRYLOCKER_H
#define THREADING_UTILS_TRYLOCKER_H

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QMutex)

namespace ThreadingUtils {

/*!
 * \brief Like QMutexLocker, but it just tries to lock the mutex.
 */
template<typename Mutex = QMutex>
class TryLocker
{
public:
    /*!
     * \brief Tries to lock the specified mutex.
     */
    TryLocker(Mutex &mutex) :
        m_mutex(mutex.tryLock() ? &mutex : nullptr)
    {}

    /*!
     * \brief Unlocks the mutex specified when constructing.
     * \remarks Does nothing if the mutex couldn't be locked in the first place.
     */
    ~TryLocker()
    {
        if(m_mutex) {
            m_mutex->unlock();
        }
    }

    /*!
     * \brief Returns whether the mutex could be locked.
     */
    bool isLocked() const
    {
        return m_mutex != nullptr;
    }

    /*!
     * \brief Returns whether the mutex could be locked.
     */
    operator bool() const
    {
        return m_mutex != nullptr;
    }

private:
    Mutex *m_mutex;
};

}

#endif // THREADING_UTILS_TRYLOCKER_H
