#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <dirent.h>
#include <sys/wait.h>

#define cd "cd"
#define debug "-d"
#define PATH "$PATH"
#define returnValue "$?"
#define MAXLINE 10
#define INPUTMAX 100
#define NBRCOMMANDS 9
#define MAXARGS 10
#define deleteScreenLine "\r\033[K"
#define backspace 8
#define space 32
#define delete 127

//HELLO WORLD

struct argument {
    char * name;
    char * parameters[10];
    int pipeFlag;
    int inputFlag;
    int outputFlag; 
    char * outFile;
    char * inFile;
}; 
int isExecutable(char *input);
void child(char *inputWithDelim[], int debugFlag);
char* findString(char* inputString[], char* requestedString);
void printEnviromentVariables(char *varName[], char *varContent[], int custom);
void echoCommand(char* inputWithDelim[], char *varContent[], char *arrayVariable[],int custom );
int setCommand(char* inputWithDelim[],char *variableArray[], char *variableContentArray[], int custom);
void cdCommand(char* inputWithDelim[], char *homeDirectory, char prevDirectory[]);
void copyStrArray(char* input[], char * copied[]);
char * copyString(char * input);
void printArray( char * array[]);
int parseArguments(char * inputWithDelim[], struct argument argumentArray[]);
void executePipe(struct argument argumentArray[],
                             int nbrPipes,
                           char * builtInCommands[],
                             int argCounter,    
                             char * variableArray[],
                             char * variableContentArray[],
                             int * customVariableCount,
                             char * homeDirectory,
                             char * prevDirectory,
                             int debugFlag,
                             char* historyOfCommands[]);
int nbrPipes(struct argument argumentArray[], int argCounter);
void closePipes(int pipefd[], int nbrPipes);
int isBuiltInCommand(struct argument * argPointer, char * builtInCommands[]);
void executeBuiltInCommand( struct argument * argPointer, 
                             char * variableArray[],
                             char * variableContentArray[],
                             int *customVariableCount,
                             char * homeDirectory,
                             char * prevDirectory,
                             int debugFlag,
                             struct argument argumentArray[],
                             int argCounter,
                             char* historyOfCommands[]);
void executeArgument( 
        struct argument argumentArray[],
        char * variableArray[],
        char * variableContentArray[],
        int *customVariableCount,
        char * homeDirectory,
        char * prevDirectory,
        int debugFlag,
        int argCounter,
        int nbrPipes,
        char * builtInCommands[],
        char* historyOfCommands[]);

void clearArgs(struct argument argumentArray[]);
void parseBashScript(char* input[], char*fileName);
struct termios originalTermio;
char *takeInput();
int checkUpOrDown();
void addHistoryCommand(char * line, char * historyOfCommands[], int * historyCounter);
char *myStringDuplication(const char *s);
void printHistory(char **hist);        
int copyFileToHistoryVariable(char *file, char**input, int length);        
void printWolfie();   
char *setQueryString(char**input, int val);
int isFileGlobbing(char** input, int val);
void fileGlobbing(char *queryString, char **input);
void printHistory(char **hist);
void clearHistory(char **hist);

char forPipeInput[200];
int acceptedCommand = 0;
int returnVal = 0;
int readInput = 0;
 int historyCount = 0;
 int historyCurrent = 0;
 int historyTracker;
 int historyCounter = 0;
 int mainCounter=0;
 int historyFileCopy=1;
 char *fileLocation; 
 int argCount=0;
 int keepingTrackOfProgTime=0;
 
/*
 * 
 */
