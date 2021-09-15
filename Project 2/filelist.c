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
#include <string.h>
#include <time.h>
#include <errno.h>
#include <utime.h>


//File I/O is done here

//----------------------COPYING A FILE------------------------------------------
// Function will write specified data from a source file to the specified
// destination file

int copyFile(char destination[], char source[]){

  //Open source location for read only access, check it was opened successfully
  int src = open(source, O_RDONLY);

  if(src == -1){
    perror("Error opening source: ");
    exit(EXIT_FAILURE);
  }


  //Open destination for write only access, check it was opened successfully
  int dst = open(destination, O_WRONLY | O_CREAT, 0666);

  if(dst == -1){
    close(src);
    perror("Error opening destination: ");
    exit(EXIT_FAILURE);
  }

  //Read contents of source file, declare buffer for holding contents
  char buffer[MAX_FILE_SIZE];
  size_t got;

  while((got = read(src, buffer, sizeof buffer)) > 0){
    //file is being read, buffer being populated

    //Write contents onto destination file
    if((write(dst, buffer, got)) != got){
      close(src);
      close(dst);
      return -1;
    }
  }

  //Set the modification time for the file to the old files modification time
  struct stat tempstruct;
  struct utimbuf times;

  stat(source, &tempstruct);

  //Access time remains same
  times.actime = tempstruct.st_atime;

  //Modification time updated on destination file
  times.modtime = time(NULL);
  utime(destination, &times);

  //Close access to files
  close(src);
  close(dst);

  //return zero for success
  return 0;

}



//------------------------------------------------------------------------------

//--------------------------READING A DIRECTORY---------------------------------
// Function will return a pointer to the structure holding the child directories
// of a given directory

int getDirectories(char *dirname, char *parentDir, char *finalDir){
  printf("Reading directory %s...\n", dirname);

  DIR *dirp;
  struct dirent *dp;

  //Open directory
  dirp = opendir(dirname);

  //Exit program if the directory is not found
  if(dirp == NULL){
    perror(progname);
    exit(EXIT_FAILURE);
  }

  char newPath[MAX_PATH_LEN];
  char relPath[MAX_PATH_LEN];

  while((dp = readdir(dirp)) != NULL){

    sprintf(newPath, "%s/%s", dirname, dp->d_name);

    //Get relative directory name
    sprintf(relPath, "%s/%s", parentDir, dp->d_name);

    //Analyse contents of directory
    struct stat stat_buffer;

    if(stat(newPath, &stat_buffer) != 0){
      //Error has occured
      perror(progname);
      exit(EXIT_FAILURE);
    }
    else if(S_ISDIR(stat_buffer.st_mode) && *dp->d_name != '.' && strcmp(dp->d_name, "..") != 0){
      //A directory is found

      //Get full directory path to the final temp directory
      char dirPath[MAX_PATH_LEN];
      sprintf(dirPath, "%s%s", finalDir, relPath);

      //Check if directory already exists
      DIR* dir = opendir(dirPath);

      if(dir){
        //Already exists
      }else if(ENOENT == errno){
        //Directory does not exist
        //Add new directory to the final temp directory
        if(mkdir(dirPath, 0700) != 0){
          //error
          perror("Error creating directory: ");
          exit(EXIT_FAILURE);
        }else{
          printf("Created directory: %s\n", dirPath);
        }
      }else{
        //Error opening directory
        perror("Error looking for directory: ");
        exit(EXIT_FAILURE);
      }

      //Recursively search through this directory too
      if(getDirectories(newPath, relPath, finalDir) == -1){
        perror("Error searching directory: ");
        exit(EXIT_FAILURE);
      }
    }
    else if(S_ISREG(stat_buffer.st_mode) && strcmp(dp->d_name, ".DS_Store") != 0 && dp->d_name[0] != '.'){
      //A regular file is found

      //Add full file path to the list
      files = realloc(files, ((nfiles+1) * sizeof(files[0])));
      files[nfiles] = strdup(newPath);
      ++nfiles;

      //Add relative file path to list
      relative = realloc(relative, ((nrelative+1) * sizeof(relative[0])));
      relative[nrelative] = strdup(relPath);
      nrelative++;

    }else{
      //An unknown entity is found
      printf("%s is unknown\n", newPath);
    }


  }

  //Close the directory
  closedir(dirp);

  return 0;

}

//------------------------------------------------------------------------------

//-------------------------GET MODIFICATION TIME--------------------------------
// Function simply returns the last modification time of specified file


time_t getModTime(char *filepath){

  struct stat stat_buffer;

  if(stat(filepath, &stat_buffer) != 0){
    //Error has occured
    perror(progname);
    exit(EXIT_FAILURE);
  }

  return stat_buffer.st_mtime;

}

//------------------------------------------------------------------------------


//-------------------------GET FILE SIZE----------------------------------------
// Function simply returns the size of specified file


int getSize(char *filepath){

  struct stat stat_buffer;

  if(stat(filepath, &stat_buffer) != 0){
    //Error has occured
    perror(progname);
    exit(EXIT_FAILURE);
  }

  return stat_buffer.st_size;

}

//------------------------------------------------------------------------------
