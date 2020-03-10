/*
 *  hash.c: Hash file routin of soroban(terakoya-system)
 *  version 1.00 1989/11/25 released on 1989/11/25
 *	programmed by JK1LOT(Dai Yokota)
 *	Copyright 1989 (C) by JK1LOT(Dai Yokota)
 *	All rights are reserved
 *
 *  version 1.01 May - June by Ruiji Suzuki(jf7wex).
 *    hash() is tuned.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dir.h>
#include <io.h>
#include <time.h>
#include <fcntl.h>
#include <sys\stat.h>
#include "soroban.h"
#include "hash.h"

#define HASHSIZE 65519

#define FALSE 0
#define TRUE 1

static FILE *datafp;
static int hashhandle;
static char datafn[MAXPATH], hashfn[MAXPATH];
static char cachebuf[BUFSIZ];

int
init_hash(char *datafile, char *hashfile)
{
	*cachebuf = '\0';
	strcpy(datafn, datafile);
	if ((datafp = fopen(datafile, "at+")) == NULL)
		return FALSE;
	rewind(datafp);
	strcpy(hashfn, hashfile);
	if ((hashhandle =
	   open(hashfile, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE)) == -1)
		return FALSE;
	return TRUE;
}

void
close_hash()
{
	fclose(datafp);
	close(hashhandle);
}

static int
creat_init_hash()
{
	register long i;
	long l[2] = {-1L, -1L};
	long tbl[512];

	close(hashhandle);
	remove(hashfn);
	if ((hashhandle
	 = open(hashfn, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE)) == -1)
		return FALSE;
	for (i = 0; i < 512; i++) {
		tbl[(unsigned)i] = -1L;
	}
	for (i = 0; i < HASHSIZE - (512 / 2); i += 512 / 2) {
		write(hashhandle, tbl, sizeof(tbl));
	}
	for (; i < HASHSIZE; i++) {
		write(hashhandle, l, sizeof(l));
	}
	return TRUE;
}

int
creat_hash(char *datatmp)
{
	FILE *dfp, *tfp;
	char buf[BUFSIZ];

	creat_init_hash();

	fclose(datafp);
	remove(datatmp);
	if ((dfp = fopen(datafn, "rt")) == NULL)
		return FALSE;
	if ((tfp = fopen(datatmp, "wt")) == NULL)
		return FALSE;
	while (fgets(buf, BUFSIZ, dfp) != NULL) {
		fputs(buf, tfp);
	}
	fclose(dfp);
	fclose(tfp);
	remove(datafn);
	if ((datafp = fopen(datafn, "at+")) == NULL)
		return FALSE;
	if ((tfp = fopen(datatmp, "rt")) == NULL)
		return FALSE;

	while(fgets(buf, BUFSIZ, tfp) != NULL) {
		add_hash(buf);
	}
	fclose(tfp);
	remove(datatmp);

	return TRUE;
}

void
add_hash(char *string)
{
	char key[BUFSIZ];
	char *p, *q;
	long hashp, datap, newhashp;
	long empty = -1L;

/*
	strcpy(cachebuf, string);
*/

	for (p = string, q = key; (*p != ' ')&& *p; p++, q++)
		*q = *p;
	*q = '\0';

	hashp = (unsigned long)hash(key) * sizeof(long) * 2;
	for (;;) {
		lseek(hashhandle, hashp, SEEK_SET);
		read(hashhandle, &datap, sizeof(datap));
		if (datap == -1) break;
		read(hashhandle, &hashp, sizeof(hashp));
	}
	fseek(datafp, 0L, SEEK_END);
	datap = ftell(datafp);
	fputs(string, datafp);

	lseek(hashhandle, 0L, SEEK_END);
	newhashp = tell(hashhandle);
	write(hashhandle, &empty, sizeof(empty));
	write(hashhandle, &empty, sizeof(empty));

	lseek(hashhandle, hashp, SEEK_SET);
	write(hashhandle, &datap, sizeof(datap));
	write(hashhandle, &newhashp, sizeof(newhashp));
}

char *
search_hash(char *key, char *buf)
{
	long hashp, datap;
	int keylen;

	keylen = strlen(key);
	if (!strncmp(key, cachebuf, keylen)) {
		strcpy(buf, cachebuf);
		return buf;
	}

	hashp = (unsigned long)hash(key) * sizeof(long) * 2;

	for (;;) {
		lseek(hashhandle, hashp, SEEK_SET);
		read(hashhandle, &datap, sizeof(datap));
		if (datap == -1) break;
		fseek(datafp, datap, SEEK_SET);
		fgets(buf, BUFSIZ, datafp);
		if (!(strncmp(key, buf, keylen))) break;
		read(hashhandle, &hashp, sizeof(hashp));
	}
	if (datap == -1) return NULL;
	strcpy(cachebuf, buf);
	return buf;
}

unsigned
hash(const char *string)
{
	register const unsigned char *p = string + 1;
	register unsigned long address = 0, sum;
	register unsigned i;

	for (;;) {
		sum = 0;
		for (i = 0; *p != '\0' && *p != '>' && i < sizeof(long) / 2; i++, p++)
			sum += (*p) << 7 * i;
		sum *= sum;
		sum >>= 2;
		address ^= sum;
		if (*p == '\0' || *p == '>') break;
	}
	address %= HASHSIZE;
	/* printf("%5ld\t%s\n", address, string); */
	return (unsigned)address;
}
