/*
 *  newsbody.c: news body èàóù routin of soroban(terakoya-system)
 *  version 1.24 1991/05/08
 *	programmed by JK1LOT(Dai Yokota) & JF1PXI(Akihiko Taniguchi)
 *	Copyright 1989 (C) by JK1LOT(Dai Yokota)
 *            1991 (C) by JF1PXI(Akihiko Taniguchi)
 *	All rights are reserved
 *
 * Ver 1.24 1991/06/16 by JF1PXI
 * Ver 1.25 1992/04/01 by Ryuji Suzuki.
 * Ver 1.26 1992/04/15 by Ryuji Suzuki.
 * Ver 1.27 1992/04/28 by Ryuji Suzuki.
 * Ver 1.28 1992/05/02 by Ryuji Suzuki.
 * Ver 1.29 1992/05/05 by Ryuji Suzuki.
 * Ver 1.30 1992/05/07 by Ryuji Suzuki.
 */

#include <stdio.h>
#include <stdlib.h>
#include <dir.h>
#include <string.h>
#include <io.h>
#include <time.h>
#include <sys/stat.h>
#include "soroban.h"
#include "tempfile.h"
#include "stdfunc.h"
#include "errors.h"
#include "sys.h"
#include "active.h"

#define CHKGRP 0
#define CHKDIST 1
#define namebody(s) (*(s)=='!' ? (s)+1 : (s))

extern char newsdir[];
extern char myhost0[];
extern char myhost[];
extern char myhostpath[];
extern int Verbose;
extern int CheckLines;
extern int newslen;

time_t getindate(char *, void *);
#define	news_end(buf)	strncmp(buf, LINEDELM, LINEDELMLEN)


