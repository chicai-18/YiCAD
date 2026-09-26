/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file MinizipNgArchive.cpp
 * @brief minizip-ng implementation of the ArchiveWriter/ArchiveReader interfaces.
 *
 * Part of Step 9.2: Replace zipios++ with minizip-ng.
 */

#include "MinizipNgArchive.h"

#include <cstring>
#include <exception>
#include <limits>
#include <stdexcept>

#include <mz.h>
#include <mz_strm.h>
#include <mz_strm_mem.h>
#include <mz_strm_os.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>

// ---------------------------------------------------------------------------
//  Helper: fill mz_zip_file struct for a new entry
// ---------------------------------------------------------------------------
static mz_zip_file makeEntryInfo(const std::string& name)
{
    mz_zip_file info;
    std::memset(&info, 0, sizeof(info));
    info.filename = name.c_str();
    info.filename_size = static_cast<uint16_t>(name.size());
    info.flag = MZ_ZIP_FLAG_UTF8;
    info.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
    info.zip64 = MZ_ZIP64_AUTO;
    return info;
}

// ---------------------------------------------------------------------------
//  MinizipNgArchiveWriter
// ---------------------------------------------------------------------------

MinizipNgArchiveWriter::MinizipNgArchiveWriter(const std::string& filename)
    : writerHandle(nullptr)
    , osStreamHandle(nullptr)
    , memStreamHandle(nullptr)
    , compressionLevel(MZ_COMPRESS_LEVEL_DEFAULT)
    , isOpen(false)
    , useStreamMode(false)
    , outputStream(nullptr)
{
    writerHandle = mz_zip_writer_create();
    if (!writerHandle)
        throw std::runtime_error("MinizipNgArchiveWriter: failed to create writer handle");

    int32_t err = mz_zip_writer_open_file(writerHandle, filename.c_str(), 0, 0);
    if (err != MZ_OK) {
        mz_zip_writer_delete(&writerHandle);
        throw std::runtime_error("MinizipNgArchiveWriter: failed to open file for writing, error " + std::to_string(err));
    }
    isOpen = true;
}

MinizipNgArchiveWriter::MinizipNgArchiveWriter(std::ostream& os)
    : writerHandle(nullptr)
    , osStreamHandle(nullptr)
    , memStreamHandle(nullptr)
    , compressionLevel(MZ_COMPRESS_LEVEL_DEFAULT)
    , isOpen(false)
    , useStreamMode(true)
    , outputStream(&os)
{
    writerHandle = mz_zip_writer_create();
    if (!writerHandle)
        throw std::runtime_error("MinizipNgArchiveWriter: failed to create writer handle");

    // Create a memory stream to buffer the ZIP output
    memStreamHandle = mz_stream_mem_create();
    if (!memStreamHandle) {
        mz_zip_writer_delete(&writerHandle);
        throw std::runtime_error("MinizipNgArchiveWriter: failed to create memory stream");
    }

    int32_t err = mz_stream_mem_open(memStreamHandle, nullptr, MZ_OPEN_MODE_CREATE);
    if (err != MZ_OK) {
        mz_stream_mem_delete(&memStreamHandle);
        mz_zip_writer_delete(&writerHandle);
        throw std::runtime_error("MinizipNgArchiveWriter: failed to open memory stream, error " + std::to_string(err));
    }

    err = mz_zip_writer_open(writerHandle, memStreamHandle, 0);
    if (err != MZ_OK) {
        mz_stream_mem_close(memStreamHandle);
        mz_stream_mem_delete(&memStreamHandle);
        mz_zip_writer_delete(&writerHandle);
        throw std::runtime_error("MinizipNgArchiveWriter: failed to open writer with memory stream, error " + std::to_string(err));
    }
    isOpen = true;
}

MinizipNgArchiveWriter::~MinizipNgArchiveWriter()
{
    try {
        close();
    } catch (...) {
        // Destructor must not throw.
    }
}

void MinizipNgArchiveWriter::flushCurrentEntry()
{
    if (currentEntryName.empty())
        return; // No entry to flush

    // Prepare entry info
    mz_zip_file info = makeEntryInfo(currentEntryName);

    // Get the buffered data
    std::string data = entryBuffer.str();
    if (data.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
        throw std::runtime_error("MinizipNgArchiveWriter: entry '" +
            currentEntryName + "' is too large to write from memory");
    }
    info.uncompressed_size = static_cast<int64_t>(data.size());

    // Write the entry to the archive
    const char* buffer = data.empty() ? "" : data.data();
    int32_t err = mz_zip_writer_add_buffer(writerHandle,
        const_cast<char*>(buffer),
        static_cast<int32_t>(data.size()),
        &info);

    if (err != MZ_OK) {
        throw std::runtime_error("MinizipNgArchiveWriter: failed to write entry '" +
            currentEntryName + "', error " + std::to_string(err));
    }

    // Reset buffer
    entryBuffer.str(std::string());
    entryBuffer.clear();
    currentEntryName.clear();
}

