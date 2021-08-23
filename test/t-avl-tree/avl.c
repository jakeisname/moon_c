/*
 ** Implementation of Threaded AVL tree.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "avl.h"

/* Local function declarations. */

void *KeyOfItem(void *dataP);
void *CreateItem(void *dataP);
void FreeItem(void *dataP);
void *CopyItem(void *destDataP, void *srcDataP);

/* memory alloc function. you should replace the "true" routine in it. */
void *MemAlloc(size_t size);
void MemFree(void *ptr);

/* Avl alloc function. use MemAlloc() and MemFree(). */
void *avl_malloc (tAvlAllocator *, size_t);
void avl_free (tAvlAllocator *, void *);

/* tavl local function. */
static TAVL_nodeptr remove_node(TAVL_treeptr tree, TAVL_nodeptr p, char *deltaht);
static TAVL_nodeptr remove_max(TAVL_nodeptr p, TAVL_nodeptr *maxnode, char *deltaht);
static TAVL_nodeptr remove_min(TAVL_nodeptr p, TAVL_nodeptr *minnode, char *deltaht);


/* Macro API definitions. */

#ifdef AVL_DEBUG
#define assert(condM) ((!(condM)) ? AVL_MESG_OUT("assert failed") : 0)
#else /* AVL_DEBUG */
#define assert(condM) ((void)0)
#endif /* AVL_DEBUG */

#define UnUseArg(arg)

/* Global variable definitions */

/* Default memory allocator. */
tAvlAllocator avl_allocator_default =
{
	avl_malloc,
	avl_free
};


/**************************************************************************
 ** TAVL library public functions, types & constants.
 **************************************************************************/

/* Constants for "replace" parameter of "tavl_insert" */
#define REPLACE     1
#define NO_REPLACE  0

/* prototypes */

TAVL_treeptr tavl_init(
		int (*compare)(void *, void *,void *),/* compares identifiers */
		void *paramP, /* third parameter of compare(). */

		void *(*key_of)(void *DataObject),
		void *(*make_item)(void *DataObject),
		void (*free_item)(void *DataObject),
		void *(*copy_item)(void *Destination_DataObject,\
			void *Source_DataObject),
		void *(*alloc)(size_t),
		void (*dealloc)(void *)
		);
/*
   Returns pointer to empty tree on success, NULL if insufficient
   memory.  The function pointers passed to "tavl_init" determine
   how that instance of tavl_tree will behave & how it will use
   dynamic memory.

   parameters-
compare:      Compares identifiers, same form as "strcmp".
key_of:       Gets pointer to a data object's identifier.
make_item:    Creates new data object that is a copy of
 *DataObject.
free_item:    Complements make_item. Releases any memory
allocated to DataObject by "make_item".
copy_item:    Copies data object *Source to buffer *Dest
alloc:        Memory allocator.
dealloc:      Deallocates dynamic memory - complements
"alloc"
*/

TAVL_nodeptr tavl_insert(TAVL_treeptr tree, void *item, int replace);
/*
   Using the user supplied "key_of" & "compare" functions,
 *tree is searched for a node which matches *item. If a
 match is found, the new item replaces the old if & only
 if replace != 0.  If no match is found the item is
 inserted into *tree.  "tavl_insert" returns a pointer to
 the node inserted or found, or NULL if there is not enough
 memory to create a new node and copy "item".  Uses functions
 "key_of" and "compare" for comparisons and to retrieve
 identifiers from data objects, "make_item" to create a copy
 of "item", "alloc" to get memory for the new tree node, and
 "dealloc" if "make_item" fails.
 */
void *tavl_delete(TAVL_treeptr tree, void *key, tAvlTraverser *trav);
/*
   Delete node identified by "key" from *tree.
   Returns a Data-Pointer if found and deleted, NULL if not found.
   Uses "compare", "key_of", "free_item" and "dealloc".
   See function tavl_init.
   */

void tavl_destroy(TAVL_treeptr tree);
/*
   Destroy the tree. Uses functions "free_item" and "dealloc"
   to restore pool memory used. See function tavl_init.
   */

TAVL_nodeptr tavl_find(TAVL_treeptr tree, void *key);
/*
   Returns pointer to node which contains data item
   in *tree whose identifier equals "key". Uses "key_of"
   to retrieve identifier of data items in the tree,
   "compare" to compare the identifier retrieved with
 *key.  Returns NULL if *key is not found.
 */

