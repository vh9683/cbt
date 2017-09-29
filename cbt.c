#define _POSIX_C_SOURCE 200112
#define uint8 uint8_t
#define uint32 uint32_t

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include <sys/types.h>
#include <errno.h>
#include "cbt.h"

#define CBT_SAVE_DATA(t,dptr,data) \
{ \
  if(t->data_size) \
  { \
    dptr = t->allocator(t->data_size); \
    memcpy(dptr,data,t->data_size); \
  } \
  else \
  { \
    dptr = data; \
  } \
}

#define CBT_CLEAR_DATA(t,dptr) \
{ \
  if(t->data_size) \
  { \
    t->deallocator(dptr); \
  } \
  dptr = NULL; \
}

void cbt_initialize(cbt_tree_t *t,
                    size_t data_size,
                    size_t key_size,
                    char* (*key)(void*),
                    void* (*allocator)(size_t),
                    void (*deallocator)(void*),
                    int (*comparator)(const void*, const void*, size_t))
{
  t->root = NULL;
  t->count = 0;
  t->key = key;
  t->data_size = data_size;
  t->key_size = key_size;
  t->allocator = allocator;
  t->deallocator = deallocator;
  t->comparator = comparator;
}

int cbt_contains(cbt_tree_t *t, void *data)
{
  cbt_t *p = t->root;
  char *u = t->key(data);
  uint8 dir;

  if(!p)
  {
    return 0;
  }

  while(1)
  {
    dir = (u[p->diffbyte] > p->byte) ? 1 : 0;
    if(p->isext[dir])
    {
      break;
    }
    p = (cbt_t*)p->ptr[dir];
  }

  if(!p->ptr[dir])
  {
    return 0;
  }

  return (0 == t->comparator(data,p->ptr[dir],t->key_size));
}


int cbt_insert(cbt_tree_t *t, void *data)
{
  cbt_t *p= t->root;
  char *u = t->key(data);

  if(!p)
  {
    // empty tree
    t->root = t->allocator(sizeof(cbt_t));
    t->root->byte = 0;
    t->root->diffbyte = 0;
    CBT_SAVE_DATA(t,t->root->ptr[1],data);
    t->root->ptr[0] = NULL;
    t->root->isext[0] = true;
    t->root->isext[1] = true;
    t->count++;
    return 2;
  }
  else if(!t->root->ptr[1])
  {
    // single member tree
    uint8 i;
    uint8 dir;
    void *odata = t->root->ptr[1];
    char *q = t->key(odata);

    for(i = 0; i < t->key_size; i++)
    {
      if(u[i] != q[i])
      {
        dir = (u[i] > q[i]) ? 1 : 0;
        CBT_SAVE_DATA(t,t->root->ptr[dir],data);
        t->root->ptr[1-dir] = odata;
        q = t->key(t->root->ptr[0]);
        t->root->byte = q[i];
        t->root->diffbyte = i;
        t->root->isext[dir] = true;
        t->root->isext[1-dir] = true;
        t->count++;
        return 2;
      }
    }
    // string already exists
    return 1;
  }
  else
  {
    // multiple member tree
    cbt_t *p = t->root;
    uint8 i;
    void *odata;
    char *q;
    uint8 dir;

    while(1)
    {
      dir = (u[p->diffbyte] > p->byte) ? 1 : 0;
      if(p->isext[dir])
      {
        break;
      }
      p = (cbt_t*)p->ptr[dir];
    }

    if(t->comparator(data,p->ptr[dir],t->key_size))
    {
      // element already present and not same
      cbt_t *r = t->allocator(sizeof(cbt_t));
      odata = p->ptr[dir];
      p->ptr[dir] = r;
      p->isext[dir] = false;
      q = t->key(odata);
      for(i = 0; i < t->key_size; i++)
      {
        if(u[i] != q[i])
        {
          r->diffbyte = i;
          dir = (u[i] > q[i]) ? 1 : 0;
          break;
        }
      }
      CBT_SAVE_DATA(t,r->ptr[dir],data);
      r->ptr[1-dir] = odata;
      r->byte = dir ? q[i] : u[i];
      r->isext[0] = r->isext[1] = true;
      t->count++;
      return 2;
    }
    // string already exists
    return 1;
  }
}

int cbt_delete(cbt_tree_t *t, void *data)
{
  cbt_t *p = t->root;
  cbt_t *q = t->root;
  char *u = t->key(data);
  uint8 dir, odir;

  if(!p)return 0;

  if(p->isext[1] && p->isext[0])
  {
    // max two members in tree
    dir = (u[p->diffbyte] > p->byte) ? 1 : 0;
    if (!p->ptr[dir] ||
        t->comparator(data,p->ptr[dir],t->key_size))
    {
      // element not present
      return 0;
    }
    else if(p->ptr[1-dir])
    {
      CBT_CLEAR_DATA(t,p->ptr[dir]);
      if(dir)
      {
        p->ptr[dir] = p->ptr[1-dir];
      }
      p->diffbyte = 0;
      p->byte = 0;
      t->count--;
      return 1;
    }
    else
    {
      CBT_CLEAR_DATA(t,p->ptr[dir]);
      t->deallocator(t->root);
      t->root = NULL;
      t->count--;
      return 1;
    }
  }

  while(1)
  {
    dir = (u[p->diffbyte] > p->byte) ? 1 : 0;
    if(p->isext[dir])
    {
      break;
    }
    q = p;
    odir = dir;
    p = (cbt_t*)p->ptr[dir];
  }

  if(t->comparator(data,p->ptr[dir],t->key_size))
  {
    // element not found
    return 0;
  }

  if (p == q)
  {
    // root is changing
    CBT_CLEAR_DATA(t,p->ptr[dir]);
    t->root = p->ptr[1-dir];
    t->deallocator(p);
    t->count--;
    return 1;
  }

  CBT_CLEAR_DATA(t,p->ptr[dir]);
  q->ptr[odir] = p->ptr[1-dir];
  q->isext[odir] = p->isext[1-dir];
  t->deallocator(p);
  t->count--;
  return 1;
}


static void cbt_node_traverse(cbt_t *p,
                              void (*handler)(void*))
{
  if(!p)
  {
    return;
  }

  if(p->isext[0] && p->isext[1])
  {
    if(p->ptr[0]) handler(p->ptr[0]);
    if(p->ptr[1]) handler(p->ptr[1]);
    return;
  }
  else if(p->isext[0] && !p->isext[1])
  {
    if(p->ptr[0]) handler(p->ptr[0]);
    cbt_node_traverse(p->ptr[1],handler);
  }
  else if(p->isext[1] && !p->isext[0])
  {
    if(p->ptr[1]) handler(p->ptr[1]);
    cbt_node_traverse(p->ptr[0],handler);
  }
  else
  {
    cbt_node_traverse(p->ptr[0],handler);
    cbt_node_traverse(p->ptr[1],handler);
  }
}

void cbt_traverse(cbt_tree_t *t,
                  void (*handler)(void*))
{
  cbt_node_traverse(t->root,handler);
}

void cbt_clear(cbt_tree_t *t)
{
  while(t->root)
  {
    if(t->root->isext[0] && t->root->ptr[0])
    {
      cbt_delete(t,t->root->ptr[0]);
    }
    if(t->root->isext[1] && t->root->ptr[1])
    {
      cbt_delete(t,t->root->ptr[1]);
    }
  }
}

unsigned int cbt_count(cbt_tree_t *t)
{
  return t->count;
}

