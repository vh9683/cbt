#ifndef CBT_H_INCLUDED
#define CBT_H_INCLUDED

#define true 1
#define false 0

#define MIN(a,b) ((a) < (b) ? a : b)

typedef struct
{
  unsigned char isext[2];
  unsigned char diffbyte;
  unsigned char byte;
  void *ptr[2];
} cbt_t;

typedef struct
{
  cbt_t *root; // actual tree
  size_t data_size; // if 0, tree will not allocate space for data
  size_t key_size; // passed to comparator, used as length of key
  unsigned int count; // number of elements in tree
  char* (*key)(void*); // function to get key from data
  void* (*allocator)(size_t); // function used to allocate space for cbt_t as well as data
  void (*deallocator)(void*); // function used to deallocate space for cbt_t as well as data 
  int (*comparator)(const void*, const void*, size_t); // function used to comapre two elements
} cbt_tree_t;

void cbt_initialize(cbt_tree_t *t,
                    size_t data_size,
                    size_t key_size,
                    char* (*key)(void*),
                    void* (*allocator)(size_t),
                    void (*deallocator)(void*),
                    int (*comparator)(const void*, const void*, size_t));

int cbt_contains(cbt_tree_t *t, void *data);

int cbt_insert(cbt_tree_t *t, void *data);

int cbt_delete(cbt_tree_t *t, void *data);

void cbt_traverse(cbt_tree_t *t,
                  void (*handler)(void*));

void cbt_clear(cbt_tree_t *t);

unsigned int cbt_count(cbt_tree_t *t);

#endif // CBT_H_INCLUDED