/********************************************************************
  Following three functions allow you to treat tavl_trees as a
  doubly linked sorted list with a head node.  This is the point
  of threaded trees - it is almost as efficient to move from node
  to node or back with a threaded tree as it is with a linked list.
 *********************************************************************/

TAVL_nodeptr tavl_reset(TAVL_treeptr tree);
/*
   Returns pointer to begin/end of *tree (the head node).
   A subsequent call to tavl_succ will return a pointer
   to the node containing first (least) item in the tree;
   just as a call to tavl_pred would return the last
   (greatest).  Pointer returned can only be used a parameter
   to "tavl_succ" or "tavl_pred" - the head node contains no
   user data.
   */

TAVL_nodeptr tavl_succ(TAVL_nodeptr p);
/*
   Returns successor of "p", or NULL if "p" has no successor.
   */

TAVL_nodeptr tavl_pred(TAVL_nodeptr p);
/*
   Returns predecessor of "p", or NULL if no predecessor.
   */

/**************      END PUBLIC DEFINITIONS     *******************/

/* Private: for internal use by tavl*.c library routines only! */

/*   See note below
     ... recommended that TAVL_USE_BIT_FIELDS remain commented out,
     ... both for efficiency (speed) and universiality.
#define TAVL_USE_BIT_FIELDS
*/

typedef struct tavlnode {
	void *dataptr;
	struct tavlnode *Lptr, *Rptr;
	/* see NOTE below */
	signed  char bf;            /* assumes values -2..+2 */
	char Lbit;          /* 0 or 1 */
	char Rbit;          /* 0 or 1 */
} TAVL_NODE;

typedef struct tavltree {
	TAVL_nodeptr head;
	int (*cmp)(void *, void *, void*);
	void *(*key_of)(void *);
	void *(*make_item)(void *);
	void (*free_item)(void *);
	void *(*copy_item)(void *, void *);
	void *(*alloc)(size_t);
	void (*dealloc)(void *);
	int avl_count;
	void *paramP;
} TAVL_TREE;

/* end private */

/**************************************************************************
 ** TAVL library private definitions.
 **************************************************************************/

#define RIGHT   -1
#define LEFT    +1
#define THREAD  0
#define LINK    1
#define LLINK(x)    ((x)->Lbit)
#define RLINK(x)    ((x)->Rbit)
#define LTHREAD(x)  (!LLINK(x))
#define RTHREAD(x)  (!RLINK(x))
#define Leftchild(x)    (LLINK(x) ? (x)->Lptr : NULL)
#define Rightchild(x)   (RLINK(x) ? (x)->Rptr : NULL)
#define Is_Head(x)      ((x)->Rptr == (x))
/* always true for head node of initialized */
/* tavl_tree, and false for all other nodes */

#define RECUR_STACK_SIZE 40  /* this is extremely enormous */

TAVL_nodeptr rebalance_tavl(TAVL_nodeptr a, char *deltaht);
/*  Returns pointer to root of rebalanced tree "a".  If rebalance reduces
    the height of tree "a", *deltaht = 1, otherwise *deltaht = 0.
    "rebalance_tavl" is called ONLY by "tavl_insert" and "tavl_delete".
 *deltaht is always 1 when "rebalance_tavl" is called by "tavl_insert";
 however, *deltaht may return 1 or 0 when called by "tavl_delete".
 */

int rebalanced = 0;

/**************************************************************************
 ** AVL library wrapper functions.
 **************************************************************************/

struct avl_table *avl_create(avl_comparison_func *compare, void *param, tAvlAllocator *foo)
{
	struct avl_table *tree;
	TAVL_treeptr avlTreeP;
	tAvlAllocator *allocator;

	assert (compare != NULL);

	if (foo == NULL)
		allocator = &avl_allocator_default;
	else
		allocator = foo;

	tree = allocator->avl_malloc (allocator, sizeof *tree);
	if (tree == NULL) {
		AVL_MESG_OUT("alloc memory for tree struct failed.\n");
		return (NULL);
	}

	avlTreeP = tavl_init(compare, param, KeyOfItem, CreateItem, FreeItem,
			CopyItem, (void*)MemAlloc, MemFree);
	if (avlTreeP == NULL) {
		AVL_MESG_OUT("tavl_init() failed.\n");
		allocator->avl_free (allocator, tree);
		return (NULL);
	}