void MinizipNgArchiveWriter::putNextEntry(const std::string& name)
{
    // Flush any previously open entry
    flushCurrentEntry();

    currentEntryName = name;
    entryBuffer.str(std::string());
    entryBuffer.clear();

    // Set locale for numeric formatting (match zipios++ behavior)
#ifdef _MSC_VER
    entryBuffer.imbue(std::locale::empty());
#else
    entryBuffer.imbue(std::locale::classic());
#endif
    entryBuffer.precision(std::numeric_limits<double>::digits10 + 1);
    entryBuffer.setf(std::ios::fixed, std::ios::floatfield);
}

void MinizipNgArchiveWriter::setComment(const std::string& comment)
{
    globalComment = comment;
    if (isOpen && writerHandle) {
        void* zipHandle = nullptr;
        mz_zip_writer_get_zip_handle(writerHandle, &zipHandle);
        if (zipHandle) {
            mz_zip_set_comment(zipHandle, comment.c_str());
        }
    }
}

void MinizipNgArchiveWriter::setLevel(int level)
{
    if (level < 0) {
        compressionLevel = MZ_COMPRESS_LEVEL_DEFAULT;
    } else if (level > MZ_COMPRESS_LEVEL_BEST) {
        compressionLevel = MZ_COMPRESS_LEVEL_BEST;
    } else {
        compressionLevel = level;
    }

    if (writerHandle)
        mz_zip_writer_set_compress_level(writerHandle, static_cast<int16_t>(compressionLevel));
}

std::ostream& MinizipNgArchiveWriter::stream()
{
    return entryBuffer;
}

void MinizipNgArchiveWriter::close()
{
    if (!isOpen)
        return;

    std::string pendingError;

    // Flush the last entry if any. Keep closing/cleaning up even when flushing
    // fails so callers can catch the original save error without leaking handles.
    try {
        flushCurrentEntry();
    } catch (const std::exception& e) {
        pendingError = e.what();
    } catch (...) {
        pendingError = "MinizipNgArchiveWriter: failed to write entry";
    }

    // Close the zip writer
    int32_t closeErr = MZ_OK;
    if (writerHandle) {
        closeErr = mz_zip_writer_close(writerHandle);
    }

    // In stream mode, copy the memory buffer to the output stream
    if (useStreamMode && memStreamHandle) {
        const uint8_t* buf = nullptr;
        int32_t bufLen = 0;

        if (pendingError.empty() && closeErr == MZ_OK && outputStream) {
            mz_stream_mem_get_buffer(memStreamHandle, reinterpret_cast<const void**>(&buf));
            mz_stream_mem_get_buffer_length(memStreamHandle, &bufLen);

            if (buf && bufLen > 0) {
                outputStream->write(reinterpret_cast<const char*>(buf), bufLen);
                if (!(*outputStream))
                    pendingError = "MinizipNgArchiveWriter: failed to write archive to output stream";
            }
        }

        mz_stream_mem_close(memStreamHandle);
        mz_stream_mem_delete(&memStreamHandle);
        memStreamHandle = nullptr;
    }

    // Clean up
    if (writerHandle) {
        mz_zip_writer_delete(&writerHandle);
        writerHandle = nullptr;
    }
    if (osStreamHandle) {
        mz_stream_os_close(osStreamHandle);
        mz_stream_os_delete(&osStreamHandle);
        osStreamHandle = nullptr;
    }

    isOpen = false;

    if (!pendingError.empty())
        throw std::runtime_error(pendingError);
    if (closeErr != MZ_OK)
        throw std::runtime_error("MinizipNgArchiveWriter: failed to close, error " + std::to_string(closeErr));
}

// ---------------------------------------------------------------------------
//  MinizipNgArchiveReader
// ---------------------------------------------------------------------------

