/* Copyright 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009 Trevor Tonn

	This file is part of Giveme.

    Giveme is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 2.

    Giveme is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Giveme.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef GIVEME_H
#define GIVEME_H


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <regex.h>
#include <math.h>

#ifdef __OSX__
#include <sys/wait.h>
#else
#include <wait.h>
#endif

#define TMPDIR "/tmp/"
#define MAX 200
#define NUMTOOLS 15
#define DEFAULTTHREADCOUNT 2
#define VERSIONINFO "3.2.2, March 2009"


#ifndef LINKEDLISTADT_H
#include "linkedListADT.h"
#endif



typedef struct {
	char *name;
	char *encPath;
	char *decPath;
	char **decAry;
	int decArySize;
	char **encAry;
	int encArySize;
} TOOL;

typedef struct {
	char *name;
	char *shortName;
	char *ext;
	char **encA;
	char *encExe;
	char **decA;
	char *decExe;
	char *wavName;
	char *outName;
	char *outPath;
	bool processMe;
	bool deleteWav;
} FILENAME;

typedef struct {
	char *destFormat;
	char *outPath;
	char *singleFileName; // if this is NULL, do the whole CWD.
	char **customEncSettings;
	int customEncSettingsSize;
	int numThreads;
	char *extensions_regexpStr;
	regex_t *extensions_regexp;
	bool verbose;
	float totalProgress;
	float progressInc;
} JOB;


typedef struct {
	int tid;
	JOB *job;
	FILENAME *fn;
	pthread_mutex_t *mut;
	int *bth;
} PACKAGE2;

JOB *createJob(int , char **);
JOB *destroyJob(JOB *);
char *fixPath(char *path);
JOB *parseOptions(JOB *job, int argc, char **argv);
bool checkOutPath(JOB *job);
void printHelp(void);
void printVersion(void);


LL_HEADER *ls(char *, TOOL **, JOB *);
FILENAME *buildFilename(char *, TOOL **, JOB *);
char **determineCoder(char *ext, TOOL **tools, int type, char **, JOB *);
char *ghettoSubstitute(char *orig, char *target, char *replacement);
bool replaceElement(char **ary, char *target, char *replacement);
char *createOutName(char *filename, char *path);
bool deleteTempWav(FILENAME *fn);
bool haveAppropriateTool(char *ext, TOOL **tools);
char *toLower(char *);

void threadManager(TOOL **, JOB *);
void *encode(PACKAGE2 *);
void *decode(PACKAGE2 *);
void *microManager(PACKAGE2 *);
void printMyDataShort(void *list, void *node, void *data);
void printMyData(void *, void *, void *);
void trashData(void *data);
int compareKeys ( void* dataOne, void* dataTwo);
char *getConfPath(char *file);

char *buildExtensionsRegExp(TOOL **tools);
TOOL *buildTool(char **rawStrings);
TOOL **readConf(char *txtfile, int *, JOB *);
TOOL *destroyTool(TOOL *tool);
TOOL **destroyAllTools(TOOL **toolAry);
char **parseEncDecString(char *raw, int *arySize);
char **tokenizeString(char *, char , int *);
#endif
