//  CITS2002 Project 2 2020
//  Names: Darby Edwards, Samuel Kent
//  Student Numbers: 2271 3383, 2270 4037

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>

//DELCARE GLOBAL PREPROCESSOR CONSTANTS
#define MAX_FILE_SIZE 100000
#define MAX_PATH_LEN 1024

#define progname "mergetars"


//DECLARE GLOBAL FUNCTIONS
extern void unpackTar(char *, char *);
extern void checkArgs(int);
extern int getDirectories(char *, char *, char *);
extern char *removeExtension(char *);
extern void createList();
extern void createFinalTar(char *, char *);
extern int copyFile(char *, char *);
extern time_t getModTime(char *);
extern int getSize(char *);

//DELCARE GLOBAL VARIABLES
extern int nfiles;
extern char** files;

extern int nrelative;
extern char** relative;

extern int nfinal;
extern char** final;

extern int ntempDirs;
extern char ** tempDirs;

extern int level[];
extern int nlevel;