long
donews(FILE *mailfp)
{
	char *tmpfile;
	char filename[MAXPATH], grpdir[MAXPATH];
	char buf[BUFSIZ], tmps[BUFSIZ];
	char mid[MAXPATH];
	char from[MAXPATH];
	char to[MAXPATH];
	char path[BUFSIZ];
	char *p;
	int inheader, xref;
	int difflines;
	int ctlstatus;
	int newsttl;
	struct group_s *group = 0, *distri = 0, *g;
	register struct group_s *g2, *groupstack;
	struct stat statbuf;
	FILE *tfp, *fp;
	time_t expire;
	long fpos;

	tfp = tfcreat(&tmpfile);
	fgets(buf, BUFSIZ, mailfp);
	inheader = TRUE;
	while (strncmp(buf, LINEDELM, LINEDELMLEN)) {
		if (*buf == '\n') inheader = FALSE;
		if (!strnicmp(buf, FROMMARK, FROMMARKLEN)) {
			strcpy(from, strchr(buf, '@')+1);
			for (p = from; *p; p++) {
				if (*p == ' ' || *p == '\t' || *p == '\n'
				 || *p == '(' || *p == '<') {
					*p = '\0';
					break;
				}
			}
			goto donews1;
		}
		if (!strnicmp(buf, TOMARK, TOMARKLEN)) {
			strcpy(to, strchr(buf, '@')+1);
			for (p = to; *p; p++) {
				if (*p == ' ' || *p == '\t' || *p == '\n'
				 || *p == '(' || *p == '<') {
					*p = '\0';
					break;
				}
			}
			goto donews1;
		}
		strcpy(tmps, buf);
		if (!strnicmp(tmps, "path:", 5) && inheader) {
			strtok(tmps, " \t\n");
			strcpy(path, strtok(NULL, " \t\n"));
			strcat(path, "\n");
			strcpy(tmps, path);
			*(buf+6) = '\0';
			strcat(buf, myhostpath);
			strcat(buf, "!");
			strcat(buf, tmps);
		}
		fputs_shiftjis(buf, tfp);
donews1:
		if (fgets(buf, BUFSIZ, mailfp) == 0)
			break;
	}
	fpos = ftell(mailfp);
	rewind(tfp);
	distri = NULL;
	group = NULL;
	xref = news_analyze(tfp, mid, &group, &distri, &difflines, &ctlstatus, &newsttl, &expire);
	tfclose(tfp);
	if(srch_mid(mid)) {
		tfremove(tmpfile);
		return(fpos);
	}
	if ((signed long)expire > 1L && time(NULL) > expire) {
		g2 = group;
		if ((group = malloc(sizeof(struct group_s))) == NULL)
			alloc_error("donews");
		group->next = NULL;
		if ((group->group = strdup("junk")) == NULL)
			alloc_error("donews");
		xref = 1;
		for (g = g2; g; g = g2) {
			g2 = g->next;
			free(g->group);
			free(g);
		}
		errprintf("%s: news is too old to read: %s\n", comname, mid);
	}
	if (group == NULL) {
		tfp = tfopen(tmpfile, READ);
		send_news_g(group, distri, tfp, from, to);
		add_mid(mid, "No", 0, expire);
		tfclose(tfp);
		tfremove(tmpfile);
		return(fpos);
	}
	if (ctlstatus) {
		groupstack = group;
		if ((group = malloc(sizeof(struct group_s))) == NULL)
			alloc_error("donews");
		group->next = NULL;
		if ((group->group = strdup("control")) == NULL)
			alloc_error("donews");
		xref = 1;
		difflines = 0;
	}
	if (difflines && CheckLines) {
		g2 = group;
		if ((group = malloc(sizeof(struct group_s))) == NULL)
			alloc_error("donews");
		group->next = NULL;
		if ((group->group = strdup("junk")) == NULL)
			alloc_error("donews");
		xref = 1;
		for (g = g2; g; g = g2) {
			g2 = g->next;
			free(g->group);
			free(g);
		}
		errprintf("%s: insufficient lines - Message-ID: %s\n", comname, mid);
	}
	for (g = group; g; g = g->next) {
		if (active_exist(g->group)) {
			sprintf(grpdir, "%s/%s", newsdir, g->group);
			for (p = grpdir; *p; p++) {
				if (*p == '.') *p = '/';
			}
			g->filenum = active_inc(g->group);
			sprintf(filename, "%s/%d", grpdir, g->filenum);
			if ((g->filename = strdup(filename)) == NULL)
				alloc_error("donews");
			if (!(difflines && CheckLines))
				add_mid(mid, g->group, g->filenum, expire);
		} else {
			g->filenum = 0;
			g->filename = NULL;
			xref--;
		}
	}
	if (xref <= 0 && NotInActiveToJunk) {
		/* chkgroup fail so store to junk */
		/* this is written by JL1AMK */
		sprintf(grpdir, "%s/junk", newsdir) ;
		g = group;
		g->filenum = active_inc("junk");
		sprintf(filename, "%s/%d", grpdir, g->filenum);
		if ((g->filename = strdup(filename)) == NULL)
			alloc_error("donews");
		add_mid(mid, "junk", g->filenum, expire);
		errprintf("%s: %s is not in active\n", comname, g->group);
	}
	for (g = group; g; g = g->next) {
		if (g->filename == NULL) continue;
		tfp = tfopen(tmpfile, READ);
		fp = dirfopen(g->filename);
		inheader = TRUE;
		while (fgets(buf, BUFSIZ, tfp)) {
			if (inheader && !strnicmp(buf, "xref:", 5)) continue;
			if (inheader && *buf == '\n') {
				inheader = FALSE;
				if (xref > 1) {
					fprintf(fp, "Xref: %s", myhost0);
					for (g2 = group; g2; g2 = g2->next) {
/*						if (g2 == g) continue; */
						if (g2->filenum) fprintf(fp, " %s:%d", g2->group, g2->filenum);
					}
					fputs("\n\n", fp);
				} else fputc('\n', fp);
			} else fputs(buf, fp);
		}
		fclose(fp);
		tfclose(tfp);
		stat(g->filename, &statbuf);
		g->filesize = statbuf.st_size;
	}
	if (ctlstatus) {
		groupstack->filenum = group->filenum;
		groupstack->filesize = group->filesize;
		if ((groupstack->filename = strdup(group->filename)) == (char *)NULL)
			alloc_error("donews");
		for (g = group; g; g = g2) {
			g2 = g->next;
			free(g->group);
			free(g);
		}
		group = groupstack;
	}
	if (!(difflines && CheckLines) && (ctlstatus != STOPCTLMSG) 
	  && (newsttl > 0)) {
		send_neighbour(group, distri, mid, from, to, path);
	}
	tfremove(tmpfile);
	for (g = group; g; g = g2) {
		g2 = g->next;
		free(g->group);
	}
	if (group) free(group);
	return(fpos);
}

