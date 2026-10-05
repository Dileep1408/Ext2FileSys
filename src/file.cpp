#include "../include/file.h"

#include <cstring>
#include <algorithm>
#include <iomanip>

using namespace std;


// ============================================================
// CONSTRUCTOR
// ============================================================

File::File(FileSystem& fs)
    : fileSystem(fs)
{
}


// ============================================================
// READ SINGLE INDIRECT BLOCK
// ============================================================

bool File::readSingleIndirect(
    uint32_t indirectBlock,
    vector<uint32_t>& dataBlocks
)
{
    if (indirectBlock == 0)
    {
        return true;
    }

    vector<uint8_t> buffer;

    if (!fileSystem.readBlock(
            indirectBlock,
            buffer))
    {
        return false;
    }

    uint32_t pointersPerBlock =
        fileSystem.getBlockSize() /
        sizeof(uint32_t);

    for (uint32_t i = 0;
         i < pointersPerBlock;
         i++)
    {
        uint32_t blockNumber;

        memcpy(
            &blockNumber,
            buffer.data() +
            i * sizeof(uint32_t),
            sizeof(uint32_t)
        );

        if (blockNumber == 0)
        {
            break;
        }

        dataBlocks.push_back(blockNumber);
    }

    return true;
}


// ============================================================
// READ DOUBLE INDIRECT BLOCK
// ============================================================

bool File::readDoubleIndirect(
    uint32_t indirectBlock,
    vector<uint32_t>& dataBlocks
)
{
    if (indirectBlock == 0)
    {
        return true;
    }

    vector<uint8_t> firstLevel;

    if (!fileSystem.readBlock(
            indirectBlock,
            firstLevel))
    {
        return false;
    }

    uint32_t pointersPerBlock =
        fileSystem.getBlockSize() /
        sizeof(uint32_t);

    for (uint32_t i = 0;
         i < pointersPerBlock;
         i++)
    {
        uint32_t secondLevelBlock;

        memcpy(
            &secondLevelBlock,
            firstLevel.data() +
            i * sizeof(uint32_t),
            sizeof(uint32_t)
        );

        if (secondLevelBlock == 0)
        {
            break;
        }

        vector<uint8_t> secondLevel;

        if (!fileSystem.readBlock(
                secondLevelBlock,
                secondLevel))
        {
            return false;
        }

        for (uint32_t j = 0;
             j < pointersPerBlock;
             j++)
        {
            uint32_t dataBlock;

            memcpy(
                &dataBlock,
                secondLevel.data() +
                j * sizeof(uint32_t),
                sizeof(uint32_t)
            );

            if (dataBlock == 0)
            {
                break;
            }

            dataBlocks.push_back(dataBlock);
        }
    }

    return true;
}


// ============================================================
// READ TRIPLE INDIRECT BLOCK
// ============================================================

bool File::readTripleIndirect(
    uint32_t indirectBlock,
    vector<uint32_t>& dataBlocks
)
{
    if (indirectBlock == 0)
    {
        return true;
    }

    uint32_t pointersPerBlock =
        fileSystem.getBlockSize() /
        sizeof(uint32_t);

    vector<uint8_t> firstLevel;

    if (!fileSystem.readBlock(
            indirectBlock,
            firstLevel))
    {
        return false;
    }

    for (uint32_t i = 0;
         i < pointersPerBlock;
         i++)
    {
        uint32_t secondLevelBlock;

        memcpy(
            &secondLevelBlock,
            firstLevel.data() +
            i * sizeof(uint32_t),
            sizeof(uint32_t)
        );

        if (secondLevelBlock == 0)
        {
            break;
        }

        vector<uint8_t> secondLevel;

        if (!fileSystem.readBlock(
                secondLevelBlock,
                secondLevel))
        {
            return false;
        }

        for (uint32_t j = 0;
             j < pointersPerBlock;
             j++)
        {
            uint32_t thirdLevelBlock;

            memcpy(
                &thirdLevelBlock,
                secondLevel.data() +
                j * sizeof(uint32_t),
                sizeof(uint32_t)
            );

            if (thirdLevelBlock == 0)
            {
                break;
            }

            vector<uint8_t> thirdLevel;

            if (!fileSystem.readBlock(
                    thirdLevelBlock,
                    thirdLevel))
            {
                return false;
            }

            for (uint32_t k = 0;
                 k < pointersPerBlock;
                 k++)
            {
                uint32_t dataBlock;

                memcpy(
                    &dataBlock,
                    thirdLevel.data() +
                    k * sizeof(uint32_t),
                    sizeof(uint32_t)
                );

                if (dataBlock == 0)
                {
                    break;
                }

                dataBlocks.push_back(dataBlock);
            }
        }
    }

    return true;
}


