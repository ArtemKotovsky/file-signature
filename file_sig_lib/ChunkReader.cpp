//
//  ChunkReader.cpp
//  file_signature
//
//  Created by artem k on 18.11.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#include "ChunkReader.hpp"
#include <assert.h>

namespace file_sig
{
    ChunkReader::Chunk::Chunk()
        : m_reader(nullptr)
        , m_data(0)
        , m_size(0)
        , m_offset(0)
    {
    }

    ChunkReader::Chunk::Chunk(ChunkReader& reader,
                              const void * data,
                              uint32_t size,
                              uint64_t offset)
        : m_reader(&reader)
        , m_data(data)
        , m_size(size)
        , m_offset(offset)
    {
    }

    ChunkReader::Chunk::~Chunk()
    {
        free();
    }

    ChunkReader::Chunk::Chunk(Chunk&& other)
        : m_reader(other.m_reader)
        , m_data(other.m_data)
        , m_size(other.m_size)
        , m_offset(other.m_offset)
    {
        other.m_reader = nullptr;
    }

    ChunkReader::Chunk& ChunkReader::Chunk::operator=(Chunk&& other)
    {
        free();
        
        m_reader = other.m_reader;
        m_data = other.m_data;
        m_size = other.m_size;
        m_offset = other.m_offset;
        
        other.m_reader = nullptr;
        return *this;
    } 

    void ChunkReader::Chunk::free()
    {
        if (m_reader)
        {
            m_reader->freeChunk(m_data, m_size);
            m_reader = nullptr;
        }
    }

    const void * ChunkReader::Chunk::data() const
    {
        //
        // working with moved or empty object
        // is a logic error
        //
        assert(m_reader);
        return m_data;
    }

    uint32_t ChunkReader::Chunk::size() const
    {
        //
        // working with moved or empty object
        // is a logic error
        //
        assert(m_reader);
        return m_size;
    }

    uint64_t ChunkReader::Chunk::offset() const
    {
        //
        // working with moved or empty object
        // is a logic error
        //
        assert(m_reader);
        return m_offset;
    }

    bool ChunkReader::getNextChunk(Chunk& chunk)
    {
        //
        // it is a template method
        // that garantees the freeChunk function will be called anyway.
        // The ChunkReader provides only this method as a public interface
        //
        
        const void * data = nullptr;
        uint32_t size = 0;
        uint64_t offset = 0;
        
        if (!getChunk(data, size, offset))
        {
            //
            // EOF, EOS
            //
            return false;
        }
        
        chunk = Chunk(*this, data, size, offset);
        return true;
    }
}
