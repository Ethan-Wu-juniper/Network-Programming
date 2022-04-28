#include<iostream>
#include<sys/socket.h>
#include<sys/select.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include<netinet/in.h>
using namespace std;

#define MAXLINE 1024

void str_cli(FILE *fp,int sockfd)
{
	int maxfdp1;
	fd_set rset;
	char sendline[MAXLINE],recvline[MAXLINE];

	FD_ZERO(&rset);
	for(;;)
	{
		FD_SET(fileno(fp),&rset);
		FD_SET(sockfd,&rset);
		maxfdp1 = max(fileno(fp),sockfd) + 1;
		select(maxfdp1,&rset,NULL,NULL,NULL);
		memset(sendline,0,sizeof(sendline));
		memset(recvline,0,sizeof(recvline));

		if(FD_ISSET(sockfd,&rset))
		{
			if(recv(sockfd,recvline,MAXLINE,0) == 0)
			{
				return;
			}
			fputs(recvline,stdout);
		}
		if(FD_ISSET(fileno(fp),&rset))
		{
			if(fgets(sendline,MAXLINE,fp) == NULL)
				return;
			if(!strcmp(sendline,"exit\n"))
			{
				close(sockfd);
				return;
			}
			else
				send(sockfd,sendline,strlen(sendline),0);
		}
	}
}

int main(int argc, char *argv[])
{
	int clifd = socket(AF_INET, SOCK_STREAM, 0);

	int port = atoi(argv[2]);
	struct sockaddr_in servaddr;
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);

	if(connect(clifd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
	{
		perror("connect");
		exit(1);
	}

	str_cli(stdin,clifd);
	return 0;
}
