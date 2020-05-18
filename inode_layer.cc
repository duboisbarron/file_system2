#include "inode_layer.h"
#include <unistd.h>

/*
 * Part IA: inode allocation
 */

/* Allocate an inode and return its inum (inode number). */
uint32_t
inode_layer::alloc_inode(uint32_t type)
{
    /* 
     * Your Part I code goes here.
     * hint1: read get_inode() and put_inode(), read INODE_NUM, IBLOCK, etc in inode_layer.h
     * hint2: you can use time(0) to get the timestamp for atime/ctime/mtime
     */


    int where_to_alloc = NULL;

    for(int x = 0; x < INODE_NUM; x++){
//        printf("hello there\n");
//        printf("current value is: %d\n", x);
        inode* this_one = get_inode(x);
        if(this_one == NULL){
            where_to_alloc = x;
            break;
        }

//        printf()
    }

    inode* new_one = new inode();
    new_one->atime = time(0);
    new_one->ctime = time(0);
    new_one->mtime = time(0);
    new_one->size = 0;
    new_one->type = type;

    put_inode(where_to_alloc, new_one);

    return where_to_alloc;

    /* Your Part I code ends here. */
}

/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* 
inode_layer::get_inode(uint32_t inum)
{
    struct inode *ino, *ino_disk;
    char buf[BLOCK_SIZE];

    /* checking parameter */
    printf("\tim: get_inode %d\n", inum);
    if (inum < 0 || inum >= INODE_NUM) {
        printf("\tim: inum out of range\n");
        return NULL;
    }

    /* read the disk block containing the inode data structure */
    bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
    ino_disk = (struct inode*)buf + inum%IPB;

    /* try to read an inode that is not in use */
    if (ino_disk->type == 0) {
        printf("\tim: inode not exist\n");
        return NULL;
    }

    /* return the inode data structure */
    ino = (struct inode*)malloc(sizeof(struct inode));
    *ino = *ino_disk;
    return ino;
}

void
inode_layer::put_inode(uint32_t inum, struct inode *ino)
{
    char buf[BLOCK_SIZE];
    struct inode *ino_disk;

    /* checking parameter */
    printf("\tim: put_inode %d\n", inum);
    if (inum < 0 || inum >= INODE_NUM || ino == NULL) {
        printf("\tim: invalid parameter\n");
        return;
    }

    /* modify the inode data structure on disk */
    bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
    ino_disk = (struct inode*)buf + inum%IPB;
    *ino_disk = *ino;
    bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}


/*
 * Part IB: inode read/write
 */

blockid_t
block_layer::alloc_block() {
    pthread_mutex_lock(&bitmap_mutex);
    /*
     * Your Part I code goes here.
     * hint1: go through the bitmap region, find a free data block, mark the corresponding bit in the bitmap to 1 and return the block number of the data block.
     * hint2: read free_block(); remember to call pthread_mutex_unlock before all the returns
     */

    int first_data_block = IBLOCK(1023, sb.nblocks) + 1;


//
//    printf("HELLO THERE LOOK AT HIS NUMBER: %d\n", first_data_block);
//
//
//    printf("CALLING ALLOC BLOCK\n");
//
//    printf("first_data_block value is: %d\n", first_data_block);
//    printf("sb.nblocks value is: %d\n", sb.nblocks);



    for(int id=first_data_block; id<BLOCK_NUM; id++){

//        printf("WITHIN THE FOR LOOP\n");

        char buf[BLOCK_SIZE];


        d->read_block(BBLOCK(id), buf);

        uint32_t bit_offset_in_block = id % BPB;
        uint32_t byte_offset_in_block = bit_offset_in_block / 8;
        uint32_t bit_offset_in_byte = bit_offset_in_block % 8;

        char* byte = &((char*)buf)[byte_offset_in_block];


        if(*byte & ((char)1 << bit_offset_in_byte)){
            continue;
        }
        else{
            *byte ^= (char)1 << bit_offset_in_byte;
            d->write_block(BBLOCK(id), buf);
            pthread_mutex_unlock(&bitmap_mutex);
            return id;
        }

    }

    /* Your Part I code ends here. */
    pthread_mutex_unlock(&bitmap_mutex);
    /* no free data block left */
    return -1;

}

