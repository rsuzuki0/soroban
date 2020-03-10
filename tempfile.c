/*
 *	tempfile.c: �e���|�����t�@�C���Ǘ��葱��
 *	version 2.02 1989/11/25  released on 1989/11/25
 *	programmed by JK1LOT(Dai Yokota)
 *	Copyright 1988,89 (C) by JK1LOT(Dai Yokota)
 *	All rights are reserved
 * ver. 2.01 by JK1LOT 88/12/17 debugmode used.
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>		/* ms-dos */
#include <alloc.h>
#include <dir.h>
#include "errors.h"
#include "tempfile.h"
typedef long time_t;
#include "soroban.h"

extern char	tmpname[]; /* temp_filename template set at oimo.c*/
extern int debugmode;

struct tf_lists {
	struct tf_lists *next;
	char *fname;
	FILE *fp;
};

static struct tf_lists top_tf = { NULL, NULL, NULL };

/*
 * �e���|�����t�@�C����V���ɍ���ăI�[�v������B
 *   name = �I�[�v�������t�@�C������Ԃ����߂̃|�C���^
 *   �߂�l = �I�[�v�������t�@�C���|�C���^
 */
FILE *tfcreat(char **name)
{
	struct tf_lists *p;

	if ((p = malloc(sizeof(struct tf_lists))) == NULL) {
		alloc_error("tf_open");
	}
	if ((p->fname = strdup(tmpname)) == NULL) {
		alloc_error("tf_open");
	}
	mktemp(p->fname);
	if (!(p->fp = fopen(p->fname, "w+"))) {
		errprintf("%s(tfcreat()):�e���|�����t�@�C�����I�[�v���ł��܂���\n",
		  comname);
		exit(1);
	}
	p->next = top_tf.next;
	top_tf.next = p;

	*name = p->fname;
	return (p->fp);
}

/*
 * �ȑO�ɍ�����e���|�����t�@�C�����I�[�v������B
 *   name = �I�[�v���������t�@�C����
 *   mode = �I�[�v�����郂�[�h
 *   �߂�l = �I�[�v�������t�@�C���|�C���^
 */
FILE *tfopen(char *name, char *mode)
{
	struct tf_lists *p;

	for (p = top_tf.next; p; p = p->next)
		if (!strcmp(p->fname, name)) break;
	if (debugmode) {
		if (!p) {
			errprintf("(tfopen): %s �͑��݂��܂���B\n", name);
			return (NULL);
		}
	}
	if (p == NULL) return NULL;
	if (p->fp) {
		if (debugmode)
			errprintf("(tfopen): %s �͊��ɃI�[�v������Ă��܂��B\n"
			  ,name);
		fclose(p->fp);
		p->fp = NULL;
	}
	if (!(p->fp = fopen(p->fname, mode))) {
		errprintf("%s(tfopen()):�e���|�����t�@�C�����I�[�v���ł��܂���B\n",
		  comname);
		exit(1);
	}
	return (p->fp);
}

/* �e���|�����t�@�C�����N���[�Y����B�i�폜�͂��Ȃ��j*/
void tfclose(FILE *file)
{
	struct tf_lists *p;

	for (p = top_tf.next; p; p = p->next) {
		if (p->fp == file) {
			fclose(p->fp);
			p->fp = NULL;
			break;
		}
	}
	if (debugmode) {
		if (!p) {
			errprintf("���݂��Ȃ��e���|�����t�@�C���̃N���[�Y���߂ł��B\n");
		}
	}
}

/* �e���|�����t�@�C�����폜����B�i�I�[�v�����Ȃ�N���[�Y������j*/
void tfremove(char *name)
{
	struct tf_lists *p, *q;

	for (p = &top_tf; p->next; p = p->next) {
		if (!strcmp(p->next->fname, name)) {
			q = p->next;
			p->next = q->next;
			if (q->fp) {
				fclose(q->fp);
			}
			remove(q->fname);
			free(q->fname);
			free(q);
			return;
		}
	}
	if (debugmode)
		errprintf("���݂��Ȃ��e���|�����t�@�C���̍폜���߂ł��B\n");
}

/*
 * �e���|�����t�@�C����S���N���[�Y���č폜����B
 * �폜���ꂽ�t�@�C���� tf_lists �����菜���Ȃ����߂��̃��[�`����
 * �Ă�ŗǂ��̂̓v���O�����̏I���������P��݂̂ł���B
 */
void tfcloseall()
{
	struct tf_lists *p;
	int count = 0;
	for (p = top_tf.next; p; p = p->next) {
		if (p->fp)
			fclose(p->fp);
		remove(p->fname);
		count++;
	}
	if (debugmode)
		errprintf("(tfcloseall): %d �̃t�@�C�����폜���܂����B\n", count);
}
