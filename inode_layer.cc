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

    int first_data_block = IBLOCK(INODE_NUM, BLOCK_NUM) + 1;


    for (int id = first_data_block; id < BLOCK_NUM; id++) {
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
        char *byte = &((char *) buf)[byte_offset_in_block];
        /* (char)1 is (00000001)binary */
        /* (char)1 << 1 is (00000010)binary */
        /* ~((char)1 << 1) is (11111101)binary */
        /* &= makes the bit representing id to zero */
//        *byte &= ~((char)1 << bit_offset_in_byte);



        char *bit = &((char *) buf)[bit_offset_in_block];

        if (*bit == '0') {

            *byte |= ((char) 1 << bit_offset_in_byte);
            d->write_block(BBLOCK(id), buf);
            pthread_mutex_unlock(&bitmap_mutex);
            return id;

        }
    }



    /* Your Part I code ends here. */
    pthread_mutex_unlock(&bitmap_mutex);
    /* no free data block left */
    return 0;

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
    printf("total bytes to be read is: %d\n", *size);


    int index = 0;
    int indirect_index = 0;


    while(remaining_bytes_to_be_read > 0){



//        can we read a full block?
//        can we read a full block?
//        can we read a full block?
//        can we read a full block?
//        can we read a full block?
//        can we read a full block?
        if(remaining_bytes_to_be_read > BLOCK_SIZE){
            // we can read a full block

            if(num_bytes_already_read > (8*BLOCK_SIZE)){
                // we have to read a full block within the INDIRECT section

                char indirect_block[512];
                bm->read_block(ino->blocks[8], indirect_block);
                int *ptr = (int*)indirect_block;


//                address associated with block to be read is at ptr[indirect_index]
//                  pull that address out and read that data into our buffer
                char full_indirect_buffer[512];
                bm->read_block(ptr[indirect_index], full_indirect_buffer);

//                copy all that data over to the buff out
                memcpy(buf + (index * BLOCK_SIZE), full_indirect_buffer, BLOCK_SIZE);


                num_bytes_already_read += BLOCK_SIZE;
                remaining_bytes_to_be_read -= BLOCK_SIZE;

                indirect_index += 1;




            } else {
                // we are simply reading a full direct block

                char full_direct_buffer[512];
                bm->read_block(ino->blocks[index], full_direct_buffer);
                memcpy(buf + (index * BLOCK_SIZE), full_direct_buffer, BLOCK_SIZE);

                num_bytes_already_read += BLOCK_SIZE;
                remaining_bytes_to_be_read -= BLOCK_SIZE;


            }



        } else{
//            we can only read part of a block
//            we can only read part of a block
//            we can only read part of a block
//            we can only read part of a block
//            we can only read part of a block
//            we can only read part of a block
//            we can only read part of a block
//            we can only read part of a block
//            we can only read part of a block
//            we can only read part of a block
//            we can only read part of a block


            printf("READING ONLY PART OF A BLOCK\n");

            printf("INODE WE ARE READING FROM IS: %d\n", inum);

            if(num_bytes_already_read > (8*BLOCK_SIZE)){
                // we are reading part of a block addressed within the INdirect Section

                char indirect_block[512];
                bm->read_block(ino->blocks[8], indirect_block);
                int *ptr = (int*)indirect_block;


//                address associated with block to be read is at ptr[indirect_index]
//                  pull that address out and read that data into our buffer
                char partial_indirect_buffer[512];
                bm->read_block(ptr[indirect_index], partial_indirect_buffer);

//                copy ONLY the data we want into the buff stuff
                memcpy(buf + (index * BLOCK_SIZE), partial_indirect_buffer, remaining_bytes_to_be_read);


                num_bytes_already_read += remaining_bytes_to_be_read;
                remaining_bytes_to_be_read -= remaining_bytes_to_be_read;

                indirect_index += 1;


            } else {
                // we are reading part of a block from a direct block
//                read the entire block within the direct block section

                printf("READING FROM PARTIAL BLOCK IN DIRECT SECTION\n");
                char partial_direct_buffer[512];
                bm->read_block(ino->blocks[index], partial_direct_buffer);

                memcpy(buf + (index * BLOCK_SIZE), partial_direct_buffer, remaining_bytes_to_be_read);
                printf("bytes read: %s\n", buf);

                printf("INODE WE ARE READING FROM IS: %d\n", inum);


                printf("remaining bytes to be read: %d\n", remaining_bytes_to_be_read);


                num_bytes_already_read += remaining_bytes_to_be_read;
                remaining_bytes_to_be_read -= remaining_bytes_to_be_read;

                printf("remaining bytes to be read: %d\n", remaining_bytes_to_be_read);



            }

        }

        index += 1;
// end while
    }



    /* Your Part I code ends here. */
    *buf_out = buf;
    free(ino);
}

