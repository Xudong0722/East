/*
 * @Author: Xudong0722 
 * @Date: 2025-04-08 23:00:11 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-09 00:08:41
 */

#pragma once
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

namespace East {
bool is_hook_enable();
void set_hook_enable(bool enable);
}  // namespace East

extern "C" {
//declare the function pointer types

//sleep
typedef unsigned int (*sleep_func)(unsigned int seconds);
extern sleep_func sleep_f;

typedef int (*usleep_func)(useconds_t usec);
extern usleep_func usleep_f;

typedef int (*nanosleep_func)(const struct timespec* req, struct timespec* rem);
extern nanosleep_func nanosleep_f;

//socket
typedef int (*socket_func)(int domain, int type, int protocol);
extern socket_func socket_f;

typedef int (*connect_func)(int sockfd, const struct sockaddr* addr,
                            socklen_t addrlen);
extern connect_func connect_f;

typedef int (*accept_func)(int sockfd, struct sockaddr* addr,
                           socklen_t* addrlen);
extern accept_func accept_f;

//read
typedef ssize_t (*read_func)(int fd, void* buf, size_t count);
extern read_func read_f;

typedef ssize_t (*readv_func)(int fd, const struct iovec* iov, int iovcnt);
extern readv_func readv_f;

typedef ssize_t (*recv_func)(int sockfd, void* buf, size_t len, int flags);
extern recv_func recv_f;

typedef ssize_t (*recvfrom_func)(int sockfd, void* buf, size_t len, int flags,
                                 struct sockaddr* src_addr, socklen_t* addrlen);
extern recvfrom_func recvfrom_f;

typedef ssize_t (*recvmsg_func)(int sockfd, struct msghdr* msg, int flags);
extern recvmsg_func recvmsg_f;

//write
typedef ssize_t (*write_func)(int fd, const void* buf, size_t count);
extern write_func write_f;

typedef ssize_t (*writev_func)(int fd, const struct iovec* iov, int iovcnt);
extern writev_func writev_f;

typedef ssize_t (*send_func)(int sockfd, const void* buf, size_t len,
                             int flags);
extern send_func send_f;

typedef ssize_t (*sendto_func)(int sockfd, const void* buf, size_t len,
                               int flags, const struct sockaddr* dest_addr,
                               socklen_t addrlen);
extern sendto_func sendto_f;

typedef ssize_t (*sendmsg_func)(int sockfd, const struct msghdr* msg,
                                int flags);
extern sendmsg_func sendmsg_f;

typedef int (*close_func)(int fd);
extern close_func close_f;

//socket operations
typedef int (*fcntl_func)(int fd, int cmd, ... /* arg */);
extern fcntl_func fcntl_f;

typedef int (*ioctl_func)(int fd, unsigned long request, ...);
extern ioctl_func ioctl_f;

typedef int (*getsockopt_func)(int sockfd, int level, int optname, void* optval,
                               socklen_t* optlen);
extern getsockopt_func getsockopt_f;

typedef int (*setsockopt_func)(int sockfd, int level, int optname,
                               const void* optval, socklen_t optlen);
extern setsockopt_func setsockopt_f;
}