void
block_layer::free_block(uint32_t id)
{
    pthread_mutex_lock(&bitmap_mutex);
    char buf[BLOCK_SIZE];
    d->read_block(BBLOCK(id), buf);

    /* suppose id=5001, we need to modify the 5001 th bit */
    /* which is the 5001 % 4096 = 905 th bit in the block*/
    uint32_t bit_offset_in_block = id % BPB;
    /* which lives in the 905 / 8 = 113 th byte in the block */
    uint32_t byte_offset_in_block = bit_offset_in_block / 8;
    /* and is the 905 % 8 = 1 st bit in this byte */
    uint32_t bit_offset_in_byte = bit_offset_in_block % 8;

    /* You may need to learn the meaning of &= and << operators */
    char* byte = &((char*)buf)[byte_offset_in_block];
    /* (char)1 is (00000001)binary */
    /* (char)1 << 1 is (00000010)binary */
    /* ~((char)1 << 1) is (11111101)binary */
    /* &= makes the bit representing id to zero */
    *byte &= ~((char)1 << bit_offset_in_byte);

    d->write_block(BBLOCK(id), buf);
    pthread_mutex_unlock(&bitmap_mutex);
}


#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void
inode_layer::read_file(uint32_t inum, char **buf_out, int *size)
{

    rwlocks[inum].reader_enter();

    printf("READING A FILE!!!\n");

    printf("FILE SIZE IS: %d\n", size);
    /* check parameter */
    if(buf_out == NULL || size == NULL) {
        rwlocks[inum].reader_exit();
        return;
    }

    /* check existance of inode inum */
    struct inode* ino = get_inode(inum);
    if(ino == NULL) {
        rwlocks[inum].reader_exit();
        return;
    }


//    printf("INODE NUMBER IS: %d\n", inum);

    /* modify the access time of inode inum */
    ino->atime = time(NULL);
    put_inode(inum, ino);

    /* prepare the return value */
    *size = ino->size;
    char* buf = (char*)malloc(*size);

    /*
     * Your Part I code goes here.
     * hint1: read all blocks of inode inum into buf_out, including direct and indirect blocks; you will need the memcpy function
     */


    int remaining_bytes_to_be_read = *size;
    int num_bytes_already_read = 0;

    char indirect_block[512];
    bm->read_block(ino->blocks[8], indirect_block);
    int *ptr = (int *) indirect_block;

    int index = 0;
    int indirect_index = 0;

    while(remaining_bytes_to_be_read > 0){

        if(num_bytes_already_read < (8*BLOCK_SIZE)){
            // DIRECT SECTION
            char full_direct_buffer[512];
            bm->read_block(ino->blocks[index], full_direct_buffer);
            memcpy(buf + (index * BLOCK_SIZE), full_direct_buffer, MIN(BLOCK_SIZE, remaining_bytes_to_be_read));

            num_bytes_already_read += MIN(BLOCK_SIZE, remaining_bytes_to_be_read);
            remaining_bytes_to_be_read -= MIN(BLOCK_SIZE, remaining_bytes_to_be_read);


        } else {
            // INDIRECT SECTION
            char full_indirect_buffer[512];
            bm->read_block(ptr[indirect_index], full_indirect_buffer);
            memcpy(buf + (index * BLOCK_SIZE), full_indirect_buffer, MIN(BLOCK_SIZE, remaining_bytes_to_be_read));


            num_bytes_already_read += MIN(BLOCK_SIZE, remaining_bytes_to_be_read);
            remaining_bytes_to_be_read -= MIN(BLOCK_SIZE, remaining_bytes_to_be_read);
            indirect_index += 1;
        }

        index += 1;
    }

    /* Your Part I code ends here. */
    *buf_out = buf;
    free(ino);
    rwlocks[inum].reader_exit();


}

