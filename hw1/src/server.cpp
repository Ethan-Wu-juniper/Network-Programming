#include<iostream>
#include<sys/socket.h>
#include<sys/select.h>
#include<sys/time.h>
#include<string.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
using namespace std;

#define LISTENQ 10
#define FD_SIZE 10
#define MAXLINE 1024

int verify(string name,string names[])
{
	int i;
	if(!strcmp(name.c_str(),"anonymous"))
		return 1;
	for(i=0;i<FD_SIZE;i++)
	{
		if(names[i].size() > 0 && !strcmp(name.c_str(),names[i].c_str()))
			return 2;
	}
	if(name.size() < 2 || name.size() > 12)
		return 3;
	for(i=0;i<name.size();i++)
	{
		if(!((name[i]>64&&name[i]<91)||(name[i]>96&&name[i]<123)))
			return 3;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int port = atoi(argv[1]);
	int i,j, maxi, maxfd, listenfd, connfd, sockfd, nready, n;
	int client[FD_SIZE];
	char buf[MAXLINE];
	string client_datas[FD_SIZE];
	string names[FD_SIZE];
	socklen_t clilen;
	struct sockaddr_in cliaddr;
	struct sockaddr_in servaddr;
	fd_set rset, allset;
	memset(client_datas,0,sizeof(client_datas));
	memset(names,0,sizeof(names));

	listenfd = socket(AF_INET,SOCK_STREAM,0);
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

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
			if(connfd < 0)
			{
				perror("accept");
				exit(1);
			}
			/******** HELLO msg *******/
            string hello = "[Server] Hello, anonymous! From: ";
			string client_data = "";
            hello += inet_ntoa(cliaddr.sin_addr);
            hello += ":";
            hello += to_string(ntohs(cliaddr.sin_port));
			hello += "\n";
			client_data += inet_ntoa(cliaddr.sin_addr);
			client_data += ":";
			client_data += to_string(ntohs(cliaddr.sin_port));
			int hello_len = strlen(hello.c_str());
            send(connfd, hello.c_str(), hello_len, 0);
            /*************************/
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
			/***** Comming msg *******/
			for(j=0;j<=maxi;j++)
			{
				if(client[j] > 0 && client[j] != connfd)
				{
					string temp = "[Server] Someone is coming!\n";
					send(client[j],temp.c_str(),strlen(temp.c_str()),0);
				}
			}
			/******** initial *********/
			client_datas[i] = client_data;
			names[i] = "anonymous";
			/**************************/
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
				string input(buf);
				/****** shutdown *******/
				if(!strcmp(buf,"shutdown\n"))
				{
					for(j=0;j<=maxi;j++)
					{
						if(client[j] > 0)
						{	
							close(sockfd);
							FD_CLR(sockfd,&allset);
							client[i] = -1;
							client_datas[i].clear();
							names[i].clear();
						}
					}
					return 0;
				}
				/******* exit **********/
				if(n == 0)
				{
					string exit_msg = "[Server] ";
					exit_msg += names[i];
					exit_msg += " is offline.\n";
					for(j=0;j<=maxi;j++)
					{
						if(client[j] > 0 && i!=j)
						{
							send(client[j],exit_msg.c_str(),strlen(exit_msg.c_str()),0);
						}
					}
					close(sockfd);
					FD_CLR(sockfd,&allset);
					client[i] = -1;
					client_datas[i].clear();
					names[i].clear();
					error = false;
				}
				/******* who *********/
				if(!strcmp(buf,"who\n"))
				{
					for(j=0;j<=maxi;j++)
					{
						if(client[j] > 0)
						{	
							string who_msg = "[Server] ";
							who_msg += names[j];
							who_msg += " ";
							who_msg += client_datas[j];
							if(j==i)
								who_msg += " ->me";
							who_msg += "\n";
							send(sockfd,who_msg.c_str(),strlen(who_msg.c_str()),0);
						}
					}
					error = false;
				}
				/******* name *******/
				if(strlen(input.c_str()) >= 7)
				{
					string cmd = input.substr(0,4);
					string name = input.substr(5,strlen(input.c_str())-6);
					string name_tmp;
					if(!strcmp(cmd.c_str(),"name"))
					{
						int ver = verify(name,names);
						if(ver == 0)
						{
							name_tmp = names[i];
							names[i] = name;
							string name_msg = "[Server] You're now known as ";
							name_msg += name;
							name_msg += ".\n";
							send(sockfd,name_msg.c_str(),strlen(name_msg.c_str()),0);
							for(j=0;j<=maxi;j++)
							{
								if(client[j] > 0 && i!=j)
								{	
									string name_msg = "[Server] ";
									name_msg += name_tmp;
									name_msg += " is now known as ";
									name_msg += name;
									name_msg += ".\n";
									send(client[j],name_msg.c_str(),strlen(name_msg.c_str()),0);
								}
							}
						}
						else if(ver == 1)
						{
							string name_err = "[Server] ERROR: Username cannot be anonymous.\n";
							send(sockfd,name_err.c_str(),name_err.size(),0);
						}
						else if(ver == 2)
						{
							string name_err = "[Server] ERROR: ";
							name_err += name;
							name_err += " has been used by others.\n";
							send(sockfd,name_err.c_str(),name_err.size(),0);
						}
						else if(ver == 3)
						{
							string name_err = "[Server] ERROR: Username can only consists of 2~12 English letters.\n";
							send(sockfd,name_err.c_str(),name_err.size(),0);
						}
						error = false;
					}
				}
				/****** yell *********/
				if(strlen(input.c_str()) >= 7)
				{
					string cmd = input.substr(0,4);
					string yell = input.substr(0,strlen(input.c_str())-1);
					if(!strcmp(cmd.c_str(),"yell"))
					{
						string yell_msg = "[Server] ";
						yell_msg += names[i];
						yell_msg += " ";
						yell_msg += yell;
						yell_msg += "\n";
						for(j=0;j<=maxi;j++)
						{
							if(client[j] > 0)
							{
								send(client[j],yell_msg.c_str(),strlen(yell_msg.c_str()),0);
							}
						}
						error = false;
					}
				}
				/******** tell *********/
				if(strlen(input.c_str()) >= 7)
				{
					string cmd = input.substr(0,4);
					string tell = input.substr(5,strlen(input.c_str())-6);
					if(!strcmp(cmd.c_str(),"tell"))
					{
						bool err = false;
						string rcver = tell.substr(0,tell.find(" "));
						if(!strcmp(names[i].c_str(),"anonymous"))
						{
							string tell_err = "[Server] ERROR: You are anonymous.\n";
							send(sockfd,tell_err.c_str(),tell_err.size(),0);
							err = true;
						}
						if(!strcmp(rcver.c_str(),"anonymous"))
						{
							string tell_err = "[Server] ERROR: The client to which you sent is anonymous.\n";
							send(sockfd,tell_err.c_str(),tell_err.size(),0);
							err = true;
						}
						for(j=0;j<=maxi;j++)
						{
							if(!strcmp(names[j].c_str(),rcver.c_str()))
								break;
							if(j==maxi)
							{
								string tell_err = "[Server] ERROR: The receiver doesn't exist\n";
								send(sockfd,tell_err.c_str(),tell_err.size(),0);
								err = true;
							}
						}
						if(!err)
						{
							string msg = tell.substr(tell.find(" ")+1,tell.size()-tell.find(" ")-1);
							string tell_msg = "[Server] SUCCESS: Your message has been sent.\n";
							string told_msg = "[Server] ";
							told_msg += names[i];
							told_msg += " tell you ";
							told_msg += msg;
							told_msg += "\n";
							send(sockfd,tell_msg.c_str(),strlen(tell_msg.c_str()),0);
							for(j=0;j<=maxi;j++)
							{
								if(!strcmp(names[j].c_str(),rcver.c_str()))
								{
									send(client[j],told_msg.c_str(),strlen(told_msg.c_str()),0);
								}
							}
						}
						error = false;
					}
				}
				/******** error ********/
				if(error)
				{
					string error_msg = "[Server] ERROR: Error command.\n";
					send(sockfd,error_msg.c_str(),strlen(error_msg.c_str()),0);
				}
				/***********************/
				fputs(buf,stdout);
				if(--nready <= 0)
				{
					break;
				}
			}
		}
	}
	return 0;
}
