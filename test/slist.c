#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "slist.h"

void printlist(slist_t*);

void slist_init(slist_t *list)
{
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
}
void printlist(slist_t *list){
	slist_node_t* l = list->head;
	printf("List is:");
	while(l != NULL) {
		printf(" %s", (char*) l->data);
		l = l->next;
	}
}
void slist_destroy(slist_t *list, slist_destroy_t dealloc)
{
	if(dealloc == SLIST_LEAVE_DATA)
	{
		free(list);
		slist_size(list) = 0;
		list = NULL;
	}
	if(dealloc == SLIST_FREE_DATA)
	{
		slist_node_t* current = list->head;
		slist_node_t* tmp;

		while(current != NULL)
		{			
			tmp = current;
			current = current->next;
			free(tmp->data);			
			free(current);	
		}
		free(list);
		list = NULL;
	}
}

void* slist_pop_first(slist_t *list)
{
	if(list == NULL){
        //printf("No solution available, exiting program.\n");
		return NULL;
	}
	slist_node_t* newHead = list->head->next;
	slist_node_t* poped = list->head;

	list->head = newHead;
	list->size--;
	return poped;
}

int slist_append(slist_t *list, void *data)
{
	// if the list not exist
	if(list == NULL) {
		printf("ERROR: the list wasn't initialized\n");
		return -1;
	}
	// Create new node
	slist_node_t* newNode = malloc(sizeof(slist_node_t));
	newNode->data = data;
	newNode->next = NULL;
	// if list is empty
	if(list->head == NULL){
 		list->head = newNode;
		list->tail = newNode;
	}
	else // Add node to the end of list
	{
		list->tail->next = newNode;
		list->tail = newNode;
	}
	list->size++;
	return 0;
}

int slist_prepend(slist_t *list, void *data)
{
	// if the list is empty
	if(list == NULL){
		printf("ERROR: the list wasn't initialized\n");
		return -1;
    }
	// Create new node
	slist_node_t *newNode = malloc(sizeof(slist_node_t));
	newNode->data = data;
     
	if(list->head == NULL)
	{
		list->head = newNode;
		list->tail = newNode;
	}
	else
	{
		newNode->next = list->head;
		list->head = newNode;			
	}
	list->size++;
	return 0;
}

int slist_append_list(slist_t *destinationList, slist_t *sourceList)
{
	if(destinationList == NULL)
	{ 	
		printf("ERROR: the destination list wasn't initialized\n");
		return -1;
	}
	if(sourceList == NULL)
	{
		printf("ERROR: the source list wasn't initialized\n");
		return -1;
	}
	slist_node_t* current = sourceList->head;
	while(current != NULL)
	{
		slist_append(destinationList, current->data);
		current = current->next;
	}
	free(current); //free memory
	return 0;
}
