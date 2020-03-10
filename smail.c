/*
 *  smail.c: send news as mail routin of soroban(terakoya-system)
 *  version 1.21 1991/04/23
 *	programmed by JK1LOT(Dai Yokota) & JF1PXI(Akihiko Taniguchi)
 *	Copyright 1989 (C) by JK1LOT(Dai Yokota)
 *            1991 (C) by JF1PXI(Akihiko Taniguchi)
 *	All rights are reserved
 *
 * Ver 1.12 1989/9/27 by JK1LOT for main_version 1.30
 * Ver 1.13 1989/9/29 by JK1LOT sendmail_postmaster added.
 *                              sys 4th field support(sendmail).
 * Ver 1.14 1989/11/16 by JK1LOT bug-fixed.
 * Ver 1.20 1991/03/30 by JF1PXI 2.21p.pxi.7.0 -> 2.30
 * Ver 1.21 1991/04/23 by JF1PXI
 * Ver 1.22 1991/05/03 by JF1PXI
 * Ver 1.23 1992/04/01 by Ryuji Suzuki (addbatchfile())
 */
#include <stdio.h>
#include <stdlib.h>
#include <dir.h>
#include <time.h>
#include <string.h>
#include "soroban.h"
#include "tempfile.h"
#include "errors.h"
#include "stdfunc.h"
#include "sys.h"

extern char myhost[];
extern char myhost0[];
extern char myhostpath[];

struct ctllist {
	struct ctllist *next;
	char *mid;
	char *host;
};

struct sendctl {
	struct sendctl *next;
	char *dest;
	struct ctllist *ihave;
	struct ctllist *sendme;
	struct ctllist *alhave;
	struct ctllist *batch_f;
	struct ctllist *batch_m;
};

void addbatchfile(int mode, struct sendctl *cp, char *filename);
struct sendctl *sendctltop = NULL;

void
smailctl(char *desthost, int mode, char *mid, char *srchost)
{
	struct sendctl *p;
	struct ctllist *q;

	for (p = sendctltop; p; p = p->next)
		if (!strcmp(p->dest, desthost)) break;
	if (p == NULL) {
		if ((p = malloc(sizeof(struct sendctl))) == NULL)
			alloc_error("smailctl");
		if ((p->dest = strdup(desthost)) == NULL)
			alloc_error("smailctl");
		p->ihave = p->sendme = p->alhave = p->batch_f = p->batch_m = NULL;
		p->next = sendctltop;
		sendctltop = p;
	}
	if ((q = malloc(sizeof(struct ctllist))) == NULL)
		alloc_error("smailctl");
	if ((q->mid = strdup(mid)) == NULL)
		alloc_error("smailctl");
	if ((q->host = strdup(srchost)) == NULL)
		alloc_error("smailctl");
	switch (mode) {
		case IHAVE:
			q->next = p->ihave;
			p->ihave = q;
			break;
		case SENDME:
			q->next = p->sendme;
			p->sendme = q;
			break;
		case ALHAVE:
			q->next = p->alhave;
			p->alhave = q;
			break;
		case BATCH_F:
			q->next = p->batch_f;
			p->batch_f = q;
			break;
		case BATCH_M:
			q->next = p->batch_m;
			p->batch_m = q;
			break;
	}
}

void
send(void)
{
	struct sendctl *p;
	struct ctllist *q;
	FILE *tfp;
	char *tmpfile;
	struct sys_s *sp;
	char *newsfile;

	tfp = tfcreat(&tmpfile);
	tfclose(tfp);
	for (p = sendctltop; p; p = p->next) {
		for (sp = top_sys_list; sp; sp = sp->next)
			if (!strcmp(p->dest, sp->to) && *sp->flag != 'M' && *sp->flag != 'F') break;
		if (sp != NULL)
			newsfile = sp->file;
		else
			newsfile = NULL;
		if (p->ihave) {
			tfp = tfopen(tmpfile, WRITE);
			fprinthead(tfp, IHAVE, p->dest);
			fprintf(tfp, "\n");
			for (q = p->ihave; q; q = q->next) {
				fprintf(tfp, "%s\n", q->mid);
			}
			fprintf(tfp, "\n");
			rewind(tfp);
			sendmail(p->dest, tfp, newsfile);
			tfclose(tfp);
		}
		if (p->sendme) {
			tfp = tfopen(tmpfile, WRITE);
			fprinthead(tfp, SENDME, p->dest);
			fprintf(tfp, "\n");
			for (q = p->sendme; q; q = q->next) {
				fprintf(tfp, "%s\n", q->mid);
			}
			fprintf(tfp, "\n");
			rewind(tfp);
			sendmail(p->dest, tfp, newsfile);
			tfclose(tfp);
		}
		if (p->alhave) {
			tfp = tfopen(tmpfile, WRITE);
			fprinthead(tfp, ALHAVE, p->dest);
			fprintf(tfp, "\n");
			for (q = p->alhave; q; q = q->next) {
				fprintf(tfp, "%s\n", q->mid);
			}
			fprintf(tfp, "\n");
			rewind(tfp);
			sendmail(p->dest, tfp, newsfile);
			tfclose(tfp);
		}
		for (sp = top_sys_list; sp; sp = sp->next)
			if (sp && !strcmp(p->dest, sp->to) && *sp->flag == 'F')
				addbatchfile(BATCH_F, p, sp->file);
		for (sp = top_sys_list; sp; sp = sp->next)
			if (sp && !strcmp(p->dest, sp->to) && *sp->flag == 'M')
				addbatchfile(BATCH_M, p, sp->file);
	}
	tfremove(tmpfile);
}

