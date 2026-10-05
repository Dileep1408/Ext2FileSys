#include "../include/filesystem.h"

#include <cstring>
#include <algorithm>
#include <iomanip>
#include <sstream>

using namespace std;


// ============================================================
// CONSTRUCTOR
// ============================================================

FileSystem::FileSystem()
{
    blockSize = 0;
    inodeSize = 0;
    inodesPerGroup = 0;
    blocksPerGroup = 0;
}


// ============================================================
// DESTRUCTOR
// ============================================================

FileSystem::~FileSystem()
{
    closeImage();
}


// ============================================================
// OPEN IMAGE
// ============================================================

bool FileSystem::openImage(const string& path)
{
    imagePath = path;

    image.open(
        imagePath,
        ios::in | ios::out | ios::binary
    );

    if (!image.is_open())
    {
        return false;
    }

    return true;
}


// ============================================================
// CLOSE IMAGE
// ============================================================

void FileSystem::closeImage()
{
    if (image.is_open())
    {
        image.close();
    }
}


// ============================================================
// CHECK WHETHER IMAGE IS OPEN
// ============================================================

bool FileSystem::isOpen() const
{
    return image.is_open();
}


// ============================================================
// READ SUPERBLOCK
// ============================================================

bool FileSystem::readSuperblock()
{
    if (!isOpen())
    {
        return false;
    }

    // The ext2 superblock starts at byte offset 1024.
    image.clear();
    image.seekg(1024, ios::beg);

    if (!image)
    {
        return false;
    }

    image.read(
        reinterpret_cast<char*>(&superblock),
        sizeof(Ext2Superblock)
    );

    if (!image)
    {
        return false;
    }

    // Check ext2 magic number.
    if (superblock.magic != 0xEF53)
    {
        cerr << "Invalid ext2 magic number." << endl;
        return false;
    }

    // Calculate filesystem block size.
    blockSize =
        1024U << superblock.log_block_size;

    // Calculate inode size.
    if (superblock.revision_level == 0)
    {
        inodeSize = 128;
    }
    else
    {
        inodeSize = superblock.inode_size;

        if (inodeSize == 0)
        {
            inodeSize = 128;
        }
    }

    inodesPerGroup =
        superblock.inodes_per_group;

    blocksPerGroup =
        superblock.blocks_per_group;

    return true;
}


// ============================================================
// READ BLOCK GROUP DESCRIPTORS
// ============================================================

bool FileSystem::readGroupDescriptors()
{
    if (!isOpen())
    {
        return false;
    }

    if (blockSize == 0)
    {
        return false;
    }

    // Number of block groups.
    uint32_t groupCount =
        (
            superblock.blocks_count +
            blocksPerGroup -
            1
        ) / blocksPerGroup;

    groupDescriptors.clear();
    groupDescriptors.resize(groupCount);

    /*
        For a 1 KB block size:

            block 0 = boot block
            block 1 = superblock
            block 2 = group descriptor table

        For larger block sizes:

            block 0 = superblock
            block 1 = group descriptor table
    */

    uint32_t descriptorBlock;

    if (blockSize == 1024)
    {
        descriptorBlock = 2;
    }
    else
    {
        descriptorBlock = 1;
    }

    uint64_t offset =
        getBlockOffset(descriptorBlock);

    image.clear();

    image.seekg(offset, ios::beg);

    if (!image)
    {
        return false;
    }

    image.read(
        reinterpret_cast<char*>(groupDescriptors.data()),
        groupCount * sizeof(Ext2BlockGroupDescriptor)
    );

    if (!image)
    {
        return false;
    }

    return true;
}


// ============================================================
// PRINT SUPERBLOCK
// ============================================================

void FileSystem::printSuperblock() const
{
    cout << endl;
    cout << "--------------- SUPERBLOCK ---------------"
         << endl;

    cout << "Inodes count        : "
         << superblock.inodes_count
         << endl;

    cout << "Blocks count        : "
         << superblock.blocks_count
         << endl;

    cout << "Reserved blocks     : "
         << superblock.reserved_blocks_count
         << endl;

    cout << "Free blocks         : "
         << superblock.free_blocks_count
         << endl;

    cout << "Free inodes         : "
         << superblock.free_inodes_count
         << endl;

    cout << "First data block    : "
         << superblock.first_data_block
         << endl;

    cout << "Block size          : "
         << blockSize
         << " bytes"
         << endl;

    cout << "Blocks per group    : "
         << superblock.blocks_per_group
         << endl;

    cout << "Inodes per group    : "
         << superblock.inodes_per_group
         << endl;

    cout << "Inode size          : "
         << inodeSize
         << " bytes"
         << endl;

    cout << "Magic               : 0x"
         << hex
         << superblock.magic
         << dec
         << endl;

    cout << "Revision level      : "
         << superblock.revision_level
         << endl;

    cout << "First inode         : "
         << superblock.first_inode
         << endl;

    cout << "Creator OS          : "
         << superblock.creator_os
         << endl;

    cout << "------------------------------------------"
         << endl;
}


