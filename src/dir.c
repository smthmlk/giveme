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
#define _GNU_SOURCE
#include "giveme.h"

LL_HEADER *ls(char *pathname, TOOL **tools, JOB *job) {
	DIR *dp;
	struct dirent *de;
	bool done = false;
	
	LL_HEADER *list = (LL_HEADER *) calloc(1,sizeof(LL_HEADER));
	list->printData = printMyData;
	list->freeInternals = trashData;
	list->compare = compareKeys;

	if(job->singleFileName) {
		FILENAME *fn = buildFilename(job->singleFileName, tools, job);
		if(fn->ext != NULL && fn->processMe)
			LL_insert(list, (void *) fn);
		else
			trashData((void *) fn);
	}
	else {

		dp = opendir(pathname);

		while(!done) {
			de = readdir(dp);

			if(de == NULL)
				done = true;
			else {
				FILENAME *fn = buildFilename(de->d_name, tools, job);
				if(fn->ext != NULL && fn->processMe)
					LL_insert(list, (void *) fn);
				else
					trashData((void *) fn);
			}
		}

		free(dp);
	}

	return list;
}



FILENAME *buildFilename(char *n, TOOL **tools, JOB *job) {
	//regex_t *x = (regex_t *) calloc(1, sizeof(regex_t));
	int len;
	regmatch_t r[1];
	size_t nmatch = 1;
	FILENAME *fn = (FILENAME *) calloc(1, sizeof(FILENAME));

	fn->decA = NULL;
	fn->encA = NULL;
	
	len = strlen(n);

	fn->name = (char *) calloc(len+1, sizeof(char));
	strncpy(fn->name, n, len);
	

	// lolz, i cant get enough of this hack. there she blows again!
	//int res = regcomp(x, "\\.[tmMoOfFaAw][t4pPgGlLaAvm][p23gGaAcCeEv]*$", 0);
	//printf("job->extensions_regexp=%s\n", job->extensions_regexp);
//	int res = regcomp(x, job->extensions_regexpStr, REG_EXTENDED);
	int res = regexec(job->extensions_regexp, n, nmatch, r, 0);

	if(res == 0) {
		if(job->verbose) { printf("\"%s\" matches! so=%d.\n", fn->name, r[0].rm_so); }

		// both of the following are done by toLower() now [allocation & copying]
		//fn->ext = (char *) calloc(6, sizeof(char)); 
		//strcpy(fn->ext, fn->name + r[0].rm_so +1);
		fn->ext = toLower(fn->name +r[0].rm_so + 1);
		
		if(haveAppropriateTool(fn->ext, tools) && strncmp(fn->ext,job->destFormat,6) != 0) {
			// append TMPDIR string to the beginning of the wavName
			char *wavNameTmp = (char *) calloc(strnlen(fn->name, MAX)+strnlen(TMPDIR, MAX)+3, sizeof(char));
			strcpy(wavNameTmp, TMPDIR);
			strcat(wavNameTmp, fn->name);
			fn->wavName = ghettoSubstitute(wavNameTmp, fn->ext, "wav");
			free(wavNameTmp);
			
			fn->outName = ghettoSubstitute(fn->name, fn->ext, job->destFormat);	

			fn->decA = determineCoder(fn->ext, tools, 1, &fn->decExe, job);
			fn->encA = determineCoder(job->destFormat, tools, 0, &fn->encExe, job);

			if(strcmp(fn->name, fn->outName)==0) {
				fn->processMe=false;
				if(job->verbose) { printf("buildFilename(): setting processMe flag to false for \"%s\".\n", fn->name);}
			}
			else
				fn->processMe=true;

			// concat the destination dir & outName.
			fn->outName = createOutName(fn->outName, job->outPath);	

			replaceElement(fn->decA, "INFILE", fn->name);
			replaceElement(fn->decA, "OUTFILE", fn->wavName);
			replaceElement(fn->encA, "INFILE", fn->wavName);
			replaceElement(fn->encA, "OUTFILE", fn->outName);

			// if the source file is a wav, dont delete it. Also, if the desired format is wav, dont delete it.
			if(strcmp(fn->ext, "wav")==0 || strcmp(job->destFormat, "wav")==0)
				fn->deleteWav = false;
			else
				fn->deleteWav = true;
		}
		else if(job->verbose) {
			printf("no appropriate tool found to decode %s files or transcoding to same file format scenario found.\n", fn->ext); }
	}
	else if(job->verbose) {
		printf("res=%d, file \"%s\" doesnt match.\n", res, fn->name); }


	//regfree(x);
	//free(x);
	return fn;
}

bool haveAppropriateTool(char *ext, TOOL **tools) {
	int i;
	bool found = false;
	
	for(i=0; !found && tools[i] != NULL; i++)
		if(strcasecmp(tools[i]->name, ext) == 0)
			found = true;

	return found;
}



char *createOutName(char *filename, char *path) {
	char *newStr;
	int fnLen, pathLen;

	fnLen = strlen(filename);
	pathLen = strlen(path);

	newStr = (char *) calloc(fnLen+pathLen+1, sizeof(char));
	strcpy(newStr, path);
	newStr = strcat(newStr, filename);
	
	free(filename);
	return newStr;
}


