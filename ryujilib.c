#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "ryujilib.h"

/* (C) 1992 Ryuji Suzuki (JF7WEX) */
static unsigned char
RCS_ID[] = "$Header: rjslib.cv  1.1  92/09/17 04:11:26  jf7wex  Exp $";

/* strtoke() is like strtok(). */
char *
strtoke(char *src, const char *sep)
{
	register char const *r;
	register char *q;
	char *p;
	static char *srcptr = NULL;

	if (!(srcptr || src ) || !sep) {
		srcptr = NULL;
		return NULL;
	}
	if (src) {
		srcptr = src;
		p = src;
	} else {
		p = srcptr;
	}
	for (q = p; *q; q++) {
		for (r = sep; *r; r++) {
			if (*q == *r) {
				*q = NULL;
				if (*++q != '\0') {
					srcptr = q;
				} else {
					srcptr = NULL;
				}
				return p;
			}
		}
	}
	return p;
}


/* strcntchr counts that how many key characters does src string includes. */
unsigned
strcntchr(const char *src, const char key)
{
	register unsigned i;
	register unsigned char j;
	register unsigned n = 0;

	for (i = 0; (j = src[i]) != (char)'\0'; i++) {
		if (j == key) n++;
	}
	return n;
}


/* fgetsb() is like fgets() but ignores CR */
char *
fgetsb(char buf[], const int n, FILE *fp)
{
	register unsigned i;
	unsigned char c;

	buf[--n] = '\0';
	for (i = 0; i <= n; i++) {
		if (1 != fread(&c, 1, 1, fp)) {
			buf[i] = '\0';
			return NULL;
		} else buf[i] = c;
		if (c == 0x0a) {
			buf[++i] = '\0';
			break;
		}
	}
	buf[i] = '\0';
	return buf;
}

/* getmline() is also like fgets() */
/* principal differences from fgets() is that escape character ('\'), */
/* which can escape CR-LF, is available. */
char *
getmline(FILE *fp)
{
	register unsigned i;
	register int c;
	static unsigned size = 0;
	static char *ptr;

	if (0 == size) {
		size = 512;
		ptr = (char *)malloc(size + 1);
		if (ptr == NULL) return NULL;
	}
	if (feof(fp)) return NULL;
	for (i = 0;;) {
		if (EOF == (c = fgetc(fp))) {
			ptr[i] = '\0';
			return ptr;
		} else if (c == '\\') {
			if (EOF == (c = fgetc(fp))) {
				ptr[i] = '\0';
				return ptr;
			} else if (c == '\n') {
				while (EOF != (c = fgetc(fp))) {
					if (' ' != c && '\t' != c) {
						ungetc(c, fp);
						break;
					}
				}
				if (c == EOF || feof(fp)) {
					ptr[i] = NULL;
					return ptr;
				}
			} else if (c == '\\') {
				ptr[i++] = '\\';
			} else if (c == ' ') {
				ptr[i++] = ' ';
			} else if (c == '\t') {
				ptr[i++] = '\t';
			}
		} else 	{
			ptr[i++] = c;
		}
		if (c == '\n') {
			ptr[i] = '\0';
			return ptr;
		}
		if (i + 3 >= size) {
			size += 128;
			ptr = realloc(ptr, size + 1);
			if (ptr == NULL) return NULL;
		}
		if (feof(fp)) {
			ptr[i] = '\0';
			return ptr;
		}
	}
}
