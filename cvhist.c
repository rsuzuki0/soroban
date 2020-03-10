/* cvhist converts old history to new history */

/* Some bugs that indicated by JP1FOC were fixed. */
/* June 1992 Ryuji Suzuki (JF7WEX) */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <io.h>
#include "wexlib.h"
#include "ryujilib.h"
#include "netdef.h"


static char *spool = "/spool", *newsdir, *libdir;
static struct netdefcb const params[] = {
	{NETDEF_DIRE, "SpoolDir", &spool, },
	{NETDEF_DIRE, "Terakoya.newsDir", &newsdir, },
	{NETDEF_DIRE, "Terakoya.libDir", &libdir, },
	{(char *)NULL, (char *)NULL, (char **)NULL},
};


int
main(void)
{
	char mid_file[BUFSIZ];

	{ /* set params */
		if(-1 == loadnetdef(params)){
			fputs("dentaku: network.def not found.", stderr);
			exit(1);
		}
		if(!newsdir || !(*newsdir)) {
			newsdir = (char *)malloc(strlen(spool) + 6);
			strcpy(newsdir, spool);
			strcat(newsdir, "/news");
		}
		if(!libdir || !(*libdir)) {
			libdir = (char *)malloc(strlen(newsdir) + 5);
			strcpy(libdir, spool);
			strcat(libdir, "/lib");
		}
		sprintf(mid_file, "%s/history", libdir);
	}
	tzset();
	{ /* main part */
	FILE *fp;
	char *ptr, *ptr2, buf[BUFSIZ];
	time_t timer;
	unsigned newslen;

	if (!isatty(0)) fp = stdin;
	else if ((FILE *)NULL == (fp = fopen(mid_file, "rt"))) {
		fputs("history is not found.\n", stderr);
		exit(1);
	}
	newslen = strlen(newsdir);
	newslen++;
		while (fgets(buf, BUFSIZ, fp)) {
			strtok(buf, " \t\n");
			ptr = strtok(NULL, " \t\n");
			ptr2= strtok(NULL, " \t\n");
			timer = cvoldtime(ptr, ptr2);
			ptr = strtok(NULL, " \t\n");
			if (ptr == NULL) {
				fputs("WRONG FORMAT line is found in history.\n", stderr);
				exit(1);
			}
			if (isnewspath(ptr, newsdir)) {
				ptr += newslen;
				for (ptr2 = ptr; *ptr2; ++ptr2) {
					if (*ptr2 == '/') *ptr2 = '.';
				}
				ptr2 = strrchr(ptr, '.');
				if (ptr2) *ptr2 = '/';
			}
			printf("%s %ld~- %s\n", buf, timer, ptr);
		}
	}
	return 0;
}
