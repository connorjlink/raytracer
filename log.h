#ifndef LUMA_LOG_H
#define LUMA_LOH_H

#include <string>
#include <print>

namespace luma
{
    void log(const std::string& msg)
    {
        std::println("[INFORMATION]: {}", msg);
    }
}

#endif