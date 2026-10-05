#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>

#include "structures.h"

using namespace std;

// ============================================================
// FILE SYSTEM CLASS
// ============================================================

class FileSystem
{
private:

    // --------------------------------------------------------
    // Filesystem image
    // --------------------------------------------------------

    string imagePath;
    fstream image;


    // --------------------------------------------------------
    // Important ext2 information
    // --------------------------------------------------------

    Ext2Superblock superblock;

    vector<Ext2BlockGroupDescriptor> groupDescriptors;


    // --------------------------------------------------------
    // Basic filesystem information
    // --------------------------------------------------------

    uint32_t blockSize;
    uint32_t inodeSize;
    uint32_t inodesPerGroup;
    uint32_t blocksPerGroup;


    // --------------------------------------------------------
    // Helper functions
    // --------------------------------------------------------

    uint64_t getBlockOffset(uint32_t blockNumber) const;

    uint64_t getInodeOffset(uint32_t inodeNumber) const;

    uint32_t getBlockGroup(uint32_t inodeNumber) const;

    uint32_t getLocalInodeNumber(uint32_t inodeNumber) const;


public:

    // ========================================================
    // CONSTRUCTOR / DESTRUCTOR
    // ========================================================

    FileSystem();

    ~FileSystem();


    // ========================================================
    // IMAGE OPERATIONS
    // ========================================================

    bool openImage(const string& path);

    void closeImage();

    bool isOpen() const;


    // ========================================================
    // TASK 1
    // READ CORE STRUCTURES
    // ========================================================

    bool readSuperblock();

    bool readGroupDescriptors();

    void printSuperblock() const;

    void printGroupDescriptors() const;


    // ========================================================
    // BLOCK OPERATIONS
    // ========================================================

    bool readBlock(uint32_t blockNumber, vector<uint8_t>& buffer);

    bool writeBlock(uint32_t blockNumber, const vector<uint8_t>& buffer);


    // ========================================================
    // INODE OPERATIONS
    // ========================================================

    bool readInode(uint32_t inodeNumber, Ext2Inode& inode);

    bool writeInode(uint32_t inodeNumber, const Ext2Inode& inode);


    // ========================================================
    // BLOCK ALLOCATION
    // TASK 4
    // ========================================================

    int32_t findFreeBlock();

    bool allocateBlock(uint32_t& blockNumber);

    bool freeBlock(uint32_t blockNumber);


    // ========================================================
    // FILESYSTEM INFORMATION
    // ========================================================

    uint32_t getBlockSize() const;

    uint32_t getInodeSize() const;

    uint32_t getInodesPerGroup() const;

    uint32_t getBlocksPerGroup() const;

    uint32_t getBlockCount() const;

    uint32_t getInodeCount() const;


    // ========================================================
    // ACCESS TO CORE STRUCTURES
    // ========================================================

    const Ext2Superblock& getSuperblock() const;

    const vector<Ext2BlockGroupDescriptor>& getGroupDescriptors() const;
};

#endif