int main(int argc, char * argv[]) {

    int count = 0, n = 0, argCounter = 0;
    char input[INPUTMAX] = {0};
    char *inputWithDelim[INPUTMAX] = {NULL};
    char *tempInput = NULL;
    char *inputFromFile[INPUTMAX] = {NULL};
    const char *delim = " \t\n";
    char *homeDirectory = getenv("HOME");
    char prevDirectory[1024];
    char currentWorkingDirectory[1024];
    int debugFlag = 0;
    char* variableArray[20] = {NULL};
    char* variableContentArray[20] = {NULL};
    int customVariableCount = 0;
    struct argument argumentArray[MAXARGS];
    int nbrOfPipes =0;
    char * builtInCommands[NBRCOMMANDS] = {"echo", "set", "printenv", "cd", "set", "exit", "wolfie","history","clear"};
    int standOut = dup(1);
    int standIn = dup(0);
    int executable =0, runnable = 0;
    char *line = (char *) calloc(256, sizeof(char)); // current commandline
    char *historyOfCommands[50]; // history storage
    int inputReadCount = 0;
    struct stat s;
    int fileLength;
    fileLocation = malloc(strlen(homeDirectory)+14);
    strcpy(fileLocation, homeDirectory);
    strcat(fileLocation, "/history.txt");
    
    //Not sure what all this does
    if(stat(fileLocation, &s) == 0) 
    {
      fileLength = s.st_size;
    }
    
    char *fileData= malloc(sizeof(char)*fileLength);
    /*My logic for termios below hails from this site
     * http://linux.die.net/man/3/termios
     */
    tcgetattr(0, &originalTermio);
    struct termios termio = originalTermio;
    termio.c_lflag &= ~(ECHO|ICANON);
    termio.c_cc[VTIME] = 1;
    termio.c_cc[VMIN] = 1;
    tcsetattr(fileno(stdin), TCSANOW, &termio);


    clearArgs(argumentArray);
    
    /*
     
     * What if program is started with -v, -d, and and a file is passed
     
     */

    if (argc > 1) {
        executable = isExecutable(argv[1]);
        if (executable == 1) {

            runnable = 1;
        } else {
            //WE ARE HERE ONLY FOR THE BAD REASONS
            FILE *fp;
            fp = fopen(argv[1], "r");
            if (fp == NULL && (strcmp(argv[1], "-v")!=0 
                    || strcmp(argv[1], "-d")!=0
                    || strcmp(argv[1], "-t")!=0))
            {
                printf("FILE DOES NOT EXIST\n");
                returnVal=1;
            } else 
            {
                printf("File is not executable\n");
            }
        }
    }

    /*
   Does a history file exist? If yes, copy it over to my his array,
   * if not, at least we know the truth
   */
    
     int fileDesc = open(fileLocation, O_RDWR);
     int historyFileCopy=1;
     if(read(fileDesc, fileData, fileLength+1) < 0)
         historyFileCopy=0;
  
     if(historyFileCopy==1)
     {
         historyCounter = copyFileToHistoryVariable(fileData, historyOfCommands, fileLength);
         mainCounter = copyFileToHistoryVariable(fileData, historyOfCommands, fileLength);
     }
    
     
     /*
      * Has the file been opened in debug mode?
      */
     int z=0;
     for(z=0; z<argc; z++)
     {
         if(strcmp(argv[z], "-d")==0)
         {
             debugFlag=1;
             break;
         }
     }
     
     
    while (1) {

    //Get working directory for prompt and print prompt
    getcwd(currentWorkingDirectory, sizeof (currentWorkingDirectory)); fflush(stdout);
    printf("["); fflush(stdout);
    printf(strcat(currentWorkingDirectory, "] swish> "), n); fflush(stdout);

    if (runnable == 1) {
        int w = 0;
        parseBashScript(inputFromFile, argv[1]);
        while (inputFromFile[w] != NULL) {
            tempInput = inputFromFile[w++];
            for (count = 0; count < 100; count++) {
                inputWithDelim[count] = strtok(tempInput, delim);
                if (inputWithDelim[count] == NULL)
                    break;

                tempInput = NULL;
            }
            argCounter = parseArguments(inputWithDelim, argumentArray);          
            nbrOfPipes = nbrPipes(argumentArray, argCounter);
            executeArgument(            argumentArray,
                                        variableArray,
                                        variableContentArray,
                                        &customVariableCount,
                                        homeDirectory,
                                        prevDirectory,
                                        debugFlag,
                                        argCounter,
                                        nbrOfPipes,
                                        builtInCommands,
                                        historyOfCommands);
       
            fflush(stdout);
            dup2(standOut, 1);
            fflush(stdin);
            dup2(standIn, 0);
        }
        runnable = 0;
    }
           
    else {
            while (1) {
                char * inputTaken = malloc(sizeof (char) *100);
                int inputCount = read(0, inputTaken, 100);

                /*
                 * Is it one of the arrows? The reason I check for 3, is because 
                 * the up and down have 3 unique characters, so I don't want 
                 * unnecesary entry in my loop
                 */
                if (inputCount == 3 && inputTaken[0] == 27 && inputTaken[1] == 91) {
                    /*
                     * Is it the up arrow?
                     */

                    if (inputTaken[2] == 65) {
                        if (historyCounter > 0 && historyCurrent < historyCounter) {
                            free(line);
                            int historyPos = historyCounter - historyCurrent - 1;
                            printf("%s", deleteScreenLine);
                            getcwd(currentWorkingDirectory, sizeof (currentWorkingDirectory));
                            printf("[");
                            printf(strcat(currentWorkingDirectory, "] swish> "), n);
                            fflush(stdout);
                            inputReadCount = 0;
                            line = myStringDuplication(historyOfCommands[historyPos]);
                            inputReadCount = strlen(historyOfCommands[historyPos]);
                            printf("%s", historyOfCommands[historyPos]);
                            historyCurrent++;
                        }
                    }
                        /*
                         Is it the down arrow?
                         */
                    else if (inputTaken[2] == 66) {
                        if (historyCounter > 0 && historyCurrent > 1) {
                            free(line);
                            int historyPos = historyCounter - historyCurrent + 1;
                            printf("%s", deleteScreenLine); // clearing line
                            getcwd(currentWorkingDirectory, sizeof (currentWorkingDirectory));
                            printf("[");
                            printf(strcat(currentWorkingDirectory, "] swish> "), n);
                            fflush(stdout);
                            inputReadCount = 0;
                            line = myStringDuplication(historyOfCommands[historyPos]);
                            inputReadCount = strlen(historyOfCommands[historyPos]);
                            printf("%s",historyOfCommands[historyPos]);
                            historyCurrent--;
                        }
                    }
                }
                else   { /*/If enter was pressed
                         * */

                    if (inputTaken[0] == 10) {
                        int fileDesc = open(fileLocation, O_WRONLY | O_APPEND | O_CREAT,
                                S_IRUSR | S_IRGRP | S_IROTH | S_IWGRP | S_IWUSR | S_IWOTH);

                        printf("%c", '\n');
                        if (strlen(line) > 0) {
                            /*
                             We know that max history allowed is 50 lines
                             */
                            if (historyCounter < 50) {
                                historyOfCommands[historyCounter] = myStringDuplication(line);
                                write(fileDesc, historyOfCommands[historyCounter], strlen(historyOfCommands[historyCounter]));
                                write(fileDesc, "\n", 1);
                                historyCounter++;
                                historyTracker++;

                            }                                /*
                                     Free up a location to store another line
                                     */
                            else {
                                free(historyOfCommands[0]);
                                int c;
                                for (c = 0; c < 50 - 1; c++)
                                    historyOfCommands[c] = historyOfCommands[c + 1];
                                historyOfCommands[50 - 1] = myStringDuplication(line);
                            }


                        }

                        strcpy(input, line);
                        inputReadCount = 0;
                        historyCurrent = 0;
                        free(line);
                        line = (char *) calloc(256, sizeof (char));
                        //getcwd(currentWorkingDirectory, sizeof (currentWorkingDirectory));
                        //printf("[");
                        //printf(strcat(currentWorkingDirectory, "] swish> "), n); fflush(stdout);
                        break;

                    } 
                    else if (inputTaken[0] == 127 || inputTaken[0] == 8) {
                        if (inputReadCount > 0) {
                            line[inputReadCount - 1] = '\0';
                            printf("%c%c%c", backspace, space, backspace);
                            inputReadCount--;
                        }
                    }                        /*
                         IF NONE OF THE ABOVE, PRINT TO THE SCREEN
                         */
                    else {
                        line[inputReadCount] = inputTaken[0];
                        inputReadCount++;
                        printf("%c", inputTaken[0]);
                    }
                }

                fflush(stdout);
            }
            
            
            int z;
            for(z=0; z<argc; z++)
            {
                if( (strcmp(argv[z], "-t")==0))
                {
                    keepingTrackOfProgTime=1;
                }
            }
            //Get input from user
          //  if (fgets(input, sizeof (input), stdin) == NULL) {
            //    exit(0);
            //}
            tempInput = input;
            strcpy(forPipeInput, input);
            int track =0;
            //Parse the input into char* array.  Based on the example here: 
            //http://www.cplusplus.com/reference/cstring/strtok/
            //I learned that a null should be used to the strtok after 
            //the 2nd pass
            for (count = 0; count < 100; count++) {
                argCount++;
                track++;
                inputWithDelim[count] = strtok(tempInput, delim);
                if (inputWithDelim[count] == NULL)
                    break;

                tempInput = NULL;
            }
             
            argCounter = parseArguments(inputWithDelim, argumentArray);
            
            
             /*
         Logic for fileGlobbing
         */
        
       char *queryString;
       
       if(argCounter== 1 && (isFileGlobbing(inputWithDelim,track) ==1)) {
          queryString = setQueryString(inputWithDelim, argCount);
          fileGlobbing(queryString, inputWithDelim);
          acceptedCommand=1;
          continue;
       }
       track=0;
            
            
            
            nbrOfPipes = nbrPipes(argumentArray, argCounter);
            executeArgument(            argumentArray,
                                        variableArray,
                                        variableContentArray,
                                        &customVariableCount,
                                        homeDirectory,
                                        prevDirectory,
                                        debugFlag,
                                        argCounter,
                                        nbrOfPipes,
                                        builtInCommands,
                                        historyOfCommands);
     
            fflush(stdout);
            dup2(standOut, 1);
            fflush(stdin);
            dup2(standIn, 0);
    }
            *currentWorkingDirectory = '\0'; //EMPTY OUT THE ARRAY

           if(keepingTrackOfProgTime==1)
           {
               ///* use rusage.ru_utime and rusage.ru_stime for user and system time resp. */
               printf("\n");
               struct rusage rusage;
               struct rusage tusage;
               getrusage(RUSAGE_CHILDREN, &rusage); 
               printf("TIMES: ");
               printf("Real: ");
               
               /*
                first addition
                */
               tusage.ru_utime.tv_sec  = rusage.ru_utime.tv_sec + rusage.ru_stime.tv_sec;
               tusage.ru_utime.tv_usec = rusage.ru_utime.tv_usec + rusage.ru_stime.tv_usec;
               
               /*
                divide and take the mod
                */
               tusage.ru_utime.tv_sec  = tusage.ru_utime.tv_sec + tusage.ru_utime.tv_usec / 1000000;
               tusage.ru_utime.tv_usec = tusage.ru_utime.tv_usec % 1000000;
               
               printf("%ld.%06lds", tusage.ru_utime.tv_sec, tusage.ru_utime.tv_usec);
               printf("  ");
               printf("User =");
               printf("%ld.%06lds",rusage.ru_utime.tv_sec, rusage.ru_utime.tv_usec);
               printf("  ");
               printf("Sys =");
               printf("%ld.%06lds",rusage.ru_stime.tv_sec, rusage.ru_stime.tv_usec);
               acceptedCommand=1;
               printf("\n");
               
           }
        }
         
       
     
    
    return 0;
}
void executeArgument(   
                        struct argument argumentArray[],
                        char * variableArray[],
                        char * variableContentArray[],
                        int *customVariableCount,
                        char * homeDirectory,
                        char * prevDirectory,
                        int debugFlag,
                        int argCounter,
                        int nbrPipes,
                        char * builtInCommands[],
                        char * historyOfCommands[]) {

    int args = 0, in = 0, out =0;
    while (args < argCounter) {

        struct argument tempArgument = argumentArray[args];
        struct argument * argPointer;
        argPointer = &tempArgument;

        if (argPointer->inputFlag == 1) {
            fflush(stdin);
            //open input file
            in = open(argPointer->inFile, O_RDONLY);

            //replace standard input with input
            dup2(in, 0);
            close(in);
        }

        if (argPointer->outputFlag == 1) {

            fflush(stdout);
            //open output file
            out = open(argPointer->outFile, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH | S_IWGRP | S_IWUSR | S_IWOTH);
            dup2(out, 1);
            close(out);
        }

        if (argPointer->pipeFlag == 1) 
        {
            executePipe(argumentArray,
                    nbrPipes,
                    builtInCommands,
                    argCounter,
                    variableArray,
                    variableContentArray,
                    customVariableCount,
                    homeDirectory,
                    prevDirectory,
                    debugFlag,
                    historyOfCommands);
            
            clearArgs(argumentArray);
            break;
        }//}
        else {
            if (isBuiltInCommand(argPointer, builtInCommands)) {

                executeBuiltInCommand(argPointer,
                        variableArray,
                        variableContentArray,
                        customVariableCount,
                        homeDirectory,
                        prevDirectory,
                        debugFlag,
                        argumentArray,
                        argCounter,
                        historyOfCommands);

            } else {

                //TIME TO FORK!
                pid_t pid = fork();

                if (pid == -1) {
                    perror("FAILED TO FORK");
                    exit(1);
                } else if (pid == 0) {
                    
                    child(argPointer->parameters,debugFlag);
                    
                } else {
                    int status;
                    wait(&status);

                    if (status == 0) {
                        returnVal = 0;
                    } else
                    {
                        printf("I WAS SET TO ONE");fflush(stdout);
                        returnVal = 1;
                    }
                }
            }
            if (debugFlag == 1) {
                fprintf(stderr,"ENDING: %s ret = %d\n", argPointer->name, returnVal);
                fflush(stderr);
                }
        }
        
        //Free up allocated memory in arguments
        int p;
            for (p = 0; p < argCounter; p++) {
                int q;
                if (argumentArray[p].inFile != NULL)
                    free(argumentArray[p].inFile);
                if (argumentArray[p].outFile != NULL)
                    free(argumentArray[p].outFile);
                if (argumentArray[p].name != NULL)
		    free(argumentArray[p].name);
		for (q = 0; q < MAXARGS; q++) {
                    if (argumentArray[p].parameters[q] != NULL) {
                        free(argumentArray[p].parameters[q]);

                    }
                }
            }
        
        
        clearArgs(argumentArray);
        args++;
    }

}

