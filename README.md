# EXT2 File System Explorer

A C++ project for exploring and modifying an EXT2 filesystem directly through a disk image.

Instead of using normal filesystem APIs, the program reads the disk image as raw bytes and interprets those bytes using EXT2 structures such as the superblock, block group descriptors, inodes, directory entries and data blocks.

This project is being developed as part of the WEC Systems and Security SIG recruitment task.

[▶️ Watch Demo Video](https://drive.google.com/file/d/1G-NKK6Ouivd8Dez-djP5yTO_UcjxWBYu/view?usp=sharing)

## Project Structure

    ext2file/
    │
    ├── Artifacts/
    │   ├── disk-backpup.img
    │   └── disk-backpup-original.img
    │
    ├── include/
    │   ├── structures.h
    │   ├── filesystem.h
    │   ├── directory.h
    │   └── file.h
    │
    ├── src/
    │   ├── filesystem.cpp
    │   ├── directory.cpp
    │   ├── file.cpp
    │   └── main.cpp
    │
    ├── .gitignore
    └── README.md

### Folder and File Description

**`Artifacts/`**

Contains the EXT2 disk images used while developing and testing the project.

`disk-backpup.img` is the working image used by the program. `disk-backpup-original.img` is a backup of the original image so that the filesystem can be restored if required.

The disk images are kept locally and are excluded from the Git repository.

**`include/`**

Contains the header files that define the main EXT2 structures and classes used by the project.

- **`structures.h`** — Defines the EXT2 superblock, block group descriptor and inode structures. The structures are packed so their memory layout matches the corresponding data stored in the disk image.
- **`filesystem.h`** — Defines the `FileSystem` class and its low-level operations such as opening the image, reading and writing blocks and inodes, reading filesystem metadata, and handling block allocation.
- **`directory.h`** — Defines the directory entry structure and the `Directory` class used to recursively traverse directories.
- **`file.h`** — Defines the `File` class used for path lookup, finding file data blocks, displaying files, overwriting files and appending data.

**`src/`**

Contains the actual implementation of the classes declared in the header files.

- **`filesystem.cpp`** — Implements the low-level interaction with the disk image, including superblock parsing, block group descriptor parsing, block access and inode access.
- **`directory.cpp`** — Implements recursive directory traversal starting from the root inode and follows directory entries to find files and subdirectories.
- **`file.cpp`** — Implements file lookup, data block traversal, file reading, overwrite and append operations.
- **`main.cpp`** — The entry point of the program. It opens the image, displays filesystem information, displays the directory tree and provides the interactive menu.

**`.gitignore`**

Prevents files such as compiled executables, temporary files, VS Code settings and disk images from being committed to Git.

---

## How the EXT2 Filesystem Is Being Read

The disk image is treated as a sequence of raw bytes. The program does not directly know where a particular file is.

Instead, it follows the EXT2 filesystem structures step by step:

    Disk Image
        |
        v
    Superblock
        |
        v
    Block Group Descriptors
        |
        +------> Block Bitmap
        |
        +------> Inode Bitmap
        |
        +------> Inode Table
                       |
                       v
                     Inode
                       |
                       v
                 Block Pointers
                       |
                       v
                  Data Blocks

For directories, the data blocks contain directory entries. These entries provide information such as the inode number, filename and file type.

For regular files, the inode contains pointers to the blocks containing the actual file contents.

The supplied filesystem image has:

- Block size: 1024 bytes
- Number of blocks: 12288
- Number of inodes: 3072
- Inode size: 256 bytes
- Blocks per group: 8192
- Inodes per group: 1536
- Root inode: 2

---

## Currently Working

### 1. EXT2 Image Opening

The program can open the supplied EXT2 disk image and use it as the filesystem being inspected.

The image is opened directly rather than being mounted as a normal filesystem.

### 2. Superblock Parsing

The program reads the EXT2 superblock and displays important filesystem information including:

- Total number of inodes
- Total number of blocks
- Free blocks
- Free inodes
- Block size
- Blocks per group
- Inodes per group
- Inode size
- EXT2 magic number
- Revision level
- First inode

### 3. Block Group Descriptor Parsing

The program reads the block group descriptor table and displays information about each block group.

This includes the locations of:

- Block bitmap
- Inode bitmap
- Inode table
- Free blocks
- Free inodes
- Number of directories

### 4. Inode Reading

The program can locate an inode using its inode number.

The basic process is:

    Inode Number
         |
         v
    Block Group
         |
         v
    Local Inode Number
         |
         v
    Inode Table
         |
         v
    Inode

The inode then provides information about the file or directory and its block pointers.

### 5. Directory Traversal

The program starts from the root inode, which is inode `2`, and recursively explores the filesystem.

It reads directory entries and uses their inode numbers to find the corresponding files and directories.

The traversal currently handles:

- Root directory
- Nested directories
- `.` and `..`
- Direct blocks
- Single indirect blocks
- Double indirect blocks
- Visited inode tracking to avoid unwanted loops

The result is printed as a filesystem tree.

### 6. File Lookup

Files can be located using their path.

For example:

    /readthis.txt

The program searches through directory entries until it finds the corresponding inode.

The inode can then be used to locate the actual data blocks of the file.

### 7. File Reading

Once the inode is found, the program follows its block pointers and reads the file contents from the disk image.

The current implementation supports:

- Direct blocks
- Single indirect blocks
- Double indirect blocks

Text files can be displayed directly. Binary files such as PDFs, images and videos require binary-safe handling rather than treating their contents as normal text.

### 8. File Overwrite

The program can overwrite the contents of an existing file.

The general process is:

    Existing File
         |
         v
    New Content
         |
         v
    Determine Required Blocks
         |
         v
    Write New Data
         |
         v
    Update Inode

The inode size and data block information must be updated when the file changes.

### 9. File Append

The program can append content to an existing file.

The basic process is:

    Existing File
         |
         v
    Find Current Size
         |
         v
    Find Last Data Block
         |
         v
    Use Remaining Space
         |
         v
    Allocate More Blocks If Required
         |
         v
    Update File Size

---

## Program Menu

The current program provides the following options:

    1. Display file contents
    2. Overwrite file
    3. Append to file
    4. Display filesystem information
    5. Display directory tree
    6. Exit

A simple demo is to display the filesystem tree, open `/readthis.txt`, overwrite it with new content and then append additional text.

---

## Parts Still Being Completed and Tested

The main remaining work is making file modification completely robust for different file sizes and situations.

### 1. Robust Block Allocation

When a file grows and requires more blocks, the program needs to:

    Find a free block
          |
          v
    Mark the block as used
          |
          v
    Update free-block information
          |
          v
    Connect the block to the inode
    or indirect block
          |
          v
    Write the new data

The allocation process needs to be tested carefully for files that grow across multiple blocks and indirect-block boundaries.

### 2. Correctly Freeing Unused Blocks

When a file is overwritten with smaller content, blocks that are no longer required should be released.

The process is:

    Old File
       |
       v
    New Smaller File
       |
       v
    Find Unused Blocks
       |
       v
    Clear Them From Block Bitmap
       |
       v
    Update Free Block Count
       |
       v
    Remove Their Pointers
       |
       v
    Update Inode

This is important because simply changing the file size does not actually free the old blocks.

### 3. File Growth and Shrinking

More testing is required for cases such as:

- Overwriting a large file with a smaller file
- Overwriting a small file with a larger file
- Overwriting with an empty file
- Appending data that fits in the current last block
- Appending data that requires a new block
- Appending data that requires indirect blocks

### 4. Filesystem Consistency

After modifying a file, the following information needs to remain consistent:

    Inode Size
         |
         v
    Number of Allocated Blocks
         |
         v
    Block Pointers
         |
         v
    Block Bitmap
         |
         v
    Free Block Count

The final implementation should make sure that modifying a file does not leave inconsistent filesystem metadata.

### 5. Larger Files

The current filesystem image is small enough that direct, single-indirect and double-indirect block handling covers the required files.

Triple-indirect block support can be added later if larger files need to be supported.

---

## Build

From the project root, compile the project using:

    g++ -std=c++17 -Iinclude src/filesystem.cpp src/directory.cpp src/file.cpp src/main.cpp -o ext2explorer.exe

Then run:

    ./ext2explorer.exe

The program expects the working disk image at:

    Artifacts/disk-backpup.img

---

## Current Status

### Working

- EXT2 disk image opening
- Superblock parsing
- Block group descriptor parsing
- Inode reading
- Block reading
- Block writing
- Recursive directory traversal
- Path-based file lookup
- File content reading
- Direct block handling
- Single indirect block handling
- Double indirect block handling
- Basic overwrite operation
- Basic append operation
- Interactive command-line menu

### Still Being Tested / Improved

- Fully robust block allocation
- Fully robust block freeing
- File shrinking
- File growing
- Empty-file overwrite
- Append across block boundaries
- Repeated file modifications
- Filesystem consistency after modifications
- More extensive testing
- Optional triple-indirect block support
- Final cleanup and testing

---

## Disk Image and Backup

The working disk image is modified during testing.

For this reason, an original copy is kept separately:

    disk-backpup-original.img

The `.img` files are excluded from Git using `.gitignore`.

The compiled executable is also excluded from Git.

---

## Final Goal

The goal of the project is to build a small EXT2 filesystem explorer that can understand the filesystem directly from its raw disk representation and perform basic file operations without relying on the operating system's normal filesystem interface.

The important part of the project is not just reading files, but understanding how the different EXT2 structures connect together:

    Superblock
        ↓
    Block Groups
        ↓
    Inodes
        ↓
    Directory Entries
        ↓
    Block Pointers
        ↓
    Data Blocks

Once this flow is understood, operations such as finding, reading, appending and overwriting a file become a matter of correctly navigating and updating those structures.