// ============================================================
// PRINT GROUP DESCRIPTORS
// ============================================================

void FileSystem::printGroupDescriptors() const
{
    cout << endl;
    cout << "----------- BLOCK GROUP DESCRIPTORS -----------"
         << endl;

    for (size_t i = 0;
         i < groupDescriptors.size();
         i++)
    {
        const Ext2BlockGroupDescriptor& group =
            groupDescriptors[i];

        cout << endl;

        cout << "Group " << i << endl;

        cout << "  Block bitmap      : "
             << group.block_bitmap
             << endl;

        cout << "  Inode bitmap      : "
             << group.inode_bitmap
             << endl;

        cout << "  Inode table       : "
             << group.inode_table
             << endl;

        cout << "  Free blocks       : "
             << group.free_blocks_count
             << endl;

        cout << "  Free inodes       : "
             << group.free_inodes_count
             << endl;

        cout << "  Used directories  : "
             << group.used_dirs_count
             << endl;
    }

    cout << endl;
    cout << "-----------------------------------------------"
         << endl;
}


// ============================================================
// GET BLOCK OFFSET
// ============================================================

uint64_t FileSystem::getBlockOffset(
    uint32_t blockNumber
) const
{
    return
        static_cast<uint64_t>(blockNumber) *
        blockSize;
}


// ============================================================
// GET BLOCK GROUP OF INODE
// ============================================================

uint32_t FileSystem::getBlockGroup(
    uint32_t inodeNumber
) const
{
    return
        (inodeNumber - 1) /
        inodesPerGroup;
}


// ============================================================
// GET LOCAL INODE NUMBER
// ============================================================

uint32_t FileSystem::getLocalInodeNumber(
    uint32_t inodeNumber
) const
{
    return
        (inodeNumber - 1) %
        inodesPerGroup;
}


// ============================================================
// GET INODE OFFSET
// ============================================================

uint64_t FileSystem::getInodeOffset(
    uint32_t inodeNumber
) const
{
    uint32_t group =
        getBlockGroup(inodeNumber);

    uint32_t localInode =
        getLocalInodeNumber(inodeNumber);

    uint32_t inodeTableBlock =
        groupDescriptors[group].inode_table;

    return
        getBlockOffset(inodeTableBlock) +
        static_cast<uint64_t>(localInode) *
        inodeSize;
}


// ============================================================
// READ BLOCK
// ============================================================

bool FileSystem::readBlock(
    uint32_t blockNumber,
    vector<uint8_t>& buffer
)
{
    if (!isOpen())
    {
        return false;
    }

    if (blockNumber >= superblock.blocks_count)
    {
        return false;
    }

    buffer.resize(blockSize);

    uint64_t offset =
        getBlockOffset(blockNumber);

    image.clear();

    image.seekg(offset, ios::beg);

    if (!image)
    {
        return false;
    }

    image.read(
        reinterpret_cast<char*>(buffer.data()),
        blockSize
    );

    if (!image)
    {
        return false;
    }

    return true;
}


// ============================================================
// WRITE BLOCK
// ============================================================

bool FileSystem::writeBlock(
    uint32_t blockNumber,
    const vector<uint8_t>& buffer
)
{
    if (!isOpen())
    {
        return false;
    }

    if (blockNumber >= superblock.blocks_count)
    {
        return false;
    }

    if (buffer.size() != blockSize)
    {
        return false;
    }

    uint64_t offset =
        getBlockOffset(blockNumber);

    image.clear();

    image.seekp(offset, ios::beg);

    if (!image)
    {
        return false;
    }

    image.write(
        reinterpret_cast<const char*>(buffer.data()),
        blockSize
    );

    image.flush();

    return static_cast<bool>(image);
}


// ============================================================
// READ INODE
// ============================================================

bool FileSystem::readInode(
    uint32_t inodeNumber,
    Ext2Inode& inode
)
{
    if (!isOpen())
    {
        return false;
    }

    if (inodeNumber == 0 ||
        inodeNumber > superblock.inodes_count)
    {
        return false;
    }

    uint64_t offset =
        getInodeOffset(inodeNumber);

    /*
        IMPORTANT:

        The filesystem may use 256-byte inode entries,
        while our Ext2Inode structure contains the first
        128 bytes.

        Therefore we read the COMPLETE inode entry and
        copy only the part represented by our structure.
    */

    vector<uint8_t> buffer(inodeSize, 0);

    image.clear();

    image.seekg(offset, ios::beg);

    if (!image)
    {
        return false;
    }

    image.read(
        reinterpret_cast<char*>(buffer.data()),
        inodeSize
    );

    if (!image)
    {
        return false;
    }

    memset(
        &inode,
        0,
        sizeof(Ext2Inode)
    );

    size_t bytesToCopy =
        min(
            sizeof(Ext2Inode),
            static_cast<size_t>(inodeSize)
        );

    memcpy(
        &inode,
        buffer.data(),
        bytesToCopy
    );

    return true;
}