void
inode_layer::write_file(uint32_t inum, const char *buf, int size)
{
    printf("HELLO THERE LOOK AT THE BUFFER %s\n", buf);
    printf("HELLO THERE LOOK AT THE SIZE %d\n", size);
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


//    FREEING ALL THE BLOCKS
    for(int x = 0; x < NDIRECT; x++){
        bm->free_block(ino->blocks[x]);
    }
//    int indirect_size = size % (BLOCK_SIZE * 8);

    char indirect_block[512];
    bm->read_block(ino->blocks[8], indirect_block);
    int *ptr = (int*)indirect_block;

    for(int x; x < 128; x++){
        bm->free_block(ptr[x]);
    }
//    DONE FREEING ALL THE BLOCKS

    printf("FINISHED FREEING ALL THE BLOCKS\n");

    int remaining_bytes_to_write = size;
    int num_bytes_already_written = 0;

    int index = 0;
    int indirect_index = 0;

    while (remaining_bytes_to_write > 0){


//        can we write a full block?
        if(remaining_bytes_to_write > BLOCK_SIZE){
//            we can write a full block!


//          can we write this full block in the Direct section?
            if(num_bytes_already_written < (8 * BLOCK_SIZE)){
//                write a block in the direct section!!

                char full_direct_buffer[512];
                memcpy(full_direct_buffer, buf + (index * BLOCK_SIZE), BLOCK_SIZE);
                // read 512 bytes from buffer into the full_direct_buffer

                // now need to allocate a new block!
                int block_id = bm->alloc_block();





                // new stuff
                ino->blocks[index] = block_id;
                // write that block to memory
                bm->write_block(block_id, full_direct_buffer);


                // increment / decrement our while loop variables appropriately
                num_bytes_already_written += BLOCK_SIZE;
                remaining_bytes_to_write -= BLOCK_SIZE;


            } else {
//                write a block in the indirect section
//                going into the indirect section



                char full_indirect_buffer[512];
                memcpy(full_indirect_buffer, buf + (index*BLOCK_SIZE), BLOCK_SIZE);
                // copy 512 bytes of the buf into our full_indirect_buffer


                // allocate a new block!

                int block_id = bm->alloc_block();


                // write this block to memory
                bm->write_block(block_id, full_indirect_buffer);


                // need to store this blocks address into the indirect block itself

                // get the address of the block_id pointer
                int *ptr = &block_id;

                // store this pointer within the indirect block
                memcpy(&ino->blocks[8] + (indirect_index * 4), ptr, 4);



                remaining_bytes_to_write -= BLOCK_SIZE;
                num_bytes_already_written += BLOCK_SIZE;

                indirect_index += 1;

            }


        } else{
//            we can NOT write a full block, write only part of a block!


//          can we write this partial block in the Direct section?
            if(num_bytes_already_written < (8 * BLOCK_SIZE)){

                printf("WRITING A PARTIAL BLOCK IN THE DIRECT SECTION\n");
//                write a block in the direct section!!


                char partial_direct_buffer[512];
                memcpy(partial_direct_buffer, buf + (index * BLOCK_SIZE), remaining_bytes_to_write);
                // read a partial number of bytes into the partial_direct_buffer


                // allocate a new block
                int block_id = bm->alloc_block();


                // write this block to memory
//                TODO: THIS COULD BE WRONG
//                bm->write_block(block_id, partial_direct_buffer);


                // new stuff
                ino->blocks[index] = block_id;
                bm->write_block(block_id, partial_direct_buffer);
//                memcpy(&)

//                memcpy(ino->blocks[index])



                // increment / decrement our loop variables appropriately

                num_bytes_already_written += remaining_bytes_to_write;
                remaining_bytes_to_write -= remaining_bytes_to_write;



            } else {
//                write a part of a block in the indirect section
                char partial_indirect_buffer[512];
                memcpy(partial_indirect_buffer, buf + (index * BLOCK_SIZE), remaining_bytes_to_write);

                //allocate a new block!
                int block_id = bm->alloc_block();


                // TODO: could be wrong to write_block here
                bm->write_block(block_id, partial_indirect_buffer);
                // wrote this block to memory



                // need to store the pointer to this block within the indirect block
                // get the address of the block_id pointer
                int *ptr = &block_id;

                // store this pointer within the indirect block
                memcpy(&ino->blocks[8] + (indirect_index * 4), ptr, 4);


                remaining_bytes_to_write -= remaining_bytes_to_write;
                num_bytes_already_written += remaining_bytes_to_write;
                indirect_index += 1;

            }

        }

    }

    ino->size = size;

    /* Your Part I code ends here. */
    put_inode(inum, ino);
    free(ino);
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
