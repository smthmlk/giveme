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
#include "getopt.h"

JOB *createJob(int argc, char **argv) {
	JOB *j1;
	int i;

	j1 = (JOB *) calloc(1, sizeof(JOB));
	j1 = parseOptions(j1, argc, argv);

	// fixing path	
	if(j1->outPath == NULL) 
		j1->outPath = "./";
	char *apath = (char *) calloc(strlen(j1->outPath)+1, sizeof(char));
	strcpy(apath, j1->outPath);
	j1->outPath = apath;
	j1->outPath = fixPath(j1->outPath);
	// done.

	// setting default number of threads.
	if(! j1->numThreads > 0)
		j1->numThreads = DEFAULTTHREADCOUNT;


	// check the output dir for existance & writeability..
	if( ! checkOutPath(j1)) {
		printf(">> The output path specified could not be created/written to. Check permissions.	\n");
		j1 = destroyJob(j1);
		exit(2);
	}

	// they have to give a format.
	if(! j1->destFormat) {
		printf("you must at least specify a desired format (-f | --format).\n");
		j1 = destroyJob(j1);
		exit(1);
	}



	// lets see what we have:
	if(j1->verbose) {
		printf("Job:\n\t> format: %s\n\t> outPath: %s\n\t> numThreads: %d\n\t> singleFileName: ", j1->destFormat, j1->outPath, j1->numThreads);
		if(j1->singleFileName)
			printf("%s\n", j1->singleFileName);
		else 
			printf("(null)\n");
	
		printf("\t> customEncSettings: ");
		if(j1->customEncSettings) {
			for(i=0; j1->customEncSettings[i] != NULL; i++)
				printf("%s ", j1->customEncSettings[i]);
			printf("\n\t> customEncSettingSize: %d\n", j1->customEncSettingsSize);
		}
		else
			printf("(null)\n");

		printf("\t> verbose: ");
		if(j1->verbose)
			printf("yes\n\n");
		else
			printf("no\n\n");
	}


	return j1;
}


JOB *parseOptions(JOB *job, int argc, char **argv) {
	int c;
    //int digit_optind = 0;


    while (1) {
       // int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
            {"encsettings", 1, 0, 'e'},
            {"format", 1, 0, 'f'},
            {"help", 0, 0, 'h'},
            {"infile", 1, 0, 'i'},
            {"outdir", 1, 0, 'o'},
            {"threads", 1, 0, 't'},
            {"version", 0, 0, 'v'},
            {"verbose", 0, 0, 'V'},
            {0, 0, 0, 0}
        };
        c = getopt_long (argc, argv, "e:f:hi:o:t:vV",
                 long_options, &option_index);
        if (c == -1)
            break;
        switch (c) {
        case 0:
            printf ("option %s", long_options[option_index].name);
            if (optarg)
                printf (" with arg %s", optarg);
            printf ("\n");
            break;
        case 'e':
			job->customEncSettings = tokenizeString(optarg, ' ', &job->customEncSettingsSize);
            break;
        case 'f':
			job->destFormat = optarg;
            break;
        case 'h':
			job = destroyJob(job);
			printHelp();
			exit(0);
            break;
        case 'i':
			job->singleFileName = optarg;
            break;
        case 'o':
			job->outPath = optarg;
            break;
        case 't':
			job->numThreads = atoi(optarg);
            break;
        case 'v':
			job = destroyJob(job);
			printVersion();
			exit(0);
            break;
		case 'V':
			job->verbose = true;
			break;
        case '?':
            break;
        default:
            printf ("?? getopt returned character code 0%o ??\n", c);
        }
    }
    if (optind < argc) {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        printf ("\n");
    }

	return job;
}



JOB *destroyJob(JOB *job) {
	int i;

	if(job->outPath)
		free(job->outPath);

	if(job->customEncSettings) {
		for(i=0; job->customEncSettings[i]; i++)
			free(job->customEncSettings[i]);
		free(job->customEncSettings);
	}

	if(job->extensions_regexp) {
		regfree(job->extensions_regexp);
		free(job->extensions_regexp);
	}

	if(job->extensions_regexpStr)
		free(job->extensions_regexpStr);


	free(job);
	
	return NULL;
}


char *fixPath(char *path) {
	char *newPath = path;
	int pathLen = strlen(path);

	if(path[pathLen-1] != '/') { 
	//	printf("need to add a /! (char @ %d is \'%c\' out of \"%s\")\n", pathLen, path[pathLen-1], path);
		newPath = (char *) calloc(pathLen+2, sizeof(char));
		strcpy(newPath, path);
		newPath = strcat(newPath, "/");
		free(path);
	}
//	else
	//	printf("its ok babe\n");

	return newPath;
}


bool checkOutPath(JOB *job) {
	// dir doesnt exist, need to create.
	if( (access(job->outPath, F_OK)) == 0) {
		if(job->verbose) { printf("'%s' exists.\n", job->outPath); }
	}
	else {
		if(job->verbose) { printf("'%s' doesnt exist, creating.. ", job->outPath); }
		if( (mkdir(job->outPath, 0775)) == 0) {
			if(job->verbose) { printf("ok.\n"); }
		}
		else {
			if(job->verbose) { printf("failed.\n"); }
			return false;
		} 
	}

	if( (access(job->outPath, W_OK)) == 0) {
		if(job->verbose) { printf("'%s' is writable.\n", job->outPath); }
	}
	else {
		if(job->verbose) printf("'%s' is not writable.\n", job->outPath);
		return false;
	}
	
	printf("\n\n");
	return true;
}