void clearArgs(struct argument argumentArray[]) {
    int empty;
    for( empty = 0; empty < MAXARGS; empty++) {
        memset(&argumentArray[empty], 0, sizeof(argumentArray[empty]));     
    } 
    
    
}

int isBuiltInCommand( struct argument * argPointer,char * builtInCommands[]) {
    int q;
    for (q = 0; q < NBRCOMMANDS; q++ ) {
        if (strcmp(argPointer->name,builtInCommands[q]) == 0) {
            return 1;
        }
    }
            return 0;    
}

void executeBuiltInCommand( struct argument * argPointer, char * variableArray[],
                             char * variableContentArray[],
                             int *customVariableCount,
                             char * homeDirectory,
                             char * prevDirectory,
                             int debugFlag,
                             struct argument argumentArray[],
                             int argCounter,
                             char *historyOfCommands[50]
                             ) {
    
  
    int tempCustomVariableCount = *customVariableCount;
    
    if (debugFlag == 1) {
        fprintf(stderr,"RUNNING: %s \n", argPointer->name);
    }

    //Print variables
    if (strcmp(argPointer->name, "printenv") == 0) {
        printEnviromentVariables(variableArray, variableContentArray, tempCustomVariableCount);
    }

    //Exit the program
    if (strcmp(argPointer->name, "exit") == 0) {
        printf("Goodbye, have a beautiful sunny day!\n");
        int j;
        
        //Free up allocated memory
        for (j = 0; j < tempCustomVariableCount; j++) {
            free(variableArray[j]);
            free(variableContentArray[j]);
        }
        
        

        exit(0);
    }
        //Echo input
    else if (strcmp(argPointer->name, "echo") == 0) {
        echoCommand(argPointer->parameters, variableContentArray, variableArray, tempCustomVariableCount);
    }
        //Set variables
    else if (strcmp(argPointer->name, "set") == 0) {
        *customVariableCount = setCommand(argPointer->parameters, variableArray, variableContentArray, tempCustomVariableCount);
        //*customVariableCount = tempVC;
    }
        //Change directories
    else if (strcmp(argPointer->name, "cd") == 0) {
        cdCommand(argPointer->parameters, homeDirectory, prevDirectory);
    }
    //              Execute non-built in command
    else if (strcmp(argPointer->name, "wolfie") == 0) {
        printWolfie();
            
    }
    
    else if (strcmp(argPointer->name, "history")==0 ) {
        printHistory(historyOfCommands);
        acceptedCommand =1; 
    }
    
    else if (strcmp(argPointer->name, "clear") ==0) {
        remove(fileLocation);
        clearHistory(historyOfCommands);
        acceptedCommand =1;
        historyCounter = 0;
        historyCurrent = 0;
        historyTracker = 0;
    }
}    