	tree->tavltree = avlTreeP;
	tree->avl_compare = compare;
	tree->avl_param = param;
	tree->avl_alloc = allocator;
	tree->avl_count = 0;

	return (tree);
}

void avl_destroy(struct avl_table *tree, void *foo)
{
	UnUseArg(foo);

	/* Set compare parameter into tavl tree structure. */
	tree->tavltree->paramP = tree->avl_param;
	/* Free tavl tree. */
	tavl_destroy(tree->tavltree);
	/* Free table. */
	tree->avl_alloc->avl_free (tree->avl_alloc, tree);
}

/*
 **  Note: when an existng node matches the item to be inserted, the funtion
 **  returns a pointer to the existing node.
 */
void *avl_insert(struct avl_table *table, void *item)
{
	TAVL_nodeptr nodeP;

	/* Set compare parameter into tavl tree structure. */
	table->tavltree->paramP = table->avl_param;
	nodeP = tavl_insert(table->tavltree, item, NO_REPLACE);

	/* Update nodes count. */
	table->avl_count = table->tavltree->avl_count;

	AVL_MESG_OUT("### Tree node couter %d avl counter %d.\n",
			table->tavltree->avl_count, table->avl_count);

	return (nodeP ? nodeP->dataptr : NULL);
}

void *avl_delete(struct avl_table *table, void *item, tAvlTraverser *trav)
{
	void *dataP;

	/* Set compare parameter into tavl tree structure. */
	table->tavltree->paramP = table->avl_param;
	dataP = tavl_delete(table->tavltree, item, trav);

	/* Update nodes count. */
	table->avl_count = table->tavltree->avl_count;

	AVL_MESG_OUT("### Tree node couter %d avl counter %d.\n",
			table->tavltree->avl_count, table->avl_count);

	return (dataP);
}

void *avl_find(const struct avl_table *table, void *item)
{
	TAVL_nodeptr nodeP;

	/* Set compare parameter into tavl tree structure. */
	table->tavltree->paramP = table->avl_param;
	nodeP = tavl_find(table->tavltree, item);

	return (nodeP ? nodeP->dataptr : nodeP);
}

void *avl_t_first(struct avl_traverser *trav, struct avl_table *table)
{
	TAVL_nodeptr nodeP;

	trav->avl_table = table;
	/* Set compare parameter into tavl tree structure. */
	table->tavltree->paramP = table->avl_param;
	/* Get the least one. return the data pointer. */
	nodeP = trav->tavl_curP = tavl_succ(tavl_reset(table->tavltree));

	return (nodeP ? nodeP->dataptr : NULL);
}

void *avl_t_last(struct avl_traverser *trav, struct avl_table *table)
{
	TAVL_nodeptr nodeP;

	trav->avl_table = table;
	/* Set compare parameter into tavl tree structure. */
	table->tavltree->paramP = table->avl_param;
	/* Get the greatest one, return the data pointer. */
	nodeP = trav->tavl_curP = tavl_pred(tavl_reset(table->tavltree));

	return (nodeP ? nodeP->dataptr : NULL);
}

void *avl_t_find(struct avl_traverser *trav, struct avl_table *table, void *item)
{
	TAVL_nodeptr nodeP;

	/* Set compare parameter into tavl tree structure. */
	table->tavltree->paramP = table->avl_param;
	nodeP = tavl_find(table->tavltree, item);

	trav->avl_table = table;
	trav->tavl_curP = nodeP;

	return (nodeP ? nodeP->dataptr : NULL);
}

void *avl_t_curr(struct avl_traverser *trav)
{
	TAVL_nodeptr nodeP;

	if (trav == NULL)
		return (NULL);

	if (trav->avl_table == NULL)
		return (NULL);

	/* Set compare parameter into tavl tree structure. */
	trav->avl_table->tavltree->paramP = trav->avl_table->avl_param;
	if (trav->tavl_curP == NULL)
		/* Need init. */
		return (avl_t_first(trav, trav->avl_table));
	else 
		nodeP = trav->tavl_curP;

	return (nodeP ? nodeP->dataptr : NULL);
}

/* 1=node from/to head, 0=not from/to head */
int avl_t_ptr_to_head(struct avl_traverser *trav)
{
	TAVL_nodeptr nodeP;
	TAVL_nodeptr head;

	if (trav == NULL)
		return 1;

	if (trav->avl_table == NULL)
		return 1;

	/* Set compare parameter into tavl tree structure. */
	trav->avl_table->tavltree->paramP = trav->avl_table->avl_param;

	nodeP = trav->tavl_curP;

	if (nodeP) {
		head = tavl_reset(trav->avl_table->tavltree);

		return (nodeP->Lptr == head) || (nodeP->Rptr == head);
	}

	return 1; 
}

