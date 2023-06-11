#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <syslog.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

char webroot[100];

int open_and_validate(char *path)
{
    int fd = open(path, O_RDONLY, 0);

    // sprintf(buf,"%d\n", fd);
    // write(1, buf, strlen(buf));
    char buf[500];
    if (fd == -1)
    { // If file is not found
        sprintf(buf, "%s", "HTTP/1.1 404 NOT FOUND\r\n");
        strcat(buf, "Server: Webserver\r\n\r\n");
        strcat(buf, "<html><head><title>404 Not Found</title></head>");
        strcat(buf, "<body><h1>URL not found</h1></body></html>\r\n");
        write(1, buf, strlen(buf));
        exit(1);
    }
    return fd;
}

char* get_last_endline(char *c){
    int i;
    for( i=strlen(c)-1;i>=0 && c[i]!='\n';i--);
    return c+i+1;
}


void read_conf(){
	FILE * f = fopen("/home/george/Server/conf.txt", "r");
	char line[100];
	fgets(line,sizeof(line),f);
	fgets(line,sizeof(line),f);
	char *werbroot_conf = strtok(line, ":");
	werbroot_conf = strtok(NULL, ":");
	werbroot_conf[strlen(werbroot_conf)-1]='\0';
	strcpy(webroot, werbroot_conf);
    syslog(LOG_USER | LOG_NOTICE, webroot);
}


int main(int argc, char *argv[])
{
    struct sockaddr_in fsin; /* the from address of a client */
    char *service = "http";  /* service name or port number */
    char buf[4096];
    int cc, nfds;
    fd_set rfds;
    struct timeval tv = {1, 0};
    unsigned char *ptr, request[500] = "\0", resource[500];
    read_conf();

    

    openlog(argv[0], LOG_PID, 0);
    syslog(LOG_USER | LOG_NOTICE, "starting http-echo service on port %d\n", atoi(argv[1]));

    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    nfds = getdtablesize();

    // sprintf(buf, "HTTP/1.1 200 OK\n");
    // write(1, buf, strlen(buf));
    // sprintf(buf, "Content-Type: text/html; charset=utf-8\n\n");
    // write(1, buf, strlen(buf));
    while (1)
    {
        if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0, &tv) < 0)
            syslog(LOG_USER | LOG_NOTICE, "select failed: \n"), exit(1);

        if (FD_ISSET(0, &rfds))
        {
            char stream[500];
            int fd, length;
            (void *)recv(0, stream, strlen(stream), 0);
            strcat(request, stream);
        }
        else
            break;
    }

    // sprintf(buf,"%s\r\n\n\n-------------------------------------------------------", request);
    // write(1, buf, strlen(buf));
    // syslog(LOG_USER|LOG_NOTICE, "asdfdfsdsfdasd");
    syslog(LOG_USER| LOG_NOTICE, request);
    ptr = strstr(request, " HTTP/");
    char original_request[2000];
    strcpy(original_request, request);
    if (ptr == NULL)
    {
        syslog(LOG_USER | LOG_NOTICE, "Not a http request\n\n"), exit(1);
    }
    else
    {
        *ptr = '\0';
        ptr = NULL;
        if (strncmp(request, "GET ", 4) == 0)
        {
            ptr = request + 4;
            //     sprintf(buf,"%s\n", "GET REQUEST");
            //     write(1, buf, strlen(buf));
        }
        else if (strncmp(request, "HEAD ", 5) == 0)
        {
            ptr = request + 5;
            // sprintf(buf,"%s\n", "HEAD REQUEST");
            // write(1, buf, strlen(buf));
        }
        else if(strncmp(request, "POST ", 5)==0){
            ptr = request + 5;
        }
        char *extensie = ptr + strlen(ptr) - 4;
        syslog(LOG_USER| LOG_NOTICE, "extensie");
        if (ptr[strlen(ptr) - 1] == '/')
            strcat(ptr, "index.html");
        strcpy(resource, webroot);
        strcat(resource, ptr);
        syslog(LOG_USER| LOG_NOTICE, extensie);
        if (ptr == NULL)
        {
            syslog(LOG_USER | LOG_NOTICE, "Unknown request\n\n"), exit(1);
        }
        else if (strcmp(extensie, ".php") == 0)
        {
            syslog(LOG_USER|LOG_NOTICE, resource);
            int fd = open_and_validate(resource);
            sprintf(buf, "%s", "HTTP/1.1 200 OK\n");
            write(1, buf, strlen(buf));
            sprintf(buf, "Content-Length: %d\n", 100000);
            strcat(buf, "Content-Type: text/html\n\n");
            write(1, buf, strlen(buf));
            char* last_endl=get_last_endline(original_request);
        
            execl("/usr/bin/php", "usr/bin/php", "/home/george/Server/src/preprocess.php",last_endl, resource, NULL);
            exit(1);
        }
        else
        {

            syslog(LOG_USER | LOG_NOTICE, "got here");

            int fd = open_and_validate(resource);
            
                syslog(LOG_USER | LOG_NOTICE, "open succesful");
                sprintf(buf, "%s", "HTTP/1.1 200 OK\n");
                // strcat(buf, "Content-Type: text/html; charset=utf-8\n");
                write(1, buf, strlen(buf));
                syslog(LOG_USER | LOG_NOTICE, request);
                memcpy(buf, request, 4);
                syslog(LOG_USER| LOG_NOTICE, buf);
                if (strncmp(request, "GET", 3)==0)
                {
                    struct stat sb;
                    if (fstat(fd, &sb) == -1)
                    {
                        syslog(LOG_USER | LOG_NOTICE, "fstat failed: %m\n"), exit(1);
                        exit(1);
                    }
                    sprintf(buf, "Content-Length: %d\n\n", (int)sb.st_size);
                    write(1, buf, strlen(buf));
                    read(fd, buf, (int)sb.st_size);
                    write(1, buf, (int)sb.st_size);
                }
                else if (strncmp(request, "HEAD", 4)==0)
                {
                    struct stat sb;
                    if (fstat(fd, &sb) == -1)
                    {
                        syslog(LOG_USER | LOG_NOTICE, "fstat failed: %m\n"), exit(1);
                        exit(1);
                    }
                    syslog(LOG_USER | LOG_NOTICE, "Got Head request\n\n");
                    sprintf(buf, "Content-Length: %d\n", (int)sb.st_size);
                    strcat(buf, "Content-Type: text/html\n\n");
                    write(1, buf, strlen(buf));
                }
                
            }
    
    }

    close(0);
    close(1);
    close(2);
    exit(0);
}

