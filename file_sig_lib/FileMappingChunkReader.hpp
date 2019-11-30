//
//  FileMappingChunkReader.hpp
//  file_signature
//
//  Created by artem k on 18.11.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#pragma once

#include "ChunkReader.hpp"
#include "ScopedHandle.hpp"

#include <string>
#include <mutex>
#include <unistd.h>

namespace file_sig
{
    //
    // It is an implementation of a file reader
    // that maps file region in the caller thread context
    //
    class FileMappingChunkReader : public ChunkReader
    {
    public:
        FileMappingChunkReader(const std::string& fileName,
                               uint32_t chunkSize,
                               bool mapAllFile);
        
        FileMappingChunkReader(const FileMappingChunkReader&) = delete;
        FileMappingChunkReader& operator=(const FileMappingChunkReader&) = delete;
        
        FileMappingChunkReader(FileMappingChunkReader&&) = delete;
        FileMappingChunkReader& operator=(FileMappingChunkReader&) = delete;
        
    private:
        bool getChunk(const void *& data, uint32_t& size, uint64_t& offset) override;
        void freeChunk(const void * data, uint32_t size) override;
        
    private:
        std::mutex m_fileLock;
        std::shared_ptr<FileMappingChunkReader> m_filePtrGuard;
        utils::ScopedHandle<int, decltype(::close), ::close, -1> m_file;
        uint64_t m_fileSize = 0;
        uint64_t m_filePos = 0;
        uint32_t m_chunkSize = 0;
        uint8_t * m_filePtr = nullptr;
    };
}