void *avl_t_next(struct avl_traverser *trav)
{
	TAVL_nodeptr nodeP;

	if (trav == NULL)
		return (NULL);

	if (trav->avl_table == NULL)
		return (NULL);

	/* Set compare parameter into tavl tree structure. */
	trav->avl_table->tavltree->paramP = trav->avl_table->avl_param;
	if (trav->tavl_curP == NULL)
		/* Need init. */
		return (avl_t_first(trav, trav->avl_table));
	else {
		TAVL_nodeptr tmp = trav->tavl_curP;

		nodeP = trav->tavl_curP = tavl_succ(tmp);
#if 0
		printf("%s(%d): curr node=%p, next node=%p\n", 
				__func__, __LINE__, tmp, nodeP);
#endif
	}

	return (nodeP ? nodeP->dataptr : NULL);
}

void *avl_t_prev(struct avl_traverser *trav)
{
	TAVL_nodeptr nodeP;

	if (trav == NULL)
		return (NULL);

	if (trav->avl_table == NULL)
		return (NULL);

	/* Set compare parameter into tavl tree structure. */
	trav->avl_table->tavltree->paramP = trav->avl_table->avl_param;
	if (trav->tavl_curP == NULL)
		/* Need init. */
		return (avl_t_last(trav, trav->avl_table));
	else
		nodeP = trav->tavl_curP = tavl_pred(trav->tavl_curP);

	return (nodeP ? nodeP->dataptr : NULL);

}

/* Key(Id) of item. */
void *KeyOfItem(void *dataP)
{
	return (dataP);
}

/* Create item. */
void *CreateItem(void *dataP)
{
	/* Just return data pointer. */
	return(dataP);
}

/* Free Item. */
void FreeItem(void *dataP)
{
	/* Free memory. */
	MemFree(dataP);
}

/* Copy Item. No use now. */
void *CopyItem(void *destDataP, void *srcDataP)
{
	return (srcDataP);
}

/* avl memory alloc. */
void *avl_malloc (tAvlAllocator *allocator, size_t size)
{
	assert (allocator != NULL && size > 0);
	return MemAlloc(size);
}

/* avl memory free. */
void avl_free(tAvlAllocator *allocator, void *block)
{
	assert (allocator != NULL && block != NULL);
	MemFree(block);
}

/* memory allocator function. */
void *MemAlloc(size_t size)
{
	return malloc(size);
}

/* memory free function. */
void MemFree(void *ptr)
{
	free(ptr);
}


/**************************************************************************
 ** TAVL library wrapper functions.
 **************************************************************************/

TAVL_treeptr tavl_init(                         
		/* user supplied functions: */
		int (*compare)(void *, void *,void *),/* compares identifiers */
		void *paramP,
		void *(*key_of)(void *),    /* returns item identifier*/
		void *(*make_item)(void *), /* create copy of item */
		void (*free_item)(void *),      /* frees node's data */
		void *(*copy_item)(void *, void *),
		void *(*alloc)(size_t),
		void (*dealloc)(void *))
{
	TAVL_treeptr tree = (*alloc)(sizeof(TAVL_TREE));

	if (tree)   {
		if ((tree->head = (*alloc)(sizeof(TAVL_NODE))) != NULL) {
			tree->cmp = compare;
			tree->key_of = key_of;
			tree->make_item = make_item;
			tree->free_item = free_item;
			tree->copy_item = copy_item;
			tree->alloc = alloc;
			tree->dealloc = dealloc;
			tree->head->bf = 0;
			tree->head->Lbit = THREAD;
			tree->head->Rbit = LINK;
			tree->head->dataptr = NULL;
			tree->head->Lptr = tree->head;
			tree->head->Rptr = tree->head;
			tree->paramP = paramP;
			tree->avl_count = 0;
		} else {
			(*dealloc)(tree);
			tree = NULL;
		}
	}

	return tree;
}