void printHelp(void) {
	printVersion();

	printf("\n\nHelp for giveme 3.0.x\n=====================\n\n");
	printf(" Description:  giveme converts files of various audio\n");
 	printf("               formats to the format of your inclination.\n");
 	printf("                  By default, it will convert all audio files\n");
 	printf("               in the CWD to the desired format, though it\n");
 	printf("               can also convert individual files specified\n");
 	printf("               via argument.\n");
 	printf("                  It uses a configuration file to determine\n");
 	printf("               the encoder & decoder settings and paths,\n");
 	printf("               allowing each user on a system to have their\n");
 	printf("               own prefernce of encoder settings & versions.\n");
 	printf("                  giveme is multithreaded and can use any\n");
 	printf("               number of cores to convert files concurrently.\n");
 	printf("               The number of cores it uses by default is 2,\n");
 	printf("               but it can instructed to use more via argument.\n");
 	printf("               \n");
 	printf(" Usage:        giveme -f FORMAT [options]\n");
  	printf("               (Be sure there are audio files in your current\n");
  	printf("               directory before running the program.)\n"); 	
 	printf("               \n\n");
 	printf(" Options:      \n");
  	printf("    -e, --encsettings \"YOURSETTINGS\"\n");
 	printf("               Using this option allows you to override the\n");
 	printf("               encoder settings set in your ~/.giveme3.conf\n");
	printf("               file for this particular conversion(s). It does\n");
	printf("               not alter your giveme3.conf file. Be sure to wrap\n");
 	printf("               your encoder settings in double quotes when using\n");
 	printf("               this option, and be sure to use the proper format:\n");
 	printf("                   Examples:\n");
 	printf("               giveme -f mp3 -e \"lame -V5 INFILE OUTFILE\"\n");
 	printf("               giveme -f flac -e \"flac -4 -o OUTFILE INFILE\"\n");
 	printf("               * Note: OUTFILE and INFILE are literal: do not\n");
 	printf("                       specify an actual filename; just type\n");
 	printf("                       OUTFILE and INFILE as is shown above.\n\n");
 	printf("    -f, --format FORMAT\n");
 	printf("               The extension of the format you want to\n");
 	printf("               convert your audio file(s) to. Supported\n");
 	printf("               formats (and the names you should use w/ this\n");
 	printf("               option) are:\n");
 	printf("                  mp3, flac, wav, aac, ogg, ape, wv\n");
 	printf("               This option and an argument are required.\n");
 	printf("               \n");
 	printf("    -h, --help\n");
 	printf("               Display this help screen.\n\n");
 	printf("    -i, --infile FILENAME\n");
 	printf("               Specify a single file for conversion.\n\n");
 	printf("    -o, --outdir PATH\n");
 	printf("               Specify an output directory to place the\n");
 	printf("               newly converted files in. If this directory\n");
 	printf("               does not exist, it will be created. This\n");
 	printf("               option works for single files as well.\n\n");
 	printf("    -t, --threads NUMBER\n");
 	printf("               Set the number of concurrent threads to use.\n");
 	printf("               This is essentially the number of cores your\n");
 	printf("               computer has, or the number of cores you want\n");
 	printf("               to dedicate to converting audio at once.\n");
 	printf("               The default is 2, but any value greater than 0\n");
 	printf("               will work.\n");
 	printf("               \n");
 	printf(" Examples: \n");
 	printf("      -- Very basic: converting all the files in the CWD to\n"); 	
 	printf("         mp3's using the standard settings in ~/.giveme3.conf:\n");
 	printf("            $ giveme -f mp3   \n\n");
 	printf("      -- Converting a single file to wav: \n");
 	printf("            $ giveme -f wav -i \"01. Van Halen - Jump.mp3\"\n\n");
 	printf("      -- Converting all files in the CWD to ogg and outputting\n");
 	printf("         them to new directory somewhere else, and using 4\n");
  	printf("         threads to do it:\n");
 	printf("            $ giveme -t 4 -f ogg -o /var/tmp/MyOggz\n\n");
 	printf("      -- Using custom encoder settings to convert files and\n");
 	printf("         output them to /var/tmp:\n");
 	printf("    $ giveme -f mp3 -e \"lame -b 192 INFILE OUTFILE\" -o /var/tmp\n\n");
 	printf(" Author:       Trevor T., smthmlk@dimensionalstorm.net\n");
 	printf("               2004-2007\n\n");   
 	printf(" License:      GPLv2. There should be a copy of the license in\n");
 	printf("               the source directory.\n\n");

	return;
}

void printVersion(void) {
 	printf("giveme %s\n", VERSIONINFO);
 	printf("Author:  Trevor T., smthmlk@dimensionalstorm.net\n");
 	printf("License: GPLv2, see source dir for copy of license.\n"); 
}
