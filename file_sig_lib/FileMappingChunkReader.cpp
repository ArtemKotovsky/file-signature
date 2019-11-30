//
//  FileMappingChunkReader.cpp
//  file_signature
//
//  Created by artem k on 18.11.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#include "FileMappingChunkReader.hpp"
#include "Exceptions.hpp"

#include <algorithm>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace file_sig
{
    FileMappingChunkReader::FileMappingChunkReader(const std::string& fileName,
                                                   uint32_t chunkSize,
                                                   bool mapAllFile)
        : m_chunkSize(chunkSize)
    {
        m_file.reset(open(fileName.c_str(), O_RDONLY));
        THROW_ERRNO_IF(m_file.get() < 0, "Cannot open " << fileName);
        
        struct stat statbuf = {};
        if (fstat(m_file, &statbuf) < 0)
        {
            THROW_ERRNO("Cannot get size of " << fileName);
        }
        
        m_fileSize = statbuf.st_size;
        
        if (mapAllFile)
        {
            //
            // it allocates just virtual address space, not RAM pages
            //
            m_filePtr = static_cast<uint8_t*>(mmap(nullptr, m_fileSize, PROT_READ, MAP_PRIVATE, m_file, 0));
            THROW_ERRNO_IF(m_filePtr == MAP_FAILED, "mmap failed");
            madvise(m_filePtr, m_fileSize, MADV_SEQUENTIAL|MADV_WILLNEED);
            
            m_filePtrGuard.reset(this, [](FileMappingChunkReader* pthis)
            {
                if (pthis)
                {
                    munmap(pthis->m_filePtr, pthis->m_fileSize);
                }
            });
        }
    }

    bool FileMappingChunkReader::getChunk(const void *& data, uint32_t& size, uint64_t& offset)
    {
        {
            std::lock_guard<std::mutex> lock(m_fileLock);
            
            if (m_fileSize <= m_filePos)
            {
                return false;
            }
            
            size = static_cast<uint32_t>(std::min<uint64_t>(m_chunkSize, m_fileSize - m_filePos));
            offset = m_filePos;
            m_filePos += size;
            
            if (m_filePtr)
            {
                data = m_filePtr + offset;
                return true;
            }
        }
        
        data = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, m_file, offset);
        THROW_ERRNO_IF(data == MAP_FAILED, "mmap failed");
        
        return true;
    }

    void FileMappingChunkReader::freeChunk(const void * data, uint32_t size)
    {
        if (!m_filePtr)
        {
            int res = munmap(const_cast<void*>(data), size);
            THROW_ERRNO_IF(res == -1, "munmap failed (logic error)");
        }
    }
}