// ============================================================
// GET ALL DATA BLOCKS OF A FILE
// ============================================================

bool File::getDataBlocks(
    const Ext2Inode& inode,
    vector<uint32_t>& dataBlocks
)
{
    dataBlocks.clear();

    uint32_t blockSize =
        fileSystem.getBlockSize();

    uint32_t requiredBlocks = 0;

    if (inode.size > 0)
    {
        requiredBlocks =
            (
                inode.size +
                blockSize -
                1
            ) / blockSize;
    }

    if (requiredBlocks == 0)
    {
        return true;
    }


    // ========================================================
    // DIRECT BLOCKS
    // ========================================================

    for (int i = 0;
         i < 12 &&
         dataBlocks.size() < requiredBlocks;
         i++)
    {
        if (inode.block[i] != 0)
        {
            dataBlocks.push_back(
                inode.block[i]
            );
        }
    }


    // ========================================================
    // SINGLE INDIRECT
    // ========================================================

    if (dataBlocks.size() < requiredBlocks)
    {
        if (!readSingleIndirect(
                inode.block[12],
                dataBlocks))
        {
            return false;
        }
    }


    // ========================================================
    // DOUBLE INDIRECT
    // ========================================================

    if (dataBlocks.size() < requiredBlocks)
    {
        if (!readDoubleIndirect(
                inode.block[13],
                dataBlocks))
        {
            return false;
        }
    }


    // ========================================================
    // TRIPLE INDIRECT
    // ========================================================

    if (dataBlocks.size() < requiredBlocks)
    {
        if (!readTripleIndirect(
                inode.block[14],
                dataBlocks))
        {
            return false;
        }
    }


    // We must have enough blocks.

    if (dataBlocks.size() < requiredBlocks)
    {
        return false;
    }


    // Only return blocks actually needed.

    dataBlocks.resize(requiredBlocks);

    return true;
}


// ============================================================
// FIND ENTRY INSIDE DIRECTORY
// ============================================================

bool File::findEntryInDirectory(
    uint32_t directoryInodeNumber,
    const string& name,
    uint32_t& inodeNumber
)
{
    Ext2Inode directoryInode;

    if (!fileSystem.readInode(
            directoryInodeNumber,
            directoryInode))
    {
        return false;
    }

    // Check that inode is a directory.

    if ((directoryInode.mode & 0xF000) != 0x4000)
    {
        return false;
    }

    vector<uint32_t> dataBlocks;

    if (!getDataBlocks(
            directoryInode,
            dataBlocks))
    {
        return false;
    }

    uint32_t blockSize =
        fileSystem.getBlockSize();

    for (uint32_t blockNumber : dataBlocks)
    {
        vector<uint8_t> buffer;

        if (!fileSystem.readBlock(
                blockNumber,
                buffer))
        {
            return false;
        }

        uint32_t position = 0;

        while (position + 8 <= blockSize)
        {
            uint32_t entryInode;

            uint16_t recLength;

            uint8_t nameLength;

            uint8_t fileType;

            memcpy(
                &entryInode,
                buffer.data() + position,
                sizeof(uint32_t)
            );

            memcpy(
                &recLength,
                buffer.data() +
                position + 4,
                sizeof(uint16_t)
            );

            nameLength =
                buffer[position + 6];

            fileType =
                buffer[position + 7];

            (void)fileType;

            if (recLength < 8)
            {
                return false;
            }

            if (position + recLength >
                blockSize)
            {
                return false;
            }

            if (nameLength >
                recLength - 8)
            {
                return false;
            }

            if (entryInode != 0)
            {
                string entryName;

                for (uint32_t i = 0;
                     i < nameLength;
                     i++)
                {
                    entryName +=
                        static_cast<char>(
                            buffer[
                                position +
                                8 +
                                i
                            ]
                        );
                }

                if (entryName == name)
                {
                    inodeNumber =
                        entryInode;

                    return true;
                }
            }

            position += recLength;
        }
    }

    return false;
}


