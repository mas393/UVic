/*
 * linkedlist.c
 *
 * Based on the implementation approach described in "The Practice 
 * of Programming" by Kernighan and Pike (Addison-Wesley, 1999).
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ics.h"
#include "emalloc.h"
#include "listy.h"


node_t *new_node(event_t *val) {
    assert( val != NULL);

    node_t *temp = (node_t *)emalloc(sizeof(node_t));

    temp->val = val;
    temp->next = NULL;

    return temp;
}


node_t *add_front(node_t *list, node_t *new) {
    new->next = list;
    return new;
}


node_t *add_end(node_t *list, node_t *new) {
    node_t *curr;

    if (list == NULL) {
        new->next = NULL;
        return new;
    }

    for (curr = list; curr->next != NULL; curr = curr->next);
    curr->next = new;
    new->next = NULL;
    return list;
}

node_t *peek_front(node_t *list) {
    return list;
}


node_t *remove_front(node_t *list) {
    if (list == NULL) {
        return NULL;
    }

    return list->next;
}

/*
 * Function time_calc
 *
 * A copy of time_calc as implemented in process_cal3.c.
 * Returns a double describing the dateStr passed in. 
 *
 */
static double time_calc(const char *dateStr)
{
  double res;
  double year, month, day, hour, minute, second;

  sscanf(dateStr, "%4lf%2lf%2lfT%2lf%2lf%2lf",
	 &year, &month, &day, &hour, &minute, &second);
  res = year + month/12 + day/365 + hour/365/24 + minute/365/24/60 + second/365/24/60/60;
  
  return res;
}

/*
 * Function add_inorder
 *
 * Adds the new node to the list pointed to by *list based on chronological
 * order of dtstart values. Returns the head of the sorted list.
 *
 */
node_t *add_inorder(node_t *list, node_t *new)
{
  if (list == NULL) return add_front(list, new);
  else if (time_calc(list -> val -> dtstart) > time_calc(new -> val -> dtstart)) return add_front(list, new);
  else list -> next = add_inorder(list->next, new);
  
  return list;
}

void apply(node_t *list,
           void (*fn)(node_t *list, void *),
           void *arg)
{
    for ( ; list != NULL; list = list->next) {
        (*fn)(list, arg);
    }
}
