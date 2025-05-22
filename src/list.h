#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

typedef void *list_data_t;

typedef enum
{
	LIST_OK = 0,
	LIST_ERR_ALLOC = 1 << 1,
	LIST_ERR_EMPTY = 2 << 1,
	LIST_ERR_NOTFOUND = 3 << 1,
	LIST_ERR_ARGNULL = 4 << 1,
	LIST_ERR_FREESTACK = 5 << 1,
	LIST_ERR_NOTLINKED = 6 << 1,
	LIST_ERR_ARGVAL = 7 << 1,
} list_status_t;

typedef struct
{
	list_data_t data;
	int next;
	int prev;
	bool used;
} list_elem_t;

typedef struct
{
	list_elem_t *elements;
	size_t size;
	int cnt;
	int free;
} list_t;

list_status_t list_ctor(list_t *list, size_t initial_size);
list_status_t list_dtor(list_t *list);

list_elem_t *list_next(list_t *list, list_elem_t *elem);
list_elem_t *list_prev(list_t *list, list_elem_t *elem);

list_elem_t *list_begin(list_t *list);
list_elem_t *list_end(list_t *list);

list_status_t list_insert_before(list_t *list, int index, list_data_t data);
list_status_t list_insert_after(list_t *list, int index, list_data_t data);
list_status_t list_insert_head(list_t *list, list_data_t data);
list_status_t list_insert_tail(list_t *list, list_data_t data);

list_status_t list_remove_at(list_t *list, int index);
list_status_t list_remove_head(list_t *list);
list_status_t list_remove_tail(list_t *list);

list_status_t list_chk(list_t *list);
int list_find_val(list_t *list, list_data_t val);
int list_index(list_t *list, int logical_idx);
