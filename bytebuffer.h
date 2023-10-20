// ByteBuffer.h
// Simple and lightweight buffer with convenience functions
// 2022 M. Schallner

#ifndef BYTEBUFFER_H
#define BYTEBUFFER_H

#include <stdlib.h>

typedef unsigned char byte;
typedef unsigned int  offset;
typedef unsigned int  buflen;

class ByteBuffer {

public:
    ByteBuffer();
    ~ByteBuffer();

    buflen init(buflen l);
    void clear(byte b = 0x00);

    // --

    byte    *buf();
    buflen  len(); 

    byte    byte_at(offset o);
    bool    write_at(offset o, byte b);

    bool    err;
    bool    has_data;

private:
    byte   *bbuf;
    buflen  llen;
};

#endif
