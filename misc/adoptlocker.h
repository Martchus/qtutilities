#ifndef THEADING_UTILS_ADOPTLOCKER_H
#define THEADING_UTILS_ADOPTLOCKER_H

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QMutex)

namespace ThreadingUtils {

/*!
 * \brief Like QMutexLocker, but assumes that the mutex has already been locked.
 */
template <typename Mutex = QMutex> class AdoptLocker {
public:
    /*!
   * \brief Constructs the locker for the specified \a mutex.
   */
    AdoptLocker(Mutex &mutex)
        : m_mutex(mutex)
    {
    }

    /*!
   * \brief Unlocks the mutex specified when constructing the instance.
   */
    ~AdoptLocker()
    {
        m_mutex.unlock();
    }

private:
    Mutex &m_mutex;
};
}

#endif // THEADING_UTILS_ADOPTLOCKER_H
