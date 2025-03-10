/*
 * @Author: Xudong0722
 * @Date: 2025-02-24 20:45:27
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-05 23:49:16
 */

#pragma once
#include "LogAppender.h"
#include "LogEvent.h"
#include "Logger.h"
#include "LogFormatter.h"
#include "singleton.h"

// Log Macro
#define ELOG_LEVEL(logger, level)                                                                                                         \
    if (nullptr != logger && logger->getLevel() <= level)                                                                                 \
    East::LogEventWrap(std::make_shared<East::LogEvent>(logger, level, __FILE__, __LINE__, 0, static_cast<uint32_t>(East::getThreadId()), \
                                                        static_cast<uint32_t>(East::getFiberId()), static_cast<uint64_t>(time(0))))       \
        .getSStream()

#define ELOG_DEBUG(logger) ELOG_LEVEL(logger, East::LogLevel::DEBUG)
#define ELOG_INFO(logger) ELOG_LEVEL(logger, East::LogLevel::INFO)
#define ELOG_WARN(logger) ELOG_LEVEL(logger, East::LogLevel::WARN)
#define ELOG_ERROR(logger) ELOG_LEVEL(logger, East::LogLevel::ERROR)
#define ELOG_FATAL(logger) ELOG_LEVEL(logger, East::LogLevel::FATAL)

#define ELOG_ROOT() East::LogMgr::GetInst()->getRoot()