void tavl_destroy(TAVL_treeptr tree)
{
	register TAVL_nodeptr q;
	register TAVL_nodeptr p = tavl_succ(tavl_reset(tree));

	while (p) {
		p = tavl_succ(q = p);
		(*tree->free_item)(q->dataptr);
		(*tree->dealloc)(q);
	}
	(*tree->dealloc)(tree->head);
	(*tree->dealloc)(tree);
}

TAVL_nodeptr tavl_insert(TAVL_treeptr tree, void *item, int replace)
	/*
	   Using the user supplied (key_of) & (cmp) functions, *tree
	   is searched for a node which matches *item. If a match is
	   found, the new item replaces the old if & only if
	   replace != 0.  If no match is found the item is inserted
	   into *tree.  "tavl_insert" returns a pointer to the node
	   inserted into or found in *tree. "tavl_insert" returns
	   NULL if & only if it is unable to allocate memory for
	   a new node.
	   */
{
	TAVL_nodeptr a,y,f;
	register TAVL_nodeptr p,q;
	register int cmpval = -1; /* cmpval must be initialized - if tree is */
	int side;                 /* empty node inserted as LeftChild of head */
	char junk;

	/*  Locate insertion point for item.  "a" keeps track of most
	    recently seen node with (bf != 0) - or it is the top of the
	    tree, if no nodes with (p->bf != 0) are encountered.  "f"
	    is parent of "a".  "q" follows "p" through tree.
	    */
	q = tree->head;   
	a = q;  
	f = NULL;  
	p = Leftchild(q);

	while (p) {
		if (p->bf) { 
			a = p; 
			f = q; 
		}

		q = p;

		cmpval = (*tree->cmp)((*tree->key_of)(item),(*tree->key_of)(p->dataptr),tree->paramP);

		if (cmpval < 0)
			p = Leftchild(p);
		else if (cmpval > 0)
			p = Rightchild(p);
		else {
			if (replace) {
				void *temp = (*tree->make_item)(item);
				if (temp) {
					(*tree->free_item)(p->dataptr);
					p->dataptr = temp;
				} else 
					p = NULL;
			}
			return p;
		}
	}

	/* wasn't found - create new node as child of q */

	y = (*tree->alloc)(sizeof(TAVL_NODE));

	if (y) {
		y->bf = 0;
		y->Lbit = THREAD;
		y->Rbit = THREAD;
		if ((y->dataptr = (*tree->make_item)(item)) == NULL) {
			(*tree->dealloc)(y);
			return NULL;        /* must be out of memory */
		}
	} else 
		return NULL;           /* out of memory */

	if (cmpval < 0) {           /* connect to tree and thread it */
		y->Lptr = q->Lptr;
		y->Rptr = q;
		q->Lbit = LINK;
		q->Lptr = y;
	} else {
		y->Rptr = q->Rptr;
		y->Lptr = q;
		q->Rbit = LINK;
		q->Rptr = y;
	}

	/*  Adjust balance factors on path from a to q.  By definition of "a",
	    all nodes on this path have bf = 0, and so will change to LEFT or
	    RIGHT.
	    */
	/* Increase nodes count. */
	tree->avl_count++;

	printf("%s(%d): alloc node=%p, cnt=%d\n", 
			__func__, __LINE__, y, tree->avl_count);

	if ((a == tree->head) || ((*tree->cmp)((*tree->key_of)(item),
					(*tree->key_of)(a->dataptr),tree->paramP)< 0)) {
		p = a->Lptr; 
		side = LEFT;
	} else {
		p = a->Rptr; 
		side = RIGHT;
	}

	/* adjust balance factors */

	while (p != y) {
		if ((*tree->cmp)((*tree->key_of)(p->dataptr),(*tree->key_of)(item),tree->paramP)> 0) {
			p->bf = LEFT;   
			p = p->Lptr;
		} else {
			p->bf = RIGHT;  
			p = p->Rptr;
		}
	}

	tree->head->bf = 0;     /* if a==tree->head, tree is already balanced */

	/* Is tree balanced? */

	if (abs(a->bf += side) < 2) return y;

	p = rebalance_tavl(a,&junk);

	assert(junk);   /* rebalance always sets junk to 0 */

	assert(f);      /* f was set non-NULL by the search loop */

	if (f->Rptr != a)
		f->Lptr = p;
	else
		f->Rptr = p;

	return y;
}

/*  Development note:  the routines "remove_min" and "remove_max" are
    true recursive routines; i.e., they make calls to themselves. The
    routine "tavl_delete" simulates recursion using a stack (a very deep
    one that should handle any imaginable tree size - up to approximately
    1 million squared nodes).  I arrived at this particular mix by using
    Borland's Turbo Profiler and a list of 60K words as a test file to
    example1.c, which should be included in the distribution package.
    -BCH
    */

