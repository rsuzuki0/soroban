/*
 *	tempfile.h: header file for tempfile.c
 *  version 1.0 1988/12/19 released on 1988/12/21
 *	programmed by Shigeki Matsushima and JK1LOT(Dai Yokota)
 *	Copyright 1988 (C) by Shigeki Matsushima and Dai Yokota
 *	All rights are reserved
 *
 *  ���ӁF���̃w�b�_�[�͕K�� stdio.h �̌�ɃC���N���[�h���Ă��������B
 */

FILE *tfcreat(char **name);
FILE *tfopen(char *name, char *mode);
void tfclose(FILE *file);
void tfremove(char *name);
void tfcloseall(void);
