#include "../include/directory.h"

#include <cstring>
#include <algorithm>

using namespace std;


// ============================================================
// CONSTRUCTOR
// ============================================================

Directory::Directory(FileSystem& fs)
    : fileSystem(fs)
{
}


// ============================================================
// PRINT DIRECTORY ENTRY
// ============================================================

void Directory::printEntry(
    uint32_t inodeNumber,
    const string& name,
    uint8_t fileType,
    const string& path)
{
    if (fileType == 2)
    {
        cout << "[DIR ] "
             << path
             << name
             << "/"
             << "  (inode "
             << inodeNumber
             << ")"
             << '\n';
    }
    else if (fileType == 1)
    {
        cout << "[FILE] "
             << path
             << name
             << "  (inode "
             << inodeNumber
             << ")"
             << '\n';
    }
    else
    {
        cout << "[OTHER] "
             << path
             << name
             << "  (inode "
             << inodeNumber
             << ")"
             << '\n';
    }
}


// ============================================================
// READ ONE DIRECTORY DATA BLOCK
// ============================================================

bool Directory::readDirectoryBlock(
    uint32_t blockNumber,
    uint32_t directoryInodeNumber,
    const string& currentPath,
    vector<uint32_t>& visitedInodes)
{
    vector<uint8_t> buffer;


    // --------------------------------------------------------
    // Read the filesystem block
    // --------------------------------------------------------

    if (!fileSystem.readBlock(blockNumber, buffer))
    {
        cerr << "Failed to read directory block "
             << blockNumber
             << '\n';

        return false;
    }


    // --------------------------------------------------------
    // Current position inside the block
    // --------------------------------------------------------

    uint32_t position = 0;

    uint32_t blockSize =
        fileSystem.getBlockSize();


    // --------------------------------------------------------
    // Parse every directory entry in the block
    // --------------------------------------------------------

    while (position < blockSize)
    {
        // ----------------------------------------------------
        // Make sure enough bytes remain for the fixed header
        // ----------------------------------------------------

        if (position + 8 > blockSize)
        {
            break;
        }


        // ----------------------------------------------------
        // Read directory entry header
        // ----------------------------------------------------

        Ext2DirectoryEntry entry;


        // inode number
        memcpy(
            &entry.inode,
            buffer.data() + position,
            sizeof(uint32_t)
        );


        // record length
        memcpy(
            &entry.rec_len,
            buffer.data() + position + 4,
            sizeof(uint16_t)
        );


        // filename length
        entry.name_len =
            buffer[position + 6];


        // file type
        entry.file_type =
            buffer[position + 7];


        // ----------------------------------------------------
        // Validate record length
        // ----------------------------------------------------

        if (entry.rec_len < 8)
        {
            cerr << "Invalid directory entry record length\n";
            return false;
        }


        if (position + entry.rec_len > blockSize)
        {
            cerr << "Directory entry extends beyond block\n";
            return false;
        }


        // ----------------------------------------------------
        // Validate filename length
        // ----------------------------------------------------

        if (entry.name_len > entry.rec_len - 8)
        {
            cerr << "Invalid directory entry name length\n";
            return false;
        }


        // ----------------------------------------------------
        // Read filename
        // ----------------------------------------------------

        string name;

        for (uint32_t i = 0; i < entry.name_len; i++)
        {
            name += static_cast<char>(
                buffer[position + 8 + i]
            );
        }


        // ----------------------------------------------------
        // Process valid directory entry
        // ----------------------------------------------------

        if (entry.inode != 0)
        {
            // Ignore "." and ".."
            //
            // "."  = current directory
            // ".." = parent directory

            if (name != "." && name != "..")
            {
                // --------------------------------------------
                // Print the entry
                // --------------------------------------------

                printEntry(
                    entry.inode,
                    name,
                    entry.file_type,
                    currentPath
                );


                // --------------------------------------------
                // If this entry is a directory, recursively
                // traverse it.
                // --------------------------------------------

                if (entry.file_type == 2)
                {
                    string childPath =
                        currentPath + name + "/";


                    if (!traverseDirectory(
                            entry.inode,
                            childPath,
                            visitedInodes))
                    {
                        return false;
                    }
                }
            }
        }


        // ----------------------------------------------------
        // Move to the next directory entry
        // ----------------------------------------------------

        position += entry.rec_len;
    }


    return true;
}