// ============================================================
// WRITE INODE
// ============================================================

bool FileSystem::writeInode(
    uint32_t inodeNumber,
    const Ext2Inode& inode
)
{
    if (!isOpen())
    {
        return false;
    }

    if (inodeNumber == 0 ||
        inodeNumber > superblock.inodes_count)
    {
        return false;
    }

    uint64_t offset =
        getInodeOffset(inodeNumber);

    /*
        Read the complete inode first.

        This preserves the part of the inode that our
        Ext2Inode structure does not represent.
    */

    vector<uint8_t> buffer(inodeSize, 0);

    image.clear();

    image.seekg(offset, ios::beg);

    if (!image)
    {
        return false;
    }

    image.read(
        reinterpret_cast<char*>(buffer.data()),
        inodeSize
    );

    if (!image)
    {
        return false;
    }

    size_t bytesToCopy =
        min(
            sizeof(Ext2Inode),
            static_cast<size_t>(inodeSize)
        );

    memcpy(
        buffer.data(),
        &inode,
        bytesToCopy
    );

    // Write the complete inode entry.
    image.clear();

    image.seekp(offset, ios::beg);

    if (!image)
    {
        return false;
    }

    image.write(
        reinterpret_cast<const char*>(buffer.data()),
        inodeSize
    );

    image.flush();

    return static_cast<bool>(image);
}


// ============================================================
// FIND FREE BLOCK
// ============================================================

int32_t FileSystem::findFreeBlock()
{
    if (!isOpen())
    {
        return -1;
    }

    for (uint32_t group = 0;
         group < groupDescriptors.size();
         group++)
    {
        if (groupDescriptors[group].free_blocks_count == 0)
        {
            continue;
        }

        uint32_t bitmapBlock =
            groupDescriptors[group].block_bitmap;

        vector<uint8_t> bitmap;

        if (!readBlock(bitmapBlock, bitmap))
        {
            return -1;
        }

        uint32_t blocksInGroup =
            blocksPerGroup;

        uint32_t firstBlock =
            group * blocksPerGroup;

        /*
            Last group may contain fewer blocks.
        */

        if (firstBlock + blocksInGroup >
            superblock.blocks_count)
        {
            blocksInGroup =
                superblock.blocks_count -
                firstBlock;
        }

        for (uint32_t bit = 0;
             bit < blocksInGroup;
             bit++)
        {
            uint32_t byteIndex =
                bit / 8;

            uint32_t bitIndex =
                bit % 8;

            if (byteIndex >= bitmap.size())
            {
                break;
            }

            bool used =
                bitmap[byteIndex] &
                (1U << bitIndex);

            if (!used)
            {
                uint32_t blockNumber =
                    firstBlock + bit;

                /*
                    Block zero is not a valid data block
                    and we should not allocate blocks
                    before the filesystem's first data block.
                */

                if (blockNumber <
                    superblock.first_data_block)
                {
                    continue;
                }

                return static_cast<int32_t>(
                    blockNumber
                );
            }
        }
    }

    return -1;
}


// ============================================================
// ALLOCATE BLOCK
// ============================================================

