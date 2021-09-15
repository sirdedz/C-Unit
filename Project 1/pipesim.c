#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

/*  CITS2002 Project 1 2020
    Name:                Darby Edwards
    Student number(s):   22713383
 */


//  MAXIMUM NUMBER OF PROCESSES OUR SYSTEM SUPPORTS (PID=1..20)
#define MAX_PROCESSES                       20

//  MAXIMUM NUMBER OF SYSTEM-CALLS EVER MADE BY ANY PROCESS
#define MAX_SYSCALLS_PER_PROCESS            50

//  MAXIMUM NUMBER OF PIPES THAT ANY SINGLE PROCESS CAN HAVE OPEN (0..9)
#define MAX_PIPE_DESCRIPTORS_PER_PROCESS    10

//  TIME TAKEN TO SWITCH ANY PROCESS FROM ONE STATE TO ANOTHER
#define USECS_TO_CHANGE_PROCESS_STATE       5

//  TIME TAKEN TO TRANSFER ONE BYTE TO/FROM A PIPE
#define USECS_PER_BYTE_TRANSFERED           1


//------------------------------------------------------------------------------

//----------------------------DATA STRUCTURES-----------------------------------
//Total time taken by the simulation in microseconds (usecs)
int timetaken = 0;

//Whether the simulation has started, 1 represents started, 0 represents has not started
int started = 0;

//Process states
enum {UNUSED, NEW, READY, RUNNING, EXITED, SLEEPING, WAITING, WRITEBLOCKED, READBLOCKED};
//Syscalls
enum {SYS_COMPUTE, SYS_SLEEP, SYS_EXIT, SYS_FORK, SYS_WAIT, SYS_PIPE, SYS_WRITEPIPE, SYS_READPIPE};

//Structure for holding process data
struct {

    int state;

    //nextsyscall stores the index of the syscall the process is currently acting upon
    int nextsyscall;

    //A process may have many syscalls over its lifetime
    struct {
      int syscall;
      int usecs;
      int otherPID;
      int pipedesc;
      int nbytes;

    }syscalls[MAX_SYSCALLS_PER_PROCESS];

} process[MAX_PROCESSES];

//Structure for holding pipe data
struct {
     int readprocessID;
     int writeprocessID;
     int bytesoccupied;


     //if a process is blocked and waiting its ID will be stored as to indicate it is blocked
     int writerwaitingID;
     int readerwaitingID;

} pipes[MAX_PIPE_DESCRIPTORS_PER_PROCESS*MAX_PROCESSES];

//Structure for holding processes which are sleeping and for how long
struct {
  int processID;
  int usecs;

} sleeping[MAX_PROCESSES];

//Integer storing number of processes sleeping at any one time
int sleepCounter;

//Structure for holding processes which are waiting for child processes to terminate
struct {
  int processID;
  int childID;

} waiting[MAX_PROCESSES];

//------------------------------------------------------------------------------

//--------------------------INITIALIZE VARIABLES--------------------------------

//Initialize all variables, set value to -1 to indicate an invalid value during simulation
void init(void){
  sleepCounter = 0;

  for(int i = 0; i < MAX_PROCESSES; i++){
    process[i].state = UNUSED;
    process[i].nextsyscall = 0;

    for(int x = 0; x < MAX_SYSCALLS_PER_PROCESS; x++){
      process[i].syscalls[x].syscall = SYS_EXIT;
      process[i].syscalls[x].usecs = -1;
      process[i].syscalls[x].otherPID = -1;
      process[i].syscalls[x].pipedesc = -1;
      process[i].syscalls[x].nbytes = -1;
    }


    sleeping[i].processID = -1;
    sleeping[i].usecs = -1;

    waiting[i].processID = -1;
    waiting[i].childID = -1;
  }

  for(int i = 0; i < MAX_PROCESSES*MAX_PIPE_DESCRIPTORS_PER_PROCESS; i++){
    pipes[i].readprocessID = -1;
    pipes[i].writeprocessID = -1;

    pipes[i].bytesoccupied = 0;
    pipes[i].writerwaitingID = -1;
    pipes[i].readerwaitingID = -1;
  }
}

