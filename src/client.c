#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

struct user
{
    char name[20];
    char pass[20];
} u;

void *recvmg(void *sock)
{
	int their_sock = *((int *)sock);
	char msg[500];
	int len;
	while((len = recv(their_sock,msg,500,0)) > 0) 
	{
		msg[len] = '\0';
		fputs(msg,stdout);
		memset(msg,'\0',sizeof(msg));
	}
}

void chat(int my_sock)
{
	char msg[500],res[600];
	pthread_t recvt;
	int len;
	
	pthread_create(&recvt,NULL,recvmg,&my_sock);
	while(fgets(msg,500,stdin) > 0) 
	{
		strcat(res,msg);
		len = write(my_sock,res,strlen(res));
		if(len < 0) 
		{
			perror("message not sent");
			exit(1);
		}
		memset(msg,'\0',sizeof(msg));
		memset(res,'\0',sizeof(res));
	}
	pthread_join(recvt,NULL);
}

int login(int my_sock)
{
	int len;
	char answer[200];
	len = write(my_sock,&u,sizeof(u));
	if(len < 0) 
	{
		perror("error login\n");
		exit(1);
	}
	recv(my_sock,answer,200,0);
	if (strcmp(answer,"accepted")==0)
		return 1;
	else
		return 0;
}
int main(int argc, char *argv[])
{
	struct sockaddr_in their_addr;
	int my_sock;
	int portno;
	int c;
	char ip[INET_ADDRSTRLEN];
	if(argc > 3) 
	{
		printf("too many arguments");
		exit(1);
	}
	portno = atoi(argv[1]);
	my_sock = socket(AF_INET,SOCK_STREAM,0);
	memset(their_addr.sin_zero,'\0',sizeof(their_addr.sin_zero));
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(portno);
	their_addr.sin_addr.s_addr = inet_addr(argv[2]);
	printf("Enter username: ");
	scanf("%s",u.name);
	printf("Enter password: ");
	scanf("%s",u.pass);
	while ((c = getchar()) != '\n' && c != EOF) { }
	if(connect(my_sock,(struct sockaddr *)&their_addr,sizeof(their_addr)) < 0) 
	{
		perror("connection not esatablished");
		exit(1);
	}

	inet_ntop(AF_INET, (struct sockaddr *)&their_addr, ip, INET_ADDRSTRLEN);

	if(login(my_sock))
	{
		printf("connected to %s, start chatting\n",ip);
		chat(my_sock);
	}
	else
	{
		printf("Wrong username or password!\n");
	}
	close(my_sock);
}