void * tavl_delete (TAVL_treeptr tree, void *key, tAvlTraverser *trav)
{
	char rb, deltaht;
	int side;
	int  found = deltaht = 0;
	register TAVL_nodeptr p = Leftchild(tree->head);
	register int cmpval = -1;
	register TAVL_nodeptr q = NULL;
	void *dataP;


	struct stk_item {
		int side;
		TAVL_nodeptr p;
	} block[RECUR_STACK_SIZE];

	struct stk_item *next = block;   /* initialize recursion stack */

#define PUSH_PATH(x,y)  (next->p = (x),  (next++)->side = (y))
#define POP_PATH(x)     (x = (--next)->side, (next->p))

	tree->head->bf = 0;      /* prevent tree->head from being rebalanced */

	PUSH_PATH(tree->head,LEFT);

	while (p) {
		cmpval = (*tree->cmp)(key,(*tree->key_of)(p->dataptr),tree->paramP);
		if (cmpval > 0) {
			PUSH_PATH(p,RIGHT);
			p = Rightchild(p);
		} else if (cmpval < 0) {
			PUSH_PATH(p,LEFT);
			p = Leftchild(p);
		} else /* cmpval == 0 */ {
			q = p;
			p = NULL;
			found = 1;
		}
	} /* end while(p) */

	if (!found) 
		return NULL;

	/* decrease nodes count */
	tree->avl_count--;
	dataP = q->dataptr;

	q = remove_node(tree, q, &deltaht);

	if (trav)
		trav->tavl_curP = q;	/* Jake */

	do {
		p = POP_PATH(side);

		if (side != RIGHT)
			p->Lptr = q;
		else
			p->Rptr = q;

		q = p;  
		rb = 0;

		if (deltaht) {
			p->bf -= side;
			switch (p->bf) {
				case 0:     break;  /* longest side shrank to equal shortest */
					    /* therefor deltaht remains true */
				case LEFT:
				case RIGHT: deltaht = 0;/* other side is deeper */
					    break;

				default:    {
						    q = rebalance_tavl(p,&deltaht);
						    rb = 1;
					    }
			}
		}
	} while ((p != tree->head) && (rb || deltaht));
	return dataP;

#undef PUSH_PATH
#undef POP_PATH

} /* tavl_delete */

TAVL_nodeptr tavl_find(TAVL_treeptr tree, void *key)
	/* Return pointer to tree node containing data-item
	   identified by "key"; returns NULL if not found */
{
	register TAVL_nodeptr p = Leftchild(tree->head);
	register int side;
	while (p) {
		side = (*tree->cmp)(key,(*tree->key_of)(p->dataptr), tree->paramP);
		if (side > 0)
			p = Rightchild(p);
		else if (side < 0)
			p = Leftchild(p);
		else
			return p;
	}

	return NULL;
}

TAVL_nodeptr tavl_reset(TAVL_treeptr tree)
{
	return tree->head;
}

TAVL_nodeptr tavl_succ(TAVL_nodeptr p)
{
	register TAVL_nodeptr q;

	if (!p)
		return NULL;
#if 0
	printf("%s(%d): curr node's dataptr=%p, L:%s%p%s, R:%s%p%s\n", 
			__func__, __LINE__, p->dataptr, 
			p->Lbit ? "[" : " ", 
			p->Lptr, 
			p->Lbit ? "]" : " ", 
			p->Rbit ? "[" : " ", 
			p->Rptr,
			p->Rbit ? "]" : " ");
#endif
	q = p->Rptr;

	if (RLINK(p))
		while (LLINK(q))
			q = q->Lptr;

	return (Is_Head(q) ? NULL : q);
}

TAVL_nodeptr tavl_pred(TAVL_nodeptr p)
{
	register TAVL_nodeptr q;

	if (!p)
		return NULL;

	q = p->Lptr;

	if (LLINK(p))
		while (RLINK(q))
			q = q->Rptr;

	return (Is_Head(q) ? NULL : q);
}

