/*
 ** Definitions of AVL APIs based on threaded height-balanced tree
 ** implementation. Threaded AVL trees provide fast access, O(log N),
 ** to data nodes AND allow efficient, O(1), sequential access.
 */

#ifndef AVL_H
#define AVL_H 1

/* Include files. */
#include <stdio.h>
#include <stdlib.h>

/* Macro constant definitions. */
#define OG_AVL 1

#ifdef AVL_DEBUG
#define AVL_MESG_OUT(format, args...)\
	printf((format), ##args)
#else /* AVL_DEBUG */
#define AVL_MESG_OUT(format, args...)
#endif /* AVL_DEBUG */

/* Type definitions. */
typedef int avl_comparison_func(void *avl_a, void *avl_b, void *avl_param);
typedef struct tavlnode *TAVL_nodeptr;
typedef struct tavltree *TAVL_treeptr;

typedef struct avl_allocator
{
	void *(*avl_malloc)(struct avl_allocator *, size_t avl_size);
	void (*avl_free)(struct avl_allocator *, void *avl_block);
} tAvlAllocator;

/* Tree data structure. */
typedef struct avl_table
{
	TAVL_treeptr tavltree;              /* Point to tavltree. */
	tAvlAllocator *avl_alloc;           /* Memory allocator. */
	avl_comparison_func *avl_compare;   /* Comparison function. */
	void *avl_param;                    /* Extra argument to |avl_compare|. */
	size_t avl_count;                   /* Number of items in tree. */
	unsigned int avl_data_size;         /* Size of data in avl_node. */
} tAvlTable;

typedef struct avl_node
{
	struct avl_node *avl_link[2];  /* Subtrees. */
} tAvlNode;

/* AVL traverser structure. */
typedef struct avl_traverser
{
	struct avl_table *avl_table;        /* Tree being traversed. */
	TAVL_nodeptr tavl_curP;            /* For tavl traverser. */
} tAvlTraverser;


/* External function declarations. */

/* Table functions. */
struct avl_table *avl_create(avl_comparison_func *, void *, tAvlAllocator *);
void avl_destroy(struct avl_table *, void *);
void *avl_insert(struct avl_table *, void *);
void *avl_delete(struct avl_table *, void *, tAvlTraverser *trav);
void *avl_find(const struct avl_table *, void *);

/* Traverser functions. */
void *avl_t_first(struct avl_traverser *, struct avl_table *);
void *avl_t_last(struct avl_traverser *, struct avl_table *);
void *avl_t_find(struct avl_traverser *, struct avl_table *, void *);
void *avl_t_curr(struct avl_traverser *);
int avl_t_ptr_to_head(struct avl_traverser *trav);
void *avl_t_next(struct avl_traverser *);
void *avl_t_prev(struct avl_traverser *);


/* Macro API definitions. */


/* Global variable declarations. */

#endif /* AVL_H */
