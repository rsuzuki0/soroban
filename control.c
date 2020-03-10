/*
 *  control.c: control message 処理 routin of soroban(terakoya-system)
 *  version 1.20 1991/03/30
 *	programmed by JK1LOT(Dai Yokota) & JF1PXI(Akihiko Taniguchi)
 *	Copyright 1989 (C) by JK1LOT(Dai Yokota)
 *            1991 (C) by JF1PXI(Akihiko Taniguchi)
 *	All rights are reserved
 *
 * Ver 1.13 1989/9/29 by JK1LOT sendmail to postmaster added.
 * Ver 1.14 1989/10/27 by JK1LOT addme を受けたとき sys に (null) が入るバグを
 *　　　　　　　　　　 修正.
 * Ver 1.15 1989/11/16 by JK1LOT strdup() used.
 * Ver 1.16 1990/2/14 by JK1LOT {new,rm}group に対するメールの修正
 * Ver 1.20 1991/03/30 by JF1PXI 2.21p.pxi.7.0 -> 2.30
 * Ver 1.21 1991/04/02 by JF1PXI *.lck から '\n' が抜けていたのを修正
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <dir.h>
#include "soroban.h"
#include "tempfile.h"
#include "stdfunc.h"
#include "errors.h"
#include "sys.h"
#include "active.h"

extern char myhost[];
extern char myhost0[];
extern char newsdir[];

void
doihave(char *mid, char *host)
{
	if (*mid == '\0' || *host == '\0') return;
	if (!srch_mid(mid)) {
		smailctl(host, SENDME, mid, myhost);
	}
}

void
dosendme(char *mid, char *host)
{
	FILE *fp, *fp2;
	char *tmpf;
	char buf[BUFSIZ];
	int empty_f;

	char *newsfile;
	struct sys_s *sp;

	if (*mid == '\0' || *host == '\0') return;
	for (sp = top_sys_list; sp; sp = sp->next)
		if ((hostnamecomp(host, sp->to) < 2) && *sp->flag != 'M') break;
	if (sp != NULL)
		newsfile = sp->file;
	else
		newsfile = NULL;
/* if sp->flag is F, write path and size on batchfile. but in case of M, */
/* send news direct like the other cases why batchfiles' format of F and M */
/* much differ. (Thanks to js1cpw for his bug report) */		
	if (*(sp->flag) == 'F') {
		unsigned char newspath[MAXPATH];
		unsigned char buffer[MAXPATH];
		struct stat statbuf;
		if (!midpath(mid, newspath)) return;
		stat(newspath, &statbuf);
		sprintf(buffer, "%s %lu", newspath + strlen(newsdir) + 1, statbuf.st_size);
		if (Verbose) puts("SENDME function: F mode");
		smailctl(sp->to, BATCH_F, buffer, myhost);
	} else {
		if ((fp = midopen(mid, READ)) == NULL)
			return;
		sendmail(host, fp, newsfile);
		fclose(fp);
	}
	if ((fp = midlckopen(mid, READ)) == NULL)
		return;
	fp2 = tfcreat(&tmpf);
	empty_f = TRUE;
	while (fgets(buf, BUFSIZ, fp)) {
		if (hostnamecomp(strtok(buf, sptabnl), host) > 1) {
			empty_f = FALSE;
			fputs(buf, fp2);
			fputs("\n", fp2);
		}
	}
	fclose(fp);
	if (empty_f) {
		tfremove(tmpf);
		midlckrm(mid);
	} else {
		fp = midlckopen(mid, WRITE);
		rewind(fp2);
		copy_file(fp, fp2);
		fclose(fp);
		tfclose(fp2);
		tfremove(tmpf);
	}
}

void
doalhave(char *mid, char *host)
{
	FILE *fp, *fp2;
	char *tmpf;
	char buf[BUFSIZ];
	int empty_f;

	if (*mid == '\0' || *host == '\0') return;
	if ((fp = midlckopen(mid, READ)) == NULL)
		return;
	fp2 = tfcreat(&tmpf);
	empty_f = TRUE;
	while (fgets(buf, BUFSIZ, fp)) {
		if (hostnamecomp(strtok(buf, sptabnl), host) > 1) {
			empty_f = FALSE;
			fputs(buf, fp2);
			fputs("\n", fp2);
		}
	}
	fclose(fp);
	if (empty_f) {
		tfremove(tmpf);
		midlckrm(mid);
	} else {
		fp = midlckopen(mid, WRITE);
		rewind(fp2);
		copy_file(fp, fp2);
		fclose(fp);
		tfclose(fp2);
		tfremove(tmpf);
	}
}