void
addbatchfile(int mode, struct sendctl *cp, char *filename)
{
	char *p;
	register struct ctllist *q;
	char path[MAXPATH];
	char dest0[256];
	FILE *fp;
	
	switch (mode) {
		case BATCH_F:
			q = cp->batch_f;
			break;
		case BATCH_M:
			q = cp->batch_m;
			break;
		default:
			return;
	}
	if (q) {
		if (filename && *filename) {
			strcpy(path, batchdir);
			strcat(path, "/");
			strcat(path, filename);
		} else {
			strncpy(dest0, cp->dest, 256);
			p = strchr(dest0, '.');
			if (p != NULL) *p = '\0';
			strcpy(path, batchdir);
			strcat(path, "/");
			strcat(path, dest0);
		}
		fp = fopen(path, APPEND);
		for (; q; q = q->next) {
			fprintf(fp, "%s\n", q->mid);
		}
		fclose(fp);
	}
}


void
sendmail(char *dest, FILE *sfp, char *filename)
{
	FILE *fp;
	char path[MAXPATH];
	char buf[BUFSIZ];
	long sequence;
	static char rfc_t[28];
	struct tm gmt;
	time_t timer;
	int inheader;

	if (!file_check(sequencefile)) {
		fp = fopen(sequencefile, WRITE);
		fputc('0', fp);
		fclose(fp);
	}
	if (filename == NULL) {
		sequence = add_sequence();
		sprintf(path, "%s\\%ld.txt", mqueue, sequence);
		fp = fopen(path, WRITE);
	} else {
		fp = fopen(filename, APPEND);
	}
	(void)time(&timer);
	change_time(ctime(&timer), rfc_t);
	gmt = *gmtime(&timer);
	fprintf(fp, "Date: %s\n", rfc_t);
	fprintf(fp, "From: %s@%s\n", comname, myhost);
	fprintf(fp, "Message-Id: <%02d%02d%02d%02d%02d.AA%08ld@%s>\n",
	  gmt.tm_year, gmt.tm_mon+1, gmt.tm_mday,
		gmt.tm_hour, gmt.tm_min, sequence, myhost);
	fprintf(fp, "To: rnews@%s\n", dest);
	fprintf(fp, "Subject: from %s\n", comname);
	fprintf(fp, "\n");
	inheader = TRUE;
	while (fgets(buf, BUFSIZ, sfp)) {
		if (!strnicmp(buf, "Xref: ", 6) && inheader) continue;
		if (*buf == '\n') inheader = FALSE;
		fprintf(fp, "N");
		fputs_newjis(buf, fp);
	}
	fprintf(fp, "\n");
	fclose(fp);
	if (filename == NULL) {
		sprintf(path, "%s\\%ld.wrk", mqueue, sequence);
		fp = fopen(path, WRITE);
		fprintf(fp, "%s\n", dest);
		fprintf(fp, "%s@%s\n", comname, myhost);
		fprintf(fp, "rnews@%s\n", dest);
		fclose(fp);
	}
}

void
sendmail2(char *dest, char *subject, FILE *sfp)
{
	FILE *fp;
	char path[MAXPATH];
	char buf[BUFSIZ];
	long sequence;
	static char rfc_t[28];
	struct tm gmt;
	time_t timer;

	if (!file_check(sequencefile)) {
		fp = fopen(sequencefile, WRITE);
		fputc('0', fp);
		fclose(fp);
	}
	sequence = add_sequence();
	sprintf(path, "%s\\%ld.txt", mqueue, sequence);
	fp = fopen(path, WRITE);
	(void)time(&timer);
	change_time(ctime(&timer), rfc_t);
	gmt = *gmtime(&timer);
	fprintf(fp, "Date: %s\n", rfc_t);
	fprintf(fp, "From: %s@%s\n", comname, myhost);
	fprintf(fp, "Message-Id: <%02d%02d%02d%02d%02d.AA%08ld@%s>\n",
	  gmt.tm_year, gmt.tm_mon+1, gmt.tm_mday,
		gmt.tm_hour, gmt.tm_min, sequence, myhost);
	fprintf(fp, "To: %s\n", dest);
	fprintf(fp, "Subject: %s\n", subject);
	fprintf(fp, "\n");
	while (fgets(buf, BUFSIZ, sfp)) {
		fputs_newjis(buf, fp);
	}
	fprintf(fp, "\n");
	fclose(fp);
	sprintf(path, "%s\\%ld.wrk", mqueue, sequence);
	fp = fopen(path, WRITE);
	fprintf(fp, "%s\n", strchr(dest, '@')+1);
	fprintf(fp, "%s@%s\n", comname, myhost);
	fprintf(fp, "%s\n", dest);
	fclose(fp);
}

