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

extern int show_bmp_bymap(int *lcdmem,char *bmpname);
extern int recv_sound_flag;
extern int *lcdmem;

//接收语音的线程，等待对方的连接，并接收数据
void *recv_sound_routine(void *arg)
{
	// １，创建TCP套接字
	int fd = socket(AF_INET/*IPv4*/, SOCK_STREAM/*TCP*/, 0);
	char buf[100];
	int ret;
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);

	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = inet_addr("192.168.114.100"); // 绑定本机IP
	addr.sin_port   = htons(55000); // 端口号，范围: 1 - 65535，最好用50000以上的端口防冲突

	// ２，绑定地址（IP + PORT）
	int retb = bind(fd, (struct sockaddr *)&addr, len);
	if(retb == -1)
	{
		perror("bind sound tcp fail");
		close(fd);
		return NULL;
	}
	// ３，将套接字设置为监听状态
	// 设定之后的fd，就是监听套接字，专用来等待连接的
	int retl = listen(fd, 3/*设定最大同时连接个数*/);
	if(retl == -1)
	{
		perror("listen fail");
		close(fd);
		return NULL;
	}
	// ４，坐等对方的连接请求... ...
	// 返回的connfd，就是已连接套接字，专用来读写通信的
	struct sockaddr_in peer;
	len = sizeof(peer);

	bzero(&peer, len);
	int connfd = accept(fd, (struct sockaddr *)&peer/*获取对方的地址信息*/, &len);
	//接收对方发来的语音消息，
	if(connfd > 0)
	{
		printf("new connection: [%s:%hu]\n",
				inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
	}
	else
	{
		perror("accept() failed");
		close(connfd);
		close(fd);
		return NULL;
	}
	// ５，将收到的数据,保存到本地  ubuntu上
	// 创建空白的wav用于保存接收到的内容
	int newwavfd=open("./recv.wav",O_CREAT|O_RDWR|O_TRUNC);
	if(newwavfd==-1)
	{
		perror("create wav failed\n");
		close(fd);
		close(connfd);
		return NULL;
	}
	while(1)
	{
		bzero(buf,100); // 1000
		ret=recv(connfd,buf,100,0);
		if(ret == -1)
		{
			perror("recv failed");
			return NULL;
		}
		printf("ubuntu.c 94 recv ret_val = %d\n", ret);
		//趁热吃，放到空白的wav中
		write(newwavfd,buf,ret);
		if(ret<100)
		{
			//设置接收消息的标志位
			recv_sound_flag = 1;
			//1，刷新界面，接收按钮变成红色
			show_bmp_bymap(lcdmem,"/root/project/sound_chat/sound.bmp");
			//2，提示有语音消息发过来，通过system调用aplay播放提示音
			system("aplay /root/project/sound_chat/recv_sound.wav");
		}
	
	}
	close(newwavfd);
	close(fd);
	close(connfd);

	//退出线程
	pthread_exit(NULL);	
	
	return 0;
}

