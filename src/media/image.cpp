#include <cstdio>
#include <libpng/png.h>
#include <stdexcept>

#include "image.hpp"

void Image::loadFile( const char *path ) {

    FILE *fp = fopen( path, "rb" );

    if ( !fp ) {

        throw std::runtime_error("Error load image: couldn't read file");
    }

    unsigned char header[8];

    fread( header, 1, 8, fp );
    bool is_png = !png_sig_cmp( header, 0, 8 );
    if ( !is_png ) {

        throw std::runtime_error("Error load image: file is not PNG file");
    }
}