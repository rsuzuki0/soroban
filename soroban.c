/*
 *	soroban:
 *	version 2.40 1992/04/04
 *
 *  soroban.c: main routin of soroban(terakoya-system)
 *  version 1.60 1991/08/26
 *	programmed by JK1LOT(Dai Yokota) & JF1PXI(Akihiko Taniguchi)
 *	Copyright 1989,90 (C) by JK1LOT(Dai Yokota)
 *            1991    (C) by JF1PXI(Akihiko Taniguchi)
 *            1992    (C) by JF7WEX(Ryuji Suzuki)
 *	All rights are reserved
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <dir.h>
#include <ctype.h>
#include "soroban.h"
#include "tempfile.h"
#include "errors.h"
#include "stdfunc.h"
#include "netdef.h"
#include "ryujilib.h"
#include "wexlib.h"
#include "sys.h"
#include "active.h"

#ifdef __SMALL__
#else
unsigned _stklen = 32768U;
#endif

int debugmode = FALSE;
int ErrFlag = FALSE;
int ActiveOK = FALSE;
int NotInActiveToJunk = TRUE;
char myhost[256];
char myhost0[256];
char myhostpath[256];
int MakeLock = TRUE;
int Verbose = FALSE;
int CheckLines = FALSE;
int Allnewsflag = FALSE;
int newslen; /* length of Terakoya.newsDir + 1 */
char newsdir[MAXPATH];
char libdir[MAXPATH];
char mid_file[MAXPATH];
char mid_idx[MAXPATH];
char mid_lck[MAXPATH];
char groupfile[MAXPATH];
char grouplck[MAXPATH];
char sysfile[MAXPATH];
char tmpname[MAXPATH];
char inputfile[MAXPATH];
char comname[] = "soroban";
char *batchdir;
char *maildir;
char *mqueue;
char *sequencefile;
char *mailfile;
char *maillck;
char *bakfile;
char *interfile;
char *errlog;
char tzone[8];

const char sptabnl[] = " \t\n";

#define	news_end(buf)	strncmp(buf, LINEDELM, LINEDELMLEN)

FILE *Mailfp;

extern FILE *errlogfp;

void
main(int argc, char *argv[])
{
	int handle;
	FILE *tmpfp;
	char *tmpfile;
	void exit_func();
	char rfc_t[28];
	time_t timer;

	atexit(exit_func);
	(void)time(&timer);
	change_time(ctime(&timer), rfc_t);
	printf("%s: running  %s\n", comname, rfc_t);

option:
	if (argc > 1) {
		switch (argv[1][1]) {
		case 'v':
			printf("%s: version %s\n", comname, VER);
			exit(0);
		case 'D':
			printf("%s: create directory\n", comname);
			init_param();
			if ((errlogfp = fopen(errlog, "a")) == NULL) {
				fprintf(stderr, "%s: %s cannot open\n", comname, errlog);
				exit(1);
			}
			/* mkdir_all_ng(); */
			exit(0);
		case 'i':
			printf("%s: create index file\n", comname);
			init_param();
			if ((errlogfp = fopen(errlog, "a")) == NULL) {
				fprintf(stderr, "%s: %s cannot open\n", comname, errlog);
				exit(1);
			}
			init_mid(CONFIG);
/*			config_mid();	*/
			exit(0);
		case 'r':
			strcpy(inputfile, argv[2]);
			break;
		case 'L':
			MakeLock = FALSE;
			argv++;
			argc--;
			goto option;
		case 'V':
			Verbose = TRUE;
			argv++;
			argc--;
			goto option;
		case 'C':
			CheckLines = TRUE;
			argv++;
			argc--;
			goto option;
		case 'A':
			Allnewsflag = TRUE;
			argv++;
			argc--;
			goto option;
		case 'J':
			NotInActiveToJunk = FALSE;
			argv++;
			argc--;
			goto option;
		default:
			fprintf(stderr, "%s: Bad option '%c'\n", comname, argv[1][1]);
			exit(1);
		}
	}

	init_param();
	init_mid(NORMAL);
	if (*inputfile) {
		if ((Mailfp = fopen(inputfile, READ)) == NULL) {
			fprintf(stderr, "%s: File '%s' can't open.\n", comname, inputfile);
			exit(1);
		}
	} else if (!isatty(fileno(stdin))) {
		Mailfp = stdin;
	} else {
		if (access(interfile, 0) != -1) {
			fprintf(stderr, "%s: There's still %s.\n", comname, interfile);
			exit(0);
		}
		handle = open(mailfile, O_RDONLY);
		if (handle == -1 || filelength(handle) == 0) {
			close(handle);
			exit(0);
		}
		close(handle);
		if (access(maillck, 0) != -1) {
			fprintf(stderr, "%s: %s is locked\n", comname, mailfile);
			exit(0);
		}
		rename(mailfile, interfile);
		Mailfp = fopen(interfile, READ);
	}

	if (access(grouplck, 0) != -1) {
		fprintf(stderr, "%s: %s/%s is locked\n", comname, libdir, groupfile);
		exit(0);
	}
	if (access(mid_lck, 0) != -1) {
		fprintf(stderr, "%s: %s/%s is locked\n", comname, libdir, mid_file);
		exit(0);
	}
	mlock(newsdir, groupfile);
	mlock(newsdir, mid_file);

	if (Verbose)
		printf("%s: converting received text\n", comname);
	ErrFlag = TRUE;
	tmpfp = tfcreat(&tmpfile);
	copy_f_delh(tmpfp, Mailfp);
	tfclose(tmpfp);

	if (Verbose)
		printf("%s: analysis start\n", comname);
	analyze(tmpfile);
	tfremove(tmpfile);
	send();
	active_restore(groupfile);
	ErrFlag = FALSE;
	rmlock(newsdir, groupfile);
	rmlock(newsdir, mid_file);
	if (Mailfp != stdin) {
		fclose(Mailfp);
		remove(bakfile);
		rename(interfile,bakfile);
	}
	exit(0);
}