bool FileSystem::allocateBlock(
    uint32_t& blockNumber
)
{
    int32_t freeBlock =
        findFreeBlock();

    if (freeBlock < 0)
    {
        return false;
    }

    blockNumber =
        static_cast<uint32_t>(freeBlock);

    uint32_t group =
        blockNumber / blocksPerGroup;

    uint32_t localBlock =
        blockNumber % blocksPerGroup;

    if (group >= groupDescriptors.size())
    {
        return false;
    }

    uint32_t bitmapBlock =
        groupDescriptors[group].block_bitmap;

    vector<uint8_t> bitmap;

    if (!readBlock(bitmapBlock, bitmap))
    {
        return false;
    }

    uint32_t byteIndex =
        localBlock / 8;

    uint32_t bitIndex =
        localBlock % 8;

    if (byteIndex >= bitmap.size())
    {
        return false;
    }

    // Mark block as used.
    bitmap[byteIndex] |=
        (1U << bitIndex);

    if (!writeBlock(bitmapBlock, bitmap))
    {
        return false;
    }

    // Update group free block count.
    if (groupDescriptors[group].free_blocks_count > 0)
    {
        groupDescriptors[group].free_blocks_count--;
    }

    // Update superblock free block count.
    if (superblock.free_blocks_count > 0)
    {
        superblock.free_blocks_count--;
    }

    // Write updated group descriptor.
    uint32_t descriptorBlock;

    if (blockSize == 1024)
    {
        descriptorBlock = 2;
    }
    else
    {
        descriptorBlock = 1;
    }

    uint64_t descriptorOffset =
        getBlockOffset(descriptorBlock) +
        static_cast<uint64_t>(group) *
        sizeof(Ext2BlockGroupDescriptor);

    image.clear();

    image.seekp(
        descriptorOffset,
        ios::beg
    );

    if (!image)
    {
        return false;
    }

    image.write(
        reinterpret_cast<const char*>(
            &groupDescriptors[group]
        ),
        sizeof(Ext2BlockGroupDescriptor)
    );

    image.flush();

    if (!image)
    {
        return false;
    }

    // Rewrite the complete superblock.
    image.clear();

    image.seekp(1024, ios::beg);

    if (!image)
    {
        return false;
    }

    image.write(
        reinterpret_cast<const char*>(&superblock),
        sizeof(Ext2Superblock)
    );

    image.flush();

    if (!image)
    {
        return false;
    }

    // Clear the newly allocated block.
    vector<uint8_t> emptyBlock(
        blockSize,
        0
    );

    if (!writeBlock(
            blockNumber,
            emptyBlock))
    {
        return false;
    }

    return true;
}


// ============================================================
// FREE BLOCK
// ============================================================

bool FileSystem::freeBlock(
    uint32_t blockNumber
)
{
    if (!isOpen())
    {
        return false;
    }

    if (blockNumber >=
        superblock.blocks_count)
    {
        return false;
    }

    if (blockNumber <
        superblock.first_data_block)
    {
        return false;
    }

    uint32_t group =
        blockNumber / blocksPerGroup;

    uint32_t localBlock =
        blockNumber % blocksPerGroup;

    if (group >= groupDescriptors.size())
    {
        return false;
    }

    uint32_t bitmapBlock =
        groupDescriptors[group].block_bitmap;

    vector<uint8_t> bitmap;

    if (!readBlock(bitmapBlock, bitmap))
    {
        return false;
    }

    uint32_t byteIndex =
        localBlock / 8;

    uint32_t bitIndex =
        localBlock % 8;

    if (byteIndex >= bitmap.size())
    {
        return false;
    }

    bool currentlyUsed =
        bitmap[byteIndex] &
        (1U << bitIndex);

    if (!currentlyUsed)
    {
        return false;
    }

    // Mark block free.
    bitmap[byteIndex] &=
        ~(1U << bitIndex);

    if (!writeBlock(bitmapBlock, bitmap))
    {
        return false;
    }

    groupDescriptors[group].free_blocks_count++;

    superblock.free_blocks_count++;

    // Rewrite group descriptor.
    uint32_t descriptorBlock;

    if (blockSize == 1024)
    {
        descriptorBlock = 2;
    }
    else
    {
        descriptorBlock = 1;
    }

    uint64_t descriptorOffset =
        getBlockOffset(descriptorBlock) +
        static_cast<uint64_t>(group) *
        sizeof(Ext2BlockGroupDescriptor);

    image.clear();

    image.seekp(
        descriptorOffset,
        ios::beg
    );

    if (!image)
    {
        return false;
    }

    image.write(
        reinterpret_cast<const char*>(
            &groupDescriptors[group]
        ),
        sizeof(Ext2BlockGroupDescriptor)
    );

    image.flush();

    if (!image)
    {
        return false;
    }

    // Rewrite superblock.
    image.clear();

    image.seekp(1024, ios::beg);

    if (!image)
    {
        return false;
    }

    image.write(
        reinterpret_cast<const char*>(&superblock),
        sizeof(Ext2Superblock)
    );

    image.flush();

    return static_cast<bool>(image);
}


// ============================================================
// GETTERS
// ============================================================

uint32_t FileSystem::getBlockSize() const
{
    return blockSize;
}


uint32_t FileSystem::getInodeSize() const
{
    return inodeSize;
}


uint32_t FileSystem::getInodesPerGroup() const
{
    return inodesPerGroup;
}


uint32_t FileSystem::getBlocksPerGroup() const
{
    return blocksPerGroup;
}


uint32_t FileSystem::getBlockCount() const
{
    return superblock.blocks_count;
}


uint32_t FileSystem::getInodeCount() const
{
    return superblock.inodes_count;
}


// ============================================================
// GET SUPERBLOCK
// ============================================================

const Ext2Superblock&
FileSystem::getSuperblock() const
{
    return superblock;
}


// ============================================================
// GET GROUP DESCRIPTORS
// ============================================================

const vector<Ext2BlockGroupDescriptor>&
FileSystem::getGroupDescriptors() const
{
    return groupDescriptors;
}