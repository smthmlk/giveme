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

#include "giveme.h"

TOOL **readConf(char *txtfile, int *status, JOB *job) {
	FILE *fp;
	//int len;
	int i, j, size;
	char *res;
	char *buffer;
	//char *temp = NULL;
	//char *strtoksux;
	char **rawStrings = NULL;
	TOOL **toolAry = NULL;
	
	if ( (fp = fopen(txtfile, "r")) == NULL) {
		printf(">> Error opening file \'%s.\' Exiting.\n\n", txtfile);
		*status = -1;
	}
	else {
		//rawStrings = (char **) calloc(5, sizeof(char *));
		buffer = (char *) calloc(MAX, sizeof(char));
		toolAry = (TOOL **) calloc(NUMTOOLS+1, sizeof(TOOL *));

		if(job->verbose) { printf("\nParsing \'%s\' for tool defintions...\n\n", txtfile); }

		for( j=0; (res = fgets(buffer, MAX, fp)) != NULL; j++ ) {

			size=-1;
			rawStrings = NULL;
			rawStrings = tokenizeString(buffer, ',', &size);	

			if(rawStrings == NULL)
				printf("readConf(): rawStrings is null. segfault!  size=%d, buffer='%s'\n",size,buffer);
			else if(job->verbose) {
				printf("readConf(): rawStrings is not null, whew. size=%d, buffer='%s'\n",size,buffer);
				printf("            contents:\n");
				//for(i=0; i < size; printf("             > [%d] \"%s\"\n", i, rawStrings[i++]));
			}

			/*strtoksux = buffer;
			for(i=0; (temp = strtok(strtoksux, ",\n")) != NULL && i < 5; i++, strtoksux=NULL) {
				len = strlen(temp) + 1;
				rawStrings[i] = (char *) calloc(len, sizeof(char));
				strncpy(rawStrings[i], temp, len);
			}*/

			toolAry[j] = buildTool(rawStrings);
		
			free(rawStrings[2]);
			free(rawStrings[4]);
			free(rawStrings);
		}


		if(fclose(fp) == EOF) {
       		printf(">> Error closing file \'%s.\' Exiting.\n\n", txtfile);
	   		exit(111);
		}


		// debugging info.
		for(i=0; job->verbose && toolAry[i] != NULL; i++) {
			printf("  toolAry[%d]: \n\tname=%s\n", i, toolAry[i]->name);
			printf("\t  encPath=%s\n", toolAry[i]->encPath);
			printf("\t  decPath=%s\n", toolAry[i]->decPath);
			printf("\t  encAry(%d): ", toolAry[i]->encArySize);
		
			for(j=0; toolAry[i]->encAry[j] != NULL; j++)
				printf("[%d]=\"%s\" ", j, toolAry[i]->encAry[j]);	

			printf("\n\t  decAry(%d): ", toolAry[i]->decArySize);
			for(j=0; toolAry[i]->decAry[j] != NULL; j++)
				printf("[%d]=\"%s\" ", j, toolAry[i]->decAry[j]);

			printf("\n\n");
		} 

		//free(rawStrings);
		free(buffer);
	}

	// before we spit this out, update JOB with regular expression string & compiled regexp.
	job->extensions_regexpStr = buildExtensionsRegExp(toolAry);
	job->extensions_regexp = (regex_t *) calloc(1, sizeof(regex_t));
	regcomp(job->extensions_regexp, job->extensions_regexpStr, REG_EXTENDED|REG_ICASE);

	return toolAry;
}


char *buildExtensionsRegExp(TOOL **tools) {
	char *regexpStr;	
	int i, total;

	// traverse list, count number of chars needed.
	for(i=0, total=0; tools[i] != NULL; total += strlen(tools[i]->name) + 4, i++);
	//printf("buildExtensionRegExp(): string will be %d chars long.\n", total);
	
	regexpStr = (char *) calloc(total+1,sizeof(char));


	// traverse list again, copying 'name' field from TOOL and a | to our new string.
	for(i=0; tools[i] != NULL; i++) {
		strcat(regexpStr, "\\.");
		strncat(regexpStr, tools[i]->name,10);
		strcat(regexpStr, "$");
		if(tools[i+1] != NULL)
			strcat(regexpStr, "|");
	}
	
	//printf("buildExtensionRegExp(): regexpStr=\"%s\", %d chars long.\n", regexpStr, (int) strlen(regexpStr));
	return regexpStr;
}


