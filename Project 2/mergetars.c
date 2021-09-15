//  CITS2002 Project 2 2020
//  Names: Darby Edwards, Samuel Kent
//  Student Numbers: 2271 3383, 2270 4037

#define _POSIX_C_SOURCE 200809L

#include "mergetars.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>


//MAIN FUNCTION

int main(int argc, char *argv[])
{

  //Make sure there are no errors in arguments
  checkArgs(argc);



  //-------------------FINAL TEMP DIRECTORY------------------
  //Create temporary directory for the final output
  char finaltemp[] = "/tmp/tmpdir.XXXXXX";
  char *finalTempName = mkdtemp(finaltemp);

  //Check temporary directory creation was successful
  if(finalTempName == NULL){
    perror("mkdtemp failed: ");
    exit(EXIT_FAILURE);

  }

  //Add temporary directory to list
  tempDirs = realloc(tempDirs, ((ntempDirs+1) * sizeof(tempDirs[0])));
  tempDirs[ntempDirs] = strdup(finalTempName);
  ntempDirs++;
  //--------------------------------------------------------



  //-----------------UNPACK TAR---------------------------
  //Unpack all tar files into temporary directory
  for(int i = 1; i < argc-1; i++){

    //Make a temporary directory for storing the tar file
    char template[] = "/tmp/tmpdir.XXXXXX";
    char *tempDirName = mkdtemp(template);

    if(tempDirName == NULL){
      perror("mkdtemp failed: ");
      exit(EXIT_FAILURE);

    }

    tempDirs = realloc(tempDirs, ((ntempDirs+1) * sizeof(tempDirs[0])));
    tempDirs[ntempDirs] = strdup(tempDirName);
    ntempDirs++;

    char *filename = argv[i];

    printf("unpacking file %s \n", filename);

    unpackTar(filename, tempDirName);

    if(getDirectories(tempDirName, "", finalTempName) == -1){
      perror("Error getting directory: ");
      exit(EXIT_FAILURE);
    }
  }

  //--------------------------------------------------------


  //----------------------CREATE OUTPUT---------------------
  //First create list of file paths
  createList();

  //Create final output tar file
  createFinalTar(argv[argc-1], finalTempName);

  //Free memory used for arrays
  free(files);
  free(final);
  free(relative);
  //--------------------------------------------------------

  //------------DELETE TEMP DIRECTORIES---------------------

  //Iterate through all temp directories and traverse them
  for(int i = 0; i < ntempDirs; i++){
    DIR *dir = opendir(tempDirs[i]);
    struct dirent *n;
    char path[MAX_PATH_LEN];

    //Delete content from temp directories
    while((n = readdir(dir)) != NULL)
    {
        sprintf(path, "%s/%s", tempDirs[i], n->d_name);
        remove(path);
    }
    closedir(dir);

    //Finally delete temporary directories themselves
    if(rmdir(tempDirs[i]) == -1){
        perror("rmdir failed: ");
        exit(EXIT_FAILURE);
    }
  }

  //Free the memory used to store their names
  free(tempDirs);

  //--------------------------------------------------------


  return 0;
}
