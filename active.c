/* (C) 1992 by Ryuji Suzuki */
unsigned char RCS_ID[] = "$Header: active.cv  1.1  92/09/10 05:18:00  wex  Exp $";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "errors.h"
#include "active.h"

#define TRUE		1
#define FALSE		0

struct active_s {
	struct active_s *next;
	char *group;
	unsigned resent;
	unsigned expire;
	char *option;
};

struct active_s *top_active_list = NULL;

void
active_read(char *groupfile)
{
	register struct active_s *ap;
	register char *p;
	struct active_s *ap_prev;
	FILE *fp;
	char buf[BUFSIZ];

	ap_prev = NULL;
	fp = fopen(groupfile, "rt");
	while (fgets(buf, BUFSIZ, fp)) {
		if ((ap =(struct active_s *)malloc(sizeof(struct active_s))) == NULL)
			alloc_error("init_param");
		p = strtok(buf, " \t\n");
		if ((ap->group = strdup(p)) == NULL)
			alloc_error("init_param");
		p = strtok(NULL, " \t\n");
		ap->resent = atoi(p);
		p = strtok(NULL, " \t\n");
		ap->expire = atoi(p);
		p = strtok(NULL, " \t\n");
		if ((ap->option = strdup(p)) == NULL)
			alloc_error("init_param");
		if (ap_prev == NULL) {
			top_active_list = ap;
		} else {
			ap_prev->next = ap;
		}
		ap_prev = ap;
	}
	ap->next = NULL;
	fclose(fp);
}

void
active_restore(char *groupfile)
{
	FILE *fp;
	register struct active_s *p;

	fp = fopen(groupfile, "wt");
	for (p = top_active_list; p; p = p->next) {
		fprintf(fp, "%s %05u %05u %s\n",
			p->group, p->resent, p->expire, p->option);
	}
	fclose(fp);
}

int
active_exist(const char *group)
{
	int len;
	register struct active_s *ap;

	len = strlen(group);
	for (ap = top_active_list; ap; ap = ap->next) {
		/* check both length and contents */
		/* following _if_ block was tuned by JJ1BDX */
		if (strlen(ap->group) == len && 0 == strncmp(ap->group, group, len)) {
			return TRUE;
		}
	}
	return FALSE;
}

int
active_remove(const char *group)
{
	register struct active_s *ap, *ap2;

	if (!strcmp(top_active_list->group, group)) {
		ap = top_active_list;
		top_active_list = top_active_list->next;
		free(ap->group);
		free(ap->option);
		free(ap);
		return TRUE;
	}
	for (ap = top_active_list; ap->next; ap = ap->next) {
		if (!strcmp(ap->next->group, group)) {
			ap2 = ap->next;
			ap->next = ap->next->next;
			free(ap2->group);
			free(ap2->option);
			free(ap2);
			break;
		}
	}
	return (ap->next != NULL);
}

void
active_create(const char *group)
{
	register struct active_s *ap;

	if ((ap =(struct active_s *)malloc(sizeof(struct active_s))) == NULL)
		alloc_error("active_create");
	if ((ap->group = strdup(group)) == NULL)
		alloc_error("active_create");
	ap->resent = 0;
	ap->expire = 0;
	if ((ap->option = strdup("y")) == NULL)
		alloc_error("active_create");
	ap->next = top_active_list;
	top_active_list = ap;
}

unsigned
active_inc(const char *group)
{
	register struct active_s *p;

	for (p = top_active_list; p; p = p->next) {
		if (!strcmp(p->group, group))
			return ++p->resent;
	}
	return 1;
}