void
init_param(void)
{
	char *p, *q;
	/* char *e; */
	int	n;
	char buf[BUFSIZ];
	char *hostname, *spooldir, *myhpath, *ndir, *ldir, *nfile;
	char myhostlen = 0;
	char myhostcount;
	unsigned tmplimit, spoollimit, newslimit;

	myhostpath[0] = '\0';

	get_params(&hostname, &batchdir, &spooldir, &errlog, &myhpath, &ndir, &nfile, &ldir, &myhostlen, &tmplimit, &spoollimit, &newslimit);

	/* default for soroban.errlog */
	/* soroban.errlog <= [Spooldir]/soroban.err */
	if ((char *)NULL == errlog){
		errlog = malloc(strlen(spooldir) + 12);
		if((char *)NULL == errlog) alloc_error("init_param");
		strcpy(errlog, spooldir);
		strcat(errlog, "/soroban.err");
	}
	if ((errlogfp = fopen(errlog, "a")) == NULL) {
		fprintf(stderr, "%s: %s cannot open\n", comname, errlog);
		exit(1);
	}
	chkdfree(spooldir, spoollimit);

	/* default for soroban.batchdir */
	/* soroban.batchdir <= [SpoolDir]/batch */
	if ((char *)NULL == batchdir){
		batchdir = malloc(strlen(spooldir) + 6);
		if((char *)NULL == batchdir) alloc_error("init_param");
		strcpy(batchdir, spooldir);
		strcat(batchdir, "/batch");
	}
	/* maildir */
	strcpy(buf, spooldir);
	strcat(buf, "/mail");
	maildir = strdup(buf);
	if((char *)NULL == maildir) alloc_error("init_param");
	/* making path for newsUser */
	strcat(buf, "/");
	strcat(buf, nfile);
	n = strlen(buf);
	/* .txt - mailfile */
	strcat(buf, ".txt");
	mailfile = strdup(buf);
	if((char *)NULL == mailfile) alloc_error("init_param");
	/* .lck - maillck */
	buf[n] = (char)NULL;
	strcat(buf, ".lck");
	maillck = strdup(buf);
	if((char *)NULL == maillck) alloc_error("init_param");
	/* .bak - bakfile */
	buf[n] = (char)NULL;
	strcat(buf, ".bak");
	bakfile = strdup(buf);
	if((char *)NULL == bakfile) alloc_error("init_param");
	/* .int - interfile */
	buf[n] = (char)NULL;
	strcat(buf, ".int");
	interfile = strdup(buf);
	if((char *)NULL == interfile) alloc_error("init_param");
	/* [Spooldir]/mqueue */
	strcpy(buf, spooldir);
	strcat(buf, "/mqueue");
	mqueue = strdup(buf);
	if((char *)NULL == mqueue) alloc_error("init_param");
	/* ... sequence.seq */
	strcat(buf, "/sequence.seq");
	sequencefile = strdup(buf);
	if((char *)NULL == sequencefile) alloc_error("init_param");

	/* is set HOSTNAME for Path: field of newsheader? */
	if (myhpath != (char *)NULL){
		strcpy(myhostpath, myhpath);
	} else {
		*myhostpath = (char)NULL;
	}
	strcpy(myhost, hostname);

	/* making hostname (short) */
	for (p = myhost, q = myhost0; *p; p++, q++) {
		if (*p == '.')
			break;
		*q = *p;
	}
	*q = (char)NULL;

	/* When myhostpath is not set, */
	/* making myhostpath from Hostname.Domainname */
	/* and myhostlen */
	if ((char)NULL == *myhostpath) {
		for (p = myhost, q = myhostpath, myhostcount = 0; *p; p++, q++) {
			if (*p == '.')
				if (++myhostcount >= myhostlen)
					break;
			*q = *p;
		}
	}
	*q = (char)NULL;
	/* setting default for Terakoya.newsDir */
	/* [SpoolDir]/news */
	if (ndir == (char *)NULL){
		ndir = malloc(strlen(spooldir) + 6);
		if((char *)NULL == ndir) alloc_error("init_param");
		strcpy(ndir, spooldir);
		strcat(ndir, "/news");
	}
	newslen = strlen(ndir) + 1;
	reform_path(newsdir, ndir);
	chkdfree(newsdir, newslimit);
	/* default for Terakoya.libDir */
	/* [Terakoya.newsDir]/lib */
	if ((char *)NULL == ldir){
		strcpy(libdir, newsdir);
		strcat(libdir, "/lib");
	} else {
		strcpy(libdir, ldir);
	}

	/* set some path that is in under Terakoya.libDir */
	strcpy(mid_file, libdir);
	strcpy(mid_idx, libdir);
	strcpy(mid_lck, libdir);
	strcpy(groupfile, libdir);
	strcpy(grouplck, libdir);
	strcpy(sysfile, libdir);

	strcat(mid_file, MID_FILE);
	strcat(mid_idx, MID_IDX);
	strcat(mid_lck, MID_LCK);
	strcat(groupfile, GROUPFILE);
	strcat(grouplck, GROUPLCK);
	strcat(sysfile, SYSFILE);

	reform_path(tmpname, getenv(TMPDIR));
	chkdfree(tmpname, tmplimit);
	strcat(tmpname, TMPNAME);
	strcpy(tzone, getenv("TZ"));
	if (*tzone == '\0') strcpy(tzone, "JST");

	if (Verbose) printf("%s: checking %s file\n", comname, groupfile);
	active_read(groupfile);
	if (Verbose) printf("%s: checking %s file\n", comname, sysfile);
	sys_read(sysfile);
	ActiveOK = TRUE;
}