void printWolfie() 
{
    char  *pointe1=  " _    _  _____  __    ____  ____  ____ ";
    char  *pointe2="( \\/\\/ )(  _  )(  )  ( ___)(_  _)( ___)";
    char  *pointe3="  )   (  )(_) ( )(__  )__)  _) (_  )__) ";
    char  *pointe4="(__/\\__)(_____)(____)(__)  (____)(____)"; 
    
    printf("%s\n", pointe1);
    printf("%s\n", pointe2);
    printf("%s\n", pointe3);
    printf("%s\n", pointe4);
    acceptedCommand=1;    
}



int nbrPipes(struct argument argumentArray[], int argCounter) {
    int w;
    int count=0;
    for (w =0; w < argCounter; w++) {
        
        struct argument argStruct = argumentArray[w];
        if (argStruct.pipeFlag == 1) {
            count++;
        }   
    }
    return count;  
}

void executePipe(struct argument argumentArray[],
        int nbrPipes,
        char * builtInCommands[],
        int argCounter,
        char * variableArray[],
        char * variableContentArray[],
        int *customVariableCount,
        char * homeDirectory,
        char * prevDirectory,
        int debugFlag,
        char* historyOfCommands[]) 
{
        int q;
    
    //Initialize pipe array with number of pipes
    int *pipefd = malloc(sizeof (int) * (2 * nbrPipes));
    int j;
    for (j = 0; j < nbrPipes; j++) {
        pipe(pipefd + (j * 2));
    }

    //For testing
    //char *cat_args[] = {"cat", "temp.txt", NULL};
    //char *cut_args[] = {"cut", "-c4", NULL};
    //char *wc_args[] = {"wc", "-l", NULL};

    for (q = 0; q < (argCounter); q++) {
        struct argument tempArgument = argumentArray[q];
        struct argument * argPointer;
        argPointer = &tempArgument;


        if (q == 0) {
            if (fork() == 0) {
                dup2(pipefd[1], 1);
                closePipes(pipefd, nbrPipes);
                if(isBuiltInCommand(argPointer, builtInCommands)) {
                    executeBuiltInCommand(argPointer,
                            variableArray,
                            variableContentArray,
                            customVariableCount,
                            homeDirectory,
                            prevDirectory,
                            debugFlag,
                            argumentArray,
                            argCounter,
                            historyOfCommands);
                    
                }
                child(argPointer->parameters, debugFlag);
                if (debugFlag == 1) {
                        fprintf(stderr,"ENDING: %s \n ret = %d", argPointer->name, returnVal);
                        fflush(stderr);
                }           
                //execvp(argPointer->parameters[0], argPointer->parameters);
                //execvp("cat",cat_args);
            } else {continue;}
        }
        else if (argPointer->pipeFlag == 0) {
            if (fork() == 0) {
                //dup2(pipefd[0],0);
                //dup2(pipefd[3],1);
                //execvp(argPointer->parameters[0],argPointer->parameters);
                
		if (argPointer->outputFlag == 1) {
                        fflush(stdout);
                        //open output file
                        int out = open(argPointer->outFile, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH | S_IWGRP | S_IWUSR | S_IWOTH);
                        dup2(out, 1);
                        close(out);
                }



		dup2(pipefd[(2 * q) - 2], 0);
                closePipes(pipefd, nbrPipes);
                if(isBuiltInCommand(argPointer, builtInCommands)) {
                    executeBuiltInCommand(argPointer,
                            variableArray,
                            variableContentArray,
                            customVariableCount,
                            homeDirectory,
                            prevDirectory,
                            debugFlag,
                            argumentArray,
                            argCounter,
                            historyOfCommands);    
                }
                
                child(argPointer->parameters, debugFlag);
                //execvp(argPointer->parameters[0], argPointer->parameters);
                //execvp("wc", wc_args);
            } else {
                continue;
            }
        } else {
            if (fork() == 0) {
                dup2(pipefd[(2 * q) - 2], 0);
                //dup2(pipefd[2],0);          
                //dup2(pipefd[0], 0);
                // replace grep's stdout with write end of 2nd pipe

                dup2(pipefd[(2 * q) + 1], 1);
                closePipes(pipefd, nbrPipes);
                if(isBuiltInCommand(argPointer, builtInCommands)) {
                   executeBuiltInCommand(argPointer,
                            variableArray,
                            variableContentArray,
                            customVariableCount,
                            homeDirectory,
                            prevDirectory,
                            debugFlag,
                            argumentArray,
                            argCounter,
                           historyOfCommands);                
                }
                
                
                child(argPointer->parameters,debugFlag);
                //execvp(argPointer->parameters[0], argPointer->parameters);
                //execvp("cut", cut_args);                      
            } else {
                continue;
            }
        }
        
        
        if (debugFlag == 1)
        {
                fprintf(stderr,"ENDED: \n ret = %d",  returnVal);
                fflush(stderr);
        }
    
    
    }
    closePipes(pipefd, nbrPipes);
    int i;
    for (i = 0; i < (nbrPipes + 1); i++)
        wait(NULL);

    free(pipefd);
    //if (debugFlag == 1) {
    //    fprintf(stderr,"ENDING: %s \n ret = %d", argPointer->name, returnVal);
    //    fflush(stderr);
    //}
    if (debugFlag == 1)
    {
        fprintf(stderr,"ENDING: %s \n ret = %d\n", forPipeInput, returnVal);
        fflush(stderr);
        
    }
}
void closePipes(int pipefd[], int nbrPipes) {
    int j;
    for (j=0; j < (nbrPipes*2); j++ ) {
        close(pipefd[j]);
    }
    
}

  int parseArguments(char * inputWithDelim[], struct argument argumentArray[]) {
    int counter = 0;
    int counter2 = 0;
    int argCounter = 0;
    char * demarks[INPUTMAX];

    while (1) {

        char * check = inputWithDelim[counter];
        char tempArray[INPUTMAX] = {'\0'};
        if (check != '\0')
            strcpy(tempArray, check);
            
        
        if (check == '\0') {
            demarks[counter2] = '\0';
            argumentArray[argCounter].name = copyString(demarks[0]);
            copyStrArray(argumentArray[argCounter].parameters, demarks);
            counter2 = 0;
            argCounter++;
            break;
        }


        //if (strcmp(check, "|") != 0 && strcmp(check, ">") != 0 && strcmp(check, "<") != 0 && (tempArray[0]!= '>') && (tempArray[0]!= '<'))  {
        if ((tempArray[0]!= '>') && (tempArray[0]!= '<') && (tempArray[0] != '|')) {
        
        
            demarks[counter2] = inputWithDelim[counter];
            counter2++;    
        } 
        
        else if (tempArray[0] == '|') {
                argumentArray[argCounter].pipeFlag = 1;
                demarks[counter2] = '\0';
                //argumentArray[argCounter].name = demarks[0];
                argumentArray[argCounter].name = copyString(demarks[0]);
                copyStrArray(argumentArray[argCounter].parameters, demarks);
                counter2 = 0;
                argCounter++;
                memset(demarks, '\0', sizeof (demarks));
            } else if (tempArray[0] == '>') {
                
                argumentArray[argCounter].outputFlag = 1;
                
                if (tempArray[1] == '\0') {
                        
                        //Get the next argument as redirect
                        counter++;
                        argumentArray[argCounter].outFile = copyString(inputWithDelim[counter]);
                }
                else {
                    //get Substring
                    char * tempString = (char *) malloc(strlen(inputWithDelim[counter]));
                    strncpy(tempString, (inputWithDelim[counter] + 1), strlen(inputWithDelim[counter]));
                    argumentArray[argCounter].outFile = copyString(tempString);             
                }
                
            }
            else if (tempArray[0] == '<') {
                argumentArray[argCounter].pipeFlag = 0;
                argumentArray[argCounter].inputFlag = 1;
                
                if (tempArray[1] == '\0') {
                        
                        //Get the next argument as redirect
                        counter++;
                        argumentArray[argCounter].inFile = copyString(inputWithDelim[counter]);
                }
                else {
                    //get Substring
                    char * tempString = (char *) malloc(strlen(inputWithDelim[counter]));
                    strncpy(tempString, (inputWithDelim[counter]+1), strlen(inputWithDelim[counter]));
                    argumentArray[argCounter].inFile = copyString(tempString);             
                }
            }
            else {}

        //}
        counter++;
    }

    //int j;
    //for(j=0; j< argCounter; j++) {
    //     printf("%s\n", argumentArray[j].parameters[0]);
    //}
    return argCounter;
}














