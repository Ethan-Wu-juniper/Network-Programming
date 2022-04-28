#include<iostream>
#include<sys/socket.h>
#include<sys/select.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<sys/stat.h> 
using namespace std;

#define MAXLINE 1024

void load(char buf[MAXLINE],string s,int flag)
{
    int i;
    memset(buf,0,MAXLINE);
    for(i=0;i<s.size();i++)
        buf[i] = s[i];
    if(flag)
        buf[MAXLINE-1] = flag;
}

void str_cli(FILE *fp, int sockfd)
{
	int			maxfdp1, val, stdineof;
	int 		wfileno,fpno;
	ssize_t		n, nwritten;
	fd_set		rset, wset;
	char		to[MAXLINE], fr[MAXLINE], buf[MAXLINE];
	char		*toiptr, *tooptr, *friptr, *froptr;
	FILE *wfile = NULL;
	memset(to,0,sizeof(to));
	memset(fr,0,sizeof(fr));
	memset(buf,0,sizeof(buf));

	val = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, val | O_NONBLOCK);

	val = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, val | O_NONBLOCK);

	if(fpno > 0)
	{
		val = fcntl(fpno, F_GETFL, 0);
		fcntl(fpno, F_SETFL, val | O_NONBLOCK);
	}

	val = fcntl(STDOUT_FILENO, F_GETFL, 0);
	fcntl(STDOUT_FILENO, F_SETFL, val | O_NONBLOCK);

	for ( ; ; ) {
		memset(to,0,sizeof(to));
		memset(fr,0,sizeof(fr));
		memset(buf,0,sizeof(buf));
		toiptr = tooptr = to;	/* initialize buffer pointers */
		friptr = froptr = fr;
		stdineof = 0;
		if(wfile != NULL)
			wfileno = fileno(wfile);
		else
			wfileno = -1;
		if(fp != NULL)
			fpno = fileno(fp);
		else
			fpno = -1;
		maxfdp1 = max(max(fpno, STDOUT_FILENO), sockfd) + 1;
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		//if (stdineof == 0 && toiptr < &to[MAXLINE])
		FD_SET(STDIN_FILENO, &rset);	/* read from stdin */
		//if (friptr < &fr[MAXLINE])
		FD_SET(sockfd, &rset);			/* read from socket */
		if (tooptr != toiptr)
			FD_SET(sockfd, &wset);			/* data to write to socket */
		if (froptr != friptr)
			FD_SET(STDOUT_FILENO, &wset);	/* data to write to stdout */
		if(fpno > 0)
			FD_SET(fpno, &rset);
		
//cout << "before select" << endl;
		select(maxfdp1, &rset, &wset, NULL, NULL);
//cout << "OK0" << endl;
/* end nonb1 */
/* include nonb2 */
		if (FD_ISSET(STDIN_FILENO, &rset)) {
			//cout << "read stdin" << endl;
			string cmd;
			getline(cin,cmd);
			if(strcmp(cmd.substr(0,4).c_str(),"/put") == 0)
			{
				string fname = cmd.substr(5,cmd.size()-5);
				cout << "[Upload] " << fname << " Start!" << endl;
				cout << "Progress : [######################]" << endl;
				cout << "[Upload] " << fname << " Finish!" << endl;
				fp = fopen(fname.c_str(),"r");
				load(buf,fname,2);
				write(sockfd, buf, MAXLINE);
				struct stat Stat;
				stat(fname.c_str(),&Stat);
				int file_size = Stat.st_size;
				memset(buf,0,sizeof(buf));
				sprintf(buf,"%d",file_size);
				buf[MAXLINE-1] = 3;
				write(sockfd,buf,MAXLINE);
				//cout << "file name : " << fname << endl;
				//cout << "size : " << file_size << endl;
			}
			if(strcmp(cmd.substr(0,6).c_str(),"/sleep") == 0)
			{
				string time_str = cmd.substr(7,cmd.size()-7);
				int time = atoi(time_str.c_str());
				int i;
				cout << "The client starts to sleep." << endl;
				for(i=0;i<time;i++)
				{
					cout << "Sleep " << i + 1 << endl;
					sleep(1);
				}
				cout << "Client wakes up." << endl;
			}
			if(strcmp(cmd.substr(0,5).c_str(),"/exit") == 0)
			{
				close(sockfd);
				return;
			}
		}
		if (fpno > 0 && FD_ISSET(fpno, &rset)) {
			//cout << "read fp" << endl;
			if ( (n = read(fpno, toiptr, &to[MAXLINE] - toiptr)) < 0) {
				if (errno != EWOULDBLOCK)
				{
					perror("read error on fp");
					sleep(1);
				}

			} else if (n == 0) {
				//cout << "- file eof -" << endl;
				//char endbuf[MAXLINE];
				//string stop = "stop";
				//load(endbuf,stop,3);
				//write(sockfd,endbuf,MAXLINE);
				stdineof = 1;
				FD_CLR(fpno, &rset);
				fclose(fp);			
				fp = NULL;
			} else {
				toiptr += n;			
				FD_SET(sockfd, &wset);
			}
		}
//cout << "OK1" << endl;
		if (FD_ISSET(sockfd, &rset)) {
			//cout << "read sockfd" << endl;
			if ( (n = read(sockfd, friptr, &fr[MAXLINE] - friptr)) < 0) {
				if (errno != EWOULDBLOCK)
					perror("read error on socket");

			} else if (n == 0) {
				if (wfileno > 0)
				{
					//cout << "- wfile closed -" << endl;
					FD_CLR(wfileno,&wset);
					fclose(wfile);
					wfile = NULL;
				}	
				else
					perror("str_cli: server terminated prematurely");
				return;
			} else {
				//cout << "n : " << n << endl;
				/*if(n == 1)
				{
					cout << "hehe" << endl;
				}*/
				if(friptr[MAXLINE-1] == 2)
				{
					string wfname(friptr);
					cout << "[Download] " << wfname << " Start!" << endl;
					cout << "Progress : [######################]" << endl;
					cout << "[Download] " << wfname << " Finish!" << endl;
					//wfname += "00";
					wfile = fopen(wfname.c_str(),"w");
					FD_SET(wfileno, &wset);
					wfileno = fileno(wfile);
					//cout << "wfile fileno : " << wfileno << endl;
				}
				else
				{
					string size_cnt(friptr);
					//cout << "size cnt : " << size_cnt.size() << endl;
					//cout << size_cnt << endl;
					write(wfileno, friptr, size_cnt.size());
				}
				friptr += n;		/* # just read */
				FD_SET(STDOUT_FILENO, &wset);	/* try and write below */
			}
		}
/* end nonb2 */
/* include nonb3 */
//cout << "OK2" << endl;
		/*if (FD_ISSET(STDOUT_FILENO, &wset) && ( (n = friptr - froptr) > 0)) {
			//cout << "write stdout" << endl;
			if ( (nwritten = write(STDOUT_FILENO, froptr, n)) < 0) {
				if (errno != EWOULDBLOCK)
					perror("write error to stdout");

			} else {
				cout << endl;
				froptr += nwritten;		
				if (froptr == friptr)
					froptr = friptr = fr;
			}
		}*/
//cout << "OK3" << endl;
		/*if (wfileno > 0 && FD_ISSET(wfileno, &wset)) {
			cout << "write wfile" << endl;
			if(friptr[MAXLINE-1] != 2)
			{
				if ( (nwritten = write(wfileno, froptr, n)) < 0) {
					if (errno != EWOULDBLOCK)
						perror("write error to stdout");

				} else {
					froptr += nwritten;		
					if (froptr == friptr)
						froptr = friptr = fr;
				}
			}
		}*/

		if (FD_ISSET(sockfd, &wset) && ( (n = toiptr - tooptr) > 0)) {
			//cout << "write sockfd" << endl;
			if ( (nwritten = write(sockfd, tooptr, n)) < 0) {
				if (errno != EWOULDBLOCK)
					perror("write error to socket");

			} else {
				tooptr += nwritten;	/* # just written */
				if (tooptr == toiptr) {
					toiptr = tooptr = to;	/* back to beginning of buffer */
					if (stdineof)
						shutdown(sockfd, SHUT_WR);	/* send FIN */
				}
			}
		}
		//cout << "[loop end]" << endl;
	}
}
/* end nonb3 */

int main(int argc, char *argv[])
{
	if(argc < 4)
    {
        printf("Usage: ./client <ip> <port> <username>.\n");
        exit(-1);
    }

	int clifd = socket(AF_INET, SOCK_STREAM, 0);
	string cmd;
	FILE *file = NULL;
	char buf[MAXLINE];

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
	string name = string(argv[3]);
	load(buf,name,1);
	write(clifd, buf, MAXLINE);
	cout << "Welcome to the dropbox-like server: " + name << endl;
//cout << "OK" << endl;
	str_cli(file,clifd);
	return 0;
}