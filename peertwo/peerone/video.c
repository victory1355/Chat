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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include "video.h"

extern int updsocket;
int camerafd;
int *lcdmem;
//定义指针指向4块空间，存放各个空间的首地址和大小
struct usrbuf *addrarray;

//封装函数单独计算yuyv转rgb
int rgb(int y,int u,int v)
{
	unsigned int pixel24 = 0; //4字节
    unsigned char *pixel = (unsigned char *)&pixel24;  //此指针指向pixel24低字节
	int r, g, b;
	static int  ruv, guv, buv;
	 ruv = 1596*(v-128);
	 guv = 391*(u-128) + 813*(v-128);
     buv = 2018*(u-128);
     

     r = (1164*(y-16) + ruv) / 1000;
     g = (1164*(y-16) - guv) / 1000;
     b = (1164*(y-16) + buv) / 1000;
    if(r > 255) r = 255;
    if(g > 255) g = 255;
    if(b > 255) b = 255;
    if(r < 0) r = 0;
    if(g < 0) g = 0;
    if(b < 0) b = 0;
	
	pixel[0] = r;
    pixel[1] = g;
    pixel[2] = b;
	
    return pixel24;  //某一组RGB的值
}
/*
	YUYV转换成RGB的公式
	R = 1.164*(Y-16) + 1.159*(V-128); 
    G = 1.164*(Y-16) - 0.380*(U-128)+ 0.813*(V-128); 
    B = 1.164*(Y-16) + 2.018*(U-128)); 
*/
//封装一个函数，将yuyv格式的数据转换rgb
/*
	参数：yuyvbuf  原始的yuyv数据
	      rgbbuf   转换之后用于存放RGB的
*/
int yuyvtorgb(char *yuyvbuf,char *rgbbuf)
{
	//yuyvbuf[0] yuyvbuf[1] yuyvbuf[2] yuyvbuf[3]
	  //  Y           U         Y           V  
     unsigned int in, out;
     int y0, u, y1, v;
     unsigned int pixel24;
     unsigned char *pixel = (unsigned char *)&pixel24;
     unsigned int size = 640*480*2;

     for(in = 0, out = 0; in < size; in += 4, out += 6)
     {
          y0 = yuyvbuf[in+0];
          u  = yuyvbuf[in+1];
          y1 = yuyvbuf[in+2];
          v  = yuyvbuf[in+3];

          pixel24 = rgb(y0, u, v);
          rgbbuf[out+0] = pixel[0];  //r 
          rgbbuf[out+1] = pixel[1];  //g
          rgbbuf[out+2] = pixel[2];  //b

          pixel24 = rgb(y1, u, v);
          rgbbuf[out+3] = pixel[0];
          rgbbuf[out+4] = pixel[1];
          rgbbuf[out+5] = pixel[2];
     }
     return 0;
}
//液晶屏的初始化
int lcd_init()
{
	int lcdfd;
	
	//打开屏幕Lcd
	lcdfd = open("/dev/fb0",O_RDWR);
	if(lcdfd == -1)
	{
		printf("open lcd failed!\n");
		return -1;
	}
	lcdmem = (unsigned int *)mmap(NULL,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcdfd,0);
	if(lcdmem == MAP_FAILED)
	{
		perror("mem_p\n");
		return -1;
	}
	return 0;
}
//摄像头的初始化
int camera_init()
{
	int ret;
	int i;
	//打开摄像头的驱动
	camerafd=open("/dev/video7",O_RDWR); //这个是210开发板上的驱动     6818正常情况下是/dev/video7
	if(camerafd==-1)
	{
		perror("打开摄像头驱动!\n");
		return -1;
	}
	//设置摄像头的采集格式
	struct v4l2_format  myfmt;
	memset(&myfmt,0,sizeof(myfmt));
	myfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    myfmt.fmt.pix.width = 640;
    myfmt.fmt.pix.height = 480;
    myfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; //设置视频采集格式yuyv
    myfmt.fmt.pix.field = V4L2_FIELD_NONE;
	ret=ioctl(camerafd,VIDIOC_S_FMT,&myfmt);
	if(ret==-1)
	{
		perror("设置摄像头的采集格式!\n");
		return -1;
	}
	//申请缓冲区
	struct v4l2_requestbuffers myreqbuf;
	memset(&myreqbuf,0,sizeof(myreqbuf));  
	myreqbuf.count=4;  //一般来说 1到5之间
	myreqbuf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	myreqbuf.memory=V4L2_MEMORY_MMAP;
	ret=ioctl(camerafd,VIDIOC_REQBUFS,&myreqbuf);
	if(ret==-1)
	{
		perror("申请缓冲区失败!\n");
		return -1;
	}
	
	//分配缓冲区  循环4次，分配4次
	for(i=0; i<4; i++)
	{
		struct v4l2_buffer mybuf;
		memset(&mybuf,0,sizeof(mybuf));
		mybuf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
		mybuf.memory=V4L2_MEMORY_MMAP;
		mybuf.index=i;
		ret=ioctl(camerafd,VIDIOC_QUERYBUF,&mybuf);
		if(ret==-1)
		{
			perror("分配缓冲区失败!\n");
			return -1;
		}
		//将分配成功的缓冲区映射到用户空间  4个缓冲块
		addrarray[i].someaddr=mmap(NULL,mybuf.length,PROT_READ|PROT_WRITE,MAP_SHARED,camerafd,mybuf.m.offset);
		if(addrarray[i].someaddr==NULL)
		{
			perror("映射失败!\n");
		}
		addrarray[i].length=mybuf.length;
	}
	//设置画面入队
	for(i=0; i<4; i++)
	{
		struct v4l2_buffer inbuf;
		memset(&inbuf,0,sizeof(inbuf));
		inbuf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
		inbuf.memory=V4L2_MEMORY_MMAP;
		inbuf.index=i;
		//让摄像头采集的画面入队(进入缓冲区)
		ret=ioctl(camerafd,VIDIOC_QBUF,&inbuf);
		if(ret==-1)
		{
			perror("入队失败!\n");
			return -1;
		}
	}
	//摄像头启动
	enum v4l2_buf_type  mytype=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret=ioctl(camerafd,VIDIOC_STREAMON ,&mytype);
	if(ret==-1)
	{
		perror("摄像头启动失败!\n");
		return -1;
	}
	return 0;
}
//自己封装函数获取摄像头的rgb
int get_rgb()
{
	
	int i;
	int ret;
	int jpgfd;
	int allsize;
	//将画面显示出来     addrarray[i].length   addrarray[i].someaddr
	//定义指针分配空间用于存放转换得到的RGB
	char *rgbpoint=malloc(640*480*3);
	struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
	
	struct sockaddr_in otheraddr;
	socklen_t len = sizeof(otheraddr);

	otheraddr.sin_family = AF_INET;
	otheraddr.sin_addr.s_addr = inet_addr("192.168.114.60");
	otheraddr.sin_port = htons(50009);
	//jpg初始化
	jpeginit(640,480,65); //65表示保存的jpg画面的质量  取值0---100之间,值越小，图片大小就越小
	while(1)
	{
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(camerafd, &fds);
		int r = select(camerafd + 1, &fds, 0, 0, &timeout);
		for(i=0; i<4; i++)
		{
			struct v4l2_buffer buf;
			memset(&buf, 0, sizeof buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index=i;
			if (ioctl(camerafd, VIDIOC_DQBUF, &buf) == -1)
			{
				perror("出队失败!\n");
				return -1;
			}		
			if(ioctl(camerafd, VIDIOC_QBUF, &buf) == -1)
			{
				perror("入队失败!\n");
				return -1;
			}
			yuyvtorgb(addrarray[i].someaddr,rgbpoint);
			//保存成jpg --->大小变得很小了，至少没有跟之前一样(640*480*3=90多万字节,太大了)
			rgb2jpeg("./test.jpg",rgbpoint);//现在的test.jpg很小了
			//读写文件并发送给对方
			jpgfd=open("./test.jpg",O_RDWR);
			if(jpgfd==-1)
			{
				perror("打开jpg失败!\n");
				return -1;
			}
			allsize=lseek(jpgfd,0,SEEK_END);
			char *jpgbuf=malloc(allsize);//容器
			lseek(jpgfd,0,SEEK_SET);
			read(jpgfd,jpgbuf,allsize);
			close(jpgfd);
			ret=sendto(updsocket,jpgbuf,allsize,0,(struct sockaddr *)&otheraddr,len);
			printf("send %d byte!\n",ret);
			free(jpgbuf);
		}
    }
	
	
}
//921600  ---->1024*900