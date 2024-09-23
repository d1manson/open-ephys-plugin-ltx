

#ifndef LTXFILE_HEADER_H
#define LTXFILE_HEADER_H

#include <stdio.h>
#include <vector>
#include <chrono>
#include <memory>
#include <mutex>

namespace LTX {
    using HighResTimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

    class LTXFile
    {
        /*
          Writing the file goes through various stages:
            You can call AddHeaderValue zero or more times, and at any point within
              that call AddHeaderPlaceholder zero or one times (i.e. only one header can be a palceholder).
            You can next call WriteBinaryData zero or more times.
            You can next call FinaliseHeaderPlaceholder zero or one times.
            You should always finish by calling FinaliseFile exactly once.
          To enforce this order is not violated a status is tacked within the class.

          All these methods are guarded by a mutex, which makes them thread-safe though
          not carefully optimised for multi-threaded use.
        */

    

    public:

        LTXFile(const std::string& basePath, const std::string& extension, HighResTimePoint start_tm);
        ~LTXFile();

        template <typename T>
        void AddHeaderValue(const std::string& key, T value);
        void AddHeaderPlaceholder(const std::string& key);

        template <typename T>
        void FinaliseHeaderPlaceholder(T value);

        void WriteBinaryData(uint8_t* buffer, size_t totalBytes);

        void FinaliseFile(HighResTimePoint end_tm);

    private:
        enum FileWriteStatus {
            HEADERS,
            BINARY,
            EPILOUGE,
            CLOSED
        };

        size_t headerOffsetCustom = 0;
        size_t headerOffsetDuration = 0;
        FILE* theFile = nullptr;
        FileWriteStatus status = FileWriteStatus::HEADERS;
        HighResTimePoint start_tm;

        std::mutex mut;

    };
}



#endif // LTXFILE_HEADER_H