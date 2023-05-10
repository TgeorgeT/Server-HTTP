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
#include <fcntl.h>
#include <sys/stat.h>


int main(int argc, char *argv[])
{
    struct sockaddr_in fsin; /* the from address of a client */
    char *service = "echo"; /* service name or port number */
    char buf[4096];
    int cc, nfds;
    fd_set rfds;
    struct timeval tv = {1, 0};

    openlog(argv[0], LOG_PID, 0);    
    syslog(LOG_USER|LOG_NOTICE,"starting http-echo service on port %d\n", atoi(argv[1]));

    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    nfds = getdtablesize();
    
    
    
    sprintf(buf, "HTTP/1.1 200 OK\n");
    write(1, buf, strlen(buf));
    sprintf(buf, "Content-Type: text/html; charset=utf-8\n");
    write(1, buf, strlen(buf));
    while (1)
    {    
        if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0, &tv) < 0)
            syslog(LOG_USER|LOG_NOTICE,"select failed: %m\n"), exit(1);

        if (FD_ISSET(0, &rfds))
        {    
            int fd;
            if((fd = open("index.html", O_RDONLY)) == -1){
                syslog(LOG_USER|LOG_NOTICE,"open failed: %m\n"), exit(1);
            }
            struct stat sb;
            syslog(LOG_USER|LOG_NOTICE,"open success: %m\n");
            if (fstat(fd, &sb) == -1) {
                           syslog(LOG_USER|LOG_NOTICE,"fstat failed: %m\n"), exit(1);
                           exit(1);
                   }
            sprintf(buf, "Content-Length: %d\n\n", (int)sb.st_size);
            write(1, buf, strlen(buf));

            read(fd, buf, (int)sb.st_size);
            write(1, buf, (int)sb.st_size);
            FD_SET(0, &rfds);
        }
        else
            break;        
    }

    close(0);
    close(1);
    close(2);
    exit(0);
}