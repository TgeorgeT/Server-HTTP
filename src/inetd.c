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
#include <fcntl.h>
#include <signal.h>

int passiveTCP(const char *service, int qlen);
 
char pid_file_path[100]; 
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
	{"http", -1, "8080", "./http"},
	{NULL,-1,NULL,NULL}
};
 
void* reverse (char *v){
      char *str = (char *) v, *s = str, *f = str + strlen(str) - 1;
      while (s < f){
            *s ^= *f;
            *f ^= *s;
            *s ^= *f;
            s++;
            f--;
      }
      return str;
}

 
void itoa(int n, char *s){
    int i = 0;
    // generate digits in reverse order
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0); 
 
    s[i] = '\0';
    reverse(s);
}
 
 
 int write_da_pid_to_file(){
    //create_dir_if_not_exists(PID_FILE_PATH);
    // get pid and convert to char*
    char* pidstring = malloc(10);
    int pid = getpid();
    itoa(pid, pidstring);
    // TODO: use write_to_file function instead when implemented
    int fd = open(pid_file_path, O_CREAT|O_TRUNC|O_WRONLY, S_IRWXU|S_IRWXG|S_IRWXO);
    if (fd < 0){
        perror("Could not create da pid file");
		return -1;
    }
	char buf[10];
	sprintf(buf, "%s\n", pidstring);
    write(fd, buf, strlen(buf)); // write pid to file
    close(fd);
}
 
 
void sighup_handler(){
	syslog(LOG_USER | LOG_NOTICE, "got signal");
}


void read_conf(){
	FILE * f = fopen("/home/george/Server/conf.txt", "r");
	char first_line[100];
	fgets(first_line,sizeof(first_line),f);
	char *pid_file = strtok(first_line, ":");
	pid_file = strtok(NULL, ":");
	pid_file[strlen(pid_file)-1]='\0';
	strcpy(pid_file_path, pid_file);
}

int main(int argc, char *argv[])
{
	read_conf();
	struct sockaddr_in fsin; /* the from address of a client */
	int alen; /* from-address length */
	fd_set rfds, afds; /* read file descriptor set */
	int fd, nfds;
	syslog(LOG_USER | LOG_NOTICE, "server started");
	struct service *svp;
	
	pid_t pid;
	if((pid = fork()) < 0)
		exit(1);
	else if(pid)
		_exit(0);	// parent terminates

	/* child 1 continues ... */

	if(setsid() < 0)	// become session leader
		exit(1);

	signal(SIGHUP, SIG_IGN);
	if((pid = fork()) < 0)
		exit(1);
	else if(pid)
		_exit(0);	// child 1 terminates
	

	if(write_da_pid_to_file()<0){
		return 1;
	}

	struct sigaction act;
    act.sa_handler = &sighup_handler;
    sigfillset(&act.sa_mask);
    act.sa_flags=SA_RESTART;
    if(sigaction(SIGHUP, &act, NULL)==-1){
        perror("sigaction");
        exit(1);
    }
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