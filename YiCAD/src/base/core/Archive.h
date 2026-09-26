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

#ifndef YICAD_ARCHIVE_H
#define YICAD_ARCHIVE_H

/**
 * @file Archive.h
 * @brief Abstract interfaces for archive (ZIP) reading and writing.
 *
 * These interfaces isolate YiCAD business code from the concrete archive
 * library (currently minizip-ng). Business code should depend only on
 * these interfaces, not on minizip-ng types.
 *
 * Step 9.1: Created, implemented with zipios++ backend.
 * Step 9.2: Backend swapped to minizip-ng.
 */

#include <iostream>
#include <string>

/**
 * Abstract interface for writing entries to an archive.
 *
 * Usage:
 *   ArchiveWriter* writer = createWriter(...);
 *   writer->putNextEntry("entry1.xml");
 *   writer->stream() << "data";
 *   writer->putNextEntry("entry2.bin");
 *   writer->stream().write(buf, len);
 *   writer->close();
 */
class ArchiveWriter {
public:
    virtual ~ArchiveWriter() = default;

    /** Begin writing a new entry with the given name.
     *  Any previously open entry is closed automatically. */
    virtual void putNextEntry(const std::string& name) = 0;

    /** Set the archive-level comment (written into the ZIP central directory). */
    virtual void setComment(const std::string& comment) = 0;

    /** Set the compression level for subsequent entries.
     *  0 = no compression (STORED), 1-9 = fastest to best compression (DEFLATED). */
    virtual void setLevel(int level) = 0;

    /** Return a reference to the output stream for writing the current entry's data.
     *  Only valid between putNextEntry() and the next putNextEntry()/close(). */
    virtual std::ostream& stream() = 0;

    /** Finalize the archive (write central directory) and release resources.
     *  After close(), the writer must not be used again. */
    virtual void close() = 0;
};

/**
 * Abstract interface for reading entries from an archive.
 *
 * Usage:
 *   ArchiveReader* reader = createReader(...);
 *   while (reader->nextEntry()) {
 *       std::string name = reader->entryName();
 *       std::istream& is = reader->stream();
 *       // read data from is ...
 *       reader->closeEntry();
 *   }
 */
class ArchiveReader {
public:
    virtual ~ArchiveReader() = default;

    /** Advance to the next entry in the archive.
     *  Returns false when there are no more entries (end of archive).
     *  Must be called once before reading the first entry. */
    virtual bool nextEntry() = 0;

    /** Return the name of the current entry.
     *  Only valid after a successful nextEntry() call. */
    virtual std::string entryName() const = 0;

    /** Return a human-readable string representation of the current entry
     *  (typically includes name, size, method). Used for error messages. */
    virtual std::string entryToString() const = 0;

    /** Check whether the current entry is valid.
     *  Only valid after a successful nextEntry() call. */
    virtual bool entryValid() const = 0;

    /** Return a reference to the input stream for reading the current entry's data.
     *  Only valid between nextEntry() and the next nextEntry()/closeEntry(). */
    virtual std::istream& stream() = 0;

    /** Close the current entry and position the stream for the next one. */
    virtual void closeEntry() = 0;
};

#endif // YICAD_ARCHIVE_H
