#ifndef HANDLEREGISTRY_H
#define HANDLEREGISTRY_H

#include <memory>
#include <mutex>

#include "ftd2xxwrapper.h" /*!< FT_HANDLE */

/*! \brief Keeps track of the FTDI handles in use.
 *
 *  - One mutex per handle: operations on the same device are serialized, while different
 *    devices can be used in parallel by different threads.
 *  - The set of open handles: the libraries cannot be unloaded while a handle is open,
 *    otherwise a thread could call a function of a library that no longer exists.
 *
 *  \note Internal to ftdi_utils.
 */
class HandleRegistry {
public:
    /*! \brief Records a handle returned by a successful open. */
    static void addOpenHandle(FT_HANDLE handle);

    /*! \brief Forgets a handle after a successful close (both its open state and its mutex). */
    static void removeHandle(FT_HANDLE handle);

    /*! \brief true if no handle is open. */
    static bool noOpenHandles();

    /*! \brief Mutex of a handle, created at the first request.
     *  Returned as shared_ptr so that it stays alive while it is locked, even if
     *  removeHandle() is called meanwhile by another thread. */
    static std::shared_ptr<std::mutex> mutexOf(FT_HANDLE handle);
};

/*! \brief Locks the mutex of a handle for the lifetime of the object. (RAII)
 *
 *  Usage:
 *  
 *  HandleLock lock(handle);   // locked here
 *  return ftd2xx.Read(...);   // unlocked automatically when the function returns
 *  
 */
class HandleLock {
public:
    explicit HandleLock(FT_HANDLE handle);

    HandleLock(const HandleLock &) = delete;
    HandleLock &operator=(const HandleLock &) = delete;

private:
    /*! The order of the members matters: C++ constructs them in this order and destroys them
     *  in the opposite one. So the mutex exists before it is locked and is still alive when
     *  it is unlocked. */
    std::shared_ptr<std::mutex> mutex;
    std::lock_guard<std::mutex> lock;
};

#endif // HANDLEREGISTRY_H
