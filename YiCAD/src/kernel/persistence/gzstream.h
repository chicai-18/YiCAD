// gzstream.h
// Copyright (C) 2001 Deepak Bhatt <dbhatt@cs.unc.edu>
// Department of Computer Science, University of North Carolina
// at Chapel Hill.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented.
// 2. Altered source versions must be plainly marked as such.
// 3. This notice may not be removed or altered from any source distribution.

#ifndef GZSTREAM_H
#define GZSTREAM_H 1

#include <sstream>
#include <zlib.h>

#ifdef _MSC_VER
using std::ostream;
using std::istream;
#endif


#define BUFFERSIZE 47+256

// ----------------------------------------------------------------------------
// Internal classes to implement gzstream. See below for user classes.
// ----------------------------------------------------------------------------

class gzstreambuf : public std::streambuf {
private:
    static const int bufferSize;    // size of data buff
    // totals 512 bytes under g++ for igzstream at the end.

    gzFile           file;               // file handle for compressed file
    char             buffer[BUFFERSIZE]; // data buffer
    char             opened;             // open/close state of stream
    int              mode;               // I/O mode

    int flush_buffer();
public:
    gzstreambuf() : file(nullptr), opened(0), mode(0) {
        setp( buffer, buffer + (bufferSize-1));
        setg( buffer + 4,     // beginning of putback area
              buffer + 4,     // read position
              buffer + 4);    // end position
        // ASSERT: both input & output capabilities will not be used together
    }
    int is_open() { return opened; }
    gzstreambuf* open( const char* name, int open_mode, int comp);
    gzstreambuf* close();
    ~gzstreambuf() { close(); }

    virtual int     overflow( int c = EOF);
    virtual int     underflow();
    virtual int     sync();
};

class gzstreambase : virtual public std::ios {
protected:
    gzstreambuf buf;
public:
    gzstreambase() { init(&buf); }
    gzstreambase( const char* name, int open_mode, int comp);
    ~gzstreambase();
    void open( const char* name, int open_mode, int comp);
    void close();
    gzstreambuf* rdbuf() { return &buf; }
};

// ----------------------------------------------------------------------------
// User classes. Use igzstream and ogzstream analogously to ifstream and
// ofstream respectively. They read and write files based on the gz*
// function interface of the zlib. Files are compatible with gzip compression.
// ----------------------------------------------------------------------------

class igzstream : public gzstreambase, public std::istream {
public:
    igzstream()
#ifdef _MSC_VER
      : istream( &buf) {}
#else
      : std::istream( &buf) {}
#endif
    igzstream( const char* name, int open_mode = std::ios_base::in, int comp = 1)
#ifdef _MSC_VER
      : gzstreambase( name, open_mode, comp ), istream( &buf) {}
#else
      : gzstreambase( name, open_mode, comp), std::istream( &buf) {}
#endif
    gzstreambuf* rdbuf() { return gzstreambase::rdbuf(); }
    void open( const char* name, int open_mode = std::ios_base::in, int comp = 1) {
        gzstreambase::open( name, open_mode, comp);
    }
};

class ogzstream : public gzstreambase, public std::ostream {
public:
    ogzstream()
#ifdef _MSC_VER
      : ostream( &buf) {}
#else
      : std::ostream( &buf) {}
#endif
    ogzstream( const char* name, int mode = std::ios_base::out, int comp = 1)
#ifdef _MSC_VER
      : gzstreambase( name, mode, comp), ostream( &buf) {}
#else
      : gzstreambase( name, mode, comp), std::ostream( &buf) {}
#endif
    gzstreambuf* rdbuf() { return gzstreambase::rdbuf(); }
    void open( const char* name, int open_mode = std::ios_base::out, int comp = 1) {
        gzstreambase::open( name, open_mode, comp);
    }
};

#endif // GZSTREAM_H
// ============================================================================
// EOF //
