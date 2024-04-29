#pragma once

#include <fstream>
#include <istream>

using std::istream, std::ifstream, std::iostream, std::string;

class File {

public:

    void close();
    void readBinary( int count, char *data );

    int getSize();

    static File openBinary( const string& filepath );

private:

    File( int size, string filepath ) : _size(size), _filepath(filepath) {}

    int _size;
    string _filepath;
};