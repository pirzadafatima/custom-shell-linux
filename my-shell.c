#include<stdio.h>
#include <unistd.h>
#include<sys/types.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<sys/time.h>
#include<stdbool.h>
#include<string.h>
#include<fcntl.h>

int main()
{
    char str[50];
    char * cArr[20];
    char history[10][50];
    int counter = 0;
    int index = 0, cmdNum = 1;
    int status;
    
    while(1)
    {
        printf("newShell@shell~VirtualBox:");
        fgets(str, 50, stdin);
        for(int x =0;; x++)
        {
            if (str[x] == '\n' )
            {
                str[x] = ' ';
                break;
            }
        }

        if(strcmp(str, "history ") != 0 && strcmp(str, "!! ") != 0 && str[0] != '!')
        {
            strcpy(history[index], str);
            index = (index+1) % 10;
            cmdNum++;
            if (counter != 10)
                counter++;
            //printf("index: %d, cmdNUm: %d, counter %d\n", index,cmdNum, counter);
            
        }

        if(strcmp(str, "!! ") == 0)
        {
            if (counter == 0)
            {
                printf("No commands in history.\n");
            }

            strcpy(str, history[((index-1) + 10) % 10]);
            printf("%s\n", str);
            strcpy(history[index], str);
            index = (index+1) % 10;
            cmdNum++;
            if (counter != 10)
                counter++;
        }

        if(str[0] == '!' && strlen(str) > 1)
        {
            int x;
            bool error = false;
            if(counter == 10)
            {
                x= atoi(str + 1);
                //printf("x: %d\n", x);
                if (x > cmdNum-1 || x < cmdNum -10)
                    error = true;

                if(x > 10)                 
                { 
                    x = x - 10 - 1;
                    //printf("x: %d\n", x);
                }
                else
                { 
                    x = atoi(str + 1) - 1;
                    //printf("x: %d\n", x);
                }
            }
            else
            { 
                x = atoi(str + 1)- 1;
                if (x > cmdNum-1 || x < cmdNum -10)
                    error = true;
            }

            if (error == true)
                printf("This command does not exist. \n");
            else
            {
                strcpy(str, history[x]);
                printf("%s\n", str);
                strcpy(history[index], str);
                index = (index+1) % 10;
                cmdNum++;
                if (counter != 10)
                    counter++;
            }
        }

        //printf("%s\n", str);

        char * newStr = strtok(str, " ");
        if(newStr != NULL)
        {

            int i = 0;
            while(newStr != NULL)
            {
                cArr[i] = newStr;
                //printf("%s\n", cArr[i]);
                newStr = strtok(NULL, " ");     
                i++;
            }

            int length = i+1;
            char* final[length];

            i=0;
            while (i< length-1)
            {
                final[i] = cArr[i];
                i++;
            }

            final[i] = NULL;
            //printf("%d\n", i);

            if(strcmp(final[0], "history") == 0)
            {
                int x = counter - 10;
                if(x<0)
                    x = 0;

                int in = ((index-1) + 10) % 10;
                int cNo = cmdNum - 1;
                if (counter == 0)
                {
                    printf("No commands in history.\n");
                }

                while(x<counter)
                {
                    printf("%d: %s\n", cNo, history[in]);
                    cNo--;
                    in = ((in-1) + 10) % 10;
                    x++;
                }
            }

            int fd1, fd2, fd3, stdI, stdO, stdE; 
            bool iRED = false, oRED = false, eRED = false;

            for (int i = 0; i < length - 1; i++)
            {
                if (strcmp(final[i], "<") == 0)  //input redirection
                {
                    iRED = true;
                    final[i] = NULL;
                    fd1 = open(final[i + 1], O_RDONLY);
                    if (fd1 < 0)
                    {
                        perror("Error opening input file");
                        exit(1);
                    }

                    stdI = dup(0);           //saving so we can redirect it back to terminal later
                    dup2(fd1, 0);
                    close(fd1);
                }
                else if (strcmp(final[i], ">") == 0)  //output redirection
                {
                    oRED = true;
                    final[i] = NULL;
                    fd2 = open(final[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);  //o_trunc empties an already existing file before writing into it again
                    if (fd2 < 0)
                    {
                        perror("Error opening output file");
                        exit(1);
                    }

                    stdO = dup(1);
                    dup2(fd2, 1);
                    close(fd2);
                }

                else if (strcmp(final[i], "2>") == 0)
                {
                    eRED = true;
                    final[i] = NULL;
                    fd3 = open(final[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);  //0666 grants read/write permission to everyone 
                    if (fd3 < 0)
                    {
                        perror("Error opening output file");
                        exit(1);
                    }

                    stdE = dup(2);
                    dup2(fd3, 2);
                    close(fd3);
                }
                
                else if (strcmp(final[i], "|") == 0)
                {
                    char* arr[length];
                    int next = i+1;
                    final[i] = NULL;
                    int j=0;

                    while (final[next] != NULL)
                    {
                        arr[j] = final[next];
                        j++;
                        next++;
                    }
                    arr[j] = NULL;

                    int fd[2];
                    if (pipe(fd) == -1)
                    {
                        perror("Error creating pipe");
                        exit(1);
                    }

                    pid_t pid = fork();
                    if (pid == 0)
                    {
                        oRED = true;
                        stdO = dup(1);
                        dup2(fd[1], 1);
                        close(fd[0]);
                        close(fd[1]);
                        execvp(final[0], final);
                        perror("Error executing command");
                        exit(1);
                    }

                    else if (pid > 0)
                    {
                        wait(NULL);
                        iRED = true;
                        stdI = dup(0);
                        dup2(fd[0], 0);
                        close(fd[0]);
                        close(fd[1]);
                        execvp(arr[0], arr);
                    }

                    else
                    {
                        perror("child not created.");
                        exit(1);
                    }
                }
            }

            bool bcg = false;
            if(strcmp(final[length-2], "&") == 0)
            {
                final[length-2]=NULL;
                bcg = true; 
            }

            pid_t id = fork();

            if(id == 0)
            {
                //printf("This is child. \n");
                //printf("%s\n", final[0]);
                execvp(final[0], final);
                //printf("hello\n");
            }

            else if (id>0)
            {
                if(bcg == false)
                {
                    wait(&status);
                }
                else
                {
                    //does not wait
                } 
            }

            if(iRED == true)
            {
                dup2(stdI, 0);
            }

            if(oRED == true)
            {
                dup2(stdO, 1);
            }

            if(eRED == true)
            {
                dup2(stdE, 2);
            }

            if(strcmp(final[0], "exit") == 0)
                exit(0);

        }
    }

    return 0;
}