/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* Basic constants and macros */
#define WSIZE 4             /* Word and header/footer size (bytes) */
#define DSIZE 8             /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
/* ~0x0007 = 0xfff8 */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* heap checking*/
// #define HEAP_CHECK(lineno) checkheap(lineno)
#define HEAP_CHECK(lineno)

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* Private global variables */
static char *mem_heap;           /* Points to first byte of heap */
static char *mem_brk;            /* Points to last byte of heap plus 1 */
static char *mem_max_addr;       /* Max legal heap addr plus 1*/
static size_t mem_max_space = 0; /* Max space for heap*/

/* helper function*/
static void *extend_heap(size_t words);
static void *coalesce(char *bp);
int is_coalesce();
static void set_block(char *pos, size_t size, int is_allocate);
static void allocate_block(char *pos, size_t size);
static void free_block(char *pos);
static void split_space(char *cur_pos, size_t size);
static void checkheap(int lineno);
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /*构建padding block， 头节点和尾节点*/
    if ((mem_heap = mem_sbrk(4 * WSIZE)) == (void *)(-1))
    {
        return -1;
    }
    PUT(mem_heap, 0);
    PUT(mem_heap + WSIZE, PACK(DSIZE, 1));
    PUT(mem_heap + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(mem_heap + (3 * WSIZE), PACK(0, 1));
    mem_heap += (2 * WSIZE); // moved to the true start of the heap;
    mem_max_space = 2 * WSIZE;
    if ((extend_heap(CHUNKSIZE / WSIZE)) == NULL)
    {
        return -1;
    }

    HEAP_CHECK(__LINE__);
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    HEAP_CHECK(__LINE__);
    size = ALIGN(size + 2 * WSIZE);
    char *cur_pos = mem_heap;
    char *new_space;
    // interate throught the whole heap list
    while (cur_pos < mem_max_addr)
    {
        size_t cur_size = GET_SIZE(HDRP(cur_pos));
        //找到合适的位置
        if (GET_ALLOC(HDRP(cur_pos)) == 0 &&
            (size <= cur_size))
        {
            split_space(cur_pos, size);
            new_space = cur_pos;
            break;
        }
        cur_pos = NEXT_BLKP(cur_pos);
    }
    if (cur_pos >= mem_max_addr)
    {
        if ((new_space = extend_heap(CHUNKSIZE / WSIZE)) == NULL)
        {
            return NULL;
        }
        new_space = coalesce(new_space);
        split_space(new_space, size);
        HEAP_CHECK(__LINE__);
    }
    return new_space;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    HEAP_CHECK(__LINE__);
    set_block(ptr, GET_SIZE(HDRP(ptr)), 0);
    coalesce(ptr);
    HEAP_CHECK(__LINE__);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if (size == 0)
    {
        mm_free(ptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL)
    {
        return mm_malloc(size);
    }

    newptr = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if (!newptr)
    {
        return 0;
    }

    /* Copy the old data. */

    oldsize = GET_SIZE(HDRP(ptr));
    if (size < oldsize)
        oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    mm_free(ptr);

    return newptr;
}

/*
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void *extend_heap(size_t words)
{
    HEAP_CHECK(__LINE__);
    char *bp;
    size_t size;
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; // line:vm:mm:beginextend
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL; // line:vm:mm:endextend
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */           // line:vm:mm:freeblockhdr
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */           // line:vm:mm:freeblockftr
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ // line:vm:mm:newepihdr
    mem_brk = NEXT_BLKP(bp);
    mem_max_addr = FTRP(bp) - 1;
    mem_max_space += size;
    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/*
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void *coalesce(char *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc && next_alloc)
    { /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc)
    { /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc)
    { /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else
    { /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    HEAP_CHECK(__LINE__);
    return bp;
}

/*********************************************************
 * Heap checker
 ********************************************************/
static void checkheap(int lineno)

{
    char *cur_pos = mem_heap;
    int is_prev_empty = 0;
    int is_coalesce = 1;
    int is_head_tail_equal = 1;
    int is_cloased_boundary = 1;
    size_t total_space = 0;
    // interate throught the whole heap list
    while (cur_pos < mem_max_addr)
    {
        int is_alloc = GET_ALLOC(HDRP(cur_pos));
        /*检查是否coalesce*/
        if (is_prev_empty == 1 && (!is_alloc))
        {

            is_coalesce = 0;
        }
        is_prev_empty = is_alloc ? 0 : 1;

        /*检查头尾tag是否相同*/
        for (int i = 0; i < WSIZE; i++)
        {
            if (*((char *)HDRP(cur_pos) + i) != *((char *)FTRP(cur_pos) + i))
            {
                is_head_tail_equal = 0;
                break;
            }
        }
        /*检查一个block之前是否有一个boundary tag*/
        if ((cur_pos != mem_heap) && NEXT_BLKP(PREV_BLKP(cur_pos)) != cur_pos)
        {

            is_cloased_boundary = 0;
        }
        /*记录总共在标签里面的size的总和*/
        total_space += GET_SIZE(HDRP(cur_pos));
        cur_pos = NEXT_BLKP(cur_pos);
    }

    if (!(is_coalesce && is_head_tail_equal && is_cloased_boundary && (total_space == mem_max_space)))
        printf("line %d: ", lineno);

    /*检查是否coalesce*/
    if (!is_coalesce)
        printf("Free block is not coalescing in block %lx\n", (unsigned long int)cur_pos);

    /*检查头尾tag是否相同*/
    if (!is_head_tail_equal)
        printf("head block is not the same with tail block \n");

    /*检查头尾tag是否相同*/
    if (!is_cloased_boundary)
        printf("boundary is not closed\n");
    if (total_space != mem_max_space)
        printf("space size error\n");
}

static void set_block(char *pos, size_t size, int is_allocate)
{
    // head
    PUT(HDRP(pos), PACK(size, is_allocate));
    // tail
    PUT(FTRP(pos), PACK(size, is_allocate));
}

static void allocate_block(char *pos, size_t size)
{
    set_block(pos, size, 1);
}

static void free_block(char *pos)
{
    set_block(pos, GET_SIZE(HDRP(pos)), 0);
}

static void split_space(char *cur_pos, size_t size)
{
    HEAP_CHECK(__LINE__);
    size_t cur_size = GET_SIZE(HDRP(cur_pos));
    // 如果空间不够;
    if (cur_size < size)
    {
        printf("line %d space is not enough\n", __LINE__);
        return;
    }

    allocate_block(cur_pos, size);
    // split the block if current block is bigger than 2 words
    if (cur_size - size > DSIZE)
    {
        set_block(NEXT_BLKP(cur_pos), cur_size - size, 0);
    }
    HEAP_CHECK(__LINE__);
}

#ifdef MAIN
int main()
{
    if (mm_init() < 0)
    {
        printf("initialization is failed");
        return 0;
    }
    // 检查coaleasce heap checker是否起作用
    extend_heap(10);
    HEAP_CHECK(__LINE__);
    extend_heap(10);
    mm_malloc(8);
    HEAP_CHECK(__LINE__);
    return 0;
}
#endif