// ============================================================
// FIND INODE FROM PATH
// ============================================================

bool File::findInode(
    const string& path,
    uint32_t& inodeNumber
)
{
    if (path.empty())
    {
        return false;
    }


    /*
        Root directory is inode 2.
    */

    uint32_t currentInode = 2;


    /*
        If path is simply "/",
        return root inode.
    */

    if (path == "/")
    {
        inodeNumber = 2;
        return true;
    }

    size_t start = 0;


    /*
        Ignore the first '/'.
    */

    if (path[0] == '/')
    {
        start = 1;
    }

    while (start < path.size())
    {
        size_t slash =
            path.find('/', start);

        string component;

        if (slash == string::npos)
        {
            component =
                path.substr(start);

            start = path.size();
        }
        else
        {
            component =
                path.substr(
                    start,
                    slash - start
                );

            start = slash + 1;
        }

        if (component.empty())
        {
            continue;
        }

        uint32_t nextInode;

        if (!findEntryInDirectory(
                currentInode,
                component,
                nextInode))
        {
            return false;
        }

        currentInode = nextInode;
    }

    inodeNumber = currentInode;

    return true;
}


// ============================================================
// ALLOCATE DATA BLOCKS
// ============================================================

bool File::allocateDataBlocks(
    Ext2Inode& inode,
    uint32_t requiredBlocks,
    vector<uint32_t>& dataBlocks
)
{
    uint32_t blockSize =
        fileSystem.getBlockSize();

    uint32_t pointersPerBlock =
        blockSize / sizeof(uint32_t);


    /*
        We only need direct + single + double
        for this filesystem.

        12 direct blocks
        +
        256 single-indirect blocks
        +
        256 * 256 double-indirect blocks

        = 65,804 data blocks

        Your filesystem has only 12,288 blocks.
    */

    uint32_t maxBlocks =
        12 +
        pointersPerBlock +
        pointersPerBlock *
        pointersPerBlock;

    if (requiredBlocks > maxBlocks)
    {
        cerr << "File is too large for this filesystem implementation."
             << endl;

        return false;
    }


    // ========================================================
    // GET CURRENT DATA BLOCKS
    // ========================================================

    if (!getDataBlocks(
            inode,
            dataBlocks))
    {
        return false;
    }

    uint32_t currentBlocks =
        static_cast<uint32_t>(
            dataBlocks.size()
        );

    if (currentBlocks >= requiredBlocks)
    {
        return true;
    }


    // ========================================================
    // DIRECT BLOCKS
    // ========================================================

    while (currentBlocks < requiredBlocks &&
           currentBlocks < 12)
    {
        uint32_t newBlock;

        if (!fileSystem.allocateBlock(
                newBlock))
        {
            return false;
        }

        inode.block[currentBlocks] =
            newBlock;

        dataBlocks.push_back(newBlock);

        currentBlocks++;
    }


    // ========================================================
    // SINGLE INDIRECT
    // ========================================================

    if (currentBlocks < requiredBlocks)
    {
        /*
            Allocate the single-indirect metadata block
            if it does not exist.
        */

        if (inode.block[12] == 0)
        {
            uint32_t indirectBlock;

            if (!fileSystem.allocateBlock(
                    indirectBlock))
            {
                return false;
            }

            inode.block[12] =
                indirectBlock;
        }

        vector<uint8_t> buffer;

        if (!fileSystem.readBlock(
                inode.block[12],
                buffer))
        {
            return false;
        }

        while (currentBlocks < requiredBlocks &&
               currentBlocks < 12 + pointersPerBlock)
        {
            uint32_t index =
                currentBlocks - 12;

            uint32_t existingBlock;

            memcpy(
                &existingBlock,
                buffer.data() +
                index * sizeof(uint32_t),
                sizeof(uint32_t)
            );

            if (existingBlock == 0)
            {
                uint32_t newBlock;

                if (!fileSystem.allocateBlock(
                        newBlock))
                {
                    return false;
                }

                memcpy(
                    buffer.data() +
                    index * sizeof(uint32_t),
                    &newBlock,
                    sizeof(uint32_t)
                );

                dataBlocks.push_back(
                    newBlock
                );
            }
            else
            {
                dataBlocks.push_back(
                    existingBlock
                );
            }

            currentBlocks++;
        }

        if (!fileSystem.writeBlock(
                inode.block[12],
                buffer))
        {
            return false;
        }
    }


    // ========================================================
    // DOUBLE INDIRECT
    // ========================================================

    if (currentBlocks < requiredBlocks)
    {
        /*
            Allocate double-indirect root block.
        */

        if (inode.block[13] == 0)
        {
            uint32_t doubleBlock;

            if (!fileSystem.allocateBlock(
                    doubleBlock))
            {
                return false;
            }

            inode.block[13] =
                doubleBlock;
        }

        vector<uint8_t> firstLevel;

        if (!fileSystem.readBlock(
                inode.block[13],
                firstLevel))
        {
            return false;
        }

        uint32_t doubleDataStart =
            12 + pointersPerBlock;

        while (currentBlocks < requiredBlocks)
        {
            uint32_t relative =
                currentBlocks -
                doubleDataStart;

            uint32_t firstIndex =
                relative / pointersPerBlock;

            uint32_t secondIndex =
                relative % pointersPerBlock;

            if (firstIndex >= pointersPerBlock)
            {
                return false;
            }

            uint32_t secondLevelBlock;

            memcpy(
                &secondLevelBlock,
                firstLevel.data() +
                firstIndex *
                sizeof(uint32_t),
                sizeof(uint32_t)
            );


            /*
                Create second-level indirect block
                if necessary.
            */

            if (secondLevelBlock == 0)
            {
                if (!fileSystem.allocateBlock(
                        secondLevelBlock))
                {
                    return false;
                }

                memcpy(
                    firstLevel.data() +
                    firstIndex *
                    sizeof(uint32_t),
                    &secondLevelBlock,
                    sizeof(uint32_t)
                );

                vector<uint8_t> emptyBlock(
                    blockSize,
                    0
                );

                if (!fileSystem.writeBlock(
                        secondLevelBlock,
                        emptyBlock))
                {
                    return false;
                }
            }

            vector<uint8_t> secondLevel;

            if (!fileSystem.readBlock(
                    secondLevelBlock,
                    secondLevel))
            {
                return false;
            }

            uint32_t dataBlock;

            memcpy(
                &dataBlock,
                secondLevel.data() +
                secondIndex *
                sizeof(uint32_t),
                sizeof(uint32_t)
            );

            if (dataBlock == 0)
            {
                if (!fileSystem.allocateBlock(
                        dataBlock))
                {
                    return false;
                }

                memcpy(
                    secondLevel.data() +
                    secondIndex *
                    sizeof(uint32_t),
                    &dataBlock,
                    sizeof(uint32_t)
                );

                dataBlocks.push_back(
                    dataBlock
                );
            }
            else
            {
                dataBlocks.push_back(
                    dataBlock
                );
            }

            if (!fileSystem.writeBlock(
                    secondLevelBlock,
                    secondLevel))
            {
                return false;
            }

            currentBlocks++;
        }

        if (!fileSystem.writeBlock(
                inode.block[13],
                firstLevel))
        {
            return false;
        }
    }


    // ========================================================
    // FINAL CHECK
    // ========================================================

    if (dataBlocks.size() <
        requiredBlocks)
    {
        return false;
    }

    return true;
}