void
exit_func(void)
{
	FILE *fp;
	char buf[BUFSIZ];
	char rfc_t[28];
	time_t timer;

	if (ErrFlag) {
		fp = fopen(mailfile, "a");
		rewind(Mailfp);
		while (fgets(buf, BUFSIZ, Mailfp)) fputs(buf, fp);
		fclose(fp);
		errprintf("Some errors!! please remake index file\n");
	}
	close_mid();
	tfcloseall();
	fcloseall();
	(void)time(&timer);
	change_time(ctime(&timer), rfc_t);
	printf("%s: end      %s\n", comname, rfc_t);
}


void
analyze(char *file)
{
	FILE *fp;
	char buf[BUFSIZ];
	char from[BUFSIZ];
	char fromfull[BUFSIZ];
	char desthost[BUFSIZ];
	char *p, *arg1, *arg2;
	long fpos = 0;
	int state;
	int inheader;
	int newsflag;
	int approved;

	fp = tfopen(file, READ);
	while (fgets(buf, BUFSIZ, fp)) {
		if (!news_end(buf)) {
			fpos = ftell(fp);
		}
		if (cntlmsg(buf, fp)) {
			newsflag = FALSE;
			state = NOP;
			*fromfull = '\0';
			*from = '\0';
			approved = FALSE;
			fseek(fp, fpos, 0);
			for (fgets(buf, BUFSIZ, fp); news_end(buf); fgets(buf, BUFSIZ,fp)) {
				if (*buf == '\n') break;
				if (!strnicmp(buf, "reply-to:", 9)) {
					strtok(buf, " \t\n()");
					strcpy(from, strtok(NULL, " \t\n()"));
				}
				if (!strnicmp(buf, "sender:", 7)) {
					strcpy(fromfull, buf);
				}
				if (!strnicmp(buf, "from:", 5)) {
					if (*fromfull == '\0') {
						strcpy(fromfull, buf);
					}
					if (*from == '\0') {
						strtok(buf, " \t\n()");
						strcpy(from, strtok(NULL, " \t\n()"));
					}
				}
				if (!strnicmp(buf, "approved:", 9)) {
					approved = TRUE;
				}
			}
			*desthost = '\0';
			fseek(fp, fpos, 0);
			inheader = TRUE;
			for (fgets(buf, BUFSIZ, fp), inheader = TRUE; news_end(buf); fgets(buf, BUFSIZ, fp)) {
				if (*buf == '\n') {
					inheader = FALSE;
					continue;
				}
				if (!strnicmp(buf, "control:", 8)) {
					(void)strtok(buf, sptabnl);
					p = strtok(NULL, sptabnl);
					arg1 = strtok(NULL, sptabnl);
					arg2 = strtok(NULL, sptabnl);
					if (Verbose)
						printf("%s: Control message: %s %s %s\n", 
						 comname, p, arg1 ? arg1 : "", arg2 ? arg2 : "");
					if (!stricmp(p, IHAVEMSG)) {
						state = IHAVE;
						if (arg1) {
							if (arg2) doihave(arg1, arg2);
							else strcpy(desthost, arg1);
						}
					}
					if (!stricmp(p, SENDMEMSG)) {
						state = SENDME;
						if (arg1) {
							if (arg2) dosendme(arg1, arg2);
							else strcpy(desthost, arg1);
						}
					}
					if (!stricmp(p, ALHAVEMSG)) {
						state = ALHAVE;
						if (arg1) {
							if (arg2) doalhave(arg1, arg2);
							else strcpy(desthost, arg1);
						}
					}
					if (!stricmp(p, ADDMEMSG)) {
						state = NOP;
						doaddme(arg1, arg2);
					}
					if (!stricmp(p, DELMEMSG)) {
						state = NOP;
						dodelme(arg1);
					}
					if (!stricmp(p, SENDSYSMSG)) {
						state = NOP;
						newsflag = TRUE;
						dosendsys(from);
					}
					if (!stricmp(p, SENDVERMSG)) {
						state = NOP;
						newsflag = TRUE;
						dosendver(from);
					}
					if (!stricmp(p, NEWGRPMSG)) {
						state = NOP;
						newsflag = TRUE;
						if (approved) donewgroup(arg1);
					}
					if (!stricmp(p, RMGRPMSG)) {
						state = NOP;
						newsflag = TRUE;
						if (approved) dormgroup(arg1);
					}
					if (!stricmp(p, CANCELMSG)) {
						state = NOP;
						newsflag = TRUE;
						docancel(arg1, fromfull);
					}
				}
				if (inheader) continue;
				if ((arg1 = strtok(buf, sptabnl)) == NULL) continue;
				arg2 = strtok(NULL, sptabnl);
				switch (state){
					case IHAVE:
						if (arg2) doihave(arg1, arg2);
						else doihave(arg1, desthost);
						continue;
					case SENDME:
						if (arg2) dosendme(arg1, arg2);
						else dosendme(arg1, desthost);
						continue;
					case ALHAVE:
						if (arg2) doalhave(arg1, arg2);
						else doalhave(arg1, desthost);
						continue;
					default:
						continue;
				}
			}
			if (newsflag || Allnewsflag) {
				fseek(fp, fpos, 0);
				if (feof(fp)) break;
				fpos = donews(fp);
			} else fpos = ftell(fp);
		} else {
			fseek(fp, fpos, 0);
			if (feof(fp)) break;
			fpos = donews(fp);
		}
	}
	tfclose(fp);
}


