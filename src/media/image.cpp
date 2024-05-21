#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <libpng/png.h>
#include <libpng16/pngconf.h>
#include <stdexcept>

#include "image.hpp"

const int signatureSize = 8;

Image Image::loadFile( const char *path ) {

    FILE *fp = fopen( path, "rb" );

    if ( !fp ) {

        throw std::runtime_error("Error load image: couldn't read file");
    }

    unsigned char header[8];

    fread( header, 1, signatureSize, fp );
    bool is_png = !png_sig_cmp( header, 0, signatureSize );
    if ( !is_png ) {

        throw std::runtime_error("Error load image: file is not PNG file");
    }

    png_structp png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );

    if (!png_ptr) {

        throw std::runtime_error("Error load image: couldn't read struct");
    }

    png_infop info_ptr = png_create_info_struct( png_ptr );

    if (!info_ptr) {

        throw std::runtime_error("Error load image: couldn't read info");
    }

    png_init_io( png_ptr, fp );
    png_set_sig_bytes( png_ptr, signatureSize );
    png_read_info( png_ptr, info_ptr );

    uint width, height;
    int depth, colorType;

    png_get_IHDR( png_ptr, info_ptr, &width, &height, &depth, &colorType, NULL, NULL, NULL );

    int bytesPerRow = png_get_rowbytes( png_ptr, info_ptr );
    int size = height * bytesPerRow;
    unsigned char *pixels = new unsigned char[ size ];
    png_bytepp row_pointers = new png_bytep[height];

    for(int i = 0; i < height; i++) {

        row_pointers[i] = pixels + i * bytesPerRow;
    }

    png_read_image( png_ptr, row_pointers );

    png_destroy_read_struct( &png_ptr, &info_ptr, NULL );

    return Image( width, height, pixels, size );
}

Image::Image( int width, int height, unsigned char * pixels, int size ) : _width(width), _height(height), _pixels(pixels), _size(size) {

}

Image::~Image() {

    delete _pixels;
}    

inline const int Image::getWidth() {

    return _width;
}

inline const int Image::getHeight() {

    return _height;
}

inline const unsigned char * Image::getPixelPointer() {

    return _pixels;
}

inline const int Image::getSize() {

    return _size;
}