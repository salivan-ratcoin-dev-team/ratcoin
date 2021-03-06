// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"

#include "common/dimsParams.h"
#include "common/accessFile.h"

namespace common
{

struct CDiskBlock;

class CSegmentHeader;

template<> unsigned int const CSerializedTypes<CDiskBlock>::m_key = 1;

template<> unsigned int const CSerializedTypes<CSegmentHeader>::m_key = 2;

CAccessFile::CAccessFile()
{
}


bool
CAccessFile::fileExist(std::string const & _name ) const
{
	boost::filesystem::path path( _name );

    return boost::filesystem::exists(path);
}

FILE*
CAccessFile::openDiskFile(long int const pos, std::string _path, bool fReadOnly)
{
    boost::filesystem::path path(_path);
    boost::filesystem::create_directories(path.parent_path());
    FILE* file = fopen(path.string().c_str(), "rb+");
    if (!file && !fReadOnly)
        file = fopen(path.string().c_str(), "wb+");
    if (!file) {
        //LogPrintf("Unable to open file %s\n", path.string());
        return NULL;
    }
    if (pos) {
        if (fseek(file, pos, SEEK_SET)) {
          //  LogPrintf("Unable to seek to position %u of %s\n", pos, path.string());
            fclose(file);
            return NULL;
        }
    }
    return file;
}

}



