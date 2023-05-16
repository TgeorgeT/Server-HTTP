#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int passiveTCP(const char *service, int qlen);
 
#define QLEN 5
 
struct service
{
	char *name;
	int sv_sock;
	char *port;
	char *sv_prog;
};
 
struct service servers[] =
{
	{"http-echo-inetd", -1, "8002", "./http-echo-inetd"},
	{"http-inetd", -1, "8001", "./http-inetd"},
	{"http", -1, "8000", "./http"},
	{NULL,-1,NULL,NULL}
};
 
 
int main(int argc, char *argv[])
{
	struct sockaddr_in fsin; /* the from address of a client */
	int alen; /* from-address length */
	fd_set rfds, afds; /* read file descriptor set */
	int fd, nfds;
 
	struct service *svp;
 
	nfds = getdtablesize();
	FD_ZERO(&rfds);
    FD_ZERO(&afds);
	for(svp = &servers[0]; svp->name; svp++)
	{
		svp->sv_sock = passiveTCP(svp->port, QLEN);
		FD_SET(svp->sv_sock, &afds);
	}
 
	while (1) {
 
		memcpy(&rfds, &afds, sizeof(afds));
 
		if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0)
			printf("select failed: %s\n", strerror(errno)), exit(1);
 
		for(svp = &servers[0]; svp->name; svp++)
			if (FD_ISSET(svp->sv_sock, &rfds))
			{
				int ssock;
 
				alen = sizeof(fsin);
				ssock = accept(svp->sv_sock, (struct sockaddr *)&fsin, &alen);
				syslog(LOG_USER|LOG_NOTICE,"Got request from %s:%d \n", inet_ntoa(fsin.sin_addr),
                ntohs(fsin.sin_port));
				if (ssock < 0)
					printf("accept failed: %s\n", strerror(errno)), exit(1);
				if(!fork())
				{
					for(int j = 0; j < nfds; j++)
						if(j != ssock)
							close(j);
					dup(ssock);
					dup(ssock);
					dup(ssock);
					close(ssock);
					setuid(getuid());
					setgid(getgid());

					execl(svp->sv_prog, svp->sv_prog, svp->port, NULL);
					exit(1);
				}
				close(ssock);
				//FD_SET(svp->sv_sock, &rfds);
			}

        
        FD_ZERO(&rfds);
 
	}
}