void
inode_layer::write_file(uint32_t inum, const char *buf, int size)
{

    rwlocks[inum].writer_enter();
    /* check parameter */
    if(size < 0 || (uint32_t)size > BLOCK_SIZE * MAXFILE){
        printf("\tim: error! size negative or too large.\n");
        rwlocks[inum].writer_exit();
        return;
    }

    struct inode* ino = get_inode(inum);
    if(ino==NULL){
        printf("\tim: error! inode not exist.\n");
        rwlocks[inum].writer_exit();
        return;
    }


    /*
     * Your Part I code goes here.
     * hint1: the parameter "buf" is a buffer containing "size" bytes, where "size" is the other parameter. 
     * This function writes the content of "buf" to the inode inum.
     * You need to consider the situation when "size" parameter is larger or smaller than the current size of inode inum
     */

// TODO:
// TODO:
// TODO:
// TODO:
//    FREEING ALL THE BLOCKS
    for(int x = 0; x < NDIRECT; x++){
        bm->free_block(ino->blocks[x]);
    }
//    char indirect_block[512];
//    bm->read_block(ino->blocks[8], indirect_block);
//    int *ptr = (int*)indirect_block;
//    for(int x; x < 128; x++){
//        bm->free_block(ptr[x]);
//    }

//    bm->free_block(ino->blocks[8]);

//    DONE FREEING ALL THE BLOCKS

//    printf("FINISHED FREEING ALL THE BLOCKS\n");

    int remaining_bytes_to_write = size;
    int num_bytes_already_written = 0;
    bool need_to_overwrite_indirect = false;
    char final_indirect_block[512];


    int index = 0;
    int indirect_index = 0;


    int old_inode_size = ino->size;

    bool already_allocated_indirect = false;
    while (remaining_bytes_to_write > 0){

        if(num_bytes_already_written < (8 * BLOCK_SIZE)){
            // we are in the DIRECT SECTION

            char direct_buffer[512];

            memcpy(direct_buffer, buf + (index * BLOCK_SIZE), MIN(BLOCK_SIZE, remaining_bytes_to_write));
            int block_id = bm->alloc_block();


            bm->free_block(ino->blocks[index]);
            ino->blocks[index] = block_id;
            bm->write_block(block_id, direct_buffer);


            num_bytes_already_written += MIN(BLOCK_SIZE, remaining_bytes_to_write);
            remaining_bytes_to_write -= MIN(BLOCK_SIZE, remaining_bytes_to_write);

        } else {
            // in the indirect section
            need_to_overwrite_indirect = true;


            if (size >= old_inode_size){
                // we are in the indirect section already, has ino->blocks[8] been made?

                if(old_inode_size <= (8*BLOCK_SIZE)){
                    // need to allocate ino->blocks[8]
                    if(already_allocated_indirect == false) {
                        int indirect_block_id = bm->alloc_block();
                        ino->blocks[8] = indirect_block_id;
                        already_allocated_indirect = true;
                    }
                    //make sure dont do this every iteration
                }

                char indirect_buffer[512];

                memcpy(indirect_buffer, buf + (index * BLOCK_SIZE), MIN(BLOCK_SIZE, remaining_bytes_to_write));

                int block_id = bm->alloc_block();

                bm->write_block(block_id, indirect_buffer);
                memcpy(final_indirect_block + (indirect_index * sizeof(uint32_t)), &block_id, sizeof(uint32_t));

                remaining_bytes_to_write -= MIN(BLOCK_SIZE, remaining_bytes_to_write);
                num_bytes_already_written += MIN(BLOCK_SIZE, remaining_bytes_to_write);
                indirect_index += 1;

            } else {
                char indirect_block[512];
                bm->read_block(ino->blocks[8], indirect_block);
                int *ptr = (int*)indirect_block;


                char temp_buffer[512];
                memcpy(temp_buffer, buf + (index * BLOCK_SIZE), MIN(BLOCK_SIZE, remaining_bytes_to_write));


                bm->write_block(ptr[indirect_index], temp_buffer);
                memcpy(final_indirect_block + (indirect_index * sizeof(uint32_t)), &ptr[indirect_index], sizeof(uint32_t));
                remaining_bytes_to_write -= MIN(BLOCK_SIZE, remaining_bytes_to_write);
                num_bytes_already_written += MIN(BLOCK_SIZE, remaining_bytes_to_write);


                indirect_index += 1;
            }


        }
        index += 1;
    }

    //free old indirect blocks

if(need_to_overwrite_indirect && (size < old_inode_size)) {
    char indirect_block[512];
    bm->read_block(ino->blocks[8], indirect_block);
    int *ptr = (int *) indirect_block;


    int number_to_free = ((old_inode_size - size) / BLOCK_SIZE) + 1;
    int i = 0;
    while (i < number_to_free) {
        bm->free_block(ptr[indirect_index]);
        indirect_index += 1;
        i += 1;
    }

}

    if(need_to_overwrite_indirect == true){

        bm->write_block(ino->blocks[8], final_indirect_block);

    }

    if(size <= (8* BLOCK_SIZE)){
        bm->free_block(ino->blocks[8]);
    }

    ino->size = size;
    /* Your Part I code ends here. */
    put_inode(inum, ino);
    free(ino);

    rwlocks[inum].writer_exit();


}