void echoCommand(char * inputWithDelim[],
        char *variableContentArray[],
        char *variableArray[],
        int customVariableCount) {

    char *substr = "$";

    if (inputWithDelim[1] != NULL) {
        if (strcmp(inputWithDelim[1], PATH) == 0) {
            char *printPath = getenv("PATH");
            printf("%s\n", printPath);
        }
        else if (strcmp(inputWithDelim[1], returnValue) == 0) {
            printf("%d\n", returnVal);
        }//Print out variable
        else if (strncmp(inputWithDelim[1], substr, strlen(substr)) == 0) {
            int j;
            for (j = 0; j < customVariableCount; j++) {
                if (strcmp(variableArray[j], inputWithDelim[1]) == 0)
                    printf("%s\n", variableContentArray[j]);
            }
        }//Need to fix
        else {
            int i = 1;
            //while (inputWithDelim[i] != NULL) {
            //    totalString = totalString + strlen(inputWithDelim[i]);
            //}

            //int i=1;
            //char * temp = malloc(totalString + 1);
            //while (inputWithDelim[i] != NULL ) {
            //    strcat_s(temp,totalString,inputWithDelim[i]);
            //}

            while (inputWithDelim[i] != NULL) {
                printf("%s ", inputWithDelim[i++]);
            }
        }
    }
}

