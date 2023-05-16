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

#define WEBROOT "./pagini"









int main(int argc, char *argv[])
{
    struct sockaddr_in fsin; /* the from address of a client */
    char *service = "http"; /* service name or port number */
    char buf[4096];
    int cc, nfds;
    fd_set rfds;
    struct timeval tv = {1, 0};
    unsigned char *ptr, request[500]="\0", resource[500];

    openlog(argv[0], LOG_PID, 0);    
    syslog(LOG_USER|LOG_NOTICE,"starting http-echo service on port %d\n", atoi(argv[1]));

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
            syslog(LOG_USER|LOG_NOTICE,"select failed: \n"), exit(1);

        if (FD_ISSET(0, &rfds))
        {
            char stream[500];
            int fd, length;
            (void *)recv(0, stream, strlen(stream), 0);
            strcat(request,stream);
        }
        else
            break;        
    }


    //sprintf(buf,"%s\r\n\n\n-------------------------------------------------------", request);
    //write(1, buf, strlen(buf));

    ptr = strstr(request, " HTTP/");
    if(ptr ==  NULL){
         syslog(LOG_USER|LOG_NOTICE,"Not a http request\n\n"), exit(1);
    } else {
        *ptr = '\0';
        ptr = NULL;
        if(strncmp(request, "GET ", 4) == 0){
            ptr = request+4;
        //     sprintf(buf,"%s\n", "GET REQUEST");
        //     write(1, buf, strlen(buf));          
         }
        if(strncmp(request, "HEAD ", 5) == 0){
            ptr = request+5;
            // sprintf(buf,"%s\n", "HEAD REQUEST");
            // write(1, buf, strlen(buf));  
        }

        if(ptr == NULL){
            syslog(LOG_USER|LOG_NOTICE,"Unknown request\n\n"), exit(1);
        } else {
            // sprintf(buf,"%s\n", ptr);
            // write(1, buf, strlen(buf)); 
             if(ptr[strlen(ptr) - 1] == '/')
                strcat(ptr, "index.html");
            strcpy(resource,WEBROOT);
            strcat(resource, ptr);
            int fd = open(resource, O_RDONLY, 0);
            // sprintf(buf,"%d\n", fd);
            // write(1, buf, strlen(buf));  
            if(fd == -1) { // If file is not found
                sprintf(buf,"%s","HTTP/1.1 404 NOT FOUND\r\n");
                strcat(buf, "Server: Webserver\r\n\r\n");
                strcat(buf, "<html><head><title>404 Not Found</title></head>");
                strcat(buf, "<body><h1>URL not found</h1></body></html>\r\n");
                write(1, buf, strlen(buf));
            } else{
                sprintf(buf, "%s", "HTTP/1.1 200 OK\n");
                // strcat(buf, "Content-Type: text/html; charset=utf-8\n");
                write(1, buf, strlen(buf));
                if(ptr==request+4){
                    struct stat sb;
                    if (fstat(fd, &sb) == -1) {
                           syslog(LOG_USER|LOG_NOTICE,"fstat failed: %m\n"), exit(1);
                           exit(1);
                   }
                   sprintf(buf, "Content-Length: %d\n\n", (int)sb.st_size);
                    write(1, buf, strlen(buf));

                    read(fd, buf, (int)sb.st_size);
                    write(1, buf, (int)sb.st_size);
                }
                else if(ptr==request+5){
                    struct stat sb;
                    if (fstat(fd, &sb) == -1) {
                           syslog(LOG_USER|LOG_NOTICE,"fstat failed: %m\n"), exit(1);
                           exit(1);
                   }
                   syslog(LOG_USER|LOG_NOTICE,"Got Head request\n\n");
                   sprintf(buf, "Content-Length: %d\n", (int)sb.st_size);
                   strcat(buf, "Content-Type: text/html\n\n");
                   write(1, buf, strlen(buf));
                }
            }
        


        }
    }

    close(0);
    close(1);
    close(2);
    exit(0);
}