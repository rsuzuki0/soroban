#define TRUE		1
#define FALSE		0
#define EOS		'\0'
#define WRITE		"w+"
#define READ		"r+"
#define APPEND		"a+"

#ifdef __SMALL__
#define	VERs "s"
#else
#define VERs ""
#endif

#define	VER "3.05"VERs

#define TMPDIR		"TMP"
#define	TMPNAME		"/soroXXXXXX"	/* temporary file name */

#define	MID_FILE	"/history"
#define	MID_IDX		"/history.idx"
#define	MID_LCK		"/history.lck"
#define	GROUPFILE	"/active"
#define	GROUPLCK	"/active.lck"
#define	SYSFILE		"/sys"

extern int Verbose;
extern int newslen;
extern char comname[];
extern char mid_file[];
extern char mid_idx[];
extern char mid_lck[];
extern char groupfile[];
extern char grouplck[];
extern char sysfile[];
extern char tmpname[];
extern char libdir[];
extern char *batchdir;
extern char *maildir;
extern char *mqueue;
extern char *sequencefile;
extern char *mailfile;
extern char *maillck;
extern char *bakfile;
extern char *interfile;
extern char *autoexec;
extern char *errlog;

extern const char sptabnl[];

#define IHAVEMSG "ihave"
#define SENDMEMSG "sendme"
#define ALHAVEMSG "alhave"
#define CANCELMSG "cancel"
#define ADDMEMSG "addme"
#define DELMEMSG "delme"
#define SENDSYSMSG "sendsys"
#define SENDVERMSG "version"
#define NEWGRPMSG "newgroup"
#define RMGRPMSG "rmgroup"

#define LINEDELM "ºÞÊ»ÝÃÞÈ¶Þ²Ï¼ÃÊ"
#define LINEDELMLEN 15
#define FROMMARK "ÌÞÀÌÞÀºÌÞÀ"
#define FROMMARKLEN 10
#define TOMARK "ÀºÀº±¶ÞÚ"
#define TOMARKLEN 8
#define DFTGRP "ampr"

#define NOP 0
#define IHAVE 1
#define SENDME 2
#define ALHAVE 3
#define BATCH_F 4
#define BATCH_M 5

#define NOCTLMSG 0
#define STOPCTLMSG 1
#define THROUGHCTLMSG 2

#define NORMAL 0
#define CONFIG 1

struct group_s {
	struct group_s *next;
	char *group;
	int filenum;
	char *filename;
	long filesize;
};

extern int NotInActiveToJunk; /* if TRUE the news that is not found active file
                                 move to Junk directory */

/* derrors.c */
int chkdfree(char *path, int limit);

void exit_func(void);
void analyze(char *file);
void copy_f_delh(FILE *destfp, FILE *srcfp);
int cntlmsg(char buf[], FILE *fp);
char *GetHostName(void);
void GetNetworkDef(char *hostname, char *domainname, char *pathhostlen, 
	char *pathhostpath);

void init_mid(int mode);
void close_mid(void);
void config_mid(void);
void add_mid(char *mid, char *ng, int num, time_t);
int srch_mid(char *mid);
FILE *midopen(char *mid, char *mode);
int midrm(char *mid);
FILE *midlckopen(char *mid, char *mode);
void midlckrm(char *mid);
long filetimecomp(char *fn1, char *fn2);

long donews(FILE *mailfp);
int news_analyze(FILE *fp, char *mid, struct group_s **grp,
	struct group_s **distri, int *difflines, int *ctlflag, int *newsttl, time_t *);
int chkgrp(char *group);
void send_news_g(struct group_s *group, struct group_s *distri,
	FILE *tfp, char *from, char *mailto);
int chkgrplist(char *send, struct group_s *group, int cmode);
int chkpathlist(char *nosend, char *path);
void send_neighbour(struct group_s *group, struct group_s *distri,
	char *mid, char *from, char *mailto, char *path);
FILE *dirfopen(char *filename);
void mkdirrec(char *dir);

void doihave(char *mid, char *host);
void dosendme(char *mid, char *host);
void doalhave(char *mid, char *host);
void doaddme(char *host, char *grps);
void dodelme(char *host);
void donewgroup(char *group);
void dormgroup(char *group);
void docancel(char *mid, char *from);
void dosendsys(char *from);
void dosendver(char *from);
char hostnamecomp(char *host1, char *host2);

void smailctl(char *desthost, int mode, char *mid, char *srchost);
void send(void);
void sendmail(char *dest, FILE *sfp, char *filename);
void sendmail2(char *dest, char *subject, FILE *sfp);
void sendmail_postmaster(char *subject, FILE *sfp);
void fprinthead(FILE *fp, int mode, char *dest);
long add_sequence(void);
long get_sequence(void);
void init_param(void);
void get_params(char **hostname, char **batchdir, char **spooldir, char **errfile, char **myhostpath, char **newsdir, char **newsfile, char **libdir,char *myhostlen, unsigned *, unsigned *, unsigned *);

midpath(char *mid, unsigned char buffer[]);
