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
        FileMappingChunkReader& operator=(FileMappingChunkReader&&) = delete;
        
    private:
        bool getChunk(const void *& data, uint32_t& size, uint64_t& offset) override;
        void freeChunk(const void * data, uint32_t size) override;
        
    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