MinizipNgArchiveReader::MinizipNgArchiveReader(const std::string& filename)
    : readerHandle(nullptr)
    , entryIsValid(false)
    , reachedEnd(false)
{
    readerHandle = mz_zip_reader_create();
    if (!readerHandle)
        throw std::runtime_error("MinizipNgArchiveReader: failed to create reader handle");

    int32_t err = mz_zip_reader_open_file(readerHandle, filename.c_str());
    if (err != MZ_OK) {
        mz_zip_reader_delete(&readerHandle);
        readerHandle = nullptr;
        throw std::runtime_error("MinizipNgArchiveReader: failed to open file '" +
            filename + "', error " + std::to_string(err));
    }
}

MinizipNgArchiveReader::MinizipNgArchiveReader(std::istream& is)
    : readerHandle(nullptr)
    , entryIsValid(false)
    , reachedEnd(false)
{
    // Read the entire input stream into memory
    is.seekg(0, std::ios::end);
    auto size = is.tellg();
    is.seekg(0, std::ios::beg);

    if (size > 0) {
        memoryBuffer.resize(static_cast<size_t>(size));
        is.read(reinterpret_cast<char*>(memoryBuffer.data()), size);
        if (!is && !is.eof()) {
            throw std::runtime_error("MinizipNgArchiveReader: failed to read input stream");
        }
    }

    readerHandle = mz_zip_reader_create();
    if (!readerHandle)
        throw std::runtime_error("MinizipNgArchiveReader: failed to create reader handle");

    // Open from memory buffer (copy=0 since we keep the buffer alive)
    int32_t err = mz_zip_reader_open_buffer(readerHandle,
        memoryBuffer.data(),
        static_cast<int32_t>(memoryBuffer.size()),
        0);
    if (err != MZ_OK) {
        mz_zip_reader_delete(&readerHandle);
        readerHandle = nullptr;
        throw std::runtime_error("MinizipNgArchiveReader: failed to open from memory buffer, error " + std::to_string(err));
    }
}

MinizipNgArchiveReader::~MinizipNgArchiveReader()
{
    if (readerHandle) {
        mz_zip_reader_close(readerHandle);
        mz_zip_reader_delete(&readerHandle);
        readerHandle = nullptr;
    }
}

bool MinizipNgArchiveReader::nextEntry()
{
    if (!readerHandle || reachedEnd)
        return false;

    // Close any previously open entry
    closeEntry();

    int32_t err;
    if (currentName.empty()) {
        // First call: go to first entry
        err = mz_zip_reader_goto_first_entry(readerHandle);
    } else {
        // Subsequent calls: go to next entry
        err = mz_zip_reader_goto_next_entry(readerHandle);
    }

    if (err != MZ_OK) {
        reachedEnd = true;
        entryIsValid = false;
        return false;
    }

    // Get entry info
    mz_zip_file* fileInfo = nullptr;
    err = mz_zip_reader_entry_get_info(readerHandle, &fileInfo);
    if (err != MZ_OK || !fileInfo) {
        reachedEnd = true;
        entryIsValid = false;
        return false;
    }

    currentName = fileInfo->filename ? fileInfo->filename : "";
    entryIsValid = true;

    // Read the entire entry data into memory
    int32_t readErr = mz_zip_reader_entry_open(readerHandle);
    if (readErr != MZ_OK) {
        entryIsValid = false;
        return false;
    }

    currentData.clear();
    if (fileInfo->uncompressed_size > 0) {
        currentData.resize(static_cast<size_t>(fileInfo->uncompressed_size));
        int32_t totalRead = 0;
        while (totalRead < static_cast<int32_t>(currentData.size())) {
            int32_t bytesRead = mz_zip_reader_entry_read(readerHandle,
                currentData.data() + totalRead,
                static_cast<int32_t>(currentData.size()) - totalRead);
            if (bytesRead <= 0)
                break;
            totalRead += bytesRead;
        }
        currentData.resize(static_cast<size_t>(totalRead));
    }

    mz_zip_reader_entry_close(readerHandle);

    // Set up the data stream
    dataStream = std::istringstream(currentData, std::ios::binary);

    return true;
}

std::string MinizipNgArchiveReader::entryName() const
{
    return currentName;
}

std::string MinizipNgArchiveReader::entryToString() const
{
    if (!entryIsValid)
        return "<invalid entry>";
    return currentName + " (" + std::to_string(currentData.size()) + " bytes)";
}

bool MinizipNgArchiveReader::entryValid() const
{
    return entryIsValid;
}

std::istream& MinizipNgArchiveReader::stream()
{
    return dataStream;
}

void MinizipNgArchiveReader::closeEntry()
{
    // Data is already fully loaded; just reset the stream position
    // so the next stream() call works correctly.
    dataStream.clear();
    dataStream.seekg(0);
}