TAVL_nodeptr rebalance_tavl(TAVL_nodeptr a, char *deltaht)
{
	TAVL_nodeptr b,c,sub_root;   /* sub_root will be the return value, */
	/* and the root of the newly rebalanced*/
	/* sub-tree */

	/*  definition(tree-height(X)) : the maximum    */
	/*      path length from node X to a leaf node. */
	*deltaht = 0;   /*  *deltaht is set to 1 if and only if         */
	/*      tree-height(rebalance()) < tree-height(a)*/

	if (Is_Head(a)          /* Never rebalance the head node! */
			|| abs(a->bf) <= 1) /* tree "a" is balanced - nothing more to do */
		return(a);

	rebalanced = 1;

	if (a->bf == LEFT+LEFT) {
		b = a->Lptr;
		if (b->bf != RIGHT) {   /* LL rotation */
			printf("%s(%d): LEFT+LEFT, LL Rotation\n", __func__, __LINE__);
			if (RTHREAD(b)) {       /* b->Rptr is a thread to "a" */
				assert(b->Rptr == a);
				a->Lbit = THREAD;   /* change from link to thread */
				b->Rbit = LINK;     /* change thread to link */
			} else {
				a->Lptr = b->Rptr;
				b->Rptr = a;
			}

			*deltaht = b->bf ? 1 : 0;
			a->bf = - (b->bf += RIGHT);

			sub_root = b;
		} else {                  /* LR rotation */
			printf("%s(%d): LEFT+LEFT, LR Rotation\n", __func__, __LINE__);
			*deltaht = 1;

			c = b->Rptr;
			if (LTHREAD(c)) {
				assert(c->Lptr == b);
				c->Lbit = LINK;
				b->Rbit = THREAD;
			} else {
				b->Rptr = c->Lptr;
				c->Lptr = b;
			}

			if (RTHREAD(c)) {
				assert(c->Rptr == a);
				c->Rbit = LINK;
				a->Lptr = c;
				a->Lbit = THREAD;
			} else {
				a->Lptr = c->Rptr;
				c->Rptr = a;
			}

			switch (c->bf) {
				case LEFT:  b->bf = 0;
					    a->bf = RIGHT;
					    break;

				case RIGHT: b->bf = LEFT;
					    a->bf = 0;
					    break;

				case 0:     b->bf = 0;
					    a->bf = 0;
			}

			c->bf = 0;

			sub_root = c;
		}
	} else if (a->bf == RIGHT+RIGHT) {
		b = a->Rptr;
		if (b->bf != LEFT) {    /* RR rotation */
			printf("%s(%d): RIGHT+RIGHT, RR Rotation\n", __func__, __LINE__);
			if (LTHREAD(b)) {       /* b->Lptr is a thread to "a" */
				assert(b->Lptr == a);
				a->Rbit = THREAD;   /* change from link to thread */
				b->Lbit = LINK;     /* change thread to link */
			} else {
				a->Rptr = b->Lptr;
				b->Lptr = a;
			}
			*deltaht = b->bf ? 1 : 0;
			a->bf = - (b->bf += LEFT);

			sub_root = b;
		} else {                  /* RL rotation */
			printf("%s(%d): RIGHT+RIGHT, RL Rotation\n", __func__, __LINE__);
			*deltaht = 1;

			c = b->Lptr;
			if (RTHREAD(c)) {
				assert(c->Rptr == b);
				c->Rbit = LINK;
				b->Lbit = THREAD;
			} else {
				b->Lptr = c->Rptr;
				c->Rptr = b;
			}

			if (LTHREAD(c)) {
				assert(c->Lptr == a);
				c->Lbit = LINK;
				a->Rptr = c;
				a->Rbit = THREAD;
			} else {
				a->Rptr = c->Lptr;
				c->Lptr = a;
			}

			switch (c->bf) {
				case RIGHT: b->bf = 0;
					    a->bf = LEFT;
					    break;

				case LEFT:  b->bf = RIGHT;
					    a->bf = 0;
					    break;

				case 0:     b->bf = 0;
					    a->bf = 0;
			}

			c->bf = 0;

			sub_root = c;
		}
	}

	return sub_root;
}/* end rebalance */



