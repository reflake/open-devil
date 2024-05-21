#pragma once

class Image {

public:

    Image( int width, int height, unsigned char * pixels, int size );
    ~Image();

    const int getWidth();
    const int getHeight();
    const unsigned char * getPixelPointer();
    const int getSize();

    static Image loadFile( const char * path );

private:

    int _width;
    int _height;
    unsigned char * _pixels;
    int _size;
};