#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

#define LIST_SENTINEL 0

static list_status_t list_increase_alloc(list_t *list)
{
	size_t old_size = list->size;
	size_t new_size = old_size ? old_size * 2 : 2;

	list_elem_t *new_alloc = (list_elem_t *)realloc(list->elements, new_size * sizeof *new_alloc);
	if (!new_alloc)
		return LIST_ERR_ALLOC;

	memset(new_alloc + old_size, 0, (new_size - old_size) * sizeof *new_alloc);

	for (size_t i = old_size; i < new_size - 1; ++i)
		new_alloc[i].next = (int)(i + 1);
	new_alloc[new_size - 1].next = list->free;

	list->free = (int)old_size;
	list->elements = new_alloc;
	list->size = new_size;

	return LIST_OK;
}

static list_status_t list_maybe_increase_alloc(list_t *list)
{
	if (list->cnt < (int)list->size - 1)
		return LIST_OK;

	return list_increase_alloc(list);
}

static bool index_is_valid(const list_t *list, int index)
{
	return index >= 0 && (size_t)index < list->size;
}

static bool index_is_data(const list_t *list, int index)
{
	return index_is_valid(list, index) && list->elements[index].used;
}

list_status_t list_ctor(list_t *list, size_t initial_size)
{
	if (!list)
		return LIST_ERR_ARGNULL;

	memset(list, 0, sizeof *list);

	if (initial_size < 1)
		initial_size = 1;

	list->size = initial_size + 1;
	list->elements = (list_elem_t *)calloc(list->size, sizeof *list->elements);
	if (!list->elements)
		return LIST_ERR_ALLOC;

	for (size_t i = 1; i < list->size - 1; ++i)
		list->elements[i].next = (int)(i + 1);
	list->elements[list->size - 1].next = 0;

	list->elements[LIST_SENTINEL].next = 0;
	list->elements[LIST_SENTINEL].prev = 0;
	list->elements[LIST_SENTINEL].used = true;

	list->free = 1;

	return LIST_OK;
}

list_status_t list_dtor(list_t *list)
{
	if (!list)
		return LIST_ERR_ARGNULL;

	free(list->elements);
	memset(list, 0, sizeof *list);

	return LIST_OK;
}

list_elem_t *list_next(list_t *list, list_elem_t *elem)
{
	if (!list || !elem)
		return NULL;

	if (elem->next == 0)
		return NULL;

	return &list->elements[elem->next];
}

list_elem_t *list_prev(list_t *list, list_elem_t *elem)
{
	if (!list || !elem)
		return NULL;

	if (elem->prev == 0)
		return NULL;

	return &list->elements[elem->prev];
}

list_elem_t *list_begin(list_t *list)
{
	return list_next(list, &list->elements[LIST_SENTINEL]);
}

list_elem_t *list_end(list_t *list)
{
	return list_prev(list, &list->elements[LIST_SENTINEL]);
}

list_status_t list_chk(list_t *list)
{
	if (!list || !list->elements)
		return LIST_ERR_ALLOC;

	if (list->cnt < 0 || (size_t)list->cnt > list->size - 1)
		return LIST_ERR_ALLOC;

	for (size_t i = 1; i < list->size; ++i)
	{
		list_elem_t *e = &list->elements[i];
		if (!e->used)
			continue;

		if (!index_is_valid(list, e->next) || !index_is_valid(list, e->prev))
			return LIST_ERR_NOTLINKED;
		if (list->elements[e->next].prev != (int)i)
			return LIST_ERR_NOTLINKED;
	}
	return LIST_OK;
}

static list_status_t list_acquire_free(list_t *list, int *out)
{
	if (list_maybe_increase_alloc(list) != LIST_OK)
		return LIST_ERR_ALLOC;

	if (list->free == 0)
		return LIST_ERR_ALLOC;

	*out = list->free;
	list->free = list->elements[list->free].next;
	return LIST_OK;
}

list_status_t list_insert_before(list_t *list, int index, list_data_t data)
{
	if (!index_is_data(list, index) && index != LIST_SENTINEL)
		return LIST_ERR_ARGVAL;

	int insertion_index = 0;
	list_status_t st = list_acquire_free(list, &insertion_index);
	if (st != LIST_OK)
		return st;

	list_elem_t *elem = &list->elements[insertion_index];
	list_elem_t *next = &list->elements[index];
	list_elem_t *prev = &list->elements[next->prev];

	elem->data = data;
	elem->used = true;

	elem->next = index;
	elem->prev = next->prev;

	next->prev = insertion_index;
	prev->next = insertion_index;

	list->cnt++;
	return LIST_OK;
}

list_status_t list_insert_after(list_t *list, int index, list_data_t data)
{
	if (!index_is_data(list, index) && index != LIST_SENTINEL)
		return LIST_ERR_ARGVAL;

	return list_insert_before(list, list->elements[index].next, data);
}

list_status_t list_insert_head(list_t *list, list_data_t data)
{
	return list_insert_before(list, list->elements[LIST_SENTINEL].next, data);
}

list_status_t list_insert_tail(list_t *list, list_data_t data)
{
	return list_insert_after(list, list->elements[LIST_SENTINEL].prev, data);
}

list_status_t list_remove_at(list_t *list, int index)
{
	if (!index_is_data(list, index) || index == LIST_SENTINEL)
		return LIST_ERR_ARGVAL;

	list_elem_t *elem = &list->elements[index];

	list->elements[elem->prev].next = elem->next;
	list->elements[elem->next].prev = elem->prev;

	memset(elem, 0, sizeof *elem);

	elem->next = list->free;
	list->free = index;

	list->cnt--;
	return LIST_OK;
}

list_status_t list_remove_head(list_t *list)
{
	if (list->elements[LIST_SENTINEL].next == LIST_SENTINEL)
		return LIST_ERR_ARGVAL;

	return list_remove_at(list, list->elements[LIST_SENTINEL].next);
}

list_status_t list_remove_tail(list_t *list)
{
	if (list->elements[LIST_SENTINEL].prev == LIST_SENTINEL)
		return LIST_ERR_ARGVAL;

	return list_remove_at(list, list->elements[LIST_SENTINEL].prev);
}

int list_find_val(list_t *list, list_data_t val)
{
	for (int cur = list->elements[LIST_SENTINEL].next; cur != LIST_SENTINEL; cur = list->elements[cur].next)
		if (list->elements[cur].data == val)
			return cur;
	return -1;
}

int list_index(list_t *list, int logical_idx)
{
	if (logical_idx <= 0 || logical_idx > list->cnt)
		return -1;

	int cur = list->elements[LIST_SENTINEL].next;
	for (int pos = 1; pos < logical_idx && cur != LIST_SENTINEL; ++pos)
		cur = list->elements[cur].next;

	return (cur != LIST_SENTINEL) ? cur : -1;
}