// ============================================================
// DISPLAY FILE
// ============================================================

bool File::displayFile(
    const string& path
)
{
    uint32_t inodeNumber;

    // Find the inode corresponding to the path.
    if (!findInode(
            path,
            inodeNumber))
    {
        cout << "File not found: "
             << path
             << endl;

        return false;
    }

    Ext2Inode inode;

    // Read the inode.
    if (!fileSystem.readInode(
            inodeNumber,
            inode))
    {
        return false;
    }

    /*
        Check that it is a regular file.

        0x8000 = regular file
    */

    if ((inode.mode & 0xF000) != 0x8000)
    {
        cout << "The specified path is not a regular file."
             << endl;

        return false;
    }

    vector<uint32_t> dataBlocks;

    // Get all data blocks belonging to the file.
    if (!getDataBlocks(
            inode,
            dataBlocks))
    {
        return false;
    }

    cout << endl;
    cout << "--------------- FILE INFORMATION ---------------"
         << endl;

    cout << "Path       : " << path << endl;
    cout << "Inode      : " << inodeNumber << endl;
    cout << "File size  : " << inode.size << " bytes" << endl;
    cout << "Data blocks: " << dataBlocks.size() << endl;

    cout << "-------------------------------------------------"
         << endl;

    // Empty file.
    if (inode.size == 0)
    {
        cout << "[Empty file]" << endl;

        cout << "-------------------------------------------------"
             << endl;

        return true;
    }


    /*
        Determine whether the file is text or binary.

        We inspect up to the first 512 bytes.

        Normal printable ASCII characters and common
        whitespace characters are considered text.

        Binary/control bytes cause hexadecimal output.
    */

    bool isText = true;

    uint32_t bytesChecked = 0;

    for (uint32_t blockNumber : dataBlocks)
    {
        if (bytesChecked >= inode.size ||
            bytesChecked >= 512)
        {
            break;
        }

        vector<uint8_t> buffer;

        if (!fileSystem.readBlock(
                blockNumber,
                buffer))
        {
            return false;
        }

        uint32_t bytesToCheck =
            min(
                static_cast<uint32_t>(
                    buffer.size()
                ),
                inode.size - bytesChecked
            );

        bytesToCheck =
            min(
                bytesToCheck,
                512u - bytesChecked
            );

        for (uint32_t i = 0;
             i < bytesToCheck;
             i++)
        {
            uint8_t byte = buffer[i];

            if (!(
                byte == '\n' ||
                byte == '\r' ||
                byte == '\t' ||
                (byte >= 32 && byte <= 126)
            ))
            {
                isText = false;
                break;
            }
        }

        bytesChecked += bytesToCheck;

        if (!isText)
        {
            break;
        }
    }


    // ========================================================
    // TEXT FILE
    // ========================================================

    if (isText)
    {
        cout << endl;
        cout << "--------------- FILE CONTENT ---------------"
             << endl;

        uint32_t remaining =
            inode.size;

        for (uint32_t blockNumber : dataBlocks)
        {
            if (remaining == 0)
            {
                break;
            }

            vector<uint8_t> buffer;

            if (!fileSystem.readBlock(
                    blockNumber,
                    buffer))
            {
                return false;
            }

            uint32_t bytesToPrint =
                min(
                    remaining,
                    fileSystem.getBlockSize()
                );

            for (uint32_t i = 0;
                 i < bytesToPrint;
                 i++)
            {
                cout << static_cast<char>(
                    buffer[i]
                );
            }

            remaining -= bytesToPrint;
        }

        cout << endl;

        cout << "---------------------------------------------"
             << endl;
    }


    // ========================================================
    // BINARY FILE
    // ========================================================

    else
    {
        cout << endl;
        cout << "----------- BINARY FILE CONTENT -------------"
             << endl;

        cout << "Displaying bytes in hexadecimal:"
             << endl
             << endl;

        uint32_t remaining =
            inode.size;

        uint32_t offset = 0;

        for (uint32_t blockNumber : dataBlocks)
        {
            if (remaining == 0)
            {
                break;
            }

            vector<uint8_t> buffer;

            if (!fileSystem.readBlock(
                    blockNumber,
                    buffer))
            {
                return false;
            }

            uint32_t bytesToPrint =
                min(
                    remaining,
                    fileSystem.getBlockSize()
                );

            for (uint32_t i = 0;
                 i < bytesToPrint;
                 i++)
            {
                if (offset % 16 == 0)
                {
                    cout << hex
                         << setw(8)
                         << setfill('0')
                         << offset
                         << "  ";
                }

                cout << hex
                     << setw(2)
                     << setfill('0')
                     << static_cast<int>(
                            buffer[i]
                        )
                     << " ";

                if (offset % 16 == 15)
                {
                    cout << endl;
                }

                offset++;
            }

            remaining -= bytesToPrint;
        }

        if (offset % 16 != 0)
        {
            cout << endl;
        }

        cout << dec;
        cout << setfill(' ');

        cout << "---------------------------------------------"
             << endl;
    }

    return true;
}


