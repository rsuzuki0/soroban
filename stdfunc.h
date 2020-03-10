/*
 *	basicfun.h: header file to use basicfun.c
 *	version 1.01 1988/12/21 released on 1988/12/21
 *	programmed by Shigeki Matsushima and JK1LOT(Dai Yokota)
 *	Copyright 1988 (C) by Shigeki Matsushima and JK1LOT(Dai Yokota)
 *	All rights are reserved
 *
 *	this header is for ms-dos use only
 *	this header is for Turbo-C use only
 */

#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif
#define SI		0x0e
#define SO		0x0f

int addcur(char *path, char *group);
void fputs_shiftjis(unsigned char *buf, FILE *fp);
void basename(char *name, char *path);
void kill_cr(char *str);
void reform_path(char *buf, char *path);
void sltoyen(char *dest, char *src);
void change_time(char *unix_t, char *rfc_t);
int file_check(char *path);
void fputs_newjis(unsigned char *buf, FILE *fp);
int mlock(char *, char *);
int rmlock(char *, char *);
void copy_file(FILE *fpdest, FILE *fpsrc);
int conv_address(char *dest, char *src);
