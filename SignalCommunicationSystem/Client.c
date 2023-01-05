#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int tryOpenServerFile();
void signal_handler(int sig);
int readLine(int fd, char* buffer);
void writeLine(int fd, char* text);
void deleteFile(char* fileName);

char clientPID[50];

int main(int argc, char* argv[])
{
    sprintf(clientPID, "%d", getpid());

    int serverFd = tryOpenServerFile();

    writeLine(serverFd, clientPID); //Writing client pid
    writeLine(serverFd, argv[2]); //Writing first number
    writeLine(serverFd, argv[3]); //Writing calculation code
    writeLine(serverFd, argv[4]); //Writing second number

    lseek(serverFd, -1, SEEK_END);
    write(serverFd, NULL, 1);
    close(serverFd);

    signal(1, signal_handler);
    kill(atoi(argv[1]), 1); //Sending signal to the server by its ID (argv[1])
    pause();
    return 0;
}

int tryOpenServerFile()
{
    srand(time(0));

    for (int i = 0; i < 10; i++)
    {
        int serverFd = open("toServer.txt", O_WRONLY | O_CREAT | S_IWUSR);
        if (serverFd < 0)
        {
            int sleepTime = (rand() % 5) + 1;
            printf("Client%s - Sleep time: %d\n", clientPID, sleepTime);
            sleep(sleepTime);
        }
        else
        {
            return serverFd;
        }
    }

    printf("Client%s\n", clientPID);
    perror("Falied several times to open the server file");
    _exit(1);
}

int readLine(int fd, char* buffer)
{
    int i = 0;
    int countReaded = read(fd, &buffer[i], 1);

    while (countReaded == 1)
    {
        if (buffer[i] == '\n')
        {
            buffer[i] = '\0';
            return 0;
        }

        i++;
        countReaded = read(fd, &buffer[i], 1);
    }

    if (countReaded == -1)
    {
        printf("Client%s\n", clientPID);
        perror("Failed reading from file");
        close(fd);
        _exit(1);
    }

    //Reached EOF
    buffer[i] = '\0';
    return 1;
}

void writeLine(int fd, char* data)
{
    if (write(fd, data, strlen(data)) == -1 || write(fd, "\n", 1) == -1)
    {
        printf("Client%s\n", clientPID);
        perror("Failed writing to server file");
        close(fd);
        _exit(1);
    }
}

void signal_handler(int sig)
{
    char clientFileName[50] = "toClient";
    strcat(clientFileName, clientPID);
    strcat(clientFileName, ".txt");

    int clientFd = open(clientFileName, O_RDONLY);
    if (clientFd < 0)
    {
        printf("Client%s\n", clientPID);
        perror("Failed opening client file");
        _exit(1);
    }

    char solution[50];
    readLine(clientFd, solution);
    close(clientFd);
    printf("Client%s\nSolution:%s\n", clientPID, solution);
    deleteFile(clientFileName);
}

void deleteFile(char* fileName)
{
    int pid = fork();
    if (pid < 0)
    {
        printf("Client%s\n", clientPID);
        perror("Falied to fork");
        _exit(1);
    }
    if (pid == 0)
    {
        char* rmParameters[3] = { "rm", fileName, NULL };
        exit(execvp("rm", rmParameters));
    }
    else
    {
        int status;
        wait(&status);
        if (status == 1)
        {
            printf("Client%s\n", clientPID);
            perror("Failed to remove the file");
            _exit(1);
        }
    }
}