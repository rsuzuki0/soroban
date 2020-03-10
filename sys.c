#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "errors.h"
#include "sys.h"
#include "wexlib.h"
#include "ryujilib.h"

struct sys_s *top_sys_list = NULL;

void
sys_read(char *sysfile)
{
	register char *p;
	register struct sys_s *sp, *sp_prev = (struct sys_s *)NULL;
	char *buf, *e;
	FILE *fp;

	fp = fopen(sysfile, "rt");
	while (!feof(fp)) {
		buf = getmline(fp);
		if (buf == NULL) break;
		if ((*buf == '#') || strcntchr(buf, ':') > 3) continue;
		if ((sp = (struct sys_s *)malloc(sizeof(struct sys_s))) == NULL) {
			alloc_error("sys_read");
		}
		if ((p = strtoke(buf, ": \t\n")) != NULL) {
			if ((sp->to = strdup(p)) == NULL)
				alloc_error("sys_read");
			if ((e = strchr(sp->to, '/')) != NULL) {
				*e++ = '\0';
				sp->nosend = e;
			} else {
				sp->nosend = (char *)NULL;
			}
		} else sp->to = NULL;
		if ((p = strtoke(NULL, ":\n")) != NULL) {
			if ((sp->grps = strdup(p)) == NULL)
				alloc_error("sys_read");
		} else sp->grps = NULL;
		if ((p = strtoke(NULL, ": \t\n")) != NULL) {
			if ((sp->flag = strdup(p)) == NULL)
				alloc_error("sys_read");
		} else sp->flag = NULL;
		if ((p = strtoke(NULL, ": \t\n")) != NULL) {
			if ((sp->file = strdup(p)) == NULL)
				alloc_error("init_param");
		} else sp->file = NULL;
		if (sp_prev == NULL) {
			top_sys_list = sp;
		} else {
			sp_prev->next = sp;
		}
		sp_prev = sp;
	}
	sp_prev->next = NULL;
	fclose(fp);
}