//  ----------------------------------------------------------------------------


//-----------------------------READY QUEUE--------------------------------------
//Simple Queue Implementation for ready queue

//NOTE: Length of the queue is bound by SHRT_MAX, the maximum value of a short integer
int queue[SHRT_MAX];

//Index of the front of the queue
int front = 0;

//Index of the back of the queue
int back = -1;

//Storing the length of the queue
int size = 0;

//Function for inserting a processID into the queue, returns nothing
void insert_to_queue(int processID){
  if(size < SHRT_MAX){

    //if the back of the queue reaches the end of the array, it wraps back around to -1
    if(back == SHRT_MAX){
      back = -1;
    }

    queue[back+1] = processID;
    back++;
    size++;
  }
}

//Function for removing and returning the processID at the front of the queue
int remove_from_queue(void){
  if(size > 0){
    int processID = queue[front];

    front++;

    //if the front of the queue reaches the end of the queue, it wraps back around to zero
    if(front == SHRT_MAX){
      front = 0;
    }


    size--;

    return processID;
  }else{
    return -1;
  }
}


//  ----------------------------------------------------------------------------



//  ----------------------PROCESS SYSCALL ANALYSIS------------------------------
//Analyse the process passed from the ready queue and perform its syscall
void process_analysis(int processID, int timequantam, int pipesize){

  printf("%i......Starting analysis for process... %i  queue size %i\n", timetaken, processID, size);

  //Change process state to running, note that the very first process does not add state change time
  if(started == 0 && processID == 1){
    started = 1;
    process[processID-1].state = RUNNING;

  }else{
    process[processID-1].state = RUNNING;
    timetaken += USECS_TO_CHANGE_PROCESS_STATE;
    printf("%i......Moved process %i from READY to RUNNING \n", timetaken, processID);
  }

  //Gather process data
  int nextsyscall = process[processID-1].nextsyscall;
  int usecs = process[processID-1].syscalls[nextsyscall].usecs;
  int otherPID = process[processID-1].syscalls[nextsyscall].otherPID;
  int pipedesc = process[processID-1].syscalls[nextsyscall].pipedesc;
  int nbytes = process[processID-1].syscalls[nextsyscall].nbytes;

  //Declaring variables for later use
  int bytesinpipe;
  int bytesavailable;
  int bytesused;

  //Switch case for different syscalls
  switch (process[processID-1].syscalls[nextsyscall].syscall) {
    case SYS_COMPUTE:

        //Computation can be completed in one timequantam
        if(usecs <= timequantam){
            printf("%i......Process %i computing for %i usecs (finished)\n", timetaken, processID, usecs);

            timetaken += usecs;

            //State change
            process[processID-1].state = READY;
            timetaken += USECS_TO_CHANGE_PROCESS_STATE;
            printf("%i......Process %i moved from RUNNING to READY\n", timetaken, processID);

            //Compute has finished, increment for next syscall
            process[processID-1].nextsyscall++;
        }else{
          //Computation cannot be completed in one timequantam
          printf("%i......Process %i computing for %i usecs (unfinished)\n", timetaken, processID, timequantam);
          timetaken += timequantam;

          //Update to show amount of time to compute remaining
          process[processID-1].syscalls[nextsyscall].usecs -= timequantam;

          //State change
          process[processID-1].state = READY;
          timetaken += USECS_TO_CHANGE_PROCESS_STATE;
          printf("%i.......Process %i moved from RUNNING to READY \n", timetaken, processID);

        }

        //Add process to the ready queue ready for the next syscall
        insert_to_queue(processID);

        break;

    case SYS_SLEEP:

        printf("%i......Process %i is set to sleep for %i usecs \n", timetaken, processID, usecs);

        //State change
        process[processID-1].state = SLEEPING;
        timetaken += USECS_TO_CHANGE_PROCESS_STATE;
        printf("%i......Changed state of process %i from RUNNING to SLEEPING \n", timetaken, processID);


        //Calculate time when the process can be pushed back into the ready queue
        int sleeptime = usecs + timetaken;

        //Add process to sleeping structure
        sleeping[processID-1].processID = processID;
        sleeping[processID-1].usecs = sleeptime;
        sleepCounter++;

        //Increment for the next syscall
        process[processID-1].nextsyscall++;

        break;

    case SYS_EXIT:

        //Add time for state change from RUNNING to EXITED
        process[processID-1].state = EXITED;
        timetaken += USECS_TO_CHANGE_PROCESS_STATE;

        printf("%i......Process %i terminated. \n", timetaken, processID);

        //Check to see if a parent process is waiting on process termination
        for(int i = 0; i < MAX_PROCESSES; i++){
          if(waiting[i].childID == processID){
            //Change state
            process[waiting[i].processID-1].state = READY;
            timetaken += USECS_TO_CHANGE_PROCESS_STATE;

            //Add parent process back into ready queue
            insert_to_queue(waiting[i].processID);

            //Remove parent from waiting array
            waiting[i].processID = -1;
            waiting[i].childID = -1;

            printf("%i......Process %i was waiting for process %i to terminate and has been added back to the ready queue \n", timetaken, waiting[i].processID, processID);
          }
        }

        //De-allocate pipes
        for(int i = 0; i < MAX_PROCESSES*MAX_PIPE_DESCRIPTORS_PER_PROCESS; i++){
          if(pipes[i].readprocessID == processID){
            pipes[i].readprocessID = -1;
          }

          if(pipes[i].writeprocessID == processID){
            pipes[i].writeprocessID = -1;
          }
        }

        break;

    case SYS_FORK:
        //child is put into ready queue, THEN the parent is put back into the ready queue
        printf("%i......Process %i forking to create process %i \n", timetaken, processID, otherPID);

        //Add the child process to the Queue and change state
        insert_to_queue(otherPID);
        process[otherPID-1].state = READY;
        timetaken += USECS_TO_CHANGE_PROCESS_STATE;

        //Change state of the parent process and add it back to the Queue
        process[processID-1].state = READY;
        timetaken += USECS_TO_CHANGE_PROCESS_STATE;
        insert_to_queue(processID);

        //Fork has finished, increment for next syscall
        process[processID-1].nextsyscall++;

        //Check for any pipes the parent writes to and add the child to the reader if none already exists
        for(int i = 0; i < MAX_PROCESSES*MAX_PIPE_DESCRIPTORS_PER_PROCESS; i++){
          if(pipes[i].writeprocessID == processID && pipes[i].readprocessID == -1){
            pipes[i].readprocessID = otherPID;
          }
        }

        break;

    case SYS_WAIT:

        printf("%i......Process %i set to wait for process %i \n", timetaken, processID, otherPID);
        //Change state
        process[processID-1].state = WAITING;
        timetaken += USECS_TO_CHANGE_PROCESS_STATE;

        //Add process to waiting array
        waiting[processID-1].processID = processID;
        waiting[processID-1].childID = otherPID;

        //Increment ready for next syscall
        process[processID-1].nextsyscall++;

        break;

    case SYS_PIPE:

        printf("%i......Process %i creating pipe %i \n", timetaken, processID, pipedesc);
        //Add process data to the correct pipe
        pipes[pipedesc-1].writeprocessID = processID;

        //Change state
        process[processID-1].state = READY;
        timetaken += USECS_TO_CHANGE_PROCESS_STATE;

        //Increment for next syscall and add back to queue
        process[processID-1].nextsyscall++;
        insert_to_queue(processID);

        break;

    case SYS_WRITEPIPE:
        //Calculate bytes already in pipe
        bytesused = pipes[pipedesc-1].bytesoccupied;

        //Writing can be done in one instance
        if(nbytes <= pipesize-bytesused && nbytes > 0){
          timetaken += nbytes;

          //Change state and increment nextsyscall
          process[processID-1].state = READY;
          timetaken += USECS_TO_CHANGE_PROCESS_STATE;

          printf("%i......Process %i wrote %i bytes (finished) \n", timetaken, processID, nbytes);


          //Add bytes to pipe
          pipes[pipedesc-1].bytesoccupied += nbytes;

          //increment to next syscall and add to ready Queue
          process[processID-1].nextsyscall++;

          insert_to_queue(processID);

          //Process is not waiting to write
          pipes[pipedesc-1].writerwaitingID = -1;
        }else if(nbytes > 0){

          bytesavailable = pipesize - bytesused;

          //Subtract bytes that were added to the pipe
          process[processID-1].syscalls[nextsyscall].nbytes -= bytesavailable;
          timetaken += bytesavailable;
          printf("%i......Process %i wrote %i bytes (unfinished) \n", timetaken, processID, bytesavailable);

          //Writing becomes blocked, change of state
          process[processID-1].state = WRITEBLOCKED;
          timetaken += USECS_TO_CHANGE_PROCESS_STATE;
          printf("%i......Process %i changed from RUNNING TO WRITEBLOCKED \n", timetaken, processID);

          //Add bytes to fill the pipe
          pipes[pipedesc-1].bytesoccupied = pipesize;

          //process is now waiting for the pipe to clear
          pipes[pipedesc-1].writerwaitingID = processID;

        }

        //Check if there is a process waiting to read from the pipe
        if(pipes[pipedesc-1].readerwaitingID != -1 && process[pipes[pipedesc-1].readerwaitingID-1].state == READBLOCKED){

          int readerID = pipes[pipedesc-1].readerwaitingID;

          //change state of reading process
          process[readerID-1].state = READY;
          timetaken += USECS_TO_CHANGE_PROCESS_STATE;
          printf("%i.......Process %i changed from READBLOCKED TO READY \n", timetaken, readerID);

          //add reading process back to ready queue
          insert_to_queue(readerID);
        }

        break;

    case SYS_READPIPE:
        bytesinpipe = pipes[pipedesc-1].bytesoccupied;

        //Reading can be done in one instance
        if(nbytes <= bytesinpipe && nbytes > 0){
            timetaken += nbytes;

            printf("%i......Process %i read %i bytes (finished) \n", timetaken, processID, nbytes);

            //Change state and increment nextsyscall, add back to ready queue
            process[processID-1].state = READY;
            timetaken += USECS_TO_CHANGE_PROCESS_STATE;
            printf("%i......Process %i changed from RUNNING TO READY \n", timetaken, processID);

            //increment for next syscall and add back to ready queue
            process[processID-1].nextsyscall++;
            insert_to_queue(processID);


            //Process is not waiting to read
            pipes[pipedesc-1].readerwaitingID = -1;

            //data removed from pipe
            pipes[pipedesc-1].bytesoccupied -= nbytes;


        }else if(nbytes > 0){
          //Reading cannot be done in one instance

            //Read whatever data is in the pipe
            process[processID-1].syscalls[nextsyscall].nbytes -= pipes[pipedesc-1].bytesoccupied;
            timetaken += pipes[pipedesc-1].bytesoccupied;
            printf("%i......Process %i read %i bytes (unfinished) \n", timetaken, processID, pipes[pipedesc-1].bytesoccupied);


            //Change of state
            process[processID-1].state = READBLOCKED;
            timetaken += USECS_TO_CHANGE_PROCESS_STATE;
            printf("%i......Process %i changed from RUNNING TO READBLOCKED \n", timetaken, processID);

            //data removed from pipe
            pipes[pipedesc-1].bytesoccupied = 0;

            //process is now waiting for the pipe to fill
            pipes[pipedesc-1].readerwaitingID = processID;

        }

        //Check if there is a process waiting to write
        if(pipes[pipedesc-1].writerwaitingID != -1 && process[pipes[pipedesc-1].writerwaitingID-1].state == WRITEBLOCKED){

          int writerID = pipes[pipedesc-1].writerwaitingID;

          //change state of writing process
          process[writerID-1].state = READY;
          timetaken += USECS_TO_CHANGE_PROCESS_STATE;
          printf("%i......Process %i changed from WRITEBLOCKED TO READY \n", timetaken, writerID);

          //add writing process back to ready Queue
          insert_to_queue(writerID);

        }

        break;

    default:
        //Syscall not valid, exit program
        printf("Syscall not found... \n");
        exit(EXIT_FAILURE);
        break;

  }
}



