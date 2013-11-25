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

int main(int argc, char **argv)
{
	JOB *job1;
	TOOL **tools;
	char *conffile;
	int res = 0;

	// read the arguments given, build a job.
	job1 = createJob(argc, argv);  

	// parse our available tools.
	conffile = getConfPath("/.giveme3.conf");
	tools = readConf(conffile, &res, job1); 
	free(conffile);

	if(res != 0) 
		printf(">> Are you running as root?\n");
	else {
		// do we even have an encoder for the desired format?
		if( haveAppropriateTool(job1->destFormat, tools) )
			// if so, do the job.
			threadManager(tools, job1); 
		else
			printf("\n\n>> No encoder found to create '%s' files.\n   Try another format, or install an appropriate encoder.\n\n", job1->destFormat);
	}

	// clean up
	job1 = destroyJob(job1);
	tools = destroyAllTools(tools); 
	return 0;
}



char *getConfPath(char *file) {
	char *homedir = getenv("HOME");
	char *x;

	x = calloc(strlen(homedir) + strlen(file) + 1, sizeof(char));

	strcpy(x, homedir);
	x = strcat(x, file);

	return x;
}


void threadManager(TOOL **tools, JOB *job) {
	int t_index, i, busyThreads, completedFiles, retval;
	LL_NODE *currentNode;

	// basic thread init stuff
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_t *thr;

	// basic mutex init stuff, no attr needed
	pthread_mutex_t *mymutex = (pthread_mutex_t *) calloc(1, sizeof(pthread_mutex_t));
	pthread_mutex_init(mymutex,NULL);

	// build a list of the files in the CWD.
	LL_HEADER *list = ls(".", tools, job);
	job->totalProgress = 0;
	job->progressInc = (float) 100/(3*list->count); // 2 ops per file

	// just for my own sanity
	if(job->verbose) {
		printf("\n\nlist has %d nodes....\n", list->count); 
		LL_traverseList(list, printMyData);
		printf("\n\n");
	}

	// for freeing later, use arrays for packages and threads to keep track of what we create
	PACKAGE2 **pkgAry = (PACKAGE2 **) calloc(list->count, sizeof(PACKAGE2 *));
	pthread_t **pthAry = (pthread_t **) calloc(list->count, sizeof(pthread_t *));
	

	// LOOP TIME
	completedFiles = 0;
	busyThreads = 0;
	currentNode = list->LL_head;
	t_index = 0;
	int busyThreadsCopy = 0;

	// the non-verbose output is a simple progress indicator
	if(! job->verbose) { 
		printf("giveme %s.\n", VERSIONINFO);
		printf("Progress: %3.0f%%", 0.0); fflush(stdout); }

	for(i=0; completedFiles < list->count ;) {
		pthread_mutex_lock(mymutex);
		busyThreadsCopy = busyThreads;
		pthread_mutex_unlock(mymutex);

		if( busyThreadsCopy < job->numThreads ) {
			// build package using the current item in the list
			pkgAry[i] = (PACKAGE2 *) calloc(1, sizeof(PACKAGE2));
			pkgAry[i]->tid = t_index++;
			pkgAry[i]->job = job;
			pkgAry[i]->fn  = (FILENAME *) currentNode->dataPtr;
			pkgAry[i]->bth = &busyThreads;
			pkgAry[i]->mut = mymutex;
	
			// increment busyThreads
			pthread_mutex_lock(mymutex);
			busyThreads++;
			pthread_mutex_unlock(mymutex);			

			// launch
			pthAry[i] = (pthread_t *) calloc(1, sizeof(pthread_t));
			retval = pthread_create(pthAry[i], &attr, (void *) microManager, pkgAry[i]);
			if(job->verbose) { printf(">> created thread %d|%p, running.\n", pkgAry[i]->tid,pthAry[i]); }

			// advance along the list; this shouldn't be null or we have a big logic error
			currentNode = currentNode->link;
			completedFiles++;
			i++;
		}
	}

	// clean up the assignment & thread arrays.
	for(i=0; i < list->count; i++) {
		thr = (pthread_t *) pthAry[i];
		retval = pthread_join(*thr, NULL);
		free((pthread_t *) pthAry[i]);
		free((PACKAGE2 *) pkgAry[i]);
	}
	free(pthAry);
	free(pkgAry);
	list = LL_destroyList(list);
	pthread_mutex_destroy(mymutex);
	free(mymutex);
	printf("\n");
}



void *microManager(PACKAGE2 *pkg) {
	decode(pkg);
	// for our progress indicator
	pthread_mutex_lock(pkg->mut);
	pkg->job->totalProgress += pkg->job->progressInc;
	if(! pkg->job->verbose) { printf("\b\b\b\b%3.0f%%", pkg->job->totalProgress); fflush(stdout); }
	pthread_mutex_unlock(pkg->mut);


	encode(pkg);
	// file done; decrement busyThreads counter & update progress bar
	pthread_mutex_lock(pkg->mut);
	*(pkg->bth) = *(pkg->bth) - 1;
	pkg->job->totalProgress += pkg->job->progressInc;
	if(! pkg->job->verbose) { printf("\b\b\b\b%3.0f%%", pkg->job->totalProgress); fflush(stdout); }
	pthread_mutex_unlock(pkg->mut);

	if(pkg->job->verbose) { printf("Thread %d has finished.\n", pkg->tid); }

	return NULL;
}


void *encode(PACKAGE2 *pkg) {
	pid_t cpid;
	int status;
	FILENAME *fn = (FILENAME *) pkg->fn;


	if(pkg->job->verbose) { printf("[%d] encoding started (\"%s\")\n", pkg->tid, fn->name ); }

	cpid = fork();
	if(cpid==0) {
		execv(fn->encExe, fn->encA);
	}
	else {
		waitpid(cpid, &status, 0);
		if(pkg->job->verbose) { printf("[%d] encoding done (\'%s\')\n", pkg->tid, fn->name); }
		// delete wav file
		//if(fn->deleteWav) // we always delete wav files now because we always copy to /tmp
			if(! deleteTempWav(fn))
				printf("-- [%d] couldn't delete temp wav file, '%s'.\n", pkg->tid, fn->wavName);
	}			

	if(pkg->job->verbose) printf("\n");

	pthread_mutex_lock(pkg->mut);
	pkg->job->totalProgress += pkg->job->progressInc;
	if(! pkg->job->verbose) { printf("\b\b\b\b%3.0f%%", pkg->job->totalProgress); fflush(stdout); }
	pthread_mutex_unlock(pkg->mut);

	return NULL;
}



void *decode(PACKAGE2 *pkg) {
	pid_t cpid;
	int status;
	FILENAME *fn = (FILENAME *) pkg->fn;


	if(pkg->job->verbose) { printf("[%d] decoding started (\"%s\")\n", pkg->tid, fn->name ); }

	cpid = fork();
	if(cpid==0) {
		execv(fn->decExe, fn->decA);
	}
	else {
		waitpid(cpid, &status, 0);
		if(pkg->job->verbose) { printf("[%d] decoding done (\'%s\')\n", pkg->tid, fn->name); }
	}

	if(pkg->job->verbose) printf("\n");
	return NULL;
}

