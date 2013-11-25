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

#ifndef linkedListADT_h
#define linkedListADT_h



typedef struct llnode {
	void *dataPtr;
	struct llnode *link;
} LL_NODE;


typedef struct {
	int count; 	// number of LL_NODES in this linked list.
	LL_NODE *LL_head;
	int (*compare)( void *dataOne, void *dataTwo );
	void (*printData)( void *, void *, void *);
	void (*freeInternals)(void *);
} LL_HEADER;






// prototypes

void 		LL_insert		(LL_HEADER *list, void *dataPtr);
LL_HEADER	*LL_destroyList	(LL_HEADER *list);
int 		LL_searchList	(LL_HEADER *pList, LL_NODE **pPre, LL_NODE **pCur, void *targetData);
void 		LL_delete		(LL_HEADER *list, LL_NODE *pPre, LL_NODE *pCur);
void 		LL_traverseList	(LL_HEADER *list, void (*process)());


#endif
