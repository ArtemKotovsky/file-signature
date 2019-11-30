//
//  main.cpp
//  file_signature
//
//  Created by artem k on 30.10.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#include "Hash.hpp"
#include "Utils.hpp"
#include "Conio.hpp"
#include "SigPipeline.hpp"
#include "SigRecords.hpp"
#include "ChunkReader.hpp"
#include "FileStreamChunkReader.hpp"
#include "FileMappingChunkReader.hpp"

#include <iostream>
#include <string>

namespace
{
    struct Arguments
    {
        std::string inFilePath;
        std::string outFilePath;
        std::string hasher = "crc32";
        std::string reader = "stream";
        uint32_t chunkSize = 0;
        bool verbose = false;
        
        const uint32_t kDefaultChunkSize = 1024 * 1024;
        const uint32_t kHugeChunkSize = 100 * 1024 * 1024;
        
        void help()
        {
            std::cout << "Usage: \n";
            std::cout << "  --file=<file path>            - path to file than the signature will be calculated for\n";
            std::cout << "  --chunk-size=<bytes>          - optional, default: 1MB\n";
            std::cout << "                                  buffer size that hash will be calculated for\n";
            std::cout << "                                  The tool is NOT adopted for huge chunk size\n";
            std::cout << "  --out=<out file>              - optional, default: <file path>.signature\n";
            std::cout << "                                  out file with calculated signature\n";
            std::cout << "                                  If the file exists it will be rewritten\n";
            std::cout << "  --hash=<sha2|crc32>           - optional, default: srs32\n";
            std::cout << "                                  hash type for one chunk\n";
            std::cout << "  --reader=<map|mapall|stream>  - optional, default: stream\n";
            std::cout << "                                  opening method for <file path>\n";
            std::cout << "  --verbose                     - optional, detailed output\n";
        }
        
        bool parseArg(const std::string& argv, const std::string& prefix, std::string& value) const
        {
            if (prefix == argv.substr(0, prefix.size()))
            {
                value = argv.substr(prefix.size());
                return true;
            }
            
            return false;
        }

        bool parse(int argc, const char * argv[])
        {
            if (argc <= 1 || argc > 7)
            {
                help();
                return false;
            }
            
            std::string cmd;
            std::string val;
            
            for (int i = 1; i < argc; ++i)
            {
                cmd.assign(argv[i]);
                
                if (parseArg(cmd, "--file=", inFilePath)
                    || parseArg(cmd, "--out=", outFilePath)
                    || parseArg(cmd, "--hash=", hasher)
                    || parseArg(cmd, "--reader=", reader))
                {
                    continue;
                }
                else if (cmd == "--verbose")
                {
                    verbose = true;
                }
                else if (parseArg(cmd, "--chunk-size=", val))
                {
                    chunkSize = utils::toUnsigned<uint32_t>(val);
                }
                else
                {
                    std::cerr << "Unknown argument: '" << cmd << "'\n";
                    help();
                    return false;
                }
            }
            
            if (inFilePath.empty())
            {
                //
                // it is a compulsory parameter
                //
                std::cerr << "--file is not set\n";
                return false;
            }
            
            if (outFilePath.empty())
            {
                outFilePath = inFilePath + ".signature";
            }
            
            if (0 == chunkSize)
            {
                chunkSize = kDefaultChunkSize;
            }
            
            if (chunkSize > kHugeChunkSize && reader != "map" && reader != "reader")
            {
                std::cout << "\nWARNING! The chunk size is huge! Using --reader=map is recommending.\n\n";
            }
            
            if (chunkSize % 4096 != 0 && (hasher == "crc32" || reader == "map" || reader == "mapall"))
            {
                std::cout << "\nWARNING! The chunk size is not aligned to page size.";
                std::cout << "\n         Performance might be lower!";
                std::cout << "\n         'map' file reader does not support non aligned chunk size!\n\n";
            }
            
            return true;
        }
        
        uint32_t getWorkerThreads() const
        {
            if (reader == "stream")
            {
                //
                // For buffered FileStream reader it is enough
                // to have the same threads count as we have cores:
                //  * N-cores threads that calculate hashes all the time -
                //    they almost don't have blocking operations
                //  * one another thread read a file data for those
                //    N-threads
                //
                return std::thread::hardware_concurrency();
            }
            else if (reader == "map" || reader == "mapall")
            {
                //
                // For the current implementation,
                // mapping is a part of the same thread that calculate a hash.
                // Working with a mapped memory has a blocked IO operations:
                // page fault at the first access to a mapped memory that
                // has not been read from a file yet.
                // It makes sense to use more threads than cores.
                //
                return 3 * std::thread::hardware_concurrency();
            }
            
            THROW("Unknown reader type: " << reader);
        }
        
        file_sig::SigPipeline::Hasher createHasher() const
        {
            if (hasher == "crc32")
            {
                return utils::crc32;
            }
            else if (hasher == "sha2")
            {
                return utils::sha2;
            }
            
            THROW("Unknown hasher type: " << hasher);
        }
        
