#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <linux/input.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <pthread.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "video.h"

//接收语音标志位
extern int recv_sound_flag;
//发送语音标志位
extern int send_sound_flag;


extern int video_open_flag;
//进程退出标志位
int exit_flag;
int updsocket;

extern struct usrbuf *addrarray;
extern int *lcdmem;

int scan_lcd_input(void)
{
	//1，打开触摸屏设备
	int fd = open("/dev/input/event0", O_RDWR);
	if(fd  < 0)
	{
		perror("open enent0 device fail");
		return -1;
	}

	//2.读取触摸数据
	struct input_event inpt;
	int x,y;

	//线程
	pthread_t tid_send,tid_recv,tid_recvsound;
	
	//语音消息
	int ret_sound;
	int ret_sound_fd;
	
	//判断是否接收到语音消息
	pthread_create(&tid_recvsound, NULL ,recv_sound_routine, NULL);
	
	while(1)
	{
		int ret = read(fd, &inpt, sizeof(inpt));//当没有触摸数据的时候read阻塞
		//获取坐标位置
		if(inpt.type==EV_ABS && inpt.code == ABS_X)
		{
			x = inpt.value;	
		}
		if(inpt.type==EV_ABS && inpt.code == ABS_Y)
		{
			y = inpt.value;
		}
		//判断功能区	
		if((x>0 && x<100) && (y>0 && y<195))
		{
			//打开摄像头
			if(video_open_flag == 0)
			{
				printf("first time open video\n");
				//创建接收和发送线程
				pthread_create(&tid_send, NULL ,send_routine, NULL);
				pthread_create(&tid_recv, NULL ,recv_routine, NULL);
			}
			
			
		}
		if((x>700 && x<800) && (y>0 && y<195))
		{
			//关闭摄像头
			printf("close video\n");
			//接收的数据不显示到LCD上，不断开连接，函数未定
		}
		if((x>0 && x<100) && (y>270 && y<480))
		{
			//按下发送语音按钮
			if(inpt.type == EV_KEY && inpt.code == BTN_TOUCH)
			{
				if(inpt.value > 0)
				{
					
					//判断是否是第一次按下该按钮
					if(send_sound_flag == 0)
					{
						//1，第一次就建立TCP连接
						ret_sound_fd = setup_sound_tcp();
					}
					else
					{
						//2，第二次就直接发送消息
						//设置标志位
						printf("send sound message\n");
						send_sound_flag = 1;
						send_sound(ret_sound_fd);
					}
					
				}
			}
		}
		if((x>700 && x<800) && (y>270 && y<480))
		{
			//接收语音消息
			printf("recv sound message module\n");
			//通过标志位判断是否有语音文件读
			if(recv_sound_flag == 1)
			{
				//1，通过aplay播放"./recv.wav"文件
				system("aplay ./recv.wav &");
				//2，刷新界面，按钮恢复成原来的颜色
				//3，恢复标志位
				recv_sound_flag = 0;
			}
			
		}
	}
	
	pthread_join(tid_send, NULL);
	pthread_join(tid_recv, NULL);
	pthread_join(tid_recvsound,NULL);
	close(fd);
	return 0;
}

//创建线程接收对方的数据
void *recv_routine(void *arg)
{
	int ret;
	int jpgfd;
	
	char *recvbuf=calloc(1,40000);
	struct sockaddr_in bindaddr;
	struct sockaddr_in otheraddr;
	socklen_t len = sizeof(bindaddr);

	bindaddr.sin_family = AF_INET;
	bindaddr.sin_addr.s_addr = inet_addr("192.168.114.60");//李四自己的ip
	bindaddr.sin_port = htons(50009);
	

	//创建udp套接字
	int udpsock = socket(AF_INET/*IPv4*/, SOCK_DGRAM/*TCP*/, 0);
	if(udpsock==-1)
	{
		perror("create udpsock failed \n");
		return NULL;
	}
	
	//绑定
	ret=bind(udpsock,(struct sockaddr *)&bindaddr,len);
	if(ret==-1)
	{
		perror("bind udpsock failed\n");
		close(udpsock);
		return NULL;
	}
	
	video_open_flag = 1;
	
	while(1)
	{
		//接收对方发送过来的RGB数据，并显示在lcd上
		ret=recvfrom(udpsock,recvbuf,40000,0,(struct sockaddr *)&otheraddr,&len);
		printf("recv %d byte \n",ret);
		//保存成jpg，然后显示出来
		jpgfd=open("./recv.jpg",O_CREAT|O_RDWR|O_TRUNC);
		if(jpgfd==-1)
		{
			perror("create jpg fail!\n");
			return NULL;
		}
		write(jpgfd,recvbuf,ret);
		close(jpgfd);
		//显示出来
		read_JPEG_file (lcdmem,"./recv.jpg");
		if(exit_flag == 1)
		{
			break;
		}
	}
	pthread_exit(NULL);
	

}
//发送数据给对方
void *send_routine(void *arg)
{
	int ret;

	struct sockaddr_in bindaddr;
	socklen_t len = sizeof(bindaddr);
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_addr.s_addr = inet_addr("192.168.114.60");
	bindaddr.sin_port = htons(60000);

	//创建udp套接字
	updsocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(updsocket==-1)
	{
		perror("udpsocket failed\n");
		return NULL;
	}
	
	//绑定
	ret=bind(updsocket,(struct sockaddr *)&bindaddr,len);
	if(ret==-1)
	{
		perror("bind updsocket failed\n");
		close(updsocket);
		return NULL;
	}
	video_open_flag = 2;
	//将摄像头采集的画面(转换成了RGB)发送给对方
	get_rgb();
}