//  ---------------------------------------------------------------------

//----------------------RUNNING THE SIMULATION --------------------------

/*
Run the simulation through performing analysis on each element in the ready queue until none remain

Note: Processes may be sleeping with no processes in the ready queue,
      in which case the CPU waits for the process to finish sleeping

Timequantam refers to the time a process is allowed to consume on the CPU in one instance (microseconds, usecs)

Pipesize refers to the maximum amount of data a pipe can store at any one time (bytes)
*/
void run_simulation(int timequantam, int pipesize){

    //Insert first process to the Queue
    insert_to_queue(1);

    //Run analysis whilst there remains processes in the ready queue or processes sleeping
    while(size > 0 || sleepCounter > 0){
      if(size > 0){
        //Check to see if any sleeping processes can be added back to the ready queue
        for(int i = 0; i < MAX_PROCESSES; i++){
          if(sleeping[i].usecs <= timetaken && sleeping[i].usecs >= 0){
            //Add sleeping process back to ready queue
            insert_to_queue(sleeping[i].processID);

            //State Change
            process[sleeping[i].processID-1].state = READY;
            timetaken += USECS_TO_CHANGE_PROCESS_STATE;


            //Effectively remove from sleeping structure
            sleeping[i].processID = -1;
            sleeping[i].usecs = -1;
            sleepCounter--;
          }
        }

        //Remove the processID at the front of the queue and analyse that process
        process_analysis(remove_from_queue(), timequantam, pipesize);
      }else{
        //Only sleeping tasks remain, shortest sleep time should simply be waited out

        //Find the process to be awoken first
        int shortestSleep = SHRT_MAX;
        int sleepID = -1;

        for(int i = 0; i < MAX_PROCESSES; i++){
          if(sleeping[i].processID != -1){
            if(sleeping[i].usecs < shortestSleep){
              shortestSleep = sleeping[i].usecs;
              sleepID = sleeping[i].processID;
            }
          }
        }

        //Providing the processID is valid
        if(sleepID != -1){

          //Increment timetaken to when the CPU should wait until
          timetaken = shortestSleep;
          printf("%i......CPU slept for %i usecs \n", timetaken, shortestSleep);

          //Add process set to be awoken first back into ready queue, effectively remove from sleeping
          insert_to_queue(sleepID);
          sleeping[sleepID-1].processID = -1;
          sleeping[sleepID-1].usecs = -1;
          sleepCounter--;

          //State Change
          printf("%i......Process %i changed from SLEEPING to READY \n", timetaken, sleepID);
          process[sleepID-1].state = READY;
          timetaken += USECS_TO_CHANGE_PROCESS_STATE;
        }

      }
    }

}

