#include <sys/types.h>
#include <sys/socket.h>
#include "wrappers.h"
#include <netinet/in.h>
#include <arpa/inet.h>


int main(int argc, char* argv[]) {   
    int fd = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in adr = {0};
    adr.sin_family = AF_INET;
    adr.sin_port = htons(34543);
    Connect(fd, (struct sockaddr*) &adr, sizeof(adr));
    write(fd, argv[1], 6);
    char buf[256];
    int num_read = read(fd, buf, 256);
    write(fd, buf, 256);
    close(fd);
    return 0;
}