void cdCommand(char* inputWithDelim[], char *homeDirectory, char prevDirectory[]) {
    int visitRequestedDirectory = 1;

    if (inputWithDelim[1] == '\0') {
        printf("%s is the home", homeDirectory);
        chdir(homeDirectory);
        visitRequestedDirectory = 0;
    } else if (strcmp(inputWithDelim[1], "-") == 0) {
        chdir(prevDirectory);
        visitRequestedDirectory = 0;
    }

    if (visitRequestedDirectory == 1) {
        getcwd(prevDirectory, sizeof (prevDirectory));
        chdir(inputWithDelim[1]);
    }


}

int setCommand(char* inputWithDelim[], char*variableArray[], char*variableContentArray[], int customVariableCount) {


    //LOGIC FOR PATH VARIABLE
    if (strcmp(inputWithDelim[1], "$PATH") == 0) {
        char *setPath = malloc(sizeof(char)*100);
        strcpy(setPath,"PATH=");
        char *userInput;
        //GET THE INPUT AFTER THE EQUAL SIGN
        userInput = findString(inputWithDelim, "=");
        acceptedCommand = 1;
        //user
        char*largerString = malloc(sizeof(char)*200);
        strcat(setPath, userInput);
        strcpy(largerString, setPath);
        putenv(largerString);
    } 
    
    else {
        int j = 0;
        //ADD MORE ALPHANUMERIC COMPARISONS
        if (inputWithDelim[1][j] == '$')
        {
            char * tempVariable = malloc(strlen(inputWithDelim[1]));
            char * tempContentArray = malloc(strlen(inputWithDelim[3]));
            strcpy(tempVariable, inputWithDelim[1]);
            strcpy(tempContentArray, inputWithDelim[3]);
            variableArray[customVariableCount] = tempVariable;
            //the 3rd position has the content, since the equal sign is the 2nd,
            variableContentArray[customVariableCount] = tempContentArray;
            customVariableCount++;
            
        }
        else
        {
            printf("Please use a $ sign when you are declaring variables. Thanks");
            returnVal=1; //SINCE THATS AN ERROR
        }
        
        acceptedCommand = 1;

    }
    return customVariableCount;
    
}

