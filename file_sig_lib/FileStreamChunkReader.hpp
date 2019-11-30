//
//  FileStreamChunkReader.hpp
//  file_signature
//
//  Created by artem k on 18.11.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#pragma once

#include "ChunkReader.hpp"

#include <fstream>
#include <future>
#include <vector>
#include <list>

namespace file_sig
{
    //
    // It is an implementation of a file reader
    // that reads the file in a separate thread
    // and uses caches for proactive reading
    //
    class FileStreamChunkReader : public ChunkReader
    {
    private:
        struct Chunk
        {
            std::vector<char> buffer;
            uint64_t offset = 0;
        };

    public:
        FileStreamChunkReader(const std::string& fileName,
                              uint32_t cachedChunksCount,
                              uint32_t chunkSize);
        
        ~FileStreamChunkReader();
        
        FileStreamChunkReader(const FileStreamChunkReader&) = delete;
        FileStreamChunkReader& operator=(const FileStreamChunkReader&) = delete;
        
        FileStreamChunkReader(FileStreamChunkReader&&) = delete;
        FileStreamChunkReader& operator=(FileStreamChunkReader&&) = delete;
        
        void stop(bool sync);
        
    private:        
        bool getChunk(const void *& data, uint32_t& size, uint64_t& offset) override;
        void freeChunk(const void * data, uint32_t size) override;
        void readerThread();
        
    private:
        std::mutex m_chunkMutex;
        std::condition_variable m_readyCv;
        std::condition_variable m_freeCv;
        std::exception_ptr m_exception;
        std::list<Chunk> m_ready; // chunks with ready to use data
        std::list<Chunk> m_busy;  // chunks that are used now (getChunk has been called, but not the freeChunk
        std::list<Chunk> m_free;  // chunks that have been freed by calling freeChunk, they may be reused by the m_thread
        bool m_stopped = false;
        bool m_eof = false;
        
        std::mutex m_threadMutex;
        std::future<void> m_thread;
        std::ifstream m_file;
        std::vector<char> m_fileIo;
    };
}
