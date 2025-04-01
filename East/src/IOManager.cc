/*
 * @Author: Xudong0722 
 * @Date: 2025-04-01 22:55:58 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-01 23:59:59
 */

#include <sys/epoll.h>
#include <unistd.h>
#include "Elog.h"
#include "Macro.h"
#include "IOManager.h"

namespace East
{
static East::Logger::sptr g_logger = ELOG_NAME("system");

IOManager::IOManager(size_t threads, bool use_caller,
    const std::string& name) {

}
IOManager::~IOManager() {

}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb = nullptr) {

}

bool IOManager::removeEvent(int fd, Event event) {

}
bool IOManager::cancelEvent(int fd, Event event) {

}
bool IOManager::cancalAll(int fd) {

}

IOManager* IOManager::GetThis(){

}

void tickle() {

}

void idle() {

}

bool stopping() {

}

};