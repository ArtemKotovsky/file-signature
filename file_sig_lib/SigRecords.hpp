//
//  SigResult.hpp
//  file_signature
//
//  Created by artem k on 22.11.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#pragma once

#include <set>
#include <string>
#include <functional>
#include <condition_variable>
#include <future>

//
// TODO: the class has to be covered by unit tests
//

namespace file_sig
{
    //
    // This class stores records with signatures (hashes)
    // It also sorts the records because they may come in random order
    // The class provides 2 ways to get ready record (in sorted order):
    //   * tryPopRecord - wait for a new ready record or timeout
    //   * setOnRecordCallback - a callback function will be called by this class automatically
    //      this method has higer priority than tryPopRecord,
    //      however, the callback must not use long operations or wait functions
    //      because it may decrease performance of the SigPipeline
    // The class is also responsible for marshaling an exception
    // if the exception happened while next record is prepared
    //
    template<typename SigHashType>
    class SigRecords
    {
    public:
        using Hasher = std::function<SigHashType(const void* data, size_t size)>;

        struct HashRecord
        {
            uint32_t size = 0;
            uint64_t offset = 0;
            SigHashType hash;
            
            bool operator < (const HashRecord& other) const
            {
                return offset < other.offset;
            }
        };

        using OnHashRecord = std::function<void(HashRecord record)>;
        
        enum class RecordResult
        {
            timeout,  // timeout, a record is not ready
            ready,    // record is ready
            finished, // records are empty, task is finished
            canceled  // task was canceled
        };
        
    public:
        bool pushRecord(HashRecord record);
        RecordResult tryPopRecord(uint32_t timeoutMs, HashRecord& record);
        RecordResult waitForResult(uint32_t timeoutMs) const;
        void checkException() const;
        
        void setOnRecordCallback(OnHashRecord cb);
        void setException(std::exception_ptr ex);
        void setCleanup();
        void setFreez();
        
    private:
        bool tryPopNextRecord(HashRecord& record);
        
    private:
        mutable std::mutex m_mutex;
        mutable std::condition_variable m_cv;
        std::set<HashRecord> m_records;
        std::exception_ptr m_exception;
        OnHashRecord m_onHashRecord;
        uint64_t m_offset = 0;
        bool m_cleaned = false;
        bool m_freezed = false;
    };

    template<typename SigHashType>
    bool SigRecords<SigHashType>::pushRecord(HashRecord record)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_cleaned)
        {
            //
            // the object is cleaned
            // need not to save a new record
            //
            return false;
        }
        
        if (m_freezed)
        {
            throw std::logic_error("The object was freezed, new records are not allowed anymore");
        }
        
        m_records.insert(std::move(record));
        
        if (m_onHashRecord)
        {
            HashRecord r;
            
            while (tryPopNextRecord(r))
            {
                m_onHashRecord(std::move(r));
            }
        }
        else
        {
            m_cv.notify_one();
        }
        
        return true;
    }

    template<typename SigHashType>
    typename SigRecords<SigHashType>::RecordResult SigRecords<SigHashType>::tryPopRecord(uint32_t timeoutMs,
                                                                                         HashRecord& record)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        auto timepoint = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        
        for (;;)
        {
            if (m_exception)
            {
                std::rethrow_exception(m_exception);
            }
            
            if (m_cleaned)
            {
                return RecordResult::canceled;
            }
            
            if (tryPopNextRecord(record))
            {
                return RecordResult::ready;
            }
            
            if (m_freezed)
            {
                //
                // there are no records in the container
                // and new records won't be added
                //
                return RecordResult::finished;
            }
            
            //
            // wait until, not wait_for
            // not to wait much more time in case of
            // a few spurious wakeups
            //
            auto cv = m_cv.wait_until(lock, timepoint);
            
            if (cv == std::cv_status::timeout)
            {
                return RecordResult::timeout;
            }
        }
    }

    template<typename SigHashType>
    bool SigRecords<SigHashType>::tryPopNextRecord(HashRecord& record)
    {
        if (m_records.empty())
        {
            return false;
        }
        
        auto it = m_records.begin();
        
        if (it->offset != m_offset)
        {
            return false;
        }
        
        record = std::move(*it);
        m_offset += it->size;
        m_records.erase(it);
        return true;
    }

    template<typename SigHashType>
    typename SigRecords<SigHashType>::RecordResult SigRecords<SigHashType>::waitForResult(uint32_t timeoutMs) const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        auto timepoint = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        
        for (;;)
        {
            if (m_exception)
            {
                std::rethrow_exception(m_exception);
            }
            
            if (m_cleaned)
            {
                return RecordResult::canceled;
            }
            
            if (m_freezed && m_records.empty())
            {
                //
                // there are no records in the container
                // and new records won't be added
                //
                return RecordResult::finished;
            }
            
            //
            // wait until, not wait_for
            // not to wait much more time in case of
            // a few spurious wakeups
            //
            auto cv = m_cv.wait_until(lock, timepoint);
            
            if (cv == std::cv_status::timeout)
            {
                return RecordResult::timeout;
            }
        }
    }

    template<typename SigHashType>
    void SigRecords<SigHashType>::checkException() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_exception)
        {
            std::rethrow_exception(m_exception);
        }
    }
    
    template<typename SigHashType>
    void SigRecords<SigHashType>::setOnRecordCallback(OnHashRecord cb)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_onHashRecord = std::move(cb);
    }

    template<typename SigHashType>
    void SigRecords<SigHashType>::setException(std::exception_ptr ex)
    {
        //
        // save the first exception
        //
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_exception)
        {
            //assert(!"Second exception has been thrown");
            return;
        }
        
        m_exception = std::move(ex);
        m_cv.notify_all();
    }

    template<typename SigHashType>
    void SigRecords<SigHashType>::setCleanup()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cleaned = true;
        m_records.clear();
        m_cv.notify_all();
    }

    template<typename SigHashType>
    void SigRecords<SigHashType>::setFreez()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_freezed = true;
        m_cv.notify_all();
    }
}