bool replaceElement(char **ary, char *target, char *replacement) {
	int i;
	bool res = false;

	for(i=0; ary[i] != NULL; i++) {
		if(strcmp(ary[i],target) == 0) {
			free(ary[i]);
			ary[i] = (char *) calloc(strlen(replacement)+1, sizeof(char));
			strcpy(ary[i], replacement);
			res = true;
		}		
	}
	return res;
}


// type: 0=enc, 1=dec
char **determineCoder(char *ext, TOOL **tools, int type, char **exe, JOB *job) {
	int i, j, srcArySize;
	char **destAry = NULL;
	char **srcAry = NULL;
	bool found = false;


	//traverse tools to find a matching ext. return null if no match.
	for(i=0; ! found && tools[i] != NULL ; i++) {
		//printf("comparing tools[i]->name=\'%s\' with ext=\'%s\'.\n", tools[i]->name, ext);

		if(strcmp(tools[i]->name, ext) == 0) {
			found = true;
			
			// ENC
			if(type == 0) {
				*exe = tools[i]->encPath;
				if(job->customEncSettings && job->customEncSettingsSize > 0) {
					srcAry = job->customEncSettings;
					srcArySize = job->customEncSettingsSize;
				}
				else {
					srcAry = tools[i]->encAry;
					srcArySize = tools[i]->encArySize;
				}
			}
			// DEC
			else {
				*exe = tools[i]->decPath;
				srcArySize = tools[i]->decArySize;				
				srcAry = tools[i]->decAry;
			}

			destAry = (char **) calloc(srcArySize + 2, sizeof(char *)); 
			for(j=0; j < srcArySize ; j++) {
				destAry[j] = (char *) calloc(strlen(srcAry[j])+1, sizeof(char)); // sigh.
				strcpy(destAry[j],srcAry[j]); // mega-sigh
			//	printf("copying \"%s\" from srcAry.\n", destAry[j]);
			}
			destAry[j+1] = (char *) 0;
		//	printf("\n\n");

		}
	}
	//printf("destAry=%p\n", destAry);
	return destAry;
}



char *ghettoSubstitute(char *orig, char *target, char *replacement) {
	char *newStr;
	int n, newLen, origLen, targetLen, replacementLen;
	
	origLen = strlen(orig);
	targetLen = strlen(target);
	replacementLen = strlen(replacement);
	newLen = origLen - targetLen + replacementLen;
	n=origLen - targetLen;

	newStr = (char *) calloc(newLen + 1, sizeof(char));

	strncpy(newStr, orig, n);
	strcat(newStr, replacement);

	return newStr;
}


bool deleteTempWav(FILENAME *fn) {
	int res;

	if( (res = unlink(fn->wavName)) == 0)
		return true;	

	return false;	
}



char *toLower(char *str) {
	int i, len;
	char *lowered;

	len = strlen(str);
	lowered = (char *) calloc(len+2, sizeof(char));
	
	for(i=0; i < len; i++) { lowered[i] = tolower(str[i]); }
	lowered[i+1] = '\0';

	return lowered;
}

/* ************************************************/
/* ************* LINKED LIST STUFF ****************/
/* ************************************************/

// module for 'traverseList()'
void printMyDataShort(void *list, void *node, void *data) {
	FILENAME *x = (FILENAME *) data;

	printf("%s -> %s\n", x->name, x->outName);

	return;
}


// module for 'traverseList()'
void printMyData(void *list, void *node, void *data) {
	FILENAME *x = (FILENAME *) data;
	int i;

	printf("\t--   name=\'%s\'", x->name);
	printf("\n\t     shortName='%s'", x->shortName);
	printf("\n\t     deleteWav=%d", x->deleteWav);
	if(x->ext)
		printf("\n\t     ext=%s", x->ext);
	if(x->wavName)
		printf("\n\t     wavName=%s", x->wavName);
	if(x->outName)
		printf("\n\t     outName=%s", x->outName);
	if(x->decExe)
		printf("\n\t     decExe=%s", x->decExe);
	if(x->decA) {
		printf("\n\t     decA:");
		for(i=0; x->decA[i] != NULL; i++)
			printf(" %s", x->decA[i]);
	}
	if(x->encExe)
		printf("\n\t     encExe=%s", x->encExe);
	if(x->encA) {
		printf("\n\t     encA:");
		for(i=0; x->encA[i] != NULL; i++)
			printf(" %s", x->encA[i]);
	}
	printf("\n\n");
	return;
}


void trashData(void *data) {
	int i;
	FILENAME *x = (FILENAME *) data;
	
	//printMyData(NULL, NULL, data);

	free(x->name);	

	if(x->ext)
		free(x->ext);
	if(x->wavName)
		free(x->wavName);
	if(x->outName)
		free(x->outName);
	if(x->decA) {
		for(i=0; x->decA[i] != NULL; i++)
			free(x->decA[i]);
		free(x->decA);
	}
	if(x->encA) {
		for(i=0; x->encA[i] != NULL; i++)
			free(x->encA[i]);
		free(x->encA);
	}
	
	if(x->shortName)
		free(x->shortName);

	free(x);
	return;
}


// -1 -- arg1 < arg2 
//  0 -- exact match
//  1 -- arg1 > arg2
int compareKeys ( void* dataOne, void* dataTwo)
{
	int res;

    FILENAME *data1;
    FILENAME *data2;

	data1 = (FILENAME *) dataOne;
    data2 = (FILENAME *) dataTwo;


	res = strcmp(data1->name, data2->name);
	
	return res;
} 
/* ************************************************/
/* ************************************************/
