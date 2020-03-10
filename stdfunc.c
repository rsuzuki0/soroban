/*
 *	stdfunc.c: basic function for oimo system
 *	version 1.20 1990/7/4
 *	programmed by Shigeki Matsushima and JK1LOT(Dai Yokota)
 *	Copyright 1988,89,89 (C) by Shigeki Matsusima and JK1LOT(Dai Yokota)
 *	All rights are reserved
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>		/* ms-dos */
#include <jctype.h>		/* ms-dos Turbo C 2.0 kanji header */
#include <jstring.h>    /* ms-dos Turbo C 2.0 kanji header */
#include <dir.h>
#include <io.h>
#include "stdfunc.h"
#include "errors.h"
typedef long time_t;
#include "soroban.h"
#include "active.h"

extern char tzone[];

static char *Kana2jis[64] = {
	"\x21\x21", /* a0 ' ' "@" */
	"\x21\x23", /* a1 'F "B" */
	"\x21\x56", /* a2 '¥ "u" */
	"\x21\x57", /* a3 '‚F "v" */
	"\x21\x22", /* a4 '‚¥ "A" */
	"\x21\x26", /* a5 'ƒF "E" */
	"\x25\x72", /* a6 'ƒ¥ */
	"\x25\x21", /* a7 '„F */
	"\x25\x23", /* a8 '„¥ */
	"\x25\x25", /* a9 'ŠF */
	"\x25\x27", /* aa '…¥ */
	"\x25\x29", /* ab '†F */
	"\x25\x63", /* ac '†¥ */
	"\x25\x65", /* ad '‡F */
	"\x25\x67", /* ae '‡¥ */
	"\x25\x43", /* af 'ˆF */
	"\x21\x3c", /* b0 'ˆ¥ */
	"\x25\x22", /* b1 '‰F */
	"\x25\x24", /* b2 '‰¥ */
	"\x25\x26", /* b3 'ŠF */
	"\x25\x28", /* b4 'Š¥ */
	"\x25\x2a", /* b5 '‹F */
	"\x25\x2b", /* b6 '‹¥ */
	"\x25\x2d", /* b7 'ŒF */
	"\x25\x2f", /* b8 'Œ¥ */
	"\x25\x31", /* b9 'F */
	"\x25\x33", /* ba '¥ */
	"\x25\x35", /* bb 'ŽF */
	"\x25\x37", /* bc 'Ž¥ */
	"\x25\x39", /* bd 'F */
	"\x25\x3b", /* be '¥ */
	"\x25\x3d", /* bf 'F */
	"\x25\x3f", /* c0 '¥ */
	"\x25\x41", /* c1 '‘F */
	"\x25\x44", /* c2 '‘¥ */
	"\x25\x46", /* c3 '’F */
	"\x25\x48", /* c4 '’¥ */
	"\x25\x4a", /* c5 '“F */
	"\x25\x4b", /* c6 '“¥ */
	"\x25\x4c", /* c7 '”F */
	"\x25\x4d", /* c8 '”¥ */
	"\x25\x4e", /* c9 '•F */
	"\x25\x4f", /* ca '•¥ */
	"\x25\x52", /* cb '–F */
	"\x25\x55", /* cc '–¥ */
	"\x25\x58", /* cd '—F */
	"\x25\x5b", /* ce '—¥ */
	"\x25\x5e", /* cf '˜F */
	"\x25\x5f", /* d0 '˜¥ */
	"\x25\x60", /* d1 '™F */
	"\x25\x61", /* d2 '™¥ */
	"\x25\x62", /* d3 'šF */
	"\x25\x64", /* d4 'š¥ */
	"\x25\x66", /* d5 '›F */
	"\x25\x68", /* d6 '›¥ */
	"\x25\x69", /* d7 'œF */
	"\x25\x6a", /* d8 'œ¥ */
	"\x25\x6b", /* d9 'F */
	"\x25\x6c", /* da '¥ */
	"\x25\x6d", /* db 'žF */
	"\x25\x6f", /* dc 'ž¥ */
	"\x25\x73", /* dd 'ŸF */
	"\x21\x2b", /* de 'Ÿ¥ */
	"\x21\x2c"  /* df 'àF */
};