int
news_analyze(FILE *fp, char *mid, struct group_s **grp, struct group_s **distri, int *difflines, int *ctlstatus, int *newsttl, time_t *expire)
{
	char buf[BUFSIZ], tmp[BUFSIZ];
	register char *p;
	register struct group_s *gp;
	int retval;
	int lines1, lines2;
	int linesinheader = FALSE;
	int newsttl1, newsttl2;

	retval = 0;
	*grp = NULL;
	*distri = NULL;
	lines1 = -1;
	newsttl1 = newsttl2 = 0;
	*ctlstatus = NOCTLMSG;
	*expire = (time_t)0;
	while (fgets(buf, BUFSIZ, fp)) {
		if (*buf == '\n') break;
		if (!strnicmp(buf, "message-id:", 11)) {
			strtok(buf, " \t\n");
			strcpy(mid, strtok(NULL, " \t\n"));
			if (Verbose)
				printf("%s: news Message-Id: %s\n", comname, mid);
		}
		if (!strnicmp(buf, "control:", 8)) {
			*ctlstatus = THROUGHCTLMSG;
			strtok(buf, ", \t\n");
			if (((p = strtok(NULL, ", \t\n")) != NULL) &&
				(!stricmp(p, IHAVEMSG) || !stricmp(p, SENDMEMSG) ||
				!stricmp(p, ALHAVEMSG))) {
					if ((gp = malloc(sizeof(struct group_s))) == NULL)
						alloc_error("news_analyze");
					sprintf(tmp, "to.%s.ctl", myhost0);
					if ((gp->group = strdup(tmp)) == NULL)
						alloc_error("news_analyze");
					gp->filename = NULL;
					gp->next = *grp;
					*grp = gp;
					retval++;
					*ctlstatus = STOPCTLMSG;
/*
					printf("%s: news for Newsgroup: %s\n", comname, gp->group);
*/
			}
		}
		if (!strnicmp(buf, "newsgroups:", 11)) {
			strtok(buf, ", \t\n");
			while ((p = strtok(NULL, ", \t\n")) != NULL) {
				if (Verbose)
					printf("%s: news for Newsgroup: %s\n", comname, p);
/*
				if (!strcmp(p, "all.all.ctl")) continue;
*/
				if ((gp = malloc(sizeof(struct group_s))) == NULL)
					alloc_error("news_analyze");
				if ((gp->group = strdup(p)) == NULL)
					alloc_error("news_analyze");
				gp->filename = NULL;
				gp->next = *grp;
				*grp = gp;
				retval++;
			}
		}
		if (!strnicmp(buf, "distribution:", 13)) {
			strtok(buf, ", \t\n");
			while ((p = strtok(NULL, ", \t\n")) != NULL) {
				if ((gp = malloc(sizeof(struct group_s))) == NULL)
					alloc_error("news_analyze");
				if ((gp->group = strdup(p)) == NULL)
					alloc_error("news_analyze");
				gp->filename = NULL;
				gp->next = *distri;
				*distri = gp;
			}
		}
		if (CheckLines && !strnicmp(buf, "lines:", 6)) {
			strtok(buf, ", \t\n");
			lines1 = atoi(strtok(NULL, ", \t\n"));
			linesinheader = TRUE;
		}
		if (!strnicmp(buf, "terakoya-news-ttl:", 18)) {
			strtok(buf, ", \t\n");
			newsttl1 = atoi(strtok(NULL, ", \t\n"));
		}
		if (!strnicmp(buf, "path:", 5)) {
			strtok(buf, ", \t\n");
			for (p = strtok(NULL, ", \t\n"); *p != NULL; p++)
				if (*p == '!') newsttl2++;
		}
		if (!strnicmp(buf, "expires:", 8)) {
			strtok(buf, " ,\t\n");
			p = strtok(NULL, "\n");
			*expire = getindate(p, NULL);
		}
	}
	if (CheckLines && linesinheader) {
		lines2 = 0;
		while (fgets(buf, BUFSIZ, fp)) {
			lines2++;
		}
		*difflines = lines1 - lines2;
	} else *difflines = 0;
	*newsttl = (newsttl1) ? (newsttl1 - newsttl2) : 1;
	return retval;
}


