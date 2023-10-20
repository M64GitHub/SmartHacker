// ByteBuffer.cpp
// Simple and lightwieght buffer with convenience functions
// 2022 M. Schallner
//
// Functions set err == true || false

#include "bytebuffer.h"

ByteBuffer::ByteBuffer()
{
    bbuf = 0;
    llen=0;
    has_data = false;
    err = false;
}

buflen ByteBuffer::init(buflen l)
{
    if(has_data) free(bbuf);

    bbuf = (byte *) malloc(l);

    if(bbuf) has_data = true;
    else  {
        has_data = false;
        err = true;
        llen = 0;
        return 0;
    }

    llen = l;
    return l;
}

void ByteBuffer::clear(byte b) 
{
    if(!has_data) return;

    for(int i=0; i<llen; i++)
        bbuf[i] = b;
}

byte *ByteBuffer::buf()
{
    if(has_data) { err = false; return bbuf; }
    else { err = true; return 0; }
}

buflen ByteBuffer::len()
{
    if(has_data) return llen;
    else return 0;
}

byte ByteBuffer::byte_at(offset o)
{
    if(has_data && (o < llen)) {
        err = false;
        return bbuf[o];
    }

    err = true;
    return 0;
}

bool ByteBuffer::write_at(offset o, byte b)
{
    if(has_data && (o < llen)) {
        err = false;
        bbuf[o] = b;
        return true;
    } 

    err = true;
    return false;
}

ByteBuffer::~ByteBuffer()
{
    if(has_data) free(bbuf);
}

