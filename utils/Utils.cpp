//
//  Utils.cpp
//  file_signature
//
//  Created by artem k on 23.11.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#include "Utils.hpp"
#include "Exceptions.hpp"

#include <time.h>
#include <fstream>

namespace utils
{
    uint64_t getFileSize(const std::string& filepath)
    {
        std::ifstream file;
        file.open(filepath, std::ios::binary | std::ios::ate);
        THROW_IF(!file, "Cannot open " << filepath);
        return file.tellg();
    }

    std::string getLocalTime()
    {
        std::time_t time = std::time(nullptr);
        
        if (time == static_cast<std::time_t>(-1))
        {
            return std::string();
        }

        std::tm localTime{};
        
        if (!localtime_r(&time, &localTime))
        {
            return std::string();
        }

        std::stringstream st;
        st << std::put_time(&localTime, "%Y-%m-%d %X");
        return st.str();
    }
}
