#ifndef FILE_H
#define FILE_H

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

#include "filesystem.h"

using namespace std;

class File
{
private:
    FileSystem& fileSystem;

    // Find the inode number of a file/directory
    bool findInode(
        const string& path,
        uint32_t& inodeNumber
    );

    // Find an entry inside a directory
    bool findEntryInDirectory(
        uint32_t directoryInodeNumber,
        const string& name,
        uint32_t& inodeNumber
    );

    // Read all data block numbers belonging to an inode
    bool getDataBlocks(
        const Ext2Inode& inode,
        vector<uint32_t>& dataBlocks
    );

    // Read blocks from a single-indirect block
    bool readSingleIndirect(
        uint32_t indirectBlock,
        vector<uint32_t>& dataBlocks
    );

    // Read blocks from a double-indirect block
    bool readDoubleIndirect(
        uint32_t indirectBlock,
        vector<uint32_t>& dataBlocks
    );

    // Read blocks from a triple-indirect block
    bool readTripleIndirect(
        uint32_t indirectBlock,
        vector<uint32_t>& dataBlocks
    );

    // Allocate enough blocks for a file
    bool allocateDataBlocks(
        Ext2Inode& inode,
        uint32_t requiredBlocks,
        vector<uint32_t>& dataBlocks
    );

public:
    File(FileSystem& fs);

    // Find and display complete contents of a file
    bool displayFile(const string& path);

    // Overwrite an existing file
    bool overwriteFile(
        const string& path,
        const string& content
    );

    // Append content to an existing file
    bool appendToFile(
        const string& path,
        const string& content
    );
};

#endif