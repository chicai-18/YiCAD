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

#ifndef YICAD_MINIZIP_NG_ARCHIVE_H
#define YICAD_MINIZIP_NG_ARCHIVE_H

/**
 * @file MinizipNgArchive.h
 * @brief minizip-ng implementation of the ArchiveWriter/ArchiveReader interfaces.
 *
 * This is the ONLY file that includes minizip-ng headers. All other YiCAD
 * code should include Archive.h instead.
 *
 * Part of Step 9.2: Replace zipios++ with minizip-ng.
 */

#include "Archive.h"

#include <sstream>
#include <string>
#include <vector>

/**
 * ArchiveWriter implementation backed by minizip-ng.
 *
 * Data written via stream() is buffered internally. The actual ZIP entry is
 * written to the archive when the next putNextEntry() or close() is called.
 */
class MinizipNgArchiveWriter : public ArchiveWriter {
public:
    /** Construct from a filename (creates a new ZIP file). */
    explicit MinizipNgArchiveWriter(const std::string& filename);

    /** Construct from an existing output stream (writes ZIP data to it).
     *  The ZIP data is buffered in memory and flushed to the output stream
     *  when close() is called. */
    explicit MinizipNgArchiveWriter(std::ostream& os);

    ~MinizipNgArchiveWriter() override;

    // ArchiveWriter interface
    void putNextEntry(const std::string& name) override;
    void setComment(const std::string& comment) override;
    void setLevel(int level) override;
    std::ostream& stream() override;
    void close() override;

private:
    /** Flush the current entry's buffered data to the minizip-ng archive. */
    void flushCurrentEntry();

    void* writerHandle;     // mz_zip_writer*
    void* osStreamHandle;   // mz_stream_os* (file mode) or nullptr (stream mode)
    void* memStreamHandle;  // mz_stream_mem* (stream mode) or nullptr (file mode)

    std::string currentEntryName;
    std::ostringstream entryBuffer;
    int compressionLevel;
    std::string globalComment;
    bool isOpen;
    bool useStreamMode;
    std::ostream* outputStream;  // non-owning, only used in stream mode
};

/**
 * ArchiveReader implementation backed by minizip-ng.
 *
 * Each entry's data is loaded entirely into memory when nextEntry() is called,
 * then provided via a stringstream for the caller to read.
 */
class MinizipNgArchiveReader : public ArchiveReader {
public:
    /** Construct from a filename (opens an existing ZIP file). */
    explicit MinizipNgArchiveReader(const std::string& filename);

    /** Construct from an existing input stream (reads ZIP data from it).
     *  The entire stream is read into memory first. */
    explicit MinizipNgArchiveReader(std::istream& is);

    ~MinizipNgArchiveReader() override;

    // ArchiveReader interface
    bool nextEntry() override;
    std::string entryName() const override;
    std::string entryToString() const override;
    bool entryValid() const override;
    std::istream& stream() override;
    void closeEntry() override;

private:
    void* readerHandle;  // mz_zip_reader*

    std::string currentName;
    std::string currentData;
    std::istringstream dataStream;
    bool entryIsValid;
    bool reachedEnd;

    // For stream mode: the entire ZIP data held in memory
    std::vector<uint8_t> memoryBuffer;
};

#endif // YICAD_MINIZIP_NG_ARCHIVE_H
