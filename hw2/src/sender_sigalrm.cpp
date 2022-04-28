#include<iostream>
#include<fstream>
#include<sys/socket.h>
#include<sys/select.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include<netinet/in.h>
#include<sys/stat.h>
#include<signal.h>
using namespace std;

#define MAXLINE 1024

void handler(int sig)
{
    //cout << endl << "time out" << endl;
    return;
}

int send(int sockfd, struct sockaddr * pservaddr, socklen_t servlen, char *sendline, int sendlen)
{
    int	n;
	char recvline[MAXLINE + 1];
    memset(recvline,0,sizeof(recvline));
    sendto(sockfd, sendline, sendlen, 0, pservaddr, servlen);
    ualarm(100000,0);
    n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
    if(n < 0)
    {
        //cout << "recv : " << n << endl;
        return n;
    }
    recvline[n] = 0;	// null terminate 
    //fputs(recvline, stdout);
    memset(sendline,0,sizeof(sendline));
    memset(recvline,0,sizeof(recvline));
    return n;
}

int main(int argc, char **argv)
{
    signal(SIGALRM,handler);
    siginterrupt(SIGALRM,1);
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

    int	n = 1,m = 1;
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
        while(1)
        {
            m = send(sockfd,(struct sockaddr *) &servaddr,sizeof(servaddr),sendline,n+1);
            if(m > 0) break;
        }
        
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