void fputs_shiftjis(unsigned char *buf, FILE *fp)
{
	register int shiftin = FALSE;
	register unsigned char hi, lo;

	while (*buf) {
		if ((*buf == '\033') && (*(buf+1) == '$')
		&& ((*(buf+2) == 'B') || (*(buf+2) == '@'))) {
			shiftin = TRUE;
			buf += 3;
		} else if ((*buf == '\033') && (*(buf+1) == '(')
			 && ((*(buf+2) == 'J') || (*(buf+2) == 'B') || (*(buf+2) == 'H'))){
			shiftin = FALSE;
			buf += 3;
		} else if (shiftin) {
			hi = *buf++;
			if ((lo = *buf++) == '\0')
				break;
			if (hi & 1) lo += 0x1f;
			else lo += 0x7d;
			if (lo >= 0x7f) lo++;
			hi = ((hi - 0x21) >> 1) + 0x81;
			if (hi > 0x9f) hi += 0x40;
			if (fputc(hi, fp) == EOF)
				disk_error("fputs_sjis");
			if (fputc(lo, fp) == EOF)
				disk_error("fputs_sjis");
		} else {
			if (fputc(*buf, fp) == EOF)
				disk_error("fputs_sjis");
			buf++;
		}
	}
}

void fputs_newjis(unsigned char *buf, FILE *fp)
{
	register int kanjiflag = FALSE;
	register unsigned char hi, lo;

	while (*buf) {
		if (iskanji(*buf) && iskanji2(*(buf+1))) {
			if (kanjiflag == FALSE) {
				kanjiflag = TRUE;
				if (fputs("\033$B", fp) == EOF)
					disk_error("fputs_jis");
			}
			hi = *buf++;
			if ((lo = *buf++) == '\0')
				break;
			hi -= (hi <= 0x9f) ? 0x71 : 0xb1;
			hi = hi * 2 + 1;
			if (lo > 0x7f) lo -= 1;
			if (lo >= 0x9e) {
				lo -= 0x7d;
				hi += 1;
			} else lo -= 0x1f;
			if (fputc(hi, fp) == EOF)
				disk_error("fputs_jis");
			if (fputc(lo, fp) == EOF)
				disk_error("fputs_jis");
		} else if (iskana(*buf)) {
			/* convert 8bit kana code to JIS-KANJI code */
			if (kanjiflag == FALSE) {
				kanjiflag = TRUE;
				if (fputs("\033$B", fp) == EOF)
					disk_error("fputs_jis");
			}
			lo = *buf++;
			lo -= 0xa0;
			if (fputs(Kana2jis[lo], fp) == EOF)
				disk_error("fputs_jis");
		} else if (*buf & 0x80) {
			buf++;
		} else {
			lo = *buf++;
			if (kanjiflag == TRUE) {
				kanjiflag = FALSE;
				if (fputs("\033(J", fp) == EOF)
					disk_error("fputs_jis");
			}
			fputc(lo, fp);
		}
	}
	if (kanjiflag)	fputs("\033(J", fp);
}

void basename(char *name, char *path)
{
	char tmps[MAXPATH];
	register char *p;

	strcpy(tmps, path);
	p = tmps + strlen(tmps);
	while (p >= tmps) {
		if (*p == '.') {
			*p = '\0';
		} else if (*p == '\\' || *p == '/') {
			p++;
			break;
		}
		p--;
	}
	strcpy(name, p);
}

void
kill_cr(char *str)
{
	register char *ptr;
	if ((ptr = strchr(str, '\n')) != NULL)
		*ptr = '\0';
}

