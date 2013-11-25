/* Copyright 1998, 1999, 2000, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009 Trevor Tonn

	This file is part of Giveme.

    Giveme is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 2.

    Giveme is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Giveme.  If not, see <http://www.gnu.org/licenses/>.
*/

/* linkedListADT.c 

	LINKED LIST ADT FUNCTION DEFINITIONS

	This file contains definitions for all the functions you would need except
	for LL_createList(). I have omitted writting that function because we do
	not need to create a LL_HEADER -- one is created for each row in the hash table.

	These functions have been fairly well tested, but please play around with
	llplayground.c to see how they work, and be sure to READ llplayground.c to see
	how you can use these functions in the project. 
*/

#include <stdio.h>
#include <stdlib.h>
#include "linkedListADT.h"



/*===================================================================================
  ========================================================  LL_insert ===============
	Insert a new node with the new data attached to it (dataPtr). 

	PRE:	'list' must exist, but can have 0 nodes.
			'dataPtr' must point to some sort of DATA structure in memory.
	POST:	the data is attached to the new node, and the new node is inserted 
			into the proper place in the linkedlist to keep the list sorted.
*/
void LL_insert (LL_HEADER *list, void *dataPtr)
{
	LL_NODE *newNode;
	LL_NODE *pCur;
	LL_NODE *pPre;

	newNode = (LL_NODE *) malloc(sizeof(LL_NODE));

	newNode->dataPtr = dataPtr;
	newNode->link = NULL;

	if(list->LL_head == NULL) // new list
		list->LL_head = newNode;

	else
	{
		LL_searchList(list, &pPre, &pCur, dataPtr);

		if(pPre == NULL)
		{
			newNode->link = list->LL_head;
			list->LL_head = newNode;
		} 
		else 
    	{
			newNode->link = pPre->link;
			pPre->link = newNode;
		}
	}

	list->count++;
}



/*===================================================================================
  ========================================================  LL_printList ============
	We can use this function traverse the list and print every node. This uses
	the list->printData() pointer-to-function to access the void pointer. We can
	in theory have this function do more than just printData, but perform any
	operation on every node in the list through the use of pointers to functions.

	PRE:	list should exist, but can be empty.
	POST:	the list is traversed and list->printData() is called for each node.
*/
void LL_traverseList(LL_HEADER *list, void (*process)())
{
	LL_NODE *node;
	LL_NODE *nextNode;

	node = list->LL_head;

	while(node != NULL)
	{
		nextNode = node->link;
		process(list, node, node->dataPtr);
		node = nextNode;
	}

	return;
}



/*===================================================================================
  ========================================================  LL_destroy ==============
	This traverses the list and frees every node, with a hook to call another 
	pointer-to-function to free the internals of dataPtr (not needed for our project).

	PRE:	'list' must exist, but can be empty.
	POST:	list is emptied.
	RETURN:	a null pointer is returned. 
*/
LL_HEADER *LL_destroyList(LL_HEADER *list)
{

	LL_NODE *pCur;
	LL_NODE *pNext;

	pCur = list->LL_head;

	while(pCur != NULL)
	{
		pNext = pCur->link;

		list->freeInternals(pCur->dataPtr);
		free(pCur); // ..then we free the LL_NODE.
		pCur = pNext;	
	}


	list->LL_head = NULL;
	free(list);

	return list;
}



/*===================================================================================
  ========================================================  LL_searchList ===========
	Searches the list in an odd manner: the LL_NODE pointers in the argument list
	must exist in whatever function calls LL_searchList. 

	If the targetData is found (this uses list->compare() pointer-to-function) then
	this function returns 1, and pCur is set to the exact-matches address. Functions 
	returns 1 if an exact match is found.This is useful for search manager functions
	where the user just wants to know if an item is in the list or not. 

	If the targetData is not found, pCur is set to where it would go, and pPre is
	set to the address of the node which would come before the node you searched
	for. Returns 0 if not found. This is useful for insert functions who want to know
	where they should insert the new node to in order to keep the list sorted. 

	PRE:	'list' must exist, but can be empty. 
			'pPre' and 'pCur' must exist in the function calling LL_searchList().
			'targetData' is a structure of the same type we're searching for.
				We use list->compare() to access targetData.
	POST:	read the paragraphs above.
	RETURN: 1 if exact match is found, 0 if exact match not found.  
*/
int LL_searchList(LL_HEADER *pList, LL_NODE **pPre, LL_NODE **pCur, void *targetData)
{
	int found = 0;

	*pPre = NULL;
	*pCur = pList->LL_head;

	
	while(*pCur != NULL &&   pList->compare(targetData, (*pCur)->dataPtr) > 0  )
    {
		*pPre = *pCur;
		*pCur = (*pCur)->link;
	}

	if(*pCur && (pList->compare((*pCur)->dataPtr, targetData)) == 0)
		found = 1;


	return found;
}



/*===================================================================================
  ========================================================  LL_delete ===============
	Deletes a single node from the linked list.

	This function must be used in conjunction with LL_searchList() in order to set
	the pPre and pCur pointers correctly. Please read the documentation for 
	LL_searchList() and see the accompanying test-driver program, llplayground.c. 

	PRE:	'list' must exist, ubt can be empty.
			'pPre' and 'pCur' must exist in the function calling LL_delete().
	POST:	a node is deleted for sure.
*/
void LL_delete(LL_HEADER *list, LL_NODE *pPre, LL_NODE *pCur)
{
	if(pPre == NULL)
		list->LL_head = pCur->link;
	else
		pPre->link = pCur->link;

	list->freeInternals(pCur->dataPtr);
	free(pCur);
	
	list->count--;
	return;
}