void
doaddme(char *host, char *grps)
{
	FILE *tfp;
	char *tmpfile;

	/* check parameters */
	if (host == NULL || *host == '\0') return;
	if (grps == NULL || *grps == '\0') {
		grps = DFTGRP;
	}

	/* mail to postmaster */
	tfp = tfcreat(&tmpfile);
	fprintf(tfp, "Control News addme received\n");
	fprintf(tfp, "\tFrom %s\n", host);
	fprintf(tfp, "\tNewsgroups: %s\n", grps);
	rewind(tfp);
	sendmail_postmaster("addme received", tfp);
	tfclose(tfp);
	tfremove(tmpfile);
}

void
dodelme(char *host)
{
	FILE *tfp;
	char *tmpfile;

	/* mail to postmaster */
	tfp = tfcreat(&tmpfile);
	fprintf(tfp, "Control News delme received\n");
	fprintf(tfp, "\tFrom %s\n", host);
	rewind(tfp);
	sendmail_postmaster("delme received", tfp);
	tfclose(tfp);
}

void
donewgroup(char *group)
{
	FILE *tfp;
	char *tmpfile;

	if (*group == '\0') return;
	if (!active_exist(group)) {
		active_create(group);
		/* mail to postmaster */
		tfp = tfcreat(&tmpfile);
		fprintf(tfp, "Control News newgroup received\n");
		fprintf(tfp, "\tNew News Group %s created\n", group);
		rewind(tfp);
		sendmail_postmaster("newgroup received", tfp);
		tfclose(tfp);
		tfremove(tmpfile);
	} else {
		/* mail to postmaster */
		tfp = tfcreat(&tmpfile);
		fprintf(tfp, "Control News newgroup received\n");
		fprintf(tfp, "\tBut, News Group %s is already existed\n", group);
		rewind(tfp);
		sendmail_postmaster("newgroup received", tfp);
		tfclose(tfp);
		tfremove(tmpfile);
	}
}

void
dormgroup(char *group)
{
	FILE *tfp;
	char *tmpfile;

	if (*group == '\0') return;
	if (!active_remove(group)) {
		/* mail to postmaster */
		tfp = tfcreat(&tmpfile);
		fprintf(tfp, "Control News rmgroup received\n");
		fprintf(tfp, "\tBut, News Group %s is not existed\n", group);
		rewind(tfp);
		sendmail_postmaster("rmgroup received", tfp);
		tfclose(tfp);
		tfremove(tmpfile);
	} else {
		/* mail to postmaster */
		tfp = tfcreat(&tmpfile);
		fprintf(tfp, "Control News rmgroup received\n");
		fprintf(tfp, "\tNews Group %s was deleted\n", group);
		rewind(tfp);
		sendmail_postmaster("rmgroup received", tfp);
		tfclose(tfp);
		tfremove(tmpfile);
	}
}

void
docancel(char *mid, char *from)
{
	FILE *fp;
	char buf[BUFSIZ];

	if (*mid == '\0' || *from == '\0') return;
	if ((fp = midopen(mid, READ)) == NULL) {
		add_mid(mid, "Cancel", 0, 0L);
		return;
	}
	while (fgets(buf, BUFSIZ, fp)) {
		if (!strcmp(from, buf)) {
			fclose(fp);
			midrm(mid);
			return;
		}
	}
	fclose(fp);
}

void
dosendsys(char *from)
{
	char name[MAXPATH];
	FILE *fp;

	if (*from == '\0') return;
	sprintf(name, "%s/"SYSFILE, libdir);
	if ((fp = fopen(name, "r")) == NULL) {
		errprintf("sendsys recieved. but cannot open sys file\n");
		return;
	}
	sendmail2(from, "My sys file", fp);
	fclose(fp);
}

void
dosendver(char *from)
{
	char *name;
	FILE *fp;
	char buf[BUFSIZ];

	if (*from == '\0') return;
	fp = tfcreat(&name);
	fprintf(fp, "%s version %s\n", comname, VER);
	rewind(fp);
	sprintf(buf, "My %s version", comname);
	sendmail2(from, buf, fp);
	tfclose(fp);
	tfremove(name);
}

/* hostnamecomp :   0 : all , 1 : part , 127 : not */
char hostnamecomp(char *host1, char *host2)
{
	int	hostlen1, hostlen2, shortlen;

	hostlen1 = strlen(host1);
	hostlen2 = strlen(host2);
	if (hostlen1 > hostlen2) {
		shortlen = hostlen2;
	} else {
		shortlen = hostlen1;
	}
	if (strncmp(host1, host2, shortlen)) {
		return 127;
	} else {
		switch (*(host1+shortlen)+*(host2+shortlen)) {
			case 0:
				return 0;
			case '.':
				return 1;
			default:
				return 126;
		}
	}
}
