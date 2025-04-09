/*
 * @Author: Xudong0722 
 * @Date: 2025-04-10 00:13:42 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-10 00:26:40
 */

#include "FdManager.h"

namespace East
{
    FdContext::FdContext(int fd) {

    }
    
    FdContext::~FdContext(){

    }

    void FdContext::init() {

    }
    FdManager::FdManager() {

    }
    
    FdManager::~FdManager() {}
    
    FdContext::sptr FdManager::getFd(int fd, bool create_when_notfound = false) {

    }

    void FdManager::deleteFd(int fd) {

    }
} // namespace East
