#include <stdlib.h>
#include "cache.h"
#define LINE_SIZE 64
#define OFFSET_BITS 6
#define OFFSET_MASK 0x3f
#define CACHE_HIT 0
#define CACHE_MISS 1

typedef struct {
    void *block;
    unsigned long tag;
    char valid;
} cache_line;

typedef struct {
    cache_line *lines;
    unsigned int victim_counter;
    char initialized;
} cache_info;

// Function to initialize the cache
static void init(unsigned int num_lines, cache_info *cache) {
    cache->initialized = 1;
    cache->victim_counter = 0; // eviction method, goes full circle
    unsigned int lines_size = num_lines * sizeof(cache_line);

    // Allocate memory for cache lines and set block pointers
    cache->lines = (cache_line *) (cache + sizeof(char) + sizeof(long));
    char *block_start = (char *)(cache->lines + lines_size);
    for (int i = 0; i < num_lines; i++) {
        cache->lines[i].valid = 0;
        cache->lines[i].block = block_start + i * LINE_SIZE;
    }
}

extern int cache_get(unsigned long address, unsigned long *value) {
    // Fully Associative Cache
    cache_info *cache = (cache_info *) c_info.F_memory;
    unsigned int num_lines = c_info.F_size / LINE_SIZE;
    unsigned long tag = address >> (OFFSET_BITS);
    unsigned long offset = address & OFFSET_MASK;

    if (!cache->initialized) {
        init(num_lines, cache);
    }

    cache_line *lines = cache->lines;
    // Search for data in the cache
    for (int i = 0; i < num_lines; i++) {
        cache_line *line = &lines[i];
        // Data found in cache
        if(line->tag == tag && line->valid==1) {
            // If data spans two blocks
            if ((64 - offset) < sizeof(long)) {
                for (int j = 0; j < num_lines; ++j) {
                    cache_line *line2 = &lines[j];
                    if(line2->tag == tag+1) {
                        unsigned long first_block = *(unsigned long *)(line->block + offset);
                        unsigned long second_block = *(unsigned long *)(line2->block);

                        // split into two parts according the offset
                        unsigned long second_part = second_block << (64-offset)*8;
                        unsigned long first_part = (first_block<<((8-(64-offset))*8))>>((8-(64-offset))*8);

                        //combine
                        *value = second_part | first_part;
                        return CACHE_HIT;
                    }
                }
            } else {
                // Data fits within a single block
                *value = *(unsigned long *) (lines[i].block + offset);
                return CACHE_HIT;
            }
        }
    }

    // Data not found in cache, perform cache miss operations
    unsigned long victim_counter = cache->victim_counter;
    cache_line *victim_line = &lines[victim_counter];
    cache->victim_counter = (victim_counter + 1) % num_lines;
    victim_line->tag = tag;
    victim_line->valid = 1;

    void *buffer = victim_line->block;
    unsigned long address_at_block_start = address-offset;

    // If data spans two blocks
    if ((64 - offset) < sizeof(long)) {
        memget(address_at_block_start, buffer, LINE_SIZE);

        // store to next block
        unsigned long next_address = address_at_block_start + LINE_SIZE;
        cache->victim_counter = (victim_counter + 1) % num_lines;
        cache_line *next_line = &lines[cache->victim_counter];
        next_line->tag = tag + 1;
        next_line->valid = 1;
        void *next_buffer = next_line->block;
        memget(next_address, next_buffer, LINE_SIZE);

        unsigned long first_block = *(unsigned long *)(buffer + offset);
        unsigned long second_block = *(unsigned long *)(next_buffer);

        // split into two parts according the offset
        unsigned long second_part = second_block << (64-offset)*8;
        unsigned long first_part = (first_block<<((8-(64-offset))*8))>>((8-(64-offset))*8);

        // combine
        *value = second_part | first_part;
        return CACHE_MISS;
    } else {
        memget(address_at_block_start, buffer, LINE_SIZE);
    }

    *value = *(unsigned long*)(buffer+offset);
    return CACHE_MISS;
}
