#include "alloc.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

const int hsize = sizeof(struct header);

void *init_break;
struct header *head = NULL;
enum algs current_alg;
uint64_t current_limit = 0;
uint64_t current_brk_size = 0;

void initial_alloc(void) {
  init_break = sbrk(0);
  head = init_break;

  struct header *init_header_ptr = (struct header *)init_break;

  sbrk(INCREMENT);
  current_brk_size = INCREMENT;

  struct header init_header = {INCREMENT, NULL};
  *init_header_ptr = init_header;
}

void remove_node(struct header *remove) {

  struct header *curr = head;
  struct header *prev = NULL;

  if (head == remove) {
    head = head->next;
    return;
  }

  while (curr != NULL) {
    if (curr == remove) {
      if (curr == head) {
        head = curr->next;
      } else if (prev != NULL) {
        prev->next = curr->next;
      }
      return;
    }

    prev = curr;
    curr = curr->next;
  }
}

void split_block(int nbytes, struct header *block) {

  int og_size = block->size;
  // Block which is being created, leftover size
  int leftover_size = og_size - nbytes - hsize;

  struct header *prev_head = head;

  // Returned block size
  int ret_size = og_size - leftover_size;

  // Last block which not big enough.
  if (leftover_size <= hsize) {
    remove_node(block);
    return;
  }

  block->size = ret_size;
  struct header new = {leftover_size, prev_head->next};

  void *new_block_ptr = ((void *)block) + ret_size;

  remove_node(block);

  head = new_block_ptr;
  *head = new;
}

void *first_fit(int nbytes) {

  struct header *curr = head;
  while (curr != NULL) {

    if ((int)curr->size >= nbytes) {
      return curr;
    }

    curr = curr->next;
  }

  return NULL;
}

void *best_fit(int nbytes) {
  uint64_t current_best_size = current_brk_size + 1;
  struct header *current_best = NULL;

  struct header *curr = head;
  while (curr != NULL) {
    if ((int)curr->size - hsize >= nbytes) {
      if (curr->size < current_best_size) {
        current_best_size = curr->size;
        current_best = curr;
      }
    }
    curr = curr->next;
  }

  return current_best;
}

void *worst_fit(int nbytes) {
  uint64_t current_worst_size = 0;
  struct header *current_worst = NULL;

  struct header *curr = head;
  while (curr != NULL) {
    if ((int)curr->size - hsize >= nbytes) {
      if (curr->size > current_worst_size) {
        current_worst_size = curr->size;
        current_worst = curr;
      }
    }
    curr = curr->next;
  }

  return current_worst;
}

void coalesce(void) {

  struct header *curr = head;
  while (curr != NULL) {
    struct header *next = curr->next;

    while (next != NULL) {
      if (((void *)curr + curr->size) == (void *)next) {
        curr->size = curr->size + next->size;
        remove_node(next);
        next = curr->next;
      } else if (((void *)next + next->size) == (void *)curr) {
        next->size = next->size + curr->size;
        remove_node(curr);
        next = curr->next;
      } else {
        next = next->next;
      }
    }

    curr = curr->next;
  }
}

// Return 0 if success
// Return -1 if error
int increase_brk() {
  if (current_brk_size + INCREMENT > current_limit) {
    return -1;
  }

  struct header *new_block_ptr = (struct header *)sbrk(INCREMENT);

  if (new_block_ptr == (void *)-1) {
    return -1;
  }

  current_brk_size = current_brk_size + INCREMENT;

  struct header *new_next_ptr = NULL;

  if (head != NULL) {
    new_next_ptr = head;
  }

  struct header new = {INCREMENT, new_next_ptr};

  head = new_block_ptr;
  *head = new;

  coalesce();

  return 0;
}

void *alloc(int nbytes) {

  // First alloc
  if (current_brk_size == 0) {
    initial_alloc();
  }

  struct header *ret = NULL;

  while (ret == NULL) {
    switch (current_alg) {
    case FIRST_FIT:
      ret = first_fit(nbytes);
      break;
    case BEST_FIT:
      ret = best_fit(nbytes);
      break;
    case WORST_FIT:
      ret = worst_fit(nbytes);
      break;
    default:
      ret = first_fit(nbytes);
      break;
    }

    if (ret == NULL) {
      int brk_response = increase_brk();

      if (brk_response == -1) {
        return NULL;
      }
    }
  }

  split_block(nbytes, ret);

  ret->next = NULL;
  void *return_ptr = (void *)ret;
  return_ptr = return_ptr + sizeof(struct header);
  return return_ptr;
}

void dealloc(void *ptr) {

  ptr = ptr - hsize;
  struct header *header_ptr = (struct header *)ptr;

  header_ptr->next = head;
  head = header_ptr;

  coalesce();
}

void allocopt(enum algs alg, int size_limit) {

  // Reset everything
  head = NULL;
  brk(init_break);
  current_brk_size = 0;

  // Set vars
  current_alg = alg;
  current_limit = size_limit;
}

struct allocinfo allocinfo(void) {
  int free_size = 0;
  int free_chunks = 0;
  int largest_free_chunk_size = 0;
  int smallest_free_chunk_size = current_brk_size + 1;

  struct header *curr = head;
  while (curr != NULL) {

    free_size = free_size + curr->size - hsize;
    free_chunks++;

    if ((int)curr->size > largest_free_chunk_size) {
      largest_free_chunk_size = curr->size;
    }

    if ((int)curr->size < smallest_free_chunk_size) {
      smallest_free_chunk_size = curr->size;
    }

    curr = curr->next;
  }

  struct allocinfo ret = {free_size, free_chunks, largest_free_chunk_size,
                          smallest_free_chunk_size};

  return ret;
}