/*
 * Part IC: inode free/remove
 */

void
inode_layer::free_inode(uint32_t inum)
{
    /* 
     * Your Part I code goes here.
     * hint1: simply mark inode inum as free
     */

    /* Your Part I code ends here. */


    struct inode* ino = get_inode(inum);
    ino->type = 0;
    put_inode(inum, ino);
    free(ino);

}



void
inode_layer::remove_file(uint32_t inum)
{
    struct inode* ino = get_inode(inum);
    if(ino == NULL)
        return;

    /*
     * Your Part I code goes here.
     * hint1: first, free all data blocks of inode inum (use bm->free_block function); second, free the inode
     */


    int current_size = ino->size;

    for(int x = 0; x < 8; x++){
        bm->free_block(ino->blocks[x]);
        current_size -= BLOCK_SIZE;
    }

    int num_indirect_blocks = (current_size/BLOCK_SIZE) + 1;
    if (current_size > 0){
        char indirect_buffer[512];
        bm->read_block(ino->blocks[8], indirect_buffer);

        int *ptr = (int *) indirect_buffer;

        for(int x = 0; x < num_indirect_blocks; x++){

            bm->free_block(ptr[x]);

        }

        bm->free_block(ino->blocks[8]);



    }


    inode_layer::free_inode(inum);


    /* Your Part I code ends here. */
    free(ino);
    return;
}



/*
 * Helper Functions
 */

/* inode layer ---------------------------------------- */

inode_layer::inode_layer()
{
    bm = new block_layer();
    uint32_t root_dir = alloc_inode(fs_protocol::T_DIR);
    if (root_dir != 0) {
        printf("\tim: error! alloc first inode %d, should be 0\n", root_dir);
        exit(0);
    }
}

void
inode_layer::getattr(uint32_t inum, fs_protocol::attr &a)
{
    struct inode* ino = get_inode(inum);
    if(ino == NULL)
        return;
    a.type = ino->type;
    a.size = ino->size;
    a.atime = ino->atime;
    a.mtime = ino->mtime;
    a.ctime = ino->ctime;
    free(ino);
}


/* block layer ---------------------------------------- */

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_layer::block_layer()
{
    d = new disk();

    // format the disk
    sb.size = BLOCK_SIZE * BLOCK_NUM;
    sb.nblocks = BLOCK_NUM;
    sb.ninodes = INODE_NUM;

}

void
block_layer::read_block(uint32_t id, char *buf)
{
    d->read_block(id, buf);
}

void
block_layer::write_block(uint32_t id, const char *buf)
{
    d->write_block(id, buf);
}


/* disk layer ---------------------------------------- */

disk::disk()
{
  bzero(blocks, sizeof(blocks));
}

void
disk::read_block(blockid_t id, char *buf)
{
    if(id < 0 || id > BLOCK_NUM || buf == NULL)
        return;
    memcpy(buf, blocks[id], BLOCK_SIZE);

    if (DISK_ACCESS_LATENCY)
        usleep(DISK_ACCESS_LATENCY);
}

void
disk::write_block(blockid_t id, const char *buf)
{
    if(id < 0 || id > BLOCK_NUM || buf == NULL)
        return;
    memcpy(blocks[id], buf, BLOCK_SIZE);

    if (DISK_ACCESS_LATENCY)
        usleep(DISK_ACCESS_LATENCY);
}
