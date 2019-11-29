//
//  ChunkReader.hpp
//  file_signature
//
//  Created by artem k on 18.11.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#pragma once

#include <stdint.h>

//
// TODO: the class has to be covered by unit tests
//

namespace file_sig
{
    //
    // This abstract class is used to read chunks that have to be hashed.
    // It can be either reading from a file or a socket or a console or a pipe etc.
    // Reading can be cached or inplace or async etc.
    //
    // e.g.: FileStreamChunkReader and FileMappingChunkReader
    //
    class ChunkReader
    {
    public:
        
        class Chunk
        {
        public:
            Chunk();
            ~Chunk();
            
            Chunk(const Chunk&) = delete;
            Chunk& operator=(const Chunk&) = delete;
            
            Chunk(Chunk&& other);
            Chunk& operator=(Chunk&& other);

            void free();

            const void * data() const;
            uint32_t size() const;
            uint64_t offset() const;
            
        private:
            Chunk(ChunkReader& reader, const void * data, uint32_t size, uint64_t offset);
            friend class ChunkReader;
            
        private:
            const void * m_data;
            uint32_t m_size;
            uint64_t m_offset;
            ChunkReader * m_reader;
        };
        
    public:
        virtual ~ChunkReader() = default;
        
        //
        // reads next chunk
        // returns true if chunk has been read
        // otherwice false - EOF,EOS
        //
        bool getNextChunk(Chunk& chunk);
        
    private:
        //
        // retunrs true if the data has been successfully read
        // otherwise false - it is either End Of File or End Of Stream
        // can throws an exception in case of an error
        //
        virtual bool getChunk(const void *& data, uint32_t& size, uint64_t& offset) = 0;
        
        //
        // freeChunk method must always be called once for each getChunk
        // the function must not throw any exceptions because it is a
        // function that free resources
        //
        virtual void freeChunk(const void * data, uint32_t size) = 0;
    };
}