void
sendmail_postmaster(char * subject, FILE *sfp)
{
	FILE *fp;
	char path[MAXPATH];
	char buf[BUFSIZ];
	long sequence;
	static char rfc_t[28];
	struct tm gmt;
	time_t timer;

	if (!file_check(sequencefile)) {
		fp = fopen(sequencefile, WRITE);
		fputc('0', fp);
		fclose(fp);
	}
	sequence = add_sequence();
	sprintf(path, "%s\\%ld.txt", mqueue, sequence);
	fp = fopen(path, WRITE);
	(void)time(&timer);
	change_time(ctime(&timer), rfc_t);
	gmt = *gmtime(&timer);
	fprintf(fp, "Date: %s\n", rfc_t);
	fprintf(fp, "From: %s@%s\n", comname, myhost);
	fprintf(fp, "Message-Id: <%02d%02d%02d%02d%02d.AA%08ld@%s>\n",
	  gmt.tm_year, gmt.tm_mon+1, gmt.tm_mday,
		gmt.tm_hour, gmt.tm_min, sequence, myhost);
	fprintf(fp, "To: postmaster@%s\n", myhost);
	fprintf(fp, "Subject: %s\n", subject);
	fprintf(fp, "\n");
	while (fgets(buf, BUFSIZ, sfp)) {
		fputs_newjis(buf, fp);
	}
	fprintf(fp, "\n");
	fclose(fp);
	sprintf(path, "%s\\%ld.wrk", mqueue, sequence);
	fp = fopen(path, WRITE);
	fprintf(fp, "%s\n", myhost);
	fprintf(fp, "%s@%s\n", comname, myhost);
	fprintf(fp, "postmaster@%s\n", myhost);
	fclose(fp);
}

void
fprinthead(FILE *fp, int mode, char *dest)
{
	static char rfc_t[28];
	time_t timer;
	struct tm gmt;
	char dest0[256];
	char *p;

	strncpy(dest0, dest, 256);
	p = strchr(dest0, '.');
	if (p != NULL) *p = '\0';
	fprintf(fp, "Path: %s!%s\n", myhostpath, comname);
	fprintf(fp, "From: %s@%s\n", comname, myhost);
	fprintf(fp, "Newsgroups: to.%s.ctl\n", dest0);
	fprintf(fp, "Control: ");
	switch (mode) {
	case IHAVE:
		fprintf(fp, IHAVEMSG" %s\n", myhost);
		break;
	case SENDME:
		fprintf(fp, SENDMEMSG" %s\n", myhost);
		break;
	case ALHAVE:
		fprintf(fp, ALHAVEMSG" %s\n", myhost);
		break;
	}
	fprintf(fp, "Subject: ");
	switch (mode) {
	case IHAVE:
		fprintf(fp, IHAVEMSG" %s\n", myhost);
		break;
	case SENDME:
		fprintf(fp, SENDMEMSG" %s\n", myhost);
		break;
	case ALHAVE:
		fprintf(fp, ALHAVEMSG" %s\n", myhost);
		break;
	}
	(void)time(&timer);
	change_time(ctime(&timer), rfc_t);
	gmt = *gmtime(&timer);
	fprintf(fp, "Message-Id: <%02d%02d%02d%02d%02d.AA%08ld@%s>\n",
	  gmt.tm_year, gmt.tm_mon+1, gmt.tm_mday,
	  gmt.tm_hour, gmt.tm_min, add_sequence(), myhost);
	fprintf(fp, "Date: %s\n", rfc_t);
/*
	fprintf(fp, "Distribution: local\n");
*/
	fprintf(fp, "Posting-Front-End: terakoya(%s) version "VER, comname);
	fputc('\n', fp);
}

long add_sequence(void)
{
	FILE *fp;
	char buf[BUFSIZ];
	long sequence;

	fp = fopen(sequencefile, READ);
	fgets(buf, BUFSIZ, fp);
	sequence = atol(buf);
	if (sequence < 0L || sequence > 99999998L)
		sequence = 0L;
	sequence++;
	freopen(sequencefile, WRITE, fp);
	fprintf(fp, "%ld", sequence);
	fclose(fp);
	return (sequence);
}

long get_sequence(void)
{
	FILE *fp;
	char buf[BUFSIZ];
	long sequence;

	fp = fopen(sequencefile, READ);
	fgets(buf, BUFSIZ, fp);
	sequence = atol(buf);
	fclose(fp);
	return (sequence);
}
