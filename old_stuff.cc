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

    int number_of_FULL_blocks = *size / BLOCK_SIZE;
//    if number of blocks is greater than 8, we need to deal with indirect blocks

    int number_of_remaining_bytes = *size % BLOCK_SIZE;



    int total_blocks_in_use = number_of_FULL_blocks;


    if(number_of_remaining_bytes > 0){
//        there is another block in use
        total_blocks_in_use += 1;
    }


    bool deal_with_indirect = false;
    int total_number_of_read_block_calls = number_of_FULL_blocks;
    if(total_blocks_in_use > 8){
        deal_with_indirect = true;
        total_number_of_read_block_calls = 8;
    }


//    read in all the FULL blocks

    for(int x = 0; x < total_number_of_read_block_calls; x++){

        bm->read_block(ino->blocks[x], buf + (x*BLOCK_SIZE));

    }

    if(number_of_remaining_bytes > 0){
//        need to read this number of bytes into our buffer using memcpy

        char this_buffer[512];
        bm->read_block(ino->blocks[total_number_of_read_block_calls], this_buffer);

        memcpy(buf + (total_number_of_read_block_calls * BLOCK_SIZE), this_buffer, number_of_remaining_bytes);
    }

    if(deal_with_indirect){


        int number_bytes_remaining = *size - (8*BLOCK_SIZE);




        char indirect_block[512];
        bm->read_block(ino->blocks[8], indirect_block);
        int *ptr = (int*)indirect_block;

        int index = 0;

        while(number_bytes_remaining > 0){

//            ptr[index] holds the address pointing to the block we want to pull from

            char this_buffer[512];
            bm->read_block(ptr[index], this_buffer);

//            now read out only what we need, either 512 bytes or less if there are fewer we care about
            int bytes_to_read = std::min(BLOCK_SIZE, number_bytes_remaining);


//            copy into buff starting after the direct blocks
            memcpy(buf + (8*BLOCK_SIZE) + (index*BLOCK_SIZE), this_buffer, bytes_to_read);


            number_bytes_remaining -= bytes_to_read;

            index += 1;

        }

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
//    printf("HELLO THERE LOOK AT THE BUFFER %s\n");
//    printf("HELLO THERE LOOK AT THE BUFFER %s\n");
//    printf("HELLO THERE LOOK AT THE BUFFER %s\n");
//    printf("HELLO THERE LOOK AT THE BUFFER %s\n");
//    printf("HELLO THERE LOOK AT THE BUFFER %s\n");
//    printf("HELLO THERE LOOK AT THE BUFFER %s\n");
//    printf("HELLO THERE LOOK AT THE BUFFER %s\n");
//    printf("HELLO THERE LOOK AT THE BUFFER %s\n");
//    printf("HELLO THERE LOOK AT THE BUFFER %s\n");
//    printf("HELLO THERE LOOK AT THE BUFFER %s\n");
//    printf("HELLO THERE LOOK AT THE BUFFER %s\n");
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
    int indirect_size = size % (BLOCK_SIZE * 8);


    char indirect_block[512];
    bm->read_block(ino->blocks[8], indirect_block);
    int *ptr = (int*)indirect_block;

    for(int x; x < 128; x++){
        bm->free_block(ptr[x]);
    }
//    DONE FREEING ALL THE BLOCKS

    printf("FINISHED FREEING ALL THE BLOCKS\n");

    bool need_to_have_indirect_blocks = false;
    if(size > (8*BLOCK_SIZE)){
        need_to_have_indirect_blocks = true;
    }


    int remaining_bytes = size;
    int num_bytes_already_written = 0;





    int indirect_block_count = 0;



    int index = 0;

    while(remaining_bytes > 0){




//        can we write a full block?
        if(remaining_bytes > BLOCK_SIZE){

//            we can write a full 512 bytes


//              is this block going to be a direct block or an indirect block (ie have we already written 4096 bytes)
            if(num_bytes_already_written > (8*BLOCK_SIZE)){
//                full INDIRECT Block case
//                going into the indirect section

                int block_id = bm->alloc_block();

                char full_indirect_buffer[512];
                memcpy(full_indirect_buffer, buf + (index*BLOCK_SIZE), BLOCK_SIZE);
//                    have all what we need in this full_indirect buffer
//                      now need to write this buffer to memory and hold onto its pointer
                bm->write_block(block_id, full_indirect_buffer);


                int *ptr = &block_id;
                memcpy(&ino->blocks[8] + (indirect_block_count * 4), ptr, 4);

                num_bytes_already_written += BLOCK_SIZE;
                remaining_bytes -= BLOCK_SIZE;






            } else {
//                full direct block case
//                going into the direct section

                printf("WRIting A FULL BLOCK TO THE DIRECT SECTiON\n");
                int block_id = bm->alloc_block();
                char full_block_buffer[512];
                memcpy(full_block_buffer, buf + (index * BLOCK_SIZE), BLOCK_SIZE);


                bm->write_block(block_id, full_block_buffer);


                ino->blocks[index] = block_id;


                num_bytes_already_written += BLOCK_SIZE;
                remaining_bytes -= BLOCK_SIZE;


            }



        } else{

//            can only write part of a block


//            does this part of a block go into an indirect block or a direct block


            if (num_bytes_already_written > (8*BLOCK_SIZE)){
                printf("WRITING A PARTIAL BLOCK TO THE INDIRECT SECTION\n");
//                INDIRECT PARTIAL BLOCK CASE
//                going into the indirect section


                int block_id = bm->alloc_block();

                char partial_indirect_buffer[remaining_bytes];
                memcpy(partial_indirect_buffer, buf + (index*BLOCK_SIZE), remaining_bytes);



//                TODO: THIS MAY BE INCORRECT SAME WITH THE CALL BELOW
                bm->write_block(block_id, partial_indirect_buffer);


                int *ptr = &block_id;

                memcpy(&ino->blocks[8] + (indirect_block_count * 4), ptr, 4);



                num_bytes_already_written += remaining_bytes;
                remaining_bytes -= remaining_bytes;



            } else {
//                going into the direct section
                printf("WRITING A PARTIAL BLOCK IN THE DIRECT SECTION\n");

                int block_id = bm->alloc_block();

                // read in the number of bytes we need
                char partial_direct_buffer[remaining_bytes];
                memcpy(partial_direct_buffer, buf + (index*BLOCK_SIZE), remaining_bytes);

                // go ahead and write that block
                bm->write_block(block_id, partial_direct_buffer);
                ino->blocks[index] = block_id;

                printf("the block is stored at address: %p\n", &block_id);

                num_bytes_already_written += remaining_bytes;
                remaining_bytes -= remaining_bytes;

            }

//            end dealing with partial block
        }


        index += 1;

//        end while
    }

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
