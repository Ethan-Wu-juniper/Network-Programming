#include<iostream>
#include<fstream>
#include<sys/socket.h>
#include<sys/select.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include<netinet/in.h>
using namespace std;

#define MAXLINE 1024

int main(int argc, char **argv)
{
	if(argc != 3)
	{
		cout << "usage : ./receiver_{sockopt|select|sigalrm} [save filename] [bind port]" << endl;
		return 0;
	}
	char *fname = argv[1];
	int port = atoi(argv[2]);
	bool first = true;
	int total_size = 0,recv_size = 0,check=1;
	FILE *file;
	file = fopen(fname,"wb");
	int					sockfd;
	struct sockaddr_in	servaddr, cliaddr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(port);

	bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	int			n;
	socklen_t	len;
	char		mesg[MAXLINE+1];
	string ack = "ack  ";
	struct sockaddr *pcliaddr = (struct sockaddr *) &cliaddr;
	socklen_t clilen = sizeof(cliaddr);

	while(1)
	{
		len = clilen;
		n = recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
		sendto(sockfd, ack.c_str(), 3, 0, pcliaddr, len);
		//cout << "check : " << (int)mesg[n-1] << endl;
		if(check == mesg[n-1]) continue;
		else check = mesg[n-1];
		mesg[n] = 0;	// null terminate
		string temp(mesg);
		if(first && temp.size() != 0)
		{
			total_size = atoi(temp.c_str());
			first = false;
			cout << "total size : " << total_size << endl;
			continue;
		}
		//char mesg_cpy[];
		//strncpy(mesg,mesg,n-1);
		fwrite(mesg,1,n-1,file);
		cout << "ftell : " << ftell(file) << endl;
		if(ftell(file) == total_size) break;
	}

	fclose(file);
	return 0;
}