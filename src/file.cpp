#include <stdexcept>

#include "file.hpp"

File File::openBinary( const string& filepath ) {

	ifstream stream( filepath, iostream::ate | iostream::binary );

	if ( !stream.is_open() ) {
		throw std::runtime_error("Failed to open file");
	}

	// Look for file size
	File file( stream.tellg(), filepath );
	stream.close();

	return file;
}

void File::readBinary( int count, char *data) {

	ifstream stream( _filepath, iostream::binary );
	
	stream.read( data, count );
	stream.close();
}

void File::close() {}

int File::getSize() { return _size; }