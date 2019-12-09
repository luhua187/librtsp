#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "list.h"

struct person
{
	int age;
	int w;

	struct list_head list;
};

struct list_head person_list = LIST_HEAD_INIT(person_list);;

int aaa()
{
	struct person *tmp;
	struct list_head *pos, *n;
	int i,j;



	for(i = 0; i <10; i++ )
	{
		tmp = (struct person *)malloc(sizeof(struct person));

		tmp->age = i;
		tmp->w   = i;

		list_add(&(tmp->list), &person_list);

	}

	printf("\n===================print the list=====================\n");

	list_for_each(pos, &person_list)
	{
		tmp = list_entry(pos, struct person, list);
		printf("ags:%d w:%d \n", tmp->age, tmp->w);
	}


	return 0;
}