#include<iostream>
#include<sys/socket.h>
#include<sys/select.h>
#include<sys/types.h> 
#include<sys/stat.h>
#include<sys/time.h>
#include<string.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<vector>
#include<map>
#include<algorithm>
using namespace std;

#define LISTENQ 10
#define FD_SIZE 10
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

int main(int argc, char *argv[])
{
	FILE *file[FD_SIZE];
	int port = atoi(argv[1]);
	int i,j, maxi, maxfd, listenfd, connfd, sockfd, nready, n;
	int client[FD_SIZE];
	map<string,vector<string>> file_info;
	map<string,int> file_size;
	map<string,int> cur_size;
	char buf[MAXLINE];
	string client_datas[FD_SIZE];
	string names[FD_SIZE];
	string cur_file[FD_SIZE];
	socklen_t clilen;
	struct sockaddr_in cliaddr;
	struct sockaddr_in servaddr;
	fd_set rset, allset;
	memset(client_datas,0,sizeof(client_datas));
	memset(names,0,sizeof(names));
	memset(cur_file,0,sizeof(cur_file));

	listenfd = socket(AF_INET,SOCK_STREAM,0);
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	int flags = fcntl(listenfd, F_GETFL, 0);
	fcntl(listenfd, F_SETFL, flags | O_NONBLOCK);

	if(bind(listenfd,(struct sockaddr*) &servaddr,sizeof(servaddr)) < 0)
	{
		perror("bind");
		exit(1);
	}
	if(listen(listenfd,LISTENQ) < 0)
	{
		perror("listen");
		exit(1);
	}
	maxfd = listenfd;
	maxi = -1;
	for(i=0;i < FD_SIZE;i++)
		client[i] = -1;
	FD_ZERO(&allset);
	FD_SET(listenfd,&allset);


	for(;;)
	{
		rset = allset;
		nready = select(maxfd+1,&rset,NULL,NULL,NULL);
		if(nready < 0)
		{
			perror("select");
			exit(1);
		}
		if(FD_ISSET(listenfd,&rset))
		{
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd,(struct sockaddr*) &cliaddr, &clilen);
			if(connfd == -1 && errno == EAGAIN)
			{
				perror("no client connect");
				exit(1);
			}
			else if(connfd == -1)
			{
				perror("accept error");
				exit(-1);
			}
			for(i=0;i < FD_SIZE;i++)
			{
				if(client[i] < 0)
				{
					client[i] = connfd;
					break;
				}
			}
			if(i == FD_SIZE)
				perror("too many clients");
			FD_SET(connfd,&allset);
			if(connfd > maxfd)
				maxfd = connfd;
			if(i > maxi)
				maxi = i;
			if(--nready <= 0)
				continue;
		}
		for(i=0;i <= maxi;i++)
		{
			memset(buf,0,sizeof(buf));
			if((sockfd=client[i]) < 0)
				continue;
			if(FD_ISSET(sockfd,&rset))
			{
				bool error = true;
				n = recv(sockfd,buf,MAXLINE,0);
				//cout << "n : " << n << endl;
				string input(buf);
				/****** shutdown *******/
				if(n == 0)
				{
					close(sockfd);
					FD_CLR(sockfd,&allset);
					client[i] = -1;
					//file_info[names[i]].clear();
					//fclose(file[i]);
				}
				/***********************/
				if(n != 0)
				{
					if(buf[MAXLINE-1] == 1)
					{
						//cout << "name recieved" << endl;
						string name(buf);
						names[i] = name;
						mkdir(name.c_str(),S_IRWXU | S_IRWXG | S_IRWXO);
						file_info[name];
						vector<string>::iterator it;
						cout << "own file num : " << file_info[name].size() << endl;
						for(it=file_info[name].begin();it!=file_info[name].end();it++)
						{
							string filename = name + "/" + *it;
							cout << "file out : " << *it << endl;
							int nread;
							FILE *wfile;
							char rrbuf[MAXLINE];
							memset(rrbuf,0,sizeof(rrbuf));
							load(rrbuf,*it,2);
							send(client[i],rrbuf,MAXLINE,0);
							wfile = fopen(filename.c_str(),"r");
							while(1)
							{
								char rbuf[MAXLINE];
								memset(rbuf,0,sizeof(rbuf));
								nread = read(fileno(wfile),rbuf,MAXLINE);
								cout << "nread : " << nread;
								if(nread == 0)
								{
									char stop[MAXLINE];
									memset(stop,0,sizeof(stop));
									//write(client[i],stop,1);
									fclose(wfile);
									wfile = NULL;
									break;
								}
								else
								{
									//fputs(rbuf,stdout);
								}
								write(client[i],rbuf,MAXLINE);
								//sleep(1);
							}
						}
					}
					else if(buf[MAXLINE-1] == 2)
					{
						cout << "file name recieved" << endl;
						string fname(buf);
						vector<string>::iterator it = find(file_info[names[i]].begin(),file_info[names[i]].end(),fname);
						if(it == file_info[names[i]].end())
							file_info[names[i]].push_back(fname);
						string path = names[i] + "/" + fname;
						file[i] = fopen(path.c_str(),"w");
						cur_file[i] = fname;
						//write(fileno(file[i]),buf,n);
						/*for(j=0;j<=maxi;j++)
						{
							if(client[j] > 0 && i != j && strcmp(names[i].c_str(),names[j].c_str()) == 0)
							{
								//cout << "name is sent to client[" << j << "] : " << names[j] << endl;
								send(client[j],buf,n,0);
							}
						}*/
					}
					else if(buf[MAXLINE-1] == 3)
					{
						int size = atoi(buf);
						cout << "file size : " << size << endl;
						file_size[cur_file[i]] = size;
						cur_size[cur_file[i]] = 0;
					}
					else
					{
						if(cur_size[cur_file[i]] > file_size[cur_file[i]])
						{
							cout << "transfer spilled error!" << endl;
						}
						write(fileno(file[i]),buf,n);
						cur_size[cur_file[i]] += n;
						cout << "n : " << cur_size[cur_file[i]] << endl;
						if(cur_size[cur_file[i]] == file_size[cur_file[i]])
						{
							int j;
							cout << cur_file[i] << " transferred successfully" << endl;
							cur_size[cur_file[i]] = 0;
							for(j=0;j<=maxi;j++)
							{
								if(client[j] > 0 && i != j && strcmp(names[i].c_str(),names[j].c_str()) == 0)
								{
									//cout << "name is sent to client[" << j << "] : " << names[j] << endl;
									FILE *wfile;
									string filename = names[i] + "/" + cur_file[i];
									wfile = fopen(filename.c_str(),"r");
									char namebuf[MAXLINE];
									load(namebuf,cur_file[i],2);
									send(client[j],namebuf,MAXLINE,0);
									while(1)
									{
										int nread;
										char filebuf[MAXLINE];
										memset(filebuf,0,sizeof(filebuf));
										nread = read(fileno(wfile),filebuf,MAXLINE);
										cout << "sent file read" << endl;
										//fputs(filebuf,stdout);
										if(nread == 0)
										{
											cout << "trans end" << endl;
											fclose(wfile);
											wfile = NULL;
											break;
										}
										write(client[j],filebuf,MAXLINE);
									}
								}
							}
							cur_file[i].clear();
							fclose(file[i]);
							file[i] = NULL;
						}
						//fputs(buf,stdout);
						//cout << endl;
						/*for(j=0;j<=maxi;j++)
						{
							if(client[j] > 0 && i != j && strcmp(names[i].c_str(),names[j].c_str()) == 0)
								send(client[j],buf,MAXLINE,0);
						}*/
					}
				}
				/***********************/
				if(n != 0)
				{
					/*cout << "[server]" << endl;
					fputs(buf,stdout);
					cout << endl;
					cout << "-------------------" << endl;*/
				}
				else
				{
					//cout << "client leaved or error occured." << endl;
				}
				if(--nready <= 0)
				{
					break;
				}
			}
		}
	}
	return 0;
}