//------------------------------------------------------------------------------

//---------------------PARSING FILES AND EXTRACTING DATA------------------------
//  FUNCTIONS TO VALIDATE FIELDS IN EACH eventfile
int check_PID(char word[], int lc)
{
    int PID = atoi(word);

    if(PID <= 0 || PID > MAX_PROCESSES) {
        printf("invalid PID '%s', line %i\n", word, lc);
        exit(EXIT_FAILURE);
    }
    return PID;
}

int check_microseconds(char word[], int lc)
{
    int usecs = atoi(word);

    if(usecs <= 0) {
        printf("invalid microseconds '%s', line %i\n", word, lc);
        exit(EXIT_FAILURE);
    }
    return usecs;
}

int check_descriptor(char word[], int lc)
{
    int pd = atoi(word);

    if(pd < 0 || pd >= MAX_PIPE_DESCRIPTORS_PER_PROCESS) {
        printf("invalid pipe descriptor '%s', line %i\n", word, lc);
        exit(EXIT_FAILURE);
    }
    return pd;
}

int check_bytes(char word[], int lc)
{
    int nbytes = atoi(word);

    if(nbytes <= 0) {
        printf("invalid number of bytes '%s', line %i\n", word, lc);
        exit(EXIT_FAILURE);
    }
    return nbytes;
}