// ============================================================
// OVERWRITE FILE
// ============================================================

bool File::overwriteFile(
    const string& path,
    const string& content
)
{
    uint32_t inodeNumber;

    if (!findInode(
            path,
            inodeNumber))
    {
        cout << "File not found: "
             << path
             << endl;

        return false;
    }

    Ext2Inode inode;

    if (!fileSystem.readInode(
            inodeNumber,
            inode))
    {
        return false;
    }

    if ((inode.mode & 0xF000) != 0x8000)
    {
        cout << "The specified path is not a regular file."
             << endl;

        return false;
    }

    uint32_t blockSize =
        fileSystem.getBlockSize();

    uint32_t requiredBlocks = 0;

    if (!content.empty())
    {
        requiredBlocks =
            (
                static_cast<uint32_t>(
                    content.size()
                ) +
                blockSize -
                1
            ) / blockSize;
    }

    vector<uint32_t> dataBlocks;

    /*
        Temporarily set size to the new size so
        getDataBlocks() calculates blocks according
        to the new file size.

        We still preserve existing inode pointers.
    */

    uint32_t oldSize =
        inode.size;

    inode.size =
        static_cast<uint32_t>(
            content.size()
        );

    /*
        For allocation, use the old size first if
        existing blocks need to be reused.
    */

    inode.size = oldSize;

    if (!allocateDataBlocks(
            inode,
            requiredBlocks,
            dataBlocks))
    {
        return false;
    }


    // ========================================================
    // WRITE CONTENT
    // ========================================================

    size_t position = 0;

    for (uint32_t blockNumber : dataBlocks)
    {
        vector<uint8_t> buffer(
            blockSize,
            0
        );

        uint32_t bytesToWrite =
            min(
                static_cast<size_t>(blockSize),
                content.size() - position
            );

        if (bytesToWrite > 0)
        {
            memcpy(
                buffer.data(),
                content.data() + position,
                bytesToWrite
            );
        }

        if (!fileSystem.writeBlock(
                blockNumber,
                buffer))
        {
            return false;
        }

        position += bytesToWrite;

        if (position >= content.size())
        {
            break;
        }
    }


    // ========================================================
    // UPDATE INODE SIZE
    // ========================================================

    inode.size =
        static_cast<uint32_t>(
            content.size()
        );


    // ========================================================
    // UPDATE i_blocks
    // ========================================================

    /*
        i_blocks counts 512-byte sectors.

        We calculate the number of filesystem blocks
        currently required by the file plus the
        indirect metadata blocks required by those
        data blocks.
    */

    uint32_t totalAllocatedBlocks =
        requiredBlocks;

    uint32_t pointersPerBlock =
        blockSize / sizeof(uint32_t);

    if (requiredBlocks > 12)
    {
        // Single-indirect metadata block.
        totalAllocatedBlocks++;

        uint32_t remaining =
            requiredBlocks - 12;

        if (remaining > pointersPerBlock)
        {
            remaining -= pointersPerBlock;

            // Double-indirect root.
            totalAllocatedBlocks++;

            // Second-level blocks.
            totalAllocatedBlocks +=
                (
                    remaining +
                    pointersPerBlock -
                    1
                ) / pointersPerBlock;
        }
    }

    inode.blocks =
        totalAllocatedBlocks *
        (blockSize / 512);


    // ========================================================
    // WRITE INODE
    // ========================================================

    if (!fileSystem.writeInode(
            inodeNumber,
            inode))
    {
        return false;
    }

    return true;
}