void reform_path(char *buf, char *path)
{
	register int len;
	char tmp[MAXPATH];
	register char *p;

	*buf = '\0';
	if (path == NULL || *path == '\0')
		return;
	while (*path == ' ' || *path == '\t') path++;
	if (*path == '~' && (*(path+1) == '\\' || *(path+1) == '/')) {
		strcpy(tmp, getenv("HOME"));
		len = strlen(tmp)-1;
		if ((tmp[len] == '\\' || tmp[len] == '/') && !(iskanji(tmp[len-1])))
			tmp[len] = '\0';
		strcat(buf, tmp);
		path++;
	}
	strcat(buf, path);
#ifdef DEBUG
	fprintf(stderr, "path: %s\n", buf);
#endif
	len = strlen(buf)-1;
	if ((buf[len] == '\\' || buf[len] == '/') && !(iskanji(buf[len-1])))
		buf[len] = '\0';

	for (p = buf; *p; p++) {
		if (iskanji(*p)) {
			if (*(++p) == '\0') break;
			else continue;
		}
		if (*p == '\\') *p = '/';
	}
#ifdef DEBUG
	fprintf(stderr, "reformed path: %s\n", buf);
#endif
}

void sltoyen(char *dest, char *src)
{
	while (*src) {
		if (iskanji(*src)) {
			*dest = *src;
			if ((*(++dest) = *(++src)) == '\0') break;
			else continue;
		}
		if (*src == '/') *dest = '\\';
		else *dest = *src;
		dest++;
		src++;
	}
	*dest = '\0';
}

void change_time(char *unix_t, char *rfc_t)
{
	rfc_t[0] = unix_t[0]; rfc_t[1] = unix_t[1]; rfc_t[2] = unix_t[2];
	rfc_t[3] = ', '; rfc_t[4] = ' '; rfc_t[5] = unix_t[8];
	rfc_t[6] = unix_t[9]; rfc_t[7] = ' '; rfc_t[8] = unix_t[4];
	rfc_t[9] = unix_t[5]; rfc_t[10] = unix_t[6]; rfc_t[11] = ' ';
	rfc_t[12] = unix_t[22]; rfc_t[13] = unix_t[23]; rfc_t[14] = ' ';
	rfc_t[15] = unix_t[11]; rfc_t[16] = unix_t[12]; rfc_t[17] = unix_t[13];
	rfc_t[18] = unix_t[14]; rfc_t[19] = unix_t[15]; rfc_t[20] = unix_t[16];
	rfc_t[21] = unix_t[17]; rfc_t[22] = unix_t[18]; rfc_t[23] = ' ';
	rfc_t[24] = tzone[0]; rfc_t[25] = tzone[1]; rfc_t[26] = tzone[2];
	rfc_t[27] = '\0';
}

/*
 * filename availability check:
 * if exists then return TRUE else return FALSE
 */

int file_check(char *path)
{
	return (access(path, 0)+1);
}


/* create mail lockfile
 * This function is written by KA9Q(P. Karn)
 * Copyright 1987 (C) by Phil Karn
 */
int
mlock(char *dir, char *id)
{
	char lockname[MAXPATH];
	register int fd;
	/* Try to create the lock file in an atomic operation */
	sprintf(lockname,"%s/%s.lck",dir,id);
	if((fd = open(lockname, O_WRONLY|O_EXCL|O_CREAT,0600)) == -1)
		return -1;
	close(fd);
	return 0;
}

/* remove mail lockfile
 * This function is written by KA9Q(P. Karn)
 * Copyright 1987 (C) by Phil Karn
 */
int
rmlock(char *dir, char *id)
{
	char lockname[MAXPATH];
	sprintf(lockname,"%s/%s.lck",dir,id);
	return(unlink(lockname));
}

/*
 *	copy file
 *	from FILE *fpsrc to *fpdest
 */
void copy_file(FILE *fpdest, FILE *fpsrc)
{
	char buf[BUFSIZ];

	while (fgets(buf, BUFSIZ, fpsrc) != NULL) {
		if (fputs(buf, fpdest) == EOF)
			disk_error("copy_file");
	}
}
