/*
 * @Author: Xudong0722 
 * @Date: 2025-04-10 00:13:42 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-10 00:39:06
 */

#include <sys/stat.h>
#include <fcntl.h>
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

    }

    void FdManager::deleteFd(int fd) {

    }
} // namespace East
