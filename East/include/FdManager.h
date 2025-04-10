/*
 * @Author: Xudong0722 
 * @Date: 2025-04-10 00:13:39 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-10 00:31:35
 */

#include "Thread.h"
#include "singleton.h"
#include "Noncopyable.h"

namespace East
{
class FdContext: public std::enable_shared_from_this<FdContext> {
public:
    using sptr = std::shared_ptr<FdContext>;
    
    FdContext(int fd);
    ~FdContext();

    bool init();
    bool isInit() const { return m_init;}
    bool isSocket() const { return m_isSocket;}

    bool close();
    bool isClosed() const { return m_closed;}

    void setSysNonBlock(bool v) { m_sysNonBlock = v;}
    bool isSysNonBlock() const { return m_sysNonBlock;}

    void setUserNonBlock(bool v) { m_userNonBlock = v;}
    bool isUserNonBlock() const { return m_userNonBlock;}

    void setRecvTimeout(uint64_t v) { m_recvTimeout = v;}
    uint64_t getRecvTimeout() const { return m_recvTimeout;}

    void setSendTimeout(uint64_t v) { m_sendTimeout = v;}
    uint64_t getSendTimeout() const { return m_sendTimeout;}
private:

    bool m_init: 1;
    bool m_isSocket: 1;
    bool m_sysNonBlock: 1;
    bool m_userNonBlock: 1;
    bool m_closed: 1;
    int m_fd;
    uint64_t m_recvTimeout;
    uint64_t m_sendTimeout;
};

class FdManager: public noncopymoveable {
public:
    using RWMutexType = RWLock;
    FdManager();
    ~FdManager();
    
    FdContext::sptr getFd(int fd, bool create_when_notfound = false);
    void deleteFd(int fd);

private:
    RWMutexType m_mutex;
    std::vector<FdContext::sptr> m_fds;
};

using FdMgr = Singleton<FdManager>;
} // namespace East