void printEnviromentVariables(char* varName[], char *varContent[], int customVariableCount) {
    //Length of the array
    //int length = sizeof (varName) / sizeof (int);
    int i = 0;

    printf("Printing out variable names\n");

    //If length is 0, JUST PRINT OUT THE PATH, and RETURN VALUE, since those are the only variables
    //because the user has not yet created 'custom' variables
    printf("Name: Path  Content: ");
    printf("%s\n", getenv("PATH"));

    printf("Name: Return Value     Content:  ");
    printf("%d\n", returnVal);
    if (customVariableCount > 0) {
        for (i = 0; i < customVariableCount; i++) {

            printf("Name: %s      Content: %s\n", varName[i], varContent[i]);
        }
    }

}

void child(char *inputWithDelim[], int debugFlag) {
    int ret;
 
    if (debugFlag == 1) {
        fprintf(stderr,"RUNNING: %s \n", inputWithDelim[0]);
    }
    fflush(stderr);
    //EXECVP does a search on $PATH, whereas execvl expects you to pass the path
    ret = execvp(inputWithDelim[0], inputWithDelim);
        
    if (ret!=0 && acceptedCommand == 0) {

        printf("Couldn't find the command\n");
        returnVal=1;
    }
    exit(0);
}

/*Since all my strings are delimited, if I need a particular string from the original passed input
 I can use this function, if user passed set $PATH = hi; the set $PATH are each stored separately
 so this function will find it on demand*/


char* findString(char* inputString[], char * requestedString) {
    int found = 0;
    int i = 0;
    for (i = 0; i < 3; i++) {
        if (strcmp(inputString[i], requestedString) == 0) {
            found = i;
            break;
        }
    }
    char * returnString = (char *) malloc (strlen(inputString[found + 1]) + 1);
    strncpy(returnString, inputString[found + 1], strlen(inputString[found + 1]) + 1);
    
    return returnString;

}

void printArray(char * array[]) {
    int q;
    for (q = 0; q < 2; q++) {
        printf("%s\n", array[q]);

    }
}

char * copyString(char* input) {
    char * copy = (char *) malloc(strlen(input) + 1);
    strncpy(copy, input, strlen(input) + 1);
    return copy;   
}

void copyStrArray(char * copied[], char ** input) {
    int i = 0;
    while (input[i] != '\0') {
        copied[i] = (char *) malloc(strlen(input[i]) + 1);
        strncpy(copied[i], input[i], strlen(input[i]) + 1);
        i++;
    }
    copied[i] = '\0';


}

int isExecutable(char *input)
{
    struct stat sb;
    
    if (stat(input, &sb) == 0 && sb.st_mode & S_IXUSR)
    {
        return 1;
    }
    /* executable */
    else
    {
        return 0;
    }
    /* non-executable */  
}


