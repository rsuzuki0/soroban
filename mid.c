/*
 *  mid.c: Message-Id ä«óù routin of soroban(terakoya-system)
 *  version 2.00 1989/11/25 released on 1989/11/25
 *	programmed by JK1LOT(Dai Yokota)
 *	Copyright 1989 (C) by JK1LOT(Dai Yokota)
 *	All rights are reserved
 *  18 April, 1992 jf7wex(Ryuji Suzuki) (format of midfile)
 *   5 May, 1992 Ryuji Suzuki(jf7wex)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dir.h>
#include <string.h>
#include <io.h>
#include <sys/stat.h>
#include "soroban.h"
#include "wexlib.h"
#include "hash.h"
#include "errors.h"
#include "tempfile.h"

extern char newsdir[];

void
init_mid(int mode)
{
	if (mode == CONFIG) {
		init_hash(mid_file, mid_idx);
		config_mid();
		close_hash();
	} else {
		if ((access(mid_idx, 0) == -1) || (filetimecomp(mid_file, mid_idx) > 0)) {
			errprintf("%s: index file history.idx too old or not existed\n"
			  "\tSo now create new index file\n", comname);
			init_hash(mid_file, mid_idx);
			config_mid();
			close_hash();
		}
	}
	if (!init_hash(mid_file, mid_idx)) {
		errprintf("%s: history can't open\n", comname);
		exit(1);
	}
}

void
close_mid()
{
	close_hash();
}

void
config_mid()
{
	char *tmpfile;
	FILE *fp;
	
	fp = tfcreat(&tmpfile);
	tfclose(fp);

	if (!creat_hash(tmpfile)) {
		errprintf("%s: create index file Failed\n", comname);
		exit(1);
	}
	tfremove(tmpfile);
}

void
add_mid(char *mid, char *ng, int num, time_t expire)
{
	char buf2[BUFSIZ];
	time_t timer;

	if (*mid != '<') return;
	if (mid[strlen(mid) - 1] != '>') return;
	time(&timer);
	if ((unsigned)num) {
		if ((signed time_t)expire > 0L) sprintf(buf2, "%s %lu~%lu %s/%d\n", mid, timer, expire, ng, num);
		else sprintf(buf2, "%s %lu~- %s/%d\n", mid, timer, ng, num);
	} else {
		sprintf(buf2, "%s %lu~- %s\n", mid, timer, ng);
	}
	add_hash(buf2);
}

int
srch_mid(char *mid)
{
	char buf[BUFSIZ];

	if (search_hash(mid, buf) == NULL) return FALSE;
	return TRUE;
}

FILE *
midopen(char *mid, char *mode)
{
	char buf[BUFSIZ];
	char buf2[MAXPATH];

	if (search_hash(mid, buf) == NULL) return NULL;
	strtok(buf, sptabnl);  /* mid */
	strtok(NULL, sptabnl); /* time */
	strcpy(buf2, strtok(NULL, sptabnl));
	reformnpath(buf2, newsdir);
	return (fopen(buf2, mode));
}


int
midpath(char *mid, unsigned char buffer[])
{
	char buf[BUFSIZ];

	if (search_hash(mid, buf) == (char *)NULL) return NULL;
	strtok(buf, sptabnl);  /* mid */
	strtok(NULL, sptabnl); /* time */
	strcpy(buffer, strtok(NULL, sptabnl));
	reformnpath(buffer, newsdir);
	return 1;
}


int
midrm(char *mid)
{
	char buf[BUFSIZ];
	char buf2[BUFSIZ];
	register int i;

	if (search_hash(mid, buf) == NULL) return FALSE;
	strtok(buf, sptabnl);  /* mid */
	strtok(NULL, sptabnl); /* time */
	strcpy(buf2, newsdir);
	strcat(buf2, "/");
	strcat(buf2, strtok(NULL, sptabnl));
	for (i = 0; buf2[i]; i++)
		if (buf2[i] == '.')
			buf2[i] = '/';
	remove(buf2);
	return TRUE;
}

FILE *
midlckopen(char *mid, char *mode)
{
	char lckname[MAXPATH];
	char buf[BUFSIZ];
	int i;

	if (search_hash(mid, buf) == NULL) return NULL;

	strtok(buf, sptabnl);  /* mid */
	strtok(NULL, sptabnl); /* time */
	sprintf(lckname, "%s/%s.lck", newsdir, strtok(NULL, sptabnl));
	for (i = 0; lckname[i]; i++)
		if (lckname[i] == '.')
			lckname[i] = '/';
	return (fopen(lckname, mode));
}

void
midlckrm(char *mid)
{
	char lckname[MAXPATH];
	char buf[BUFSIZ];
	int i;

	if (search_hash(mid, buf) == NULL) return;

	strtok(buf, sptabnl);  /* mid */
	strtok(NULL, sptabnl); /* time */
	sprintf(lckname, "%s/%s.lck", newsdir, strtok(NULL, sptabnl));
	for (i = 0; lckname[i]; i++)
		if (lckname[i] == '.')
			lckname[i] = '/';
	remove(lckname);
}

long
filetimecomp(char *fn1, char *fn2)
{
	struct stat stat1, stat2;

	stat(fn1, &stat1);
	stat(fn2, &stat2);
	return (stat1.st_mtime - stat2.st_mtime);
}