// ============================================================
// RECURSIVELY TRAVERSE DIRECTORY
// ============================================================

bool Directory::traverseDirectory(
    uint32_t inodeNumber,
    const string& path,
    vector<uint32_t>& visitedInodes)
{
    // --------------------------------------------------------
    // Prevent infinite recursion
    // --------------------------------------------------------

    for (uint32_t visited : visitedInodes)
    {
        if (visited == inodeNumber)
        {
            return true;
        }
    }


    visitedInodes.push_back(inodeNumber);


    // --------------------------------------------------------
    // Read the directory's inode
    // --------------------------------------------------------

    Ext2Inode inode;

    if (!fileSystem.readInode(
            inodeNumber,
            inode))
    {
        cerr << "Failed to read inode "
             << inodeNumber
             << '\n';

        return false;
    }


    // --------------------------------------------------------
    // Make sure this inode represents a directory
    // --------------------------------------------------------

    uint16_t fileType =
        inode.mode & 0xF000;


    if (fileType != 0x4000)
    {
        cerr << "Inode "
             << inodeNumber
             << " is not a directory\n";

        return false;
    }


    // --------------------------------------------------------
    // Number of bytes occupied by the directory
    // --------------------------------------------------------

    uint32_t directorySize =
        inode.size;


    uint32_t blockSize =
        fileSystem.getBlockSize();


    // --------------------------------------------------------
    // Number of blocks required by the directory
    // --------------------------------------------------------

    uint32_t blockCount =
        (directorySize + blockSize - 1)
        / blockSize;


    // ========================================================
    // DIRECT BLOCKS
    // ========================================================
    //
    // inode.block[0] through inode.block[11]
    //
    // These directly contain directory data blocks.
    // ========================================================

    uint32_t directBlocks =
        min(blockCount, 12U);


    for (uint32_t i = 0;
         i < directBlocks;
         i++)
    {
        uint32_t blockNumber =
            inode.block[i];


        if (blockNumber == 0)
        {
            continue;
        }


        if (!readDirectoryBlock(
                blockNumber,
                inodeNumber,
                path,
                visitedInodes))
        {
            return false;
        }
    }


    // ========================================================
    // SINGLE INDIRECT BLOCK
    // ========================================================
    //
    // inode.block[12]
    //
    // This block contains an array of block numbers.
    // ========================================================

    if (blockCount > 12)
    {
        uint32_t indirectBlock =
            inode.block[12];


        if (indirectBlock != 0)
        {
            vector<uint8_t> buffer;


            if (!fileSystem.readBlock(
                    indirectBlock,
                    buffer))
            {
                return false;
            }


            uint32_t pointersPerBlock =
                blockSize / sizeof(uint32_t);


            uint32_t remainingBlocks =
                blockCount - 12;


            uint32_t count =
                min(
                    remainingBlocks,
                    pointersPerBlock
                );


            for (uint32_t i = 0;
                 i < count;
                 i++)
            {
                uint32_t dataBlock;


                memcpy(
                    &dataBlock,
                    buffer.data()
                        + i * sizeof(uint32_t),
                    sizeof(uint32_t)
                );


                if (dataBlock == 0)
                {
                    continue;
                }


                if (!readDirectoryBlock(
                        dataBlock,
                        inodeNumber,
                        path,
                        visitedInodes))
                {
                    return false;
                }
            }
        }
    }


    // ========================================================
    // DOUBLE INDIRECT BLOCK
    // ========================================================
    //
    // inode.block[13]
    //
    // First level:
    //     block numbers of indirect blocks
    //
    // Second level:
    //     actual directory data block numbers
    // ========================================================

    if (blockCount > 12)
    {
        uint32_t pointersPerBlock =
            blockSize / sizeof(uint32_t);


        uint64_t singleCapacity =
            pointersPerBlock;


        uint64_t doubleCapacity =
            singleCapacity * pointersPerBlock;


        // Only use double indirect if the directory
        // needs more blocks than direct + single indirect.

        if (blockCount >
            12 + singleCapacity)
        {
            uint32_t doubleIndirectBlock =
                inode.block[13];


            if (doubleIndirectBlock != 0)
            {
                vector<uint8_t> firstLevel;


                if (!fileSystem.readBlock(
                        doubleIndirectBlock,
                        firstLevel))
                {
                    return false;
                }


                // Number of data blocks that remain
                // after direct + single indirect.

                uint32_t remainingBlocks =
                    blockCount
                    - 12
                    - pointersPerBlock;


                uint32_t requiredFirstLevel =
                    min(
                        static_cast<uint64_t>(
                            remainingBlocks
                        ),
                        doubleCapacity
                    );


                // Each first-level block points to
                // 'pointersPerBlock' data blocks.

                uint32_t firstLevelCount =
                    (
                        requiredFirstLevel
                        + pointersPerBlock
                        - 1
                    )
                    / pointersPerBlock;


                for (uint32_t i = 0;
                     i < firstLevelCount;
                     i++)
                {
                    uint32_t secondLevelBlock;


                    memcpy(
                        &secondLevelBlock,
                        firstLevel.data()
                            + i * sizeof(uint32_t),
                        sizeof(uint32_t)
                    );


                    if (secondLevelBlock == 0)
                    {
                        continue;
                    }


                    vector<uint8_t> secondLevel;


                    if (!fileSystem.readBlock(
                            secondLevelBlock,
                            secondLevel))
                    {
                        return false;
                    }


                    uint32_t blocksFromThisLevel =
                        min(
                            pointersPerBlock,
                            requiredFirstLevel
                            - i * pointersPerBlock
                        );


                    for (uint32_t j = 0;
                         j < blocksFromThisLevel;
                         j++)
                    {
                        uint32_t dataBlock;


                        memcpy(
                            &dataBlock,
                            secondLevel.data()
                                + j * sizeof(uint32_t),
                            sizeof(uint32_t)
                        );


                        if (dataBlock == 0)
                        {
                            continue;
                        }


                        if (!readDirectoryBlock(
                                dataBlock,
                                inodeNumber,
                                path,
                                visitedInodes))
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }


    // ========================================================
    // TRIPLE INDIRECT
    // ========================================================
    //
    // inode.block[14]
    //
    // Your current filesystem has only 12,288 blocks and
    // therefore does not need triple indirect addressing.
    //
    // So we intentionally do not implement it here.
    //
    // Direct:
    //     12 blocks
    //
    // Single:
    //     256 blocks
    //
    // Double:
    //     256 * 256 = 65,536 blocks
    //
    // Total supported here:
    //     12 + 256 + 65,536
    //
    // Your filesystem:
    //     12,288 blocks
    //
    // Therefore double indirect is more than sufficient.
    // ========================================================


    return true;
}


// ============================================================
// START TRAVERSAL FROM ROOT
// ============================================================

bool Directory::traverseFromRoot()
{
    // --------------------------------------------------------
    // In ext2:
    //
    // inode 2 = root directory
    // --------------------------------------------------------

    const uint32_t rootInode = 2;


    cout << "\n";
    cout << "==============================\n";
    cout << "EXT2 FILESYSTEM LAYOUT\n";
    cout << "==============================\n";


    vector<uint32_t> visitedInodes;


    // --------------------------------------------------------
    // Print root
    // --------------------------------------------------------

    cout << "[DIR ] /  (inode 2)\n";


    // --------------------------------------------------------
    // Recursively traverse root
    // --------------------------------------------------------

    return traverseDirectory(
        rootInode,
        "/",
        visitedInodes
    );
}