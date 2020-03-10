struct sys_s {
	struct sys_s *next;
	char *to;
	char *grps;
	char *flag;
	char *file;
	char *nosend;
};

extern struct sys_s *top_sys_list;
void sys_read(char *sysfile);
