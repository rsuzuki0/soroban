/* disk error check */
/* derrors.c version 0.00 (c) June, 1992 Ryuji Suzuki. */
/*                   0.01     Nov,  1992 */
/*                        Thanks to JL1KUF for his patch */

/* this function depends on BORLAND's compiler and its library. */

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <sys/stat.h>
#include "errors.h"

int
chkdfree(char *path, int limit)
{
	struct stat statbuf;
	struct dfree dtable;
/* added by JL1KUF */
	static char alt[4] = "A:/";

	if (!limit) return 0;

/* added by JL1KUF */
	if (strlen(path) == 2 && ':' == path[1]) {
		alt[0] = path[0];
		path = alt;
	}

	if (stat(path, &statbuf)) {
		fprintf(stderr, "derrors(): path %s is incorrect\a\n", path);
		exit(1);
	}

	getdfree(statbuf.st_dev + 1, &dtable);
	if ((long)dtable.df_avail * (long)dtable.df_sclus * (long)dtable.df_bsec / 1024L < limit) {
		errprintf("soroban: Too scanty not used disk area %s (Drive: %c).\n", path, 'A' + statbuf.st_dev);
		fputs("ディスク足りないぞ！\n\a\a\a", stderr);
		exit(2);
	}
	return 0;
}
