/**

  @file
  
  @brief Implementation of a very simple single linked list.
  
*/

#ifndef SLIST_H
#define SLIST_H

//! The definition of a single linked list node
struct slist_node
{
	void *data; // Pointer to data of this node
	struct slist_node *next; // Pointer to next node on list
};

//! Single linked list node type
typedef struct slist_node slist_node_t;

//! The definition of a single linked list
struct slist
{
	slist_node_t *head; // Pointer to head of list
	slist_node_t *tail; // Pointer to tail of list
	unsigned int size; // The number of elements in the list
};

//! Single linked list type
typedef struct slist slist_t;


// you have to use these macros, do not use the inner variables of the list!!
//! Macro to get the head node of a list l
#define slist_head(l) l->head
//! Macro to get the tail node of a list l
#define slist_tail(l) l->tail
//! Macro to get the size of a list l
#define slist_size(l) l->size
//! Macro to get the next node of l
#define slist_next(n) n->next
//! Macro to get the data of node l
#define slist_data(n) n->data

//! Specifies whether slist_destroy should deallocate or not stored elements
typedef enum { SLIST_LEAVE_DATA = 0, SLIST_FREE_DATA } slist_destroy_t;

/** Initialize a single linked list
	\param list - the list to initialize */
void slist_init(slist_t *);

/** Destroy and de-allocate the memory hold by a list
	\param list - a pointer to an existing list
	\param dealloc flag that indicates whether stored data should also be de-allocated */
void slist_destroy(slist_t *,slist_destroy_t);

/** Pop the first element in the list
	\param list - a pointer to a list
	\return a pointer to the data of the element, or NULL if the list is empty */
void *slist_pop_first(slist_t *);

/** Append data to list (add as last node of the list)
	\param list - a pointer to a list
	\param data - the data to place in the list
	\return 0 on success, or -1 on failure */
int slist_append(slist_t *,void *);

/** Prepend data to list (add as first node of the list)
	\param list - a pointer to list
	\param data - the data to place in the list
	\return 0 on success, or -1 on failure
*/
int slist_prepend(slist_t *,void *);

/** \brief Append elements from the second list to the first list, use the slist_append function.
	you can assume that the data of the lists were not allocated and thus should not be deallocated in destroy 
	(the destroy for these lists will use the SLIST_LEAVE_DATA flag)
	\param to a pointer to the destination list
	\param from a pointer to the source list
	\return 0 on success, or -1 on failure
*/
int slist_append_list(slist_t*, slist_t*);

#endif
