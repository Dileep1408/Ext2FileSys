#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

#include "filesystem.h"

using namespace std;


// ============================================================
// EXT2 DIRECTORY ENTRY
// ============================================================

struct Ext2DirectoryEntry
{
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
};


// ============================================================
// DIRECTORY CLASS
// ============================================================

class Directory
{
private:

    FileSystem& fileSystem;


    // --------------------------------------------------------
    // Read one directory data block
    // --------------------------------------------------------

    bool readDirectoryBlock(
        uint32_t blockNumber,
        uint32_t directoryInodeNumber,
        const string& currentPath,
        vector<uint32_t>& visitedInodes
    );


    // --------------------------------------------------------
    // Recursively traverse a directory
    // --------------------------------------------------------

    bool traverseDirectory(
        uint32_t inodeNumber,
        const string& path,
        vector<uint32_t>& visitedInodes
    );


    // --------------------------------------------------------
    // Print one directory entry
    // --------------------------------------------------------

    void printEntry(
        uint32_t inodeNumber,
        const string& name,
        uint8_t fileType,
        const string& path
    );


public:

    // --------------------------------------------------------
    // Constructor
    // --------------------------------------------------------

    Directory(FileSystem& fs);


    // --------------------------------------------------------
    // Start traversal from root inode
    // --------------------------------------------------------

    bool traverseFromRoot();
};

#endif