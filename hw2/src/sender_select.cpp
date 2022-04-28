#include<iostream>
#include<fstream>
#include<sys/socket.h>
#include<sys/select.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include<netinet/in.h>
#include <sys/stat.h> 
using namespace std;

#define MAXLINE 1024

void send(int sockfd, struct sockaddr * pservaddr, socklen_t servlen, char *sendline, int sendlen)
{
    int	n;
	char recvline[MAXLINE + 1];
    memset(recvline,0,sizeof(recvline));
    while(1)
    {
        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(sockfd,&rset);

        int maxfdp1 = sockfd + 1;
        timeval time_to_wait;
        time_to_wait.tv_sec = 0;
        time_to_wait.tv_usec = 100000;
        sendto(sockfd, sendline, sendlen, 0, pservaddr, servlen);
        int status = select(maxfdp1,&rset,NULL,NULL,&time_to_wait);
        if(status == 0)
        {
            //cout << "Time out!" << endl;
            continue;
        }
        else break;
    }
    n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
    recvline[n] = 0;	// null terminate 
    //fputs(recvline, stdout);
    memset(sendline,0,sizeof(sendline));
    memset(recvline,0,sizeof(recvline));
}

int main(int argc, char **argv)
{
	if (argc != 4)
    {
		cout << "usage: ./sender_select [send filename] [target address] [connect port]" << endl;
        return 1;
    }
	int					sockfd;
	struct sockaddr_in	servaddr;
    int port = atoi(argv[3]);
    int check = 0;
	char sendline[MAXLINE];
    string sendbuff;
    string fname(argv[1]);
    memset(sendline,0,sizeof(sendline));

    FILE *file;
    file = fopen(fname.c_str(),"rb");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    int	n = 1;
    struct sockaddr * pservaddr = (struct sockaddr *) &servaddr;
    socklen_t servlen = sizeof(servaddr);
    /**** transfer file size ****/
    struct stat Stat;
    stat(fname.c_str(),&Stat);
    int file_size = Stat.st_size;
    sprintf(sendline,"%d",file_size);
    sendline[strlen(sendline)] = check;
    check ^= 1;
    send(sockfd,(struct sockaddr *) &servaddr,sizeof(servaddr),sendline,sizeof(sendline));
    cout << "file size : " << file_size << endl;
	while (1)
    {
        n = fread(sendline,1,MAXLINE-1,file);
        //cout << " n : " << n << " || ";
        //fputs(sendline,stdout);
        sendline[n] = check;
        check ^= 1;
        send(sockfd,(struct sockaddr *) &servaddr,sizeof(servaddr),sendline,n+1);
        cout << "complete : " << ((float)ftell(file) / (float)file_size) * 100 << "%" << endl;
        if(ftell(file) == file_size)
        {
            cout << " - file end - " << endl;
            break;
        }
        else
            cout << "\x1B[1F\x1B[0K";
	}
    
	fclose(file);
    return 0;
}