// ============================================================
// APPEND TO FILE
// ============================================================

bool File::appendToFile(
    const string& path,
    const string& content
)
{
    uint32_t inodeNumber;

    if (!findInode(
            path,
            inodeNumber))
    {
        cout << "File not found: "
             << path
             << endl;

        return false;
    }

    Ext2Inode inode;

    if (!fileSystem.readInode(
            inodeNumber,
            inode))
    {
        return false;
    }

    if ((inode.mode & 0xF000) != 0x8000)
    {
        cout << "The specified path is not a regular file."
             << endl;

        return false;
    }

    if (content.empty())
    {
        return true;
    }

    uint32_t blockSize =
        fileSystem.getBlockSize();

    uint32_t oldSize =
        inode.size;

    uint32_t newSize =
        oldSize +
        static_cast<uint32_t>(
            content.size()
        );

    uint32_t requiredBlocks =
        (
            newSize +
            blockSize -
            1
        ) / blockSize;

    vector<uint32_t> dataBlocks;

    if (!getDataBlocks(
            inode,
            dataBlocks))
    {
        return false;
    }


    /*
        Allocate additional blocks if required.
    */

    if (dataBlocks.size() <
        requiredBlocks)
    {
        if (!allocateDataBlocks(
                inode,
                requiredBlocks,
                dataBlocks))
        {
            return false;
        }
    }


    /*
        Write starting at the old file size.
    */

    size_t contentPosition = 0;

    uint32_t filePosition = oldSize;

    while (contentPosition <
           content.size())
    {
        uint32_t blockIndex =
            filePosition /
            blockSize;

        uint32_t offset =
            filePosition %
            blockSize;

        if (blockIndex >=
            dataBlocks.size())
        {
            return false;
        }

        uint32_t blockNumber =
            dataBlocks[blockIndex];

        vector<uint8_t> buffer;

        if (!fileSystem.readBlock(
                blockNumber,
                buffer))
        {
            return false;
        }

        uint32_t available =
            blockSize - offset;

        uint32_t remaining =
            static_cast<uint32_t>(
                content.size() -
                contentPosition
            );

        uint32_t bytesToWrite =
            min(
                available,
                remaining
            );

        memcpy(
            buffer.data() + offset,
            content.data() +
            contentPosition,
            bytesToWrite
        );

        if (!fileSystem.writeBlock(
                blockNumber,
                buffer))
        {
            return false;
        }

        filePosition += bytesToWrite;

        contentPosition +=
            bytesToWrite;
    }


    // ========================================================
    // UPDATE FILE SIZE
    // ========================================================

    inode.size = newSize;


    // ========================================================
    // UPDATE i_blocks
    // ========================================================

    uint32_t totalAllocatedBlocks =
        requiredBlocks;

    uint32_t pointersPerBlock =
        blockSize / sizeof(uint32_t);

    if (requiredBlocks > 12)
    {
        // Single-indirect metadata block.
        totalAllocatedBlocks++;

        uint32_t remaining =
            requiredBlocks - 12;

        if (remaining > pointersPerBlock)
        {
            remaining -= pointersPerBlock;

            // Double-indirect root.
            totalAllocatedBlocks++;

            // Double-indirect second-level blocks.
            totalAllocatedBlocks +=
                (
                    remaining +
                    pointersPerBlock -
                    1
                ) / pointersPerBlock;
        }
    }

    inode.blocks =
        totalAllocatedBlocks *
        (blockSize / 512);


    // ========================================================
    // WRITE UPDATED INODE
    // ========================================================

    if (!fileSystem.writeInode(
            inodeNumber,
            inode))
    {
        return false;
    }

    return true;
}