void
copy_f_delh(FILE *destfp, FILE *srcfp)
{
	char buf[BUFSIZ];
	int flag;

	fputs(LINEDELM "\n", destfp);
	flag = FALSE; /* If this flag is true, then in news */
	while (fgets(buf, BUFSIZ, srcfp)) {
		if (!strnicmp(buf, "From:", 5)) {
			fprintf(destfp, "%s%s", FROMMARK, buf);
		} else if (!strnicmp(buf, "To:", 3)) {
			fprintf(destfp, "%s%s", TOMARK, buf);
		} else if (*buf == 'N') {
			if (!fputs(buf+1, destfp)) {
				fputs("$TMP is full.\a\n", stderr);
				exit(1);
			}
			flag = TRUE;
		} else if (flag) {
			fputs(LINEDELM"\n", destfp);
			flag = FALSE;
		}
	}
	if (flag) {
		if (!fputs(LINEDELM"\n", destfp)) {
			fputs("$TMP is full.\a\n", stderr);
			exit(1);
		}
	}
}


int
cntlmsg(char buf[], FILE *fp)
{
	char *p;

	do {
		if (*buf == '\n') break;
		if (!strnicmp(buf, "Control:", 8)) return TRUE;
		if (!strnicmp(buf, "Newsgroups:", 11)) {
			(void)strtok(buf, sptabnl);
			p = strtok(NULL, sptabnl);
			if (!strnicmp(p, "all.all.ctl", 11)) return TRUE;
		}
	} while (fgets(buf, BUFSIZ, fp)) ;
	return FALSE;
}


