#ifndef __LIST__H__
#define __LIST__H__

struct list_head
{
	struct list_head *prev;
	struct list_head *next;
};


#define LIST_HEAD_INIT(name) {&(name), &(name)}

static  void list_init(struct list_head *head)
{
    head->prev = head;
    head->next = head;
}

static  int list_empty(struct list_head *head)
{
	return head->next == head;
}


static  void __list_add(struct list_head *newp, struct list_head *prev, struct list_head *next)
{
	next->prev  = newp;
	newp->next  = next;
	newp->prev  = prev;
	prev->next  = newp;
}

static  void list_add(struct list_head *newp, struct list_head *head)
{
	__list_add(newp, head, head->next);
}


static  void list_add_tail(struct list_head *newp, struct list_head *head)
{
	__list_add(newp, head->prev, head);
}


static  void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

static  void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

#define list_entry(ptr, type, member)  ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define list_for_each(pos, head) for(pos=(head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) for(pos=(head)->next, n=pos->next; pos != (head); pos = n, n=pos->next)


#endif