        std::unique_ptr<file_sig::ChunkReader> createReader() const
        {
            std::unique_ptr<file_sig::ChunkReader> obj;
            
            if (reader == "stream")
            {
                //
                // each worker thread has one cached value and one current value
                //
                obj.reset(new file_sig::FileStreamChunkReader(inFilePath, getWorkerThreads() * 2, chunkSize));
            }
            else if (reader == "map")
            {
                obj.reset(new file_sig::FileMappingChunkReader(inFilePath, chunkSize, false));
            }
            else if (reader == "mapall")
            {
                obj.reset(new file_sig::FileMappingChunkReader(inFilePath, chunkSize, true));
            }
            
            THROW_IF(!obj, "Unknown reader type: " << reader);
            return obj;
        }
    };

    std::ostream& operator << (std::ostream& out, const file_sig::SigPipeline::Record& record)
    {
        out << "0x" << std::hex << record.offset;
        out << ":0x" << std::hex << record.size;
        out << ":" << record.hash;
        return out;
    }

    std::string timeToStr(uint64_t seconds)
    {
        std::stringstream st;

        if (seconds > 60)
        {
            st << (seconds / 60) << " minute(s) ";
            seconds = seconds % 60;
        }
        
        st << seconds << " second(s)";
        return st.str();
    }
}

int main(int argc, const char * argv[])
{
    try
    {
        Arguments args;
        
        if (!args.parse(argc, argv))
        {
            return 1;
        }
        
        const size_t filesize = utils::getFileSize(args.inFilePath);
        
        std::cout << "Start time: " << utils::getLocalTime() << "\n";
        std::cout << "Filename: " << args.inFilePath << "\n";
        std::cout << "Filesize: " << filesize << "\n";
        std::cout << "Hash: " << args.hasher << "\n";
        std::cout << "Initialization...\n";
        
        auto hasher = args.createHasher();
        auto reader = args.createReader();
        file_sig::SigPipeline pipeline(*reader, hasher, args.getWorkerThreads());
        
        std::ofstream out;
        out.exceptions(std::ios::badbit | std::ios::failbit);
        out.open(args.outFilePath, std::ios::out | std::ios::trunc);
        
        out << "Filename: " << args.inFilePath << "\r\n";
        out << "Filesize: " << std::dec << filesize << "\r\n";
        out << "Hash: " << args.hasher << "\r\n";
        
        std::cout << "\nTo cancel press any key\n\n";
        
        const auto startTime = std::chrono::steady_clock::now();
        
        if (args.verbose)
        {
            //
            // it is just a sample
            // how to use SigPipeline::wait funstion with a record parameter
            //
            file_sig::SigPipeline::Record record;
            file_sig::SigPipeline::WaitRes res;
            
            uint64_t totalChunks = filesize / args.chunkSize + (filesize % args.chunkSize ? 1 : 0);
            uint64_t chunkId = 0;
            
            do
            {
                //
                // wait for one second or until a new record is ready
                // and check if a key has been pressed
                //
                res = pipeline.wait(1000, record);

                if (_kbhit())
                {
                    std::cout << "\nCanceling...";
                    pipeline.cancel(true);
                    std::cout << "\nStopped.\n";
                    break;
                }
                
                if (file_sig::SigPipeline::WaitRes::ready == res)
                {
                    out << record << "\r\n";
                    std::cout << std::dec << ++chunkId << "/" << totalChunks << " => " << record << "\n";
                }
            }
            while (res != file_sig::SigPipeline::WaitRes::finished);
        
            std::cout << "\rFinished: " << std::dec << totalChunks << " hashes\n";
        }
        else
        {
            //
            // it is just a sample
            // how to use SigPipeline::wait funstion without a record parameter
            // and the records callback
            //
            std::atomic<uint64_t> count{0};
            std::atomic<uint64_t> offset{0};
            
            pipeline.setRecordsCallback([&](file_sig::SigPipeline::Record record)
            {
                out << record << "\r\n";
                offset = record.offset + record.size;
                count += 1;
            });
            
            while (file_sig::SigPipeline::WaitRes::timeout == pipeline.wait(1000))
            {
                if (_kbhit())
                {
                    std::cout << "\nCanceling...";
                    pipeline.cancel(true);
                    std::cout << "\nStopped.\n";
                    break;
                }
                
                float percents = 100.0f * static_cast<float>(offset) / static_cast<float>(filesize);
                std::cout << "\r" << std::fixed << std::setprecision(2) << percents << "%";
                std::cout << " hashes:" << count << std::flush;
            }
        
            std::cout << "\rFinished: ";
            std::cout << 100.0f * static_cast<float>(offset) / static_cast<float>(filesize) << "%";
            std::cout << " hashes:" << std::dec << count << "\n";
        }
        
        auto time = std::chrono::steady_clock::now() - startTime;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time).count();
        
        std::cout << "Total time: " << timeToStr(seconds) << "\n";
        std::cout << "End: " << utils::getLocalTime() << std::endl;
        
        return 0;
    }
    catch (const std::ios::failure& ex)
    {
        std::cerr << "\nIO error: " << ex.code() << " " << ex.what() << std::endl;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "\nError: " << ex.what() << std::endl;
    }
    
    return -1;
}
