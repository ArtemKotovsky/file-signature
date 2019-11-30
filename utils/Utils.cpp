//
//  Utils.cpp
//  file_signature
//
//  Created by artem k on 23.11.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#include "Utils.hpp"
#include "Exceptions.hpp"

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
}