/*
 * check Newsgroups and Distribution field
 *   This function was tuned by JJ1BDX (and JE7LQS).
 */
int
chkgrplist(char *send, struct group_s *group, int cmode)
{
	register struct group_s *gp;
	register struct group_s *go, *gg;
	struct group_s top_g;
	char *grp;
	char *p;
	static char holder[MAXPATH];  /* JE7LQS */

	if ((grp = strdup(send)) == NULL)
			alloc_error("chkgrplst");
	p = strtok(grp, ", \t");
	if ((top_g.group = strdup(p)) == NULL)
			alloc_error("chkgrplst");
	top_g.next = NULL;
	go = &top_g;
	while ((p = strtok(NULL, ", \t")) != NULL) {
		if ((gg = malloc(sizeof(struct group_s))) == NULL)
			alloc_error("chkgrplst");
		if ((gg->group = strdup(p)) == NULL)
			alloc_error("chkgrplst");
		gg->next = NULL;
		go->next = gg;
		go = gg;
	}
	for (gp = group; gp; gp = gp->next) {
		switch (cmode) {
		/*
		 * valid case: strlen(sys) <= strlen(newsgroup)
		 * example:
		 * sys -> fj, fj.rec, fj.rec.ham
		 * newsgroup -> fj.rec.ham
		 */
		case CHKGRP:
			if (!strcmp(gp->group, "control") || 
				!strcmp(gp->group, "all.all.ctl"))
				goto chkgrpend;
			strcpy(holder, gp->group);
tryagain:
			for (gg = &top_g; gg; gg = gg->next) {
				if (!strcmp(holder, namebody(gg->group)))
					break;
			}
			if (gg) {
				if (*gg->group != '!')
					goto chkgrpend;
			} else if ((p = strrchr(holder, '.')) != NULL) {
				*p = '\0';  /* shorten newsgroup */
				goto tryagain;
			}
			break;
		/*
		 * valid case: strlen(sys) >= strlen(distribution)
		 * example:
		 * sys -> fj.rec.ham
		 * distribution -> fj, fj.rec, fj.rec.ham
		 */
		case CHKDIST:
			for (gg = &top_g; gg; gg = gg->next) {
				if (!strncmp(namebody(gg->group),gp->group,strlen(gp->group)))
					goto chkgrpend;
			}
			break;
		}
	}
chkgrpend:
	free(top_g.group);
	free(grp);
	for (gg = top_g.next; gg; gg = go) {
		free(gg->group);
		go = gg->next;
		free(gg);
	}

	if (gp == NULL) return FALSE;
	return TRUE;
}


void
send_news_g(struct group_s *group, struct group_s *distri, FILE *tfp, char *from, char *mailto)
{
	register struct sys_s *sp;

	for (sp = top_sys_list; sp; sp = sp->next) {
		/* following _if_ block was tuned by JJ1BDX */
		if (
		     chkgrplist(sp->grps, group, CHKGRP)
		  && ((distri != NULL) ? chkgrplist(sp->grps, distri, CHKDIST) : TRUE)
		  && (hostnamecomp(sp->to, from) > 1)
		  && (hostnamecomp(sp->to, mailto) > 1)) {
			rewind(tfp);
			sendmail(sp->to, tfp, sp->file);
		}
	}
}

