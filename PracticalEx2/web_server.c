#define PORT 6789
#define MAXREQ (4096 * 1024)

#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>

char buffer[MAXREQ], body[MAXREQ], msg[MAXREQ], html_buffer[MAXREQ], path[MAXREQ];

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void set_path(char *raw_request)
{
    strcpy(path, raw_request);
    char *token;
    token = strtok(path, " ");
    token = strtok(NULL, " ");
    sprintf(path,"%s%s",".",token);
}

int read_file(char *filename)
{
    char * buffer = 0;
    long length;
    FILE *f = fopen(filename, "rb");

    if (f)
    {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = malloc(length);
        if (buffer)
        {
            fread(buffer, 1, length, f);
        }
        fclose(f);
        if(buffer){
            strcpy(html_buffer, buffer);
            return 1;
        }

    }
    strcpy(html_buffer, "<html><body><h1>404 Page Not Found</h1></body></html>");
    return 0;
}

int init_producer() {
    int sockfd;
    struct sockaddr_in6 serv_addr;
    sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
        error("setsockopt(SO_REUSEADDR) failed");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_addr = in6addr_any;
    serv_addr.sin6_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
    {
        error("ERROR on binding");
    }
    return sockfd;
}

int main()
{
    int sockfd = init_producer();
    int n;
    int newsockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr;
    listen(sockfd, 5);
    while (1)
    {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr,
                           &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");
        bzero(buffer, sizeof(buffer));
        n = read(newsockfd, buffer, sizeof(buffer) - 1);
        if (n < 0)
            error("ERROR reading from socket");
        set_path(buffer);
        read_file(path);
        snprintf(body, sizeof(body),
                 "%s",
                 html_buffer);
        snprintf(msg, sizeof(msg),
                 "HTTP/0.9 200 OK\n"
                 "Content-Type: text/html\n"
                 "Content-Length: %ld\n\n%s",
                 strlen(body), body);        
        n = write(newsockfd, msg, strlen(msg));
        if (n < 0)
            error("ERROR writing to socket");
        close(newsockfd);
    }
}