static TAVL_nodeptr remove_node(TAVL_treeptr tree, TAVL_nodeptr p, char *deltaht)
{
	char dh;
	TAVL_nodeptr q;

	*deltaht = 0;

	if (p->bf != LEFT) {
		if (RLINK(p)) {
			p->Rptr = remove_min(p->Rptr,&q,&dh);
			if (dh) {
				p->bf += LEFT;  /* becomes 0 or LEFT */
				*deltaht = (p->bf) ? 0 : 1;
			}
		} else { /* leftchild(p),rightchild(p) == NULL */
			assert(p->bf == 0);
			assert(LTHREAD(p));

			*deltaht = 1;           /* p will be removed, so height changes */
			if (p->Rptr->Lptr == p) { /* p is leftchild of it's parent */
				p->Rptr->Lbit = THREAD;
				q = p->Lptr;
			} else {  /* p is rightchild of it's parent */
				assert(p->Lptr->Rptr == p);
				p->Lptr->Rbit = THREAD;
				q = p->Rptr;
			}

			printf("%s(%d): A) dealloc node=%p, return node=%p, cnt=%d\n", 
					__func__, __LINE__, p, q, tree->avl_count);

			(*tree->dealloc)(p);

			return q;
		}
	} else { /* p->bf == LEFT */
		p->Lptr = remove_max((p->Lptr),&q,&dh);
		if (dh) {
			p->bf += RIGHT;      /* becomes 0 or RIGHT */
			*deltaht = (p->bf) ? 0 : 1;
		}
	}

	p->dataptr = q->dataptr;

	printf("%s(%d): B) dealloc left/right node=%p, return node=%p, cnt=%d\n", 
			__func__, __LINE__, q, p, tree->avl_count);

	(*tree->dealloc)(q);

	return p;
}

static TAVL_nodeptr remove_min(TAVL_nodeptr p, TAVL_nodeptr *minnode, char *deltaht)
{
	char dh = *deltaht = 0;

	if (LLINK(p)) { /* p is not minimum node */
		p->Lptr = remove_min(p->Lptr,minnode,&dh);
		if (dh) {
			p->bf += RIGHT;
			switch (p->bf) {
				case 0: *deltaht = 1;
					break;
				case RIGHT+RIGHT:
					p = rebalance_tavl(p,deltaht);
			}
		}

		return p;
	} else { /* p is minimum */
		*minnode = p;
		*deltaht = 1;
		if (RLINK(p)) {
			assert(p->Rptr->Lptr == p);
			assert(LTHREAD(p->Rptr) && RTHREAD(p->Rptr));

			p->Rptr->Lptr = p->Lptr;
			return p->Rptr;
		} else
			if (p->Rptr->Lptr != p) {   /* was first call to remove_min, */
				p->Lptr->Rbit = THREAD; /* from "remove", not remove_min */
				return p->Rptr;         /* p is never rightchild of head */
			} else {
				p->Rptr->Lbit = THREAD;
				return p->Lptr;
			}
	}
}

static TAVL_nodeptr remove_max(TAVL_nodeptr p, TAVL_nodeptr *maxnode, char *deltaht)
{
	char dh = *deltaht = 0;

	if (RLINK(p)) { /* p is not maximum node */
		p->Rptr = remove_max(p->Rptr,maxnode,&dh);
		if (dh) {
			p->bf += LEFT;
			switch (p->bf) {
				case 0: *deltaht = 1;
					break;
				case LEFT+LEFT:
					p = rebalance_tavl(p,deltaht);
			}
		}
		return p;
	} else { /* p is maximum */
		*maxnode = p;
		*deltaht = 1;
		if (LLINK(p)) {
			assert(LTHREAD(p->Lptr) && RTHREAD(p->Lptr));
			assert(p->Lptr->Rptr == p);

			p->Lptr->Rptr = p->Rptr;
			return p->Lptr;
		} else
			if (p->Rptr->Lptr == p) {   /* p is leftchild of its parent */
				p->Rptr->Lbit = THREAD; /* test must use p->Rptr->Lptr */
				return p->Lptr;         /* because p may be predecessor */
			} else {                    /* of head node */
				p->Lptr->Rbit = THREAD; /* p is rightchild of its parent */
				return p->Rptr;
			}
	}
}

typedef struct foo {
	int grp;
	int v;
} t_foo;

void show_avl_node(TAVL_nodeptr node, int grp, int v)
{
	printf("%s(%d): node=%p, L:%s%p%s, R:%s%p%s, grp=%d, v=%d\n",
			__func__, __LINE__, 
			node, 
			node->Lbit ? "[" : " ", 
			node->Lptr, 
			node->Lbit ? "]" : " ", 
			node->Rbit ? "[" : " ", 
			node->Rptr, 
			node->Rbit ? "]" : " ", 
			grp, v);
}
