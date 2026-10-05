#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <cstdint>

#pragma pack(push, 1)

// ============================================================
// EXT2 SUPERBLOCK
// ============================================================

struct Ext2Superblock
{                                     // Byte Offset

    // Essential
    uint32_t inodes_count;            // 0
    uint32_t blocks_count;            // 4
    uint32_t reserved_blocks_count;   // 8
    uint32_t free_blocks_count;       // 12
    uint32_t free_inodes_count;       // 16

    // Used
    uint32_t first_data_block;        // 20
    uint32_t log_block_size;          // 24
    int32_t  log_fragment_size;       // 28 - Not used in our implementation
    uint32_t blocks_per_group;        // 32
    uint32_t fragments_per_group;     // 36 - Not used in our implementation
    uint32_t inodes_per_group;        // 40

    // Optional
    uint32_t mtime;                   // 44 - Last mount time
    uint32_t wtime;                   // 48 - Last write time
    uint16_t mount_count;             // 52
    int16_t  max_mount_count;         // 54

    // Used
    uint16_t magic;                   // 56
    uint16_t state;                   // 58
    uint16_t errors;                  // 60
    uint16_t pad;                     // 62 - Padding/reserved

    // Optional
    uint32_t lastcheck;               // 64
    uint32_t checkinterval;           // 68
    uint32_t creator_os;              // 72 - OS that created filesystem

    // Used
    uint32_t revision_level;          // 76

    // Important
    uint16_t default_uid;             // 80 - Default user ID
    uint16_t default_gid;             // 82 - Default group ID
    uint32_t first_inode;             // 84
    uint16_t inode_size;              // 88
    uint16_t block_group_number;      // 90 - Superblock's block group number
    uint32_t feature_compat;          // 92
    uint32_t feature_incompat;        // 96
    uint32_t feature_ro_compat;       // 100

    // Optional
    uint8_t  uuid[16];                // 104
    char     volume_name[16];         // 120
    char     last_mounted[64];        // 136
    uint32_t algorithm_usage_bitmap;  // 200
    uint8_t  prealloc_blocks;         // 204
    uint8_t  prealloc_dir_blocks;     // 205
    uint16_t padding1;                // 206
    uint8_t  journal_uuid[16];        // 208
    uint32_t journal_inode;           // 224
    uint32_t journal_device;          // 228
    uint32_t last_orphan;             // 232
    uint32_t hash_seed[4];            // 236
    uint8_t  default_hash_version;    // 252
    uint8_t  reserved_char_pad;       // 253
    uint16_t reserved_word_pad;       // 254
    uint32_t default_mount_options;   // 256
    uint32_t first_meta_bg;           // 260
    uint32_t reserved[190];            // 264 - Fills structure to 1024 bytes
};


// ============================================================
// EXT2 BLOCK GROUP DESCRIPTOR
// ============================================================

struct Ext2BlockGroupDescriptor
{                                     // Byte offset
    // Essential
    uint32_t block_bitmap;            // 0 - Block bitmap location
    uint32_t inode_bitmap;            // 4 - Inode bitmap location
    uint32_t inode_table;             // 8 - Inode table location
    uint16_t free_blocks_count;       // 12
    uint16_t free_inodes_count;       // 14

    // Optional
    uint16_t used_dirs_count;         // 16
    uint16_t pad;                     // 18 - Padding/reserved
    uint32_t reserved[3];             // 20 - Reserved
};


// ============================================================
// EXT2 INODE
// ============================================================

struct Ext2Inode
{                                     // Byte offset
    // Essential
    uint16_t mode;                    // 0  - File type + permissions
    uint16_t uid;                     // 2  - Owner user ID
    uint32_t size;                    // 4  - File size in bytes

    // Used
    uint32_t atime;                   // 8  - Last access time
    uint32_t ctime;                   // 12 - Last inode/status change
    uint32_t mtime;                   // 16 - Last modification time
    uint32_t dtime;                   // 20 - Deletion time

    // Essential
    uint16_t gid;                     // 24 - Owner group ID
    uint16_t links_count;             // 26 - Number of hard links
    uint32_t blocks;                  // 28 - Number of 512-byte sectors allocated

    // Used
    uint32_t flags;                   // 32 - Inode behavior flags
    uint32_t osd1;                    // 36 - OS-dependent field

    // Essential
    uint32_t block[15];               // 40 - Data block pointers

    // Optional
    uint32_t generation;              // 100 - File version for NFS
    uint32_t file_acl;                // 104 - File ACL information
    uint32_t dir_acl;                 // 108 - ACL / high file-size information
    uint32_t faddr;                   // 112 - Fragment address
    uint8_t  osd2[12];                // 116 - OS-dependent fields
};


// ============================================================
// VERIFY STRUCTURE SIZES
// ============================================================

static_assert(sizeof(Ext2Superblock) == 1024,
              "Ext2Superblock must be exactly 1024 bytes");

static_assert(sizeof(Ext2BlockGroupDescriptor) == 32,
              "Ext2GroupDescriptor must be exactly 32 bytes");

static_assert(sizeof(Ext2Inode) == 128,
              "Ext2Inode must be exactly 128 bytes");

#endif