static int
chkpathlist(char *nosend, char *path)
{
	static char tmp[BUFSIZ];
	struct g {
		struct g *next;
		char *site;
	} top_g, *go, *gg;
	char *p;

	if(nosend[0] == '\0') {
		return FALSE;
	}
	strcpy(tmp, nosend);
	p = strtok(tmp, ", \t");
	if ((top_g.site = strdup(p)) == NULL)
		alloc_error("chkpathlist");
	top_g.next = NULL;
	go = &top_g;
	while ((p = strtok(NULL, ", \t")) != NULL) {
		if ((gg = malloc(sizeof(struct g))) == NULL)
			alloc_error("chkpathlist");
		if ((gg->site = strdup(p)) == NULL)
			alloc_error("chkpathlist");
		gg->next = NULL;
		go->next = gg;
		go = gg;
	}
	
	strcpy(tmp, path);
	for(p=strtok(tmp, "!\n") ;p ;p=strtok(NULL, "!\n")) {
		for(gg = &top_g; gg; gg = gg->next) {
			if(!strcmp(gg->site, p)) {
				goto chkpathend;
			}
		}
	}
	
chkpathend:
	free(top_g.site);
	for (gg = top_g.next; gg; gg = go) {
		free(gg->site);
		go = gg->next;
		free(gg);
	}

	if (p == NULL) {
		return FALSE;
	}
	else {
		return TRUE;
	}
}

void
send_neighbour(struct group_s *group, struct group_s *distri, char *mid, char *from, char *mailto, char *path)
{
	register struct sys_s *sp;
	auto time_t timer;
	char buffer[256];

	time(&timer);
	for (sp = top_sys_list; sp; sp = sp->next) {
		/* following _if_ block was tuned by JJ1BDX */
		if (chkgrplist(sp->grps, group, CHKGRP)
		  && ((distri != NULL) ? chkgrplist(sp->grps, distri, CHKDIST) : TRUE)
		  && (hostnamecomp(sp->to, from) > 1) 
		  && (hostnamecomp(sp->to, mailto) > 1)
		  && !chkpathlist(sp->nosend, path)
			) {
			if (*(sp->flag) == 'F') {
				register struct group_s *g;
				for (g = group; g; g = g->next) if (g->filename != (char *)NULL) break;
				sprintf(buffer, "%s %lu", g->filename, g->filesize);
				smailctl(sp->to, BATCH_F,
					(strncmp(buffer, newsdir, newslen -1)) ? buffer : (buffer + newslen),
					myhost);
			} else if (*(sp->flag) == 'M') {
				sprintf(buffer, "%s %lu", mid, timer);
				smailctl(sp->to, BATCH_M, buffer, myhost);
			} else if (*(sp->flag) == 'A') {
				register struct group_s *g;
				for (g = group; g; g = g->next) if (g->filename != (char *)NULL) break;
				sprintf(buffer, "%s %s %lu %lu", g->filename, mid, g->filesize, timer);
				smailctl(sp->to, BATCH_F,
					(strncmp(buffer, newsdir, newslen -1)) ? buffer : (buffer + newslen),
					myhost);
			}
		}
	}
}

FILE *
dirfopen(char *filename)
{
	register FILE *fp;
	char dir[MAXPATH];
	char huge *p;
	register char *q;

	q = filename;
	while (*q) {
		if (*q == '+') *q = '_';
		q++;
	}
	if ((fp = fopen(filename, WRITE)) != NULL) return fp;
	strcpy(dir, filename);
	if ((p = (char huge *)max((char huge *)strrchr(dir, '/'),
	                (char huge *)strrchr(dir, '\\'))) != NULL)
		*p = '\0';
	mkdirrec(dir);
	if ((FILE *)NULL == (fp = fopen(filename, WRITE))){
		errprintf("Disk Error!\a\n");
		exit(1);
	}
	return fp;
}

void
mkdirrec(char *dir)
{
	char dir2[MAXPATH];
	register char huge *p;

	if (access(dir, 0) == 0) return;
	if (mkdir(dir) == -1) {
		strcpy(dir2, dir);
		if ((p = (char huge *)max((char huge *)strrchr(dir2, '/'),
		               (char huge *)strrchr(dir2, '\\'))) != NULL)
			*p = '\0';
		mkdirrec(dir2);
		mkdir(dir);
	}
}
