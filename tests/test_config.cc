/*
 * @Author: Xudong0722
 * @Date: 2025-03-05 23:34:28
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-06 00:42:35
 */

#include "../East/include/Config.h"
#include "../East/include/Elog.h"

int main()
{
    East::ConfigVar<int>::sptr g_int_value_config =
        East::Config::Lookup("systemp.port", (int)8080, "system port");

    ELOG_INFO(ELOG_ROOT()) << g_int_value_config->getName() << " " << g_int_value_config->getDescription();
    ELOG_INFO(ELOG_ROOT()) << g_int_value_config->toString() << " " << g_int_value_config->getValue();

    return 0;
}