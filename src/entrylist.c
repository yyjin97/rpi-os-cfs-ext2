#include "common.h"
#include "shell.h"
#include "mm.h"

#ifndef NULL
#define NULL	( ( void* )0 )
#endif

//static SHELL_ENTRY_LIST_ITEM *SHELL_ENTRY_LIST_ADDR;
SHELL_ENTRY_LIST_ITEM *SHELL_ENTRY_LIST_ADDR;


int init_entry_list(SHELL_ENTRY_LIST* list)
{
	list->count = 0;
	list->first = NULL;
	list->last = NULL;
	return 0;
}

int add_entry_list(SHELL_ENTRY_LIST* list, SHELL_ENTRY_LIST_ITEM* new)
{

	if(list->count == 0) {
		list->first = new;
		list->last = new;
	}

	else {
		list->last->next = new;
		list->last = new;
	}

	new->next = NULL;
	list->count += 1;

	return 0;
}

int release_entry_list(SHELL_ENTRY_LIST* list)
{
	SHELL_ENTRY_LIST_ITEM* item1;
	SHELL_ENTRY_LIST_ITEM* item2;

	item1 = list->first;

	while(item1){
		item2 = item1->next;
		//free(item1);
		free_page((unsigned long)item1);
		item1 = item2;
	}

	return 0;
}

