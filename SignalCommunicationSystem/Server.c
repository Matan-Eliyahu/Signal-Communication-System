#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

void signal_handler(int sig);
int openServerFile();
void readFromServerFile(int severFd, char* clientPID, int* firstNumber, int* calcCode, int* secondNumber);
int readLine(int fd, char* buffer);
void deleteFile(char* fileName);
int createClientFile(char* clientPID);
int calcSolution(int firstNumber, int calcCode, int secondNumber);
void writeLine(int fd, char* data);

int main(int argc, char* argv[])
{
    signal(1, signal_handler);

    while (1)
        pause();
}

void signal_handler(int sig)
{
    int serverFd = openServerFile();

    char clientPID[50];
    int firstNumber, calcCode, secondNumber;
    readFromServerFile(serverFd, clientPID, &firstNumber, &calcCode, &secondNumber);

    close(serverFd);
    deleteFile("toServer.txt");

    int solution = calcSolution(firstNumber, calcCode, secondNumber);
    char solutionToWrite[50];
    sprintf(solutionToWrite, "%d", solution);

    int clientFd = createClientFile(clientPID);

    writeLine(clientFd, solutionToWrite);
    close(clientFd);

    kill(atoi(clientPID), 1);
    signal(1, signal_handler);
}

int openServerFile()
{
    int serverFd = open("toServer.txt", O_RDONLY);
    if (serverFd < 0)
    {
        perror("Server falied to open server file");
        _exit(1);
    }

    return serverFd;
}

void readFromServerFile(int serverFd, char* clientPID, int* firstNumber, int* calcCode, int* secondNumber)
{
    char buffer[50];
    //Read the client pid
    readLine(serverFd, clientPID);

    //Read the first number
    readLine(serverFd, buffer);
    *firstNumber = atoi(buffer);
    memset(buffer, 0, 50);

    //Read the calculation code
    readLine(serverFd, buffer);
    *calcCode = atoi(buffer);
    memset(buffer, 0, 50);
    //Read the second number
    readLine(serverFd, buffer);
    *secondNumber = atoi(buffer);
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
        perror("Server failed reading from file");
        close(fd);
        _exit(1);
    }

    //Reached EOF
    buffer[i] = '\0';
    return 1;
}

void deleteFile(char* fileName)
{
    int pid = fork();
    if (pid < 0)
    {
        perror("Server falied to fork");
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
            perror("Server failed to remove the file");
            _exit(1);
        }
    }

}

int createClientFile(char* clientPID)
{
    char clientFileName[50] = "toClient";
    strcat(clientFileName, clientPID);
    strcat(clientFileName, ".txt");

    int clientFd = open(clientFileName, O_WRONLY | O_CREAT, 0666);
    if (clientFd < 0)
    {
        perror("Server falied to create client file");
        _exit(1);
    }

    return clientFd;
}

int calcSolution(int firstNumber, int calcCode, int secondNumber)
{
    switch (calcCode)
    {
    case 1:
        return firstNumber + secondNumber;
    case 2:
        return firstNumber - secondNumber;
    case 3:
        return firstNumber * secondNumber;
    case 4:
        if (secondNumber == 0)
        {
            perror("Division by zero");
            _exit(1);
        }

        return firstNumber / secondNumber;
    default:
        perror("Wrong calculation code (1 for +, 2 for -, 3 for *, 4 for /)");
        _exit(1);
    }
}

void writeLine(int fd, char* data)
{
    if (write(fd, data, strlen(data)) == -1)
    {
        perror("Server failed to write to client file");
        close(fd);
        _exit(1);
    }
}