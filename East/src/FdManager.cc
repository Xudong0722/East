/*
 * @Author: Xudong0722 
 * @Date: 2025-04-10 00:13:42 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-10 00:39:06
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <algorithm>
#include "FdManager.h"

namespace East
{
    FdContext::FdContext(int fd) 
        : m_init(false),
          m_isSocket(false),
          m_sysNonBlock(false),
          m_userNonBlock(false),
          m_closed(false),
          m_fd(fd),
          m_recvTimeout(-1),
          m_sendTimeout(-1) {
            init();
    }
    
    FdContext::~FdContext(){

    }

    bool FdContext::init() {
        if(m_init) {
            return true;
        }
        
        struct stat status;
        if(0 == fstat(m_fd, &status)) {    //man fstat, 获取文件信息
            m_isSocket = S_ISSOCK(status.st_mode); 
            m_init = true;
        }else{
            m_isSocket = false;
            m_init = false;
        }

        if(m_isSocket) {
            int flags = fcntl(m_fd, F_GETFL, 0);  //获取fd的标记位( Return (as the function result) the file access mode and the file status flags; arg is ignored.)
            if(!(flags & O_NONBLOCK)) { 
                fcntl(m_fd, F_SETFL, flags | O_NONBLOCK); //如果当前fd不是非阻塞，设置为非阻塞
            }
            m_sysNonBlock = true;
        }else{
            m_sysNonBlock = false;
        }

        m_userNonBlock = false;
        m_closed = false;
        return m_init;
        
    }

    FdManager::FdManager() {
        m_fds.resize(64);
    }
    
    FdManager::~FdManager() {}
    
    FdContext::sptr FdManager::getFd(int fd, bool create_when_notfound = false) {
        if(fd < 0) return nullptr;
        {
            RWMutexType::RLockGuard rlock(m_mutex);
            if(fd >= (int)m_fds.size()) {
                if(!create_when_notfound){
                    return nullptr;
                }
            }else{
                if(nullptr == m_fds.at(fd)) {
                    if(!create_when_notfound){
                        return nullptr;
                    }
                }else{
                    return m_fds.at(fd);
                }
            }
        }

        RWMutexType::WLockGuard wlock(m_mutex);
        auto new_fd = std::make_shared<FdContext>(fd);
        
        auto tmp = m_fds;
        tmp.resize(fd * 1.5);  //now, fd * 1.5 > m_fds.size()
        copy(m_fds.begin(), m_fds.end(), tmp.begin());
        m_fds.swap(tmp);
        m_fds[fd] = std::move(new_fd);
        return m_fds[fd];
    }

    void FdManager::deleteFd(int fd) {
        if(fd < 0) return ;
        // {
        //     RWMutexType::RLockGuard rlock(m_mutex);
        //     if(fd >= m_fds.size()) {
        //         return ;
        //     }
        // }

        RWMutexType::WLockGuard wlock(m_mutex);
        if(fd >= m_fds.size()) {
            return ;
        }
        m_fds[fd].reset();
    }
} // namespace East