//------------------------STORING THE DATA--------------------------------------
//  parse_eventfile() READS AND VALIDATES THE FILE'S CONTENTS
//  Data is then stored in the correct data structures
void parse_eventfile(char program[], char eventfile[])
{
#define LINELEN                 100
#define WORDLEN                 20
#define CHAR_COMMENT            '#'

//  ATTEMPT TO OPEN OUR EVENTFILE, REPORTING AN ERROR IF WE CAN'T
    FILE *fp    = fopen(eventfile, "r");

    if(fp == NULL) {
        printf("%s: unable to open '%s'\n", program, eventfile);
        exit(EXIT_FAILURE);
    }

    char    line[LINELEN], words[4][WORDLEN];
    int     lc = 0;

//  READ EACH LINE FROM THE EVENTFILE, UNTIL WE REACH THE END-OF-FILE
    while(fgets(line, sizeof line, fp) != NULL) {
        ++lc;

//  COMMENT LINES ARE SIMPLY SKIPPED
        if(line[0] == CHAR_COMMENT) {
            continue;
        }

//  ATTEMPT TO BREAK EACH LINE INTO A NUMBER OF WORDS, USING sscanf()
        int nwords = sscanf(line, "%19s %19s %19s %19s",
                                    words[0], words[1], words[2], words[3]);

//  WE WILL SIMPLY IGNORE ANY LINE WITHOUT ANY WORDS
        if(nwords <= 0) {
            continue;
        }

//  ENSURE THAT THIS LINE'S PID IS VALID
        int thisPID = check_PID(words[0], lc);

//  Mark that the processID is used
        process[thisPID-1].state = NEW;

//  OTHER VALUES ON (SOME) LINES
        int otherPID, nbytes, usecs, pipedesc;

//  IDENTIFY LINES RECORDING SYSTEM-CALLS AND THEIR OTHER VALUES
//  THIS FUNCTION ONLY CHECKS INPUT;  YOU WILL NEED TO STORE THE VALUES
        if(nwords == 3 && strcmp(words[1], "compute") == 0) {
            usecs   = check_microseconds(words[2], lc);

            //Store system call specific information
            int next = process[thisPID-1].nextsyscall;

            process[thisPID-1].syscalls[next].syscall = SYS_COMPUTE;
            process[thisPID-1].syscalls[next].usecs = usecs;

            process[thisPID-1].nextsyscall++;

            process[thisPID-1].state = NEW;

        }
        else if(nwords == 3 && strcmp(words[1], "sleep") == 0) {
            usecs   = check_microseconds(words[2], lc);

            //Store system call specific information
            int next = process[thisPID-1].nextsyscall;

            process[thisPID-1].syscalls[next].syscall = SYS_SLEEP;
            process[thisPID-1].syscalls[next].usecs = usecs;

            process[thisPID-1].nextsyscall++;

            process[thisPID-1].state = NEW;

        }
        else if(nwords == 2 && strcmp(words[1], "exit") == 0) {
          //Store system call specific information
          int next = process[thisPID-1].nextsyscall;

          process[thisPID-1].syscalls[next].syscall = SYS_EXIT;

          process[thisPID-1].nextsyscall++;

          process[thisPID-1].state = NEW;
        }
        else if(nwords == 3 && strcmp(words[1], "fork") == 0) {
            otherPID = check_PID(words[2], lc);

            //Store system call specific information
            int next = process[thisPID-1].nextsyscall;

            process[thisPID-1].syscalls[next].syscall = SYS_FORK;
            process[thisPID-1].syscalls[next].otherPID = otherPID;

            process[thisPID-1].nextsyscall++;

            process[thisPID-1].state = NEW;
        }
        else if(nwords == 3 && strcmp(words[1], "wait") == 0) {
            otherPID = check_PID(words[2], lc);

            //Store system call specific information
            int next = process[thisPID-1].nextsyscall;

            process[thisPID-1].syscalls[next].syscall = SYS_WAIT;
            process[thisPID-1].syscalls[next].otherPID = otherPID;

            process[thisPID-1].nextsyscall++;

            process[thisPID-1].state = NEW;
        }
        else if(nwords == 3 && strcmp(words[1], "pipe") == 0) {
            pipedesc = check_descriptor(words[2], lc);

            //Store system call specific information
            int next = process[thisPID-1].nextsyscall;

            process[thisPID-1].syscalls[next].syscall = SYS_PIPE;
            process[thisPID-1].syscalls[next].pipedesc = pipedesc;

            process[thisPID-1].nextsyscall++;

            process[thisPID-1].state = NEW;
        }
        else if(nwords == 4 && strcmp(words[1], "writepipe") == 0) {
            pipedesc = check_descriptor(words[2], lc);
            nbytes   = check_bytes(words[3], lc);

            //Store system call specific information
            int next = process[thisPID-1].nextsyscall;

            process[thisPID-1].syscalls[next].syscall = SYS_WRITEPIPE;
            process[thisPID-1].syscalls[next].pipedesc = pipedesc;
            process[thisPID-1].syscalls[next].nbytes = nbytes;

            process[thisPID-1].nextsyscall++;

            process[thisPID-1].state = NEW;
        }
        else if(nwords == 4 && strcmp(words[1], "readpipe") == 0) {
            pipedesc = check_descriptor(words[2], lc);
            nbytes   = check_bytes(words[3], lc);

            //Store system call specific information
            int next = process[thisPID-1].nextsyscall;

            process[thisPID-1].syscalls[next].syscall = SYS_READPIPE;
            process[thisPID-1].syscalls[next].pipedesc = pipedesc;
            process[thisPID-1].syscalls[next].nbytes = nbytes;

            process[thisPID-1].nextsyscall++;

            process[thisPID-1].state = NEW;
        }
        //  UNRECOGNISED LINE
        else {
            printf("%s: line %i of '%s' is unrecognized\n", program,lc,eventfile);
            exit(EXIT_FAILURE);
        }
    }
    fclose(fp);

#undef  LINELEN
#undef  WORDLEN
#undef  CHAR_COMMENT
}

//  ----------------------------------------------------------------------------

//---------------------------------MAIN-----------------------------------------
//  CHECK THE COMMAND-LINE ARGUMENTS, CALL parse_eventfile(), RUN SIMULATION
int main(int argc, char *argv[])
{
    int timequantam = atoi(argv[2]);
    int pipesize = atoi(argv[3]);

    //check argc == 4
    if(argc != 4){
      exit(EXIT_FAILURE);
    }

    //check timequantam > 0
    if(timequantam <= 0){
      exit(EXIT_FAILURE);
    }

    //check pipesize > 0
    if(pipesize <= 0){
      exit(EXIT_FAILURE);
    }

    //initialize variables
    init();

    //parse event file and store relevent data
    parse_eventfile(argv[0], argv[1]);

    //reset nextsyscall for all processes after parsing
    for(int i = 0; i < MAX_PROCESSES; i++){
      process[i].nextsyscall = 0;
    }

    //Run the simulation
    run_simulation(timequantam, pipesize);

    //Print the result
    printf("timetaken %i\n", timetaken);

    //Return successfully
    return 0;
}
//------------------------------------------------------------------------------
