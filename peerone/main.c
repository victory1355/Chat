/*

        测试结果：实现视频监控与语音聊天功能

*/




#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <pthread.h>
#include <semaphore.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "video.h"

int recv_sound_flag;
int send_sound_flag;

extern int *lcdmem;
extern int exit_flag;
int video_open_flag;
extern struct usrbuf *addrarray;

void handle(int sig)
{
	printf("Get the signal %d\n", sig);
	exit_flag = 1;
}

//第一次测试，运行在自己的开发板上，只发送数据给对方，测试结果：通过
//第二次测试，运行在自己的开发板上，发送数据和接收数据,测试结果：通过

int main(int argc, char **argv)
{
	addrarray=calloc(4,sizeof(struct usrbuf));
	
	recv_sound_flag = 0;
	video_open_flag = 0;
	send_sound_flag = 0;
	
	exit_flag = 0;
	send_sound_flag = 0;
	signal(SIGINT,handle);
	//打开lcd
	lcd_init();
	//初始化摄像头
	camera_init();
	
	//导入界面
	//show_fullbmp("/root/project/sound_chat/chat.bmp"); 
	show_bmp_bymap(lcdmem,"/root/project/sound_chat/chat.bmp");

	while(1)
	{
		//检测触摸屏事件
		scan_lcd_input();
		//printf("test\n");
		sleep(1);
	}
	
	return 0;
}





