//
//  FileMappingChunkReader.cpp
//  file_signature
//
//  Created by artem k on 18.11.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#include "FileMappingChunkReader.hpp"
#include "Exceptions.hpp"

#include <mutex>
#include <algorithm>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace file_sig
{
    struct FileMappingChunkReader::Impl
    {
        std::mutex fileLock;
        std::shared_ptr<FileMappingChunkReader> filePtrGuard;
        utils::ScopedHandle<int, decltype(::close), ::close, -1> file;
        uint64_t fileSize = 0;
        uint64_t filePos = 0;
        uint32_t chunkSize = 0;
        uint8_t * filePtr = nullptr;
    };

    FileMappingChunkReader::FileMappingChunkReader(const std::string& fileName,
                                                   uint32_t chunkSize,
                                                   bool mapAllFile)
        : m_impl(std::make_unique<FileMappingChunkReader::Impl>())
    {
        m_impl->chunkSize = chunkSize;
        m_impl->file.reset(open(fileName.c_str(), O_RDONLY));
        THROW_ERRNO_IF(m_impl->file.get() < 0, "Cannot open " << fileName);
        
        struct stat statbuf = {};
        if (fstat(m_impl->file, &statbuf) < 0)
        {
            THROW_ERRNO("Cannot get size of " << fileName);
        }
        
        m_impl->fileSize = statbuf.st_size;
        
        if (mapAllFile)
        {
            //
            // it allocates just virtual address space, not RAM pages
            //
            m_impl->filePtr = static_cast<uint8_t*>(mmap(nullptr, m_impl->fileSize, PROT_READ, MAP_PRIVATE, m_impl->file, 0));
            THROW_ERRNO_IF(m_impl->filePtr == MAP_FAILED, "mmap failed");
            madvise(m_impl->filePtr, m_impl->fileSize, MADV_SEQUENTIAL|MADV_WILLNEED);
            
            m_impl->filePtrGuard.reset(this, [this](FileMappingChunkReader* pthis)
            {
                if (pthis && pthis->m_impl->filePtr)
                {
                    munmap(pthis->m_impl->filePtr, pthis->m_impl->fileSize);
                }
            });
        }
    }

    bool FileMappingChunkReader::getChunk(const void *& data, uint32_t& size, uint64_t& offset)
    {
        {
            std::lock_guard<std::mutex> lock(m_impl->fileLock);
            
            if (m_impl->fileSize <= m_impl->filePos)
            {
                return false;
            }
            
            size = static_cast<uint32_t>(std::min<uint64_t>(m_impl->chunkSize, m_impl->fileSize - m_impl->filePos));
            offset = m_impl->filePos;
            m_impl->filePos += size;
            
            if (m_impl->filePtr)
            {
                data = m_impl->filePtr + offset;
                return true;
            }
        }
        
        data = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, m_impl->file, offset);
        THROW_ERRNO_IF(data == MAP_FAILED, "mmap failed");
        
        return true;
    }

    void FileMappingChunkReader::freeChunk(const void * data, uint32_t /*size*/)
    {
        if (!m_impl->filePtr)
        {
            int res = munmap(const_cast<void*>(data), size);
            THROW_ERRNO_IF(res == -1, "munmap failed (logic error)");
        }
    }
}
