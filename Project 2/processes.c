//  CITS2002 Project 2 2020
//  Names: Darby Edwards, Samuel Kent
//  Student Numbers: 2271 3383, 2270 4037

#define _POSIX_C_SOURCE 200809L

#include "mergetars.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <string.h>

#define TAR_LOCATION "/bin/tar"

//functions for performing processes here

//---------------------FUNCTION FOR CHECKING ARGUEMENTS-------------------------
// Function makes sure there are sufficient number of arguments to run

void checkArgs(int argc){
  if(argc < 3){
    printf("Not enough arguments!");
    exit(EXIT_FAILURE);
  }
}

//------------------------------------------------------------------------------

//---------------------FUNCTION FOR CHECKING ARGUEMENTS-------------------------
// Function removes the extension from the file name
char *removeExtension(char *filename){
    char *newStr;
    char *lastExt;

    if ((newStr = malloc(strlen(filename) + 1)) == NULL){
      return NULL;
      perror("Error removing file extension from name: ");
      exit(EXIT_FAILURE);
    }

    strcpy(newStr, filename);
    lastExt = strrchr(newStr, '.');

    if (lastExt != NULL){
        *lastExt = '\0';
    }


    return newStr;
}

//------------------------------------------------------------------------------


//-----------------------FUNCTION FOR UNPACKING TAR FILE------------------------
// Function unpacks tar file to the currect directory under name tempTar

void unpackTar(char *filename, char *tempDir){

  char *params[] = {TAR_LOCATION, "-C", tempDir, "-xvf", filename, NULL};
  char *paramsComp[] = {TAR_LOCATION, "-C", tempDir, "-xzvf", filename, NULL};


  //Create child process
  switch(fork()){

    //Fork failed
    case -1:
      perror("Fork failed: ");
      exit(EXIT_FAILURE);
      break;

    //Fork was successful
    case 0:

      //Unpack tar file to temporary location
      if(execv(TAR_LOCATION, params) == -1){
        //unpacking was not successfull
        printf("Failed to unpack tar file, trying to decompress first...\n");

        //Try to decompress the file and try again
        if(execv(TAR_LOCATION, paramsComp) == -1){
          //File was not able to be unpacked
          perror("Error unpacking tar: ");
          exit(EXIT_FAILURE);
        }
      }

      exit(EXIT_SUCCESS);

      break;


    default: {

      int child, status;

      //Wait for child
      child = wait(&status);

      //Exit child process
      break;
      exit(EXIT_SUCCESS);

    }
  }

}

//------------------------------------------------------------------------------



//------------------------CREATE FINAL LIST OF PATHS----------------------------
// Function searches through files for duplicates, then picks the correct file
// to keep and inserts its path into the final path list.
// If there are no duplicates then that file path goes straight into the list.

void createList(){

  //Iterate through file list
  for(int i = 0; i < nrelative; i++){
    bool found = false;

    //Iterate through file list again to compare
    for(int x = 0; x < nrelative; x++){
      if(strcmp(relative[i], relative[x]) == 0 && x != i){
        //Found duplicate file
        found = true;

        if(getModTime(files[x]) > getModTime(files[i])){
          //Add most recent to final file list

          final = realloc(final, ((nfinal+1) * sizeof(final[0])));
          final[nfinal] = strdup(files[x]);
          nfinal++;

          //Add index to relative filename
          level[nfinal] = x;
        }

        else if(getModTime(files[x]) == getModTime(files[i])){
          if(getSize(files[x]) > getSize(files[i])){
            //Add largest file to final file list

            final = realloc(final, ((nfinal+1) * sizeof(final[0])));
            final[nfinal] = strdup(files[x]);
            nfinal++;

            //Add index to relative filename
            level[nfinal] = x;


          }else if (x > i){
            //Add file from last file input to final list

            final = realloc(final, ((nfinal+1) * sizeof(final[0])));
            final[nfinal] = strdup(files[x]);
            nfinal++;

            //Add index to relative filename
            level[nfinal] = x;
          }
        }

      }
    }

    //No duplicates found, add file to final list
    if(!found){

      final = realloc(final, ((nfinal+1) * sizeof(final[0])));
      final[nfinal] = strdup(files[i]);
      nfinal++;

      //Add index to relative filename
      level[nfinal] = i;

    }
  }
}




//------------------------------------------------------------------------------


//--------------------------CREATE FINAL TAR FILE-------------------------------
// Function creates the final tar file to be outputted.
// Function uses the final file paths list and copies those files into the new
// output directory, then compresses this directory into tar file.

void createFinalTar(char *name, char *tempDirName){

    //Copy files to the temporary directory
    for(int i = 1; i < nfinal; i++){

      char newDirFileName[MAX_PATH_LEN];

      sprintf(newDirFileName, "%s%s", tempDirName, relative[level[i+1]]);

      if(copyFile(newDirFileName, final[i]) != 0){
        perror("Copying file failed: ");
        exit(EXIT_FAILURE);
      }

    }

    //Get current directory
    char current[MAX_PATH_LEN];

    if(getcwd(current, sizeof(current)) != NULL){
      //Successful
    }else{
      //Unsuccessful
      perror("Error getting current directory: ");
      exit(EXIT_FAILURE);
    }

    char finalName[MAX_PATH_LEN];

    sprintf(finalName, "%s/%s", current, name);

    //Create output tar file
    int pid = fork();

    char *params[] = {TAR_LOCATION, "-cf", finalName, tempDirName, NULL};

    //Create child process to call tar
    if (pid == 0) {
        //Successful
        if(execv(TAR_LOCATION, params) == -1){
          perror("Error creating output tar: \n");
          exit(EXIT_FAILURE);
        }
    }
    else if (pid < 0) {
        //Unsuccessful
        perror("Error creating child process: ");
        exit(EXIT_FAILURE);
    }else{
      int child, status;

      //Wait for child
      child = wait(&status);

      //Exit child process
      exit(EXIT_SUCCESS);
    }

}


//------------------------------------------------------------------------------
