/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/// @file DmFlags.h
/// @brief 标志位基类，提供位标志操作及 hash 支持

#ifndef DMFLAGS_H
#define DMFLAGS_H

#include <Stream.h>
#include <Persistence.h>

#include <cstddef>
#include <functional>

inline void hash_combine(std::size_t& seed) {}

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    int i[] = {0, (hash_combine(seed, std::forward<Rest>(rest)), 0)...};
    (void)(i);
}

#define MAKE_HASHABLE(type, ...) \
    namespace std { \
        template<> struct hash<type> { \
            std::size_t operator()(const type &t) const { \
                std::size_t ret = 0; \
                hash_combine(ret, __VA_ARGS__); \
                return ret; \
            } \
        }; \
    }

// Base class for objects which have flags.
class DmFlags: public Persistence
{
    TYPESYSTEM_HEADER();

public:
    // Constructor with initialisation to the given flags.
    // Default sets all flags to 0
    DmFlags(unsigned f = 0) : flags(f) {};

    virtual ~DmFlags() = default;

    unsigned getFlags() const { return flags; };

    void resetFlags() { flags = 0; }

    void setFlags(unsigned f) { flags = f; }

    void setFlag(unsigned f) { flags |= f; }

    void delFlag(unsigned f) { flags &= ~f; }

    void toggleFlag(unsigned f) { flags ^= f; }

    bool getFlag(unsigned f) const { return flags & f; }

    // persistent helper
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

private:
    unsigned flags = 0;
};

MAKE_HASHABLE(DmFlags, t.getFlags());

#endif // DMFLAGS_H
