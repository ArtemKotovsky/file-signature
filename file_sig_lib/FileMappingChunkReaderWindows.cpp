#include "FileMappingChunkReader.hpp"
#include "Exceptions.hpp"

#include <algorithm>
#include <mutex>
#include <Windows.h>

namespace file_sig
{
    struct FileMappingChunkReader::Impl
    {
        std::mutex fileLock;

        utils::ScopedHandle<HANDLE, decltype(::CloseHandle), ::CloseHandle, INVALID_HANDLE_VALUE> file;
        utils::ScopedHandle<HANDLE, decltype(::CloseHandle), ::CloseHandle, NULL> section;
        utils::ScopedHandle<LPVOID, decltype(::UnmapViewOfFile), ::UnmapViewOfFile, NULL> view;

        LARGE_INTEGER fileSize{ 0 };
        LARGE_INTEGER filePos{ 0 };
        uint32_t chunkSize = 0;
    };

    FileMappingChunkReader::FileMappingChunkReader(const std::string& fileName,
                                                   uint32_t chunkSize,
                                                   bool mapAllFile)
        : m_impl(std::make_unique<FileMappingChunkReader::Impl>())
    {
        m_impl->chunkSize = chunkSize;

        m_impl->file = CreateFileA(fileName.c_str()
            , GENERIC_READ
            , 0
            , NULL
            , OPEN_EXISTING
            , FILE_ATTRIBUTE_NORMAL
            , NULL);
        THROW_WIN_IF(!m_impl->file, "Cannot open " << fileName);

        LARGE_INTEGER size = {};
        THROW_WIN_IF(!GetFileSizeEx(m_impl->file, &m_impl->fileSize), "GetFileSizeEx error");

        m_impl->section = CreateFileMapping(m_impl->file
            , NULL
            , PAGE_READONLY
            , size.HighPart
            , size.LowPart
            , NULL);

        THROW_WIN_IF(!m_impl->section, "CreateFileMapping error");

        if (mapAllFile)
        {
            //
            // it allocates just virtual address space, not RAM pages
            //
            m_impl->view = MapViewOfFileEx(m_impl->section
                , FILE_MAP_READ
                , 0
                , 0
                , m_impl->fileSize.QuadPart
                , NULL);

            THROW_WIN_IF(!m_impl->view, "MapViewOfFileEx failed");
        }
    }

    bool FileMappingChunkReader::getChunk(const void *& data, uint32_t& size, uint64_t& offset)
    {
        {
            std::lock_guard<std::mutex> lock(m_impl->fileLock);
            
            if (m_impl->fileSize.QuadPart <= m_impl->filePos.QuadPart)
            {
                return false;
            }
            
            const uint64_t fileLeftSize = m_impl->fileSize.QuadPart - m_impl->filePos.QuadPart;
            size = static_cast<uint32_t>(std::min<uint64_t>(m_impl->chunkSize, fileLeftSize));

            THROW_IF(m_impl->filePos.QuadPart < 0, "Logic error");
            offset = m_impl->filePos.QuadPart;
            m_impl->filePos.QuadPart += size;
            
            if (m_impl->view)
            {
                data = static_cast<uint8_t*>(m_impl->view.get()) + offset;
                return true;
            }
        }

        LARGE_INTEGER offset2{};
        offset2.QuadPart = offset;
        
        data = MapViewOfFileEx(m_impl->section
            , FILE_MAP_READ
            , offset2.HighPart
            , offset2.LowPart
            , size
            , NULL);

        THROW_WIN_IF(!data, "MapViewOfFileEx failed");
        return true;
    }

    void FileMappingChunkReader::freeChunk(const void * data, uint32_t /*size*/)
    {
        if (!m_impl->view)
        {
            BOOL res = UnmapViewOfFile(const_cast<LPVOID>(data));
            THROW_WIN_IF(!res, "UnmapViewOfFile failed (logic error)");
        }
    }
}