void parseBashScript(char* inputFromFile[], char*fileName)
{
    int j=0;
    int i;
    FILE *fp;
    char *line = NULL;
    size_t len=0;
    ssize_t read;
    int lineCount=0;
    
    
    //Do not really get why fileName is null
    if(fileName==NULL)
    {
    for (i = 0; i < INPUTMAX; i++)
    {
         int argStrLen = strlen(inputFromFile[i]);
         for(j=0; j<argStrLen; j++)
         {
            if(inputFromFile[i][j]=='.' && inputFromFile[i][j+1]=='s' && inputFromFile[i][j+2]=='h')
            {
                fileName = inputFromFile[i];
                break;
            }

         }
    }
    }
    
    fp=fopen(fileName,"r");    
    
    if(fp==NULL)
    {
       printf("Couldnt open the file\n");
    } 
    
    else
    { 
        while((read = getline(&line, &len, fp))!=-1)
        {
            inputFromFile[lineCount]=malloc( (sizeof(char)*(strlen(line))) + 2  );
            strcpy(inputFromFile[lineCount], line);
            lineCount++;
        }
    }
    free(line);
}

int copyFileToHistoryVariable(char *file, char**input, int length)
{
 
    int c;
    int count=0;
    int keepTrack=0;
    int numMallocs=0;
    for(c=0; c<length; c++)
    {
        if(file[c]=='\n')
        {
         
         input[numMallocs++]=malloc(sizeof(char)*150);
         
        }
    }
    
    //input= (char **)malloc(numMallocs*sizeof(char *));

    strcpy(input[0],"HELLO");
    
    int i=0;
    int z=0;
    
    for(z=0; z<length; z++)
        {
          
            if(file[z]!='\n')
              {
                  input[i][keepTrack++] = file[z];
              }
           else
              {
                  //input[i][keepTrack] = '\0';
                  keepTrack=0;
                  i++;
                  count++;
              }
            
        }
   
    return count;
    
}

char *myStringDuplication(const char *s) 
{
    char *sPointer = malloc(strlen(s) + 1);
    
    if(sPointer) 
    {
        strcpy(sPointer, s); 
    }
    return sPointer;
}

void fileGlobbing(char *queryString, char **input) {
        
   DIR *dir;
   char *cwd;
   cwd = getcwd (0, 0);
   struct dirent *ent;
   int count=0;
   char* fileNames[50];
   int i=0;
   
   if ((dir = opendir (cwd)) != NULL) 
   {
       while ((ent = readdir (dir)) != NULL) 
       {
           fileNames[i]=malloc(sizeof(char)*100);
           strcpy(fileNames[i], ent->d_name);
           i++;
           count++;
       }
          closedir (dir);
   } 
   
   else
   {
       return;
   }
   
   /*
    Let's try to find some substrings matching the provided extension as
    * an input parameter!
    */
   
   int z=0;
   if(strcmp("rm", input[0])!=0)
   {    
        //printf("We found:\n");
       int found=0;
        for(z=0; z<count; z++)
        {
            if(strstr(fileNames[z], queryString))
            {
                printf("%s\n", fileNames[z]);
                found=1;
            }
        }
       
       if(found==0)
       {
           printf("Nothing found\n");
       }
   }
   
     if(strcmp("rm", input[0])==0)
   {    
     //   printf("We found:\n");
        for(z=0; z<count; z++)
        {
            if(strstr(fileNames[z], queryString))
            {
                remove(fileNames[z]);
            }
        }
   }
   
   
}

int isFileGlobbing(char** input, int val)
{
    int c=0;
    for(c=0; c<val-1; c++)
    {
        /*
         Look for a * in the input array, if found, the 
         * element after the * is what we are looking for, i.e the extension

         * 
         *          */
        if(input[c][0]=='*')
        {
            return 1;
            break;
        }
    }
    return 0;
}
    
char *setQueryString(char**input, int val) {
    int c;
    int i;
    int found=0;
    for(c=0; c<val-1; c++)
    {
        /*
         Look for a * in the input array, if found, the 
         * element after the * is what we are looking for, i.e the extension
         */
        if(input[c][0]=='*')
        {
           // printf("STRING LENGTH IS %d", strlen(input[c]));
            found=c;
            break;
        }
    }
    
    char *queryString = malloc(sizeof(char)*strlen(input[c]));
    
    for(i=0; i<strlen(input[found]); i++)
    {
        queryString[i]=input[found][i+1];
    }
    
    return queryString;
    
}


void clearHistory(char **hist){
    printf("History cleared!");
    int c;
      
    if (mainCounter==0)
    {
        mainCounter = historyTracker;
    }
    
     for(c=0; c<mainCounter;c++ )
    {
     memset(hist[c], 0, sizeof hist);
    }
      
}

void printHistory(char **hist)
{
    int c=0;        
    printf("NOW PRINTING HISTORY:\n"); 
    if (mainCounter==0)
    {
        mainCounter = historyTracker;
    }
    
    for(c=0; c<mainCounter;c++ )
    {
        if(hist[c]!=NULL)
        {
           printf("%s\n", hist[c]);
           
        }
       
    }
     
}
