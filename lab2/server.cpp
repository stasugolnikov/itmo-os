#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#include "wrappers.h"

void split(std::vector<char *> &strs, char str[]) {
    char *substr = strtok(str, " ");
    while (substr) {
        strs.push_back(substr);
        substr = strtok(NULL, " ");
    }
}

void run_process_background(std::vector<char *> &strs, int &code) {
    int fd_stdout[2];
    int fd_stderr[2];
    int fd_code[2];

    if (pipe(fd_stdout) == -1 || pipe(fd_stderr) == -1 || pipe(fd_code) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    pid_t pid_child = fork();

    if (pid_child == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (!pid_child) {
        close(fd_stdout[0]);
        close(fd_stderr[0]);
        close(fd_code[0]);
        dup2(fd_stdout[1], STDOUT_FILENO);
        dup2(fd_stderr[1], STDERR_FILENO);

        pid_t pid_grandchild = fork();
        if (!pid_grandchild) {
            uid_t uid = std::stoi(strs.front());
            Setuid(uid);
            char **args = &strs[2];
            execv(args[0], args);
        } else {
            int wstatus;
            wait(&wstatus);
            int exit_code = WEXITSTATUS(wstatus);
            write(fd_code[1], &exit_code, 8);
            close(fd_stdout[1]);
        }
    } else {
        close(fd_stdout[1]);

        read(fd_code[0], &code, 8);

        int log_file = open("file.log", O_WRONLY | O_CREAT, 0777);

        ssize_t nread;
        char buf[1000];
        while ((nread = read(fd_stdout[0], buf, sizeof(buf)))) {
            write(log_file, buf, nread);
        }
        close(fd_stdout[0]);
    }
}

void run_process_foreground(std::vector<char *> &strs, std::string &out, int &code) {
    int fd_stdout[2];
    int fd_stderr[2];

    if (pipe(fd_stdout) == -1 || pipe(fd_stderr) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (!pid) {
        close(fd_stdout[0]);

        uid_t uid = std::stoi(strs.front());
        Setuid(uid);
        char **args = &strs[2];

        dup2(fd_stdout[1], STDOUT_FILENO);
        dup2(fd_stderr[1], STDERR_FILENO);

        execv(args[0], args);
        close(fd_stdout[1]);
    } else {
        close(fd_stdout[1]);
        int wstatus;
        wait(&wstatus);
        code = WEXITSTATUS(wstatus);
        ssize_t nread;
        char buf[1000];
        while ((nread = read(fd_stdout[0], buf, sizeof(buf)))) {
            out += buf;
        }
        close(fd_stdout[0]);
    }
}
void background() {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        setsid();
    } else {
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    background();
    int server = Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in adr = {0};
    adr.sin_family = AF_INET;
    adr.sin_port = htons(34546);

    Bind(server, (struct sockaddr *)&adr, sizeof(adr));

    Listen(server, 5);

    socklen_t adrlen = sizeof(adr);
    int sock = Accept(server, (struct sockaddr *)&adr, &adrlen);

    char buf[256];

    if (read(sock, buf, 256) == -1) {
        exit(1);
    }

    std::vector<char *> strs;
    split(strs, buf);
    strs.push_back(NULL);

    if (strcmp(strs[1], "foreground") == 0) {
        std::string out;
        int code;
        run_process_foreground(strs, out, code);
        write(sock, out.c_str(), 1000);
        write(sock, &code, 8);
        return 0;
    }

    if (strcmp(strs[1], "background") == 0) {
        int code;
        run_process_background(strs, code);
        write(sock, &code, 8);
        return 0;
    }

    close(sock);
    close(server);
    return 0;
}