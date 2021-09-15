//  CITS2002 Project 2 2020
//  Names: Darby Edwards, Samuel Kent
//  Student Numbers: 2271 3383, 2270 4037

//Define all global variables and define variable sizes

#include "mergetars.h"


//Stores full filepaths for all files
char **files = NULL;
int nfiles = 0;

//Stores relative filepaths for all files
char **relative = NULL;
int nrelative = 0;

//Stores full filepaths for the final files
char **final = NULL;
int nfinal = 0;


//Stores names of all temporary directories used
char **tempDirs = NULL;
int ntempDirs = 0;

//Stores the integer for the index of the relative path for each full path (files)
int level[1000];
int nlevel = 0;
