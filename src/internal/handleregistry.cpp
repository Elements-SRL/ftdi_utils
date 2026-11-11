#include "handleregistry.h"

#include <map>
#include <set>

namespace {

/*! Protects the two containers below */
std::mutex registryMutex;

std::map<FT_HANDLE, std::shared_ptr<std::mutex>> handleMutexes;
std::set<FT_HANDLE> openHandles;

} // namespace

void HandleRegistry::addOpenHandle(FT_HANDLE handle) {
    std::lock_guard<std::mutex> guard(registryMutex);
    openHandles.insert(handle);
}

void HandleRegistry::removeHandle(FT_HANDLE handle) {
    std::lock_guard<std::mutex> guard(registryMutex);
    openHandles.erase(handle);
    handleMutexes.erase(handle);
}

bool HandleRegistry::noOpenHandles() {
    std::lock_guard<std::mutex> guard(registryMutex);
    return openHandles.empty();
}

std::shared_ptr<std::mutex> HandleRegistry::mutexOf(FT_HANDLE handle) {
    std::lock_guard<std::mutex> guard(registryMutex);

    std::shared_ptr<std::mutex> &handleMutex = handleMutexes[handle];
    if (handleMutex == nullptr) {
        handleMutex = std::make_shared<std::mutex>();
    }
    return handleMutex;
}

HandleLock::HandleLock(FT_HANDLE handle) :
    mutex(HandleRegistry::mutexOf(handle)),
    lock(*mutex) {

}