void
get_params(char **hostname, char **batchdir, char **spooldir, char **errfile, char **myhostpath, char **newsdir, char **newsfile, char **libdir, char *myhostlen, unsigned *tmplimit, unsigned *spoollimit, unsigned *newslimit)
{
	static char	*hname = (char *)NULL,
				*phostname = (char *)NULL,
				*dname = (char *)NULL,
				*ndir = (char *)NULL,
				*hosname = (char *)NULL,
				*ldir = (char *)NULL,
				*bdir = (char *)NULL,
				*sdir = "/spool",
				*nfile = "rnews",
				*efile = (char *)NULL,
				*tmpl = (char *)NULL,
				*spooll = (char *)NULL,
				*newsl = (char *)NULL,
				*phostlen = "1";

	static struct netdefcb const params[] = {
		{NETDEF_DEFI, "Hostname", &hname, },
		{NETDEF_DEFI, "Domainname", &dname, },
		{NETDEF_DIRE, "SpoolDir", &sdir, },
		{NETDEF_DIRE, "Terakoya.newsDir", &ndir, },
		{NETDEF_DIRE, "Terakoya.libDir", &ldir, },
		{NETDEF_DIRE, "Terakoya.batchDir", &bdir, },
		{NETDEF_DEFI, "Terakoya.newsUser", &nfile, },
		{NETDEF_FILE, ".errfile", &efile, },
		{NETDEF_DEFI, ".pathhostname", &phostname, },
		{NETDEF_DEFI, ".pathhostlen", &phostlen, },
		{NETDEF_DEFI, "Terakoya.tmpLimit", &tmpl, },
		{NETDEF_DEFI, "Terakoya.spoolLimit", &spooll, },
		{NETDEF_DEFI, "Terakoya.newsLimit", &newsl, },
		{(char *)NULL, (char *)NULL, (char **)NULL},
	};

	if (loadnetdef(params) == -1){
		errprintf("network.def is not found.");
		exit(1);
	}

	*libdir = ldir;
	*myhostlen = atoi(phostlen);
	*myhostpath = phostname;
	*batchdir = bdir;
	*spooldir = sdir;
	*errfile = efile;
	*newsdir = ndir;
	*newsfile = nfile;
	*tmplimit = atoi(tmpl);
	*spoollimit = atoi(spooll);
	*newslimit = atoi(newsl);

	if ((char *)NULL == *hostname){
		errprintf("%s : Set Hostname.", comname);
		exit(1);
	}

	if ((char *)NULL == (hosname = (char *)malloc((size_t)(strlen(hname) + strlen(dname) + 2)))){
		alloc_error("get_params");
	}
	if((char)NULL != dname){
		strcpy(hosname, hname);
		strcat(hosname, ".");
		strcat(hosname, dname);
	} else {
		strcpy(hosname, hname);
	}
	*hostname = hosname;
}
