//
//  Hash.hpp
//  file_signature
//
//  Created by artem k on 30.10.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#pragma once

#include <string>

namespace utils
{
    std::string crc32(const void* data, size_t size);
    std::string sha2(const void* data, size_t size);
}
