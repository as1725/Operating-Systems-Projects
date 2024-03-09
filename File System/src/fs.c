/**
 * @file fs.c
 * @author Sooms (24100180@lums.edu.pk)
 * @brief
 * @version 0.1
 * @date 2023-11-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "fs.h"
#include "log.h"

static int MOUNT_FLAG = 0;
static union block SUPERBLOCK;
static union block BLOCK_BITMAP;
static union block INODE_BITMAP;

const char *get_name_from_path(const char *path);
uint32_t allocate_inode();
uint32_t allocate_data_block();
int write_inode_to_disk(uint32_t inode_number, struct inode *inode);
struct inode *find_parent_directory(const char *path);
struct inode *get_inode(uint32_t inode_number);
int add_directory_entry(struct inode *parent_dir_inode, uint32_t inode_number, const char *name);

void fs_unmount()
{
    if (MOUNT_FLAG == 0)
    {
        printf("Error: Disk is not mounted.\n");
        return;
    }

    // SET MOUNT FLAG TO 0
    MOUNT_FLAG = 0;
}
int fs_format()
{
    if (MOUNT_FLAG == 1)
    {
        printf("Error: Disk is already mounted.\n");
        return -1;
    }
    memset(&SUPERBLOCK, 0, sizeof(union block));
    memset(&BLOCK_BITMAP, 0, sizeof(union block));
    memset(&INODE_BITMAP, 0, sizeof(union block));

    union block zero_block;
    memset(zero_block.data, 0, sizeof(union block));

    for (int i = 0; i < disk_size(); i++)
    {
        if (disk_write(i, &zero_block) == -1)
        {
            return -1;
        }
    }

    SUPERBLOCK.superblock.s_blocks_count = disk_size();
    SUPERBLOCK.superblock.s_inodes_count = disk_size();
    SUPERBLOCK.superblock.s_inode_table_block_start = 3;
    SUPERBLOCK.superblock.s_data_blocks_start = 3 + ceil((double)SUPERBLOCK.superblock.s_inodes_count / (double)INODES_PER_BLOCK);

    if (disk_write(0, &SUPERBLOCK) == -1)
    {
        return -1;
    }

    for (int i = 0; i < disk_size(); i++)
    {
        INODE_BITMAP.bitmap[i] = 0;
    }
    INODE_BITMAP.bitmap[0] = 1;

    if (disk_write(2, &INODE_BITMAP) == -1)
    {
        return -1;
    }

    for (int i = 0; i < disk_size(); i++)
    {
        BLOCK_BITMAP.bitmap[i] = 0;
    }
    for (unsigned int i = 0; i < SUPERBLOCK.superblock.s_data_blocks_start + 1; i++)
    {
        BLOCK_BITMAP.bitmap[i] = 1;
    }

    if (disk_write(1, &BLOCK_BITMAP) == -1)
    {
        return -1;
    }

    union block inodes;
    for (int i = 0; i < INODES_PER_BLOCK; i++)
    {
        memset(&inodes.inodes[i], 0, sizeof(struct inode));
        for (int j = 0; j < INODE_DIRECT_POINTERS; j++)
        {
            inodes.inodes[i].i_direct_pointers[j] = 0;
        }
    }
    if (disk_write(3, &inodes) == -1)
    {
        return -1;
    }

    struct directory_block root_dir;
    memset(&root_dir, 0, sizeof(struct directory_block));
    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++)
    {
        root_dir.entries[i].inode_number = 0;
        strcpy(root_dir.entries[i].name, "");
    }

    if (disk_write(SUPERBLOCK.superblock.s_data_blocks_start, &root_dir) == -1)
    {
        return -1;
    }

    int inode_block_number = SUPERBLOCK.superblock.s_inode_table_block_start;
    struct inode root_inode;
    memset(&root_inode, 0, sizeof(struct inode));
    root_inode.i_is_directory = 1;
    root_inode.i_size = sizeof(struct directory_block);
    root_inode.i_direct_pointers[0] = SUPERBLOCK.superblock.s_data_blocks_start;
    if (write_inode_to_disk(0, &root_inode) == -1)
    {
        return -1;
    }
    static union block zero_block1;
    for (long unsigned int i = 0; i < disk_size() / FLAGS_PER_BLOCK; i++)
    {
        zero_block1.bitmap[i] = 0;
    }
    zero_block1.bitmap[0] = 1;
    disk_write(2, &zero_block1);

    MOUNT_FLAG = 0;
    LOG_DEBUG("Superblock:\n");
    LOG_DEBUG("    Blocks: %d\n", SUPERBLOCK.superblock.s_blocks_count);
    LOG_DEBUG("    Inodes: %d\n", SUPERBLOCK.superblock.s_inodes_count);
    LOG_DEBUG("    Inode Table Block Start: %d\n", SUPERBLOCK.superblock.s_inode_table_block_start);
    LOG_DEBUG("    Data Blocks Start: %d\n", SUPERBLOCK.superblock.s_data_blocks_start);
    return 1;
}

int fs_mount()
{
    if (MOUNT_FLAG == 1)
    {
        printf("Error: Disk is already mounted.\n");
        return -1;
    }

    if (disk_read(0, &SUPERBLOCK) == -1)
    {
        return -1;
    }

    if (disk_read(1, &BLOCK_BITMAP) == -1)
    {
        return -1;
    }

    if (disk_read(2, &INODE_BITMAP) == -1)
    {
        return -1;
    }

    MOUNT_FLAG = 1;

    return 0;
}

int fs_create(char *path, int is_directory)
{
    if (MOUNT_FLAG == 0)
    {
        printf("Error: Disk is not mounted.\n");
        return -1;
    }

    char *path_copy = strdup(path);
    const char slash[] = "/";
    char *token = strtok(path_copy, slash);
    struct inode *current_dir_inode = get_inode(0);

    char *next_token = strtok(NULL, slash);
    char *prev_token = NULL;
    while (next_token)
    {
        bool found = false;
        for (int i = 0; i < INODE_DIRECT_POINTERS && !found; ++i)
        {
            LOG_DEBUG("i: %d\n", i);
            uint32_t block_num = current_dir_inode->i_direct_pointers[i];
            LOG_DEBUG("Block number: %d\n", block_num);
            if (block_num == 0)
                continue;

            union block dir_block;
            disk_read(block_num, &dir_block);

            for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; ++j)
            {
                struct directory_entry *entry = &dir_block.directory_block.entries[j];
                if (entry->inode_number != 0 && strcmp(entry->name, token) == 0)
                {
                    current_dir_inode = get_inode(entry->inode_number);
                    found = true;
                    break;
                }
            }
        }

        if (!found)
        {
            LOG_DEBUG("Directory not found.\n");
            uint32_t new_inode_number = allocate_inode();
            LOG_DEBUG("New inode number: %d\n", new_inode_number);
            if (new_inode_number == (uint32_t)-1)
            {
                free(path_copy);
                return -1;
            }

            struct inode new_inode;
            memset(&new_inode, 0, sizeof(new_inode));
            new_inode.i_size = 4096;
            new_inode.i_is_directory = 1;

            uint32_t new_block_number = allocate_data_block();
            LOG_DEBUG("New block number: %d\n", new_block_number);
            if (new_block_number == (uint32_t)-1)
            {
                free(path_copy);
                return -1;
            }
            new_inode.i_direct_pointers[0] = new_block_number;
            if (add_directory_entry(current_dir_inode, new_inode_number, token) == -1)
            {
                free(path_copy);
                return -1;
            }
            if (write_inode_to_disk(new_inode_number, &new_inode) == -1)
            {
                free(path_copy);
                return -1;
            }
            current_dir_inode = get_inode(new_inode_number);
        }
        prev_token = token;
        token = next_token;
        next_token = strtok(NULL, slash);
        if (next_token == NULL)
        {
            break;
        }
    }
    uint32_t new_inode_number = allocate_inode();
    LOG_DEBUG("New inode number for file/directory: %d\n", new_inode_number);
    if (new_inode_number == (uint32_t)-1)
    {
        printf("Error: No free inode available.\n");
        return -1;
    }

    struct inode new_inode;
    if (is_directory)
    {
        new_inode.i_size = 4096;
    }
    else
    {
        new_inode.i_size = 0;
    }
    new_inode.i_is_directory = is_directory;
    memset(new_inode.i_direct_pointers, 0, sizeof(new_inode.i_direct_pointers));
    new_inode.i_single_indirect_pointer = 0;
    new_inode.i_double_indirect_pointer = 0;
    LOG_DEBUG("New inode size: %d\n", new_inode.i_size);

    if (is_directory)
    {
        uint32_t new_block_number = allocate_data_block();
        if (new_block_number == (uint32_t)-1)
        {
            printf("Error: No free data block available.\n");
            return -1;
        }
        new_inode.i_direct_pointers[0] = new_block_number;
        union block new_dir_block;
        memset(&new_dir_block, 0, sizeof(union block));
        disk_read(new_block_number, &new_dir_block);
        union block INODES1;
        disk_read(3, &INODES1);
        if (disk_write(new_block_number, &new_dir_block) == -1)
        {
            printf("Error: Failed to write new directory block to disk.\n");
            return -1;
        }
    }
    const char *entry_name = get_name_from_path(path);
    if (entry_name == NULL)
    {
        printf("Error: Invalid path name.\n");
        return -1;
    }

    struct inode *parent_dir_inode = current_dir_inode;
    if (add_directory_entry(parent_dir_inode, new_inode_number, entry_name) == -1)
    {
        printf("Error: Failed to add directory entry.\n");
        return -1;
    }

    if (write_inode_to_disk(new_inode_number, &new_inode) == -1)
    {
        printf("Error: Failed to write new inode to disk.\n");
        return -1;
    }
    return 0;
}

int fs_remove(char *path)
{
    if (MOUNT_FLAG == 0)
    {
        printf("Error: Disk is not mounted.\n");
        return -1;
    }
    const char *entry_name = get_name_from_path(path);
    struct inode *parent_dir_inode = find_parent_directory(path);
    if (parent_dir_inode == NULL || entry_name == NULL || strlen(entry_name) == 0)
    {
        printf("Error: Invalid path or directory does not exist.\n");
        return -1;
    }

    bool found = false;
    uint32_t block_num;
    int entry_index;
    uint32_t inode_number_to_remove;
    for (int i = 0; i < INODE_DIRECT_POINTERS && !found; ++i)
    {
        block_num = parent_dir_inode->i_direct_pointers[i];
        if (block_num == 0)
            continue;

        union block dir_block;
        disk_read(block_num, &dir_block);

        for (entry_index = 0; entry_index < DIRECTORY_ENTRIES_PER_BLOCK; ++entry_index)
        {
            struct directory_entry *entry = &dir_block.directory_block.entries[entry_index];
            inode_number_to_remove = entry->inode_number;
            if (inode_number_to_remove != 0 && strcmp(entry->name, entry_name) == 0)
            {
                found = true;
                break;
            }
        }
    }

    if (!found)
    {
        printf("Error: Entry not found in parent directory.\n");
        return -1;
    }
    struct inode *inode_to_remove = get_inode(inode_number_to_remove);
    if (inode_to_remove->i_is_directory)
    {
        union block dir_block;
        disk_read(inode_to_remove->i_direct_pointers[0], &dir_block);
        for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i)
        {
            if (strcmp(dir_block.directory_block.entries[i].name, "") != 0)
            {
                char temp_path[100];
                strcpy(temp_path, path);
                strcat(temp_path, "/");
                strcat(temp_path, dir_block.directory_block.entries[i].name);
                fs_remove(temp_path);
            }
        }
        BLOCK_BITMAP.bitmap[inode_to_remove->i_direct_pointers[0]] = 0;
        disk_write(1, &BLOCK_BITMAP);
    }
    else
    {
        for (int i = 0; i < INODE_DIRECT_POINTERS; ++i)
        {
            uint32_t block_num = inode_to_remove->i_direct_pointers[i];
            if (block_num == 0)
                continue;

            BLOCK_BITMAP.bitmap[block_num] = 0;
            disk_write(1, &BLOCK_BITMAP);
        }
    }
    union block dir_block;
    disk_read(block_num, &dir_block);
    memset(&dir_block.directory_block.entries[entry_index], 0, sizeof(struct directory_entry));
    disk_write(block_num, &dir_block);

    INODE_BITMAP.bitmap[dir_block.directory_block.entries[entry_index].inode_number] = 0;
    disk_write(2, &INODE_BITMAP);

    return 0;
}

int fs_read(char *path, void *buf, size_t count, off_t offset)
{
    if (MOUNT_FLAG == 0)
    {
        printf("Error: Disk is not mounted.\n");
        return -1;
    }
    struct inode *parent_dir_inode = find_parent_directory(path);
    struct inode *file_inode = NULL;
    uint32_t file_block_num;
    for (int i = 0; i < INODE_DIRECT_POINTERS; ++i)
    {
        uint32_t block_num = parent_dir_inode->i_direct_pointers[i];
        if (block_num == 0)
            continue;

        union block dir_block;
        disk_read(block_num, &dir_block);

        for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; ++j)
        {
            struct directory_entry *entry = &dir_block.directory_block.entries[j];
            if (entry->inode_number != 0 && strcmp(entry->name, get_name_from_path(path)) == 0)
            {
                file_inode = get_inode(entry->inode_number);
                break;
            }
        }
        break;
    }
    if (file_inode == NULL)
    {
        printf("Error: File not found.\n");
        return -1;
    }

    if (file_inode->i_is_directory)
    {
        printf("Error: Cannot read a directory.\n");
        return -1;
    }

    union block file_block;
    int num = offset / BLOCK_SIZE;
    if (num >= INODE_DIRECT_POINTERS)
    {
        union block indirect_block;
        memset(&indirect_block, 0, sizeof(union block));
        disk_read(file_inode->i_single_indirect_pointer, &indirect_block);
        if (indirect_block.pointers[num - INODE_DIRECT_POINTERS] == 0)
        {
            return -1;
        }
        disk_read(indirect_block.pointers[num - INODE_DIRECT_POINTERS], &file_block);
        memcpy(buf, file_block.data, count);
    }
    else
    {
        if (file_inode->i_direct_pointers[num] == 0)
        {
            return -1;
        }
        file_block_num = file_inode->i_direct_pointers[num];
        disk_read(file_block_num, &file_block);
        memcpy(buf, file_block.data, count);
    }
    if (file_inode->i_size < offset + count)
    {
        int bytes_read = file_inode->i_size - offset;
        return bytes_read;
    }
    else if (file_inode->i_size >= offset + count)
    {
        return count;
    }

    return -1;
}

int fs_write(char *path, void *buf, size_t count, off_t offset)
{
    if (MOUNT_FLAG == 0)
    {
        printf("Error: Disk is not mounted.\n");
        return -1;
    }

    struct inode *parent_dir_inode = find_parent_directory(path);
    struct inode *file_inode = NULL;
    uint32_t file_inode_number = 0;
    uint32_t file_block_num;
    for (int i = 0; i < INODE_DIRECT_POINTERS; ++i)
    {
        uint32_t block_num = parent_dir_inode->i_direct_pointers[i];
        LOG_DEBUG("Block number: %d\n", block_num);
        if (block_num == 0)
            continue;

        union block dir_block;
        disk_read(block_num, &dir_block);

        for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; ++j)
        {
            struct directory_entry *entry = &dir_block.directory_block.entries[j];
            if (entry->inode_number != 0 && strcmp(entry->name, get_name_from_path(path)) == 0)
            {
                file_inode_number = entry->inode_number;
                file_inode = get_inode(entry->inode_number);
                file_block_num = file_inode->i_direct_pointers[0];
                break;
            }
        }
    }
    if (file_inode == NULL)
    {
        fs_create(path, 0);
        return fs_write(path, buf, count, offset);
    }

    if (file_inode->i_is_directory)
    {
        printf("Error: Cannot write to a directory.\n");
        return -1;
    }

    union block file_block;
    disk_read(file_block_num, &file_block);
    int no_blocks = (offset + count + BLOCK_SIZE - 1) / BLOCK_SIZE;

    for (int i = 0; i < no_blocks; ++i)
    {
        if (i < INODE_DIRECT_POINTERS)
        {
            uint32_t block_num = 0;
            if (file_inode->i_direct_pointers[i] == 0)
            {
                block_num = allocate_data_block();
                if (block_num == (uint32_t)-1)
                {
                    printf("Error: No free data block available.\n");
                    return -1;
                }
                file_inode->i_direct_pointers[i] = block_num;
            }
            else
            {
                block_num = file_inode->i_direct_pointers[i];
            }

            union block file_block;
            disk_read(block_num, &file_block);
            if (count < BLOCK_SIZE)
            {
                memcpy(file_block.data + offset, buf, count);
            }
            else
            {
                memcpy(file_block.data + offset, buf + i * BLOCK_SIZE, BLOCK_SIZE);
            }
            if (disk_write(block_num, &file_block) == -1)
            {
                printf("Error: Failed to write file block to disk.\n");
                return -1;
            }
        }

        else
        {
            if (file_inode->i_single_indirect_pointer == 0)
            {
                uint32_t block_num = allocate_data_block();
                if (block_num == (uint32_t)-1)
                {
                    printf("Error: No free data block available.\n");
                    return -1;
                }
                file_inode->i_single_indirect_pointer = block_num;
            }

            union block indirect_block;
            memset(&indirect_block, 0, sizeof(union block));
            disk_read(file_inode->i_single_indirect_pointer, &indirect_block);

            union block file_block;
            memset(&file_block, 0, sizeof(union block));
            memcpy(file_block.data, buf + i * BLOCK_SIZE, BLOCK_SIZE);

            uint32_t block_num = allocate_data_block();
            if (block_num == (uint32_t)-1)
            {
                printf("Error: No free data block available.\n");
                return -1;
            }
            indirect_block.pointers[i - INODE_DIRECT_POINTERS] = block_num;
            disk_write(file_inode->i_single_indirect_pointer, &indirect_block);
            disk_write(block_num, &file_block);
        }
    }

    if (offset + count > file_inode->i_size)
    {
        file_inode->i_size = offset + count;
    }
    if (write_inode_to_disk(file_inode_number, file_inode) == -1)
    {
        printf("Error: Failed to write file inode to disk.\n");
        return -1;
    }
    return count;
}

int fs_list(char *path)
{
    if (MOUNT_FLAG == 0)
    {
        printf("Error: Disk is not mounted.\n");
        return -1;
    }

    struct inode *dir_inode = NULL;
    if (strcmp(path, "/") == 0)
    {
        dir_inode = get_inode(0);
    }

    else
    {
        struct inode *parent_dir_inode = find_parent_directory(path);
        char *token;
        char *last_token = "";
        char *path_copy = strdup(path);
        token = strtok(path_copy, "/");
        while (token != NULL)
        {
            last_token = token;
            token = strtok(NULL, "/");
        }

        for (int i = 0; i < INODE_DIRECT_POINTERS; ++i)
        {
            uint32_t block_num = parent_dir_inode->i_direct_pointers[i];
            LOG_DEBUG("Block number: %d\n", block_num);
            if (block_num == 0)
                continue;

            union block dir_block;
            disk_read(block_num, &dir_block);

            for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; ++j)
            {
                struct directory_entry *entry = &dir_block.directory_block.entries[j];
                if (entry->inode_number != 0 && strcmp(entry->name, last_token) == 0)
                {
                    dir_inode = get_inode(entry->inode_number);
                    break;
                }
            }
            if (dir_inode != NULL)
                break;
        }
    }

    if (dir_inode == NULL)
    {
        printf("Error: Directory not found.\n");
        return -1;
    }

    for (int i = 0; i < INODE_DIRECT_POINTERS; ++i)
    {
        uint32_t block_num = dir_inode->i_direct_pointers[i];
        if (block_num == 0)
            continue;

        union block dir_block;
        disk_read(block_num, &dir_block);
        // printf("Directory contents:\n");
        for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; ++j)
        {
            struct directory_entry *entry = &dir_block.directory_block.entries[j];
            if (entry->inode_number != 0)
            {
                printf("%s %d\n", entry->name, get_inode(entry->inode_number)->i_size);
            }
        }
    }

    return 0;
}

void fs_stat()
{
    if (MOUNT_FLAG == 0)
    {
        printf("Error: Disk is not mounted.\n");
        return;
    }

    printf("Superblock:\n");
    printf("    Blocks: %d\n", SUPERBLOCK.superblock.s_blocks_count);
    printf("    Inodes: %d\n", SUPERBLOCK.superblock.s_inodes_count);
    printf("    Inode Table Block Start: %d\n", SUPERBLOCK.superblock.s_inode_table_block_start);
    printf("    Data Blocks Start: %d\n", SUPERBLOCK.superblock.s_data_blocks_start);
}

// Helper functions
uint32_t allocate_inode()
{
    for (int i = 0; i < INODES_PER_BLOCK; ++i)
    {
        if (!INODE_BITMAP.bitmap[i])
        {
            INODE_BITMAP.bitmap[i] = 1;
            if (disk_write(2, &INODE_BITMAP) == -1)
            {
                return -1;
            }
            return i;
        }
    }
    return -1;
}

uint32_t allocate_data_block()
{
    for (int i = 0; i < SUPERBLOCK.superblock.s_blocks_count; ++i)
    {
        if (!BLOCK_BITMAP.bitmap[i])
        {
            BLOCK_BITMAP.bitmap[i] = 1;
            if (disk_write(1, &BLOCK_BITMAP) == -1)
            {
                return -1;
            }
            return i + SUPERBLOCK.superblock.s_data_blocks_start;
        }
    }
    return -1;
}

int write_inode_to_disk(uint32_t inode_number, struct inode *inode)
{
    uint32_t block_number = SUPERBLOCK.superblock.s_inode_table_block_start +
                            (inode_number / INODES_PER_BLOCK);
    uint32_t index_within_block = inode_number % INODES_PER_BLOCK;

    union block inode_block;
    if (disk_read(block_number, &inode_block) == -1)
    {
        printf("Error: Failed to read inode block from disk.\n");
        return -1;
    }
    inode_block.inodes[index_within_block] = *inode;

    if (disk_write(block_number, &inode_block) == -1)
    {
        printf("Error: Failed to write inode block to disk.\n");
        return -1;
    }

    LOG_DEBUG("Wrote inode %d to block %d at index %d.\n", inode_number, block_number, index_within_block);
    return 0;
}

struct inode *find_parent_directory(const char *path)
{
    if (strcmp(path, "/") == 0)
    {
        LOG_DEBUG("Root directory found.\n");
        return get_inode(0);
    }

    char temp_path[strlen(path) + 1];
    strcpy(temp_path, path);

    char *token = strtok(temp_path, "/");
    LOG_DEBUG("First token: %s\n", token);
    struct inode *current_inode = get_inode(0);

    while (token != NULL)
    {
        char *next_token = strtok(NULL, "/");
        LOG_DEBUG("Next token: %s\n", next_token);
        if (next_token == NULL)
            break;

        bool found = false;
        for (int i = 0; i < INODE_DIRECT_POINTERS; ++i)
        {
            uint32_t block_num = current_inode->i_direct_pointers[i];
            if (block_num == 0)
                continue;

            union block dir_block;
            disk_read(block_num, &dir_block);

            for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; ++j)
            {
                struct directory_entry *entry = &dir_block.directory_block.entries[j];
                if (entry->inode_number != 0 && strcmp(entry->name, token) == 0)
                {
                    current_inode = get_inode(entry->inode_number);
                    found = true;
                    break;
                }
            }
            if (found)
                break;
        }

        if (!found)
            return NULL;
        token = next_token;
    }
    return current_inode;
}

struct inode *get_inode(uint32_t inode_number)
{
    uint32_t block_index = inode_number / INODES_PER_BLOCK;
    uint32_t index_within_block = inode_number % INODES_PER_BLOCK;
    uint32_t inode_block_num = SUPERBLOCK.superblock.s_inode_table_block_start + block_index;
    union block inode_block;
    disk_read(inode_block_num, &inode_block);
    struct inode *inode_within_block = &inode_block.inodes[index_within_block];
    LOG_DEBUG("Block index: %d\n", block_index);
    LOG_DEBUG("Index within block: %d\n", index_within_block);
    LOG_DEBUG("Inode block number: %d\n", inode_block_num);
    LOG_DEBUG("Inode directory? %d\n", inode_within_block->i_is_directory);
    LOG_DEBUG("Inode size: %d\n", inode_within_block->i_size);
    return inode_within_block;
}

int add_directory_entry(struct inode *parent_dir_inode, uint32_t inode_number, const char *name)
{
    for (int i = 0; i < INODE_DIRECT_POINTERS; ++i)
    {
        uint32_t block_num = parent_dir_inode->i_direct_pointers[i];
        union block dir_block;

        if (block_num == 0)
        {
            block_num = allocate_data_block();
            if (block_num == -1)
                return -1;
            parent_dir_inode->i_direct_pointers[i] = block_num;
            memset(dir_block.data, 0, BLOCK_SIZE);
        }
        else
        {
            disk_read(block_num, &dir_block);
        }

        for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; ++j)
        {
            if (dir_block.directory_block.entries[j].inode_number == 0)
            {
                dir_block.directory_block.entries[j].inode_number = inode_number;
                strncpy(dir_block.directory_block.entries[j].name, name, DIRECTORY_NAME_SIZE - 1);
                dir_block.directory_block.entries[j].name[DIRECTORY_NAME_SIZE - 1] = '\0';
                if (disk_write(block_num, &dir_block) == -1)
                {
                    return -1;
                }
                return 0;
            }
        }
    }
    return -1;
}

const char *get_name_from_path(const char *path)
{
    const char *last_slash = strrchr(path, '/');
    if (last_slash == NULL)
    {
        return path;
    }
    else if (*(last_slash + 1) == '\0')
    {
        return NULL;
    }
    else
    {
        return last_slash + 1;
    }
}