//
//  Hash.cpp
//  file_signature
//
//  Created by artem k on 30.10.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#include "Hash.hpp"

#define CRC32_USE_LOOKUP_TABLE_SLICING_BY_16
#include "Crc32.h"
#include "picosha2.h"

#include <sstream>
#include <iomanip>

namespace utils
{
    std::string crc32(const void* data, size_t size)
    {
        const uint32_t crc = crc32_fast(data, size, 0);
        
        std::stringstream st;
        st << std::setfill ('0') << std::setw(8) << std::hex << crc;
        
        return st.str();
    }

    std::string sha2(const void* data, size_t size)
    {
        auto begin = static_cast<const char*>(data);
        auto end = begin + size;
        return picosha2::hash256_hex_string(begin, end);
    }
}