TOOL *buildTool(char **rawStrings) {
	TOOL *newTool;

	newTool = (TOOL *) calloc(1, sizeof(TOOL));

	newTool->name = rawStrings[0];
	newTool->encPath = rawStrings[1];
	newTool->decPath = rawStrings[3];
	newTool->encAry = tokenizeString(rawStrings[2], ' ', &newTool->encArySize);
	newTool->decAry = tokenizeString(rawStrings[4], ' ', &newTool->decArySize);

	return newTool;
}



TOOL **destroyAllTools(TOOL **toolAry) {
	int i;

	if(toolAry) {
		for(i=0; toolAry[i] != NULL; i++)
			toolAry[i] = destroyTool(toolAry[i]);

		free(toolAry);
	}
	
	return NULL;
}



TOOL *destroyTool(TOOL *tool) {
	int i;

	free(tool->name);
	free(tool->encPath);
	free(tool->decPath);

	for(i=0; tool->encAry[i] != NULL; i++)
		free(tool->encAry[i]); 

	for(i=0; tool->decAry[i] != NULL; i++)
		free(tool->decAry[i]); 

	free(tool->encAry);
	free(tool->decAry);
	free(tool);
	
	return NULL;
}

/*
char **parseEncDecString(char *raw, int *DarySize) {
	int  j, arySize;
	char *temp;
	char *temp2;
	char **ary;
	char **tmp;


	arySize = 3;
	ary = (char **) calloc(arySize, sizeof(char *)); 
	temp2 = raw;

	for(j=1; (temp = strtok(temp2, " ")) != NULL; j++,temp2=NULL) {	
	//for(j=1; (temp = strtok(temp2, " ")) != NULL; j++,temp2=NULL) {

		if(j == arySize) { // we need to reallocate mem for this array
			arySize++;
			tmp = (char **) realloc(ary, arySize * sizeof(char *)); // lolz
			tmp[j] = NULL; // i wish realloc would initialize shit :(
			ary = tmp;
		}

		ary[j-1] = (char *) calloc(strlen(temp)+1, sizeof(char));
		strcpy(ary[j-1], temp);
	}

	*DarySize = j-1;
	return ary;
} */




char **tokenizeString(char *orig, char delim, int *size) {
	char **ary;
	char *buffer;
	int i, j, k, count, numTokens;
	bool newSpace = true;
	
	numTokens = 0;
	
	// first, count number of tokens needed, separated by spaces.
	for(i=0; orig[i] != '\0' && orig[i] != '\n'; i++) {
		//printf("orig[%d]=%c", i, orig[i]);
		if(orig[i] == delim) {
			if(newSpace) {
				numTokens++;
				newSpace = false;
			}
			//printf(" <a space!>\n");
		}
		else {
			newSpace = true;
			//printf("\n");	
		}
	}	
	
	numTokens++;
	//printf("number tokens: %d\n", numTokens);
	
	// if we were given an int to fill, fill it.
	if(size)
		*size = numTokens;
	
	// create an ary of the right size.
	ary = (char **) calloc(numTokens +1, sizeof(char *));
		
	// create a buffer
	buffer = (char *) calloc(100, sizeof(char));
	
	// now, traverse the string again and pull out tokens & allocate
	newSpace = true;
	count = 0;
	for(i=0, j=0; i <= strlen(orig) ; i++) {
		if( orig[i] != delim && orig[i] != '\n' && orig[i] != '\0') {
			buffer[j] = orig[i];
			newSpace=true;	
			j++;
		}
 		// we have the end of a token. j is the length of the token.
		else {
			if(newSpace) {
				newSpace = false;
				//printf("allocating new token in ary, i=%d, j=%d\n", i, j);
				buffer[j] = '\0';
				ary[count] = (char *) calloc(j+1, sizeof(char));
				strcpy(ary[count], buffer);
			
				for(k=0; k <= j+1; k++)
					buffer[k]='*';
			

				//printf("ary[%d] has token: '%s'\n", count, ary[count]);
				j=0;
				count++;
			}
		}
	}
	
	
	free(buffer);
	return ary;	
}
