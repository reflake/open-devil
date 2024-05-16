#pragma once

class Image {

public:

    Image( int width, int height, unsigned char * pixels );
    ~Image();

    static Image loadFile( const char * path );

private:

    int _width;
    int _height;
    unsigned char * _pixels;
};