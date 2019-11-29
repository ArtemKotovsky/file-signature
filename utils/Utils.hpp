//
//  Utils.hpp
//  file_signature
//
//  Created by artem k on 23.11.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#pragma once

#include "Exceptions.hpp"

#include <string>
#include <limits>

namespace utils
{
    template<typename T>
    T toUnsigned(const std::string& s)
    {
        size_t pos = 0;
        unsigned long long val = 0;
        
        try
        {
            val = std::stoull(s, &pos);
        }
        catch (const std::exception&)
        {
            THROW("Cannot convert '" << s << "' to unsigned");
        }
        
        THROW_IF(pos != s.size(), "Cannot convert whole string '" << s << "' to unsigned");
        THROW_IF(val > std::numeric_limits<T>::max(), "The digit is too big '" << s << "'");
        
        static_assert(std::numeric_limits<T>::min() == 0, "T must be unsigned");
        return static_cast<T>(val);
    }

    uint64_t getFileSize(const std::string& filepath);
    std::string getLocalTime();
}
