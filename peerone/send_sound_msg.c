#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <strings.h>

#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <pthread.h>
#include <semaphore.h>

#include <arpa/inet.h>
#include <sys/socket.h>

extern int send_sound_flag;


//建立语音的TCP连接
int setup_sound_tcp()
{

	// １，创建TCP套接字
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1)
	{
		perror("create sound tcp failed");
		return -1;
	}
	
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("192.168.114.60");
	addr.sin_port = htons(53000);
	// ２，向服务器发起连接请求
	int ret = connect(fd, (struct sockaddr *)&addr, len);
	if(ret == -1)
	{
		perror("connect falied");
		close(fd);
		return -1;
	}
	return fd;
}


//发送语音消息给对方
void send_sound(int fd)
{
	int wavfd;
	int filesize;
	char cmd[10];
	
	bzero(cmd,10);
	
	while(1)
	{
		printf("test 6818.c 64\n");
		// ３，录音，并将录好的wav文件发送给ubuntu
		system("arecord -d3 -c1 -r16000 -twav -fS16_LE example.wav &");
		sleep(4);
		system("killall -KILL arecord");
		//读取录音
		wavfd=open("./example.wav",O_RDWR);
		if(wavfd==-1)
		{
			perror("open wav file falied\n");
			return ;
		}
		//求录音文件的大小
		filesize=lseek(wavfd,0,SEEK_END);
		printf("the example.wav size %d\n", filesize);
		char *buf=malloc(filesize);
		//重新挪回来
		lseek(wavfd,0,SEEK_SET);
		//读取文件内容
		read(wavfd,buf,filesize);
		close(wavfd);
		//一口气将整个文件全部发送过来
		send(fd,buf,filesize,0);

	}
	
}









