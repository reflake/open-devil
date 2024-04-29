#include <stdexcept>

#include "file.hpp"

File File::openBinary( const string& filepath ) {

    ifstream file( filepath, iostream::ate | iostream::binary );

    if ( !file.is_open() ) {
        throw std::runtime_error("Failed to open file");
    }

    // Look for file size
    File hFile( file.tellg(), file );
    file.seekg(0);

    return hFile;
}

void File::readBinary( int count, char *data) {

    _stream.read( data, count );
}

void File::close() {

    _stream.close();
}

int File::getSize() { return _size; }