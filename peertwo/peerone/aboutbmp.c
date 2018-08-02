#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <termios.h> 
#include <sys/types.h>
#include <sys/select.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
/*
	关于24位bmp图片显示的所有函数都在这里
	显示全屏
	任意位置显示任意大小
	将任意大小的bmp图片缩写任意比例
*/

// 自定义函数，专门用于显示800*480bmp格式的图片
int show_fullbmp(char *bmpname)  
{
	int i;
	int x,y;
	int lcdfd;
	int bmpfd;
	// 定义一个数组用于存放bmp的颜色值
	char bmpbuf[800*480*3];  // char占一个字节    bmpbuf[0] bmpbuf[1] bmpbuf[2]一组
	int lcdbuf[800*480]; // int占4字节
	
	// 定义一个临时变量保存颜色值
	int tempbuf[800*480];
	// 打开lcd的驱动
	lcdfd = open("/dev/fb0",O_RDWR);
	// 出错处理
	if(lcdfd == -1)
	{
		printf("open lcd error!\n");
		return -1;
	}
	// 打开你要显示的bmp图片
	bmpfd = open(bmpname,O_RDONLY);
	if(bmpfd == -1)
	{
		printf("open bmp error!\n");
		return -1;
	}
	// 在读之前，先跳过前面54字节
	lseek(bmpfd,54,SEEK_SET);
	// 读取bmp图片的颜色值,从55字节位置开始读取了
	read(bmpfd,bmpbuf,800*480*3);
	
	// 将读取的颜色值3字节---》转换成4字节
	for(i=0; i<800*480; i++)
		lcdbuf[i]=bmpbuf[3*i]|bmpbuf[3*i+1]<<8|bmpbuf[3*i+2]<<16|0x00<<24;

	// 将颠倒的图片翻转过来(x,y)--->(x,479-y)
	for(x=0; x<800; x++)
		for(y=0; y<480; y++)
			//lcdbuf[(479-y)*800+x] = lcdbuf[y*800+x];
			tempbuf[(479-y)*800+x] = lcdbuf[y*800+x];
			
	
	//将转换之后的颜色值写入到lcd的驱动中
	write(lcdfd,tempbuf,800*480*4);
	
	// 关闭lcd和bmp
	close(bmpfd);
	close(lcdfd);
	return 0;
}

// 自定义函数，任意位置显示任意大小的bmp
/*
	x,y是图片显示左上角的坐标
	w,h是图片本身的宽和高   像素
*/
int show_shapebmp(int x,int y,int w,int h,char *bmpname)   //printf
{
	int i,j;
	int lcdfd;
	int bmpfd;
	int *lcdmem;
	// 定义一个数组用于存放bmp的颜色值
	char bmpbuf[w*h*3];  // char占一个字节    bmpbuf[0] bmpbuf[1] bmpbuf[2]一组
	int lcdbuf[w*h]; // int占4字节
	
	// 打开lcd的驱动
	lcdfd = open("/dev/fb0",O_RDWR);
	// 出错处理
	if(lcdfd == -1)
	{
		printf("open lcd error!\n");
		return -1;
	}

	// 将lcd的地址映射到用户空间
	lcdmem = (int *)mmap(NULL,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcdfd,0);
	if(lcdmem==NULL)
	{
		printf("map lcd error!\n");
		return -1;
	}
	// 打开你要显示的bmp图片
	bmpfd = open(bmpname,O_RDONLY);
	if(bmpfd == -1)
	{
		printf("open bmp error!\n");
		return -1;
	}
	// 在读之前，先跳过前面54字节
	lseek(bmpfd,54,SEEK_SET);
	// 读取bmp图片的颜色值,从55字节位置开始读取了
	read(bmpfd,bmpbuf,w*h*3);
	
	// 将读取的颜色值3字节---》转换成4字节
	for(i=0; i<w*h; i++)
		lcdbuf[i]=bmpbuf[3*i]|bmpbuf[3*i+1]<<8|bmpbuf[3*i+2]<<16|0x00<<24;

	// 将（i,j）---》存放在屏幕（i+x,j+y）的位置
	for(i=0; i<w; i++)
		for(j=0; j<h; j++)
			*(lcdmem+(j+y)*800+i+x) = lcdbuf[(h-1-j)*w+i];
	// 关闭lcd和bmp
	close(bmpfd);
	close(lcdfd);
	// 解除内存映射
	munmap(lcdmem,800*480*4);
	return 0;
}

/*将任意大小的图片缩小成某种比例
	参数介绍：x ，y --》左上角x,y坐标
	          w ---》图片的实际宽
			  h ---》图片的实际高
			  scale---》缩小的倍数
			  bmpname---》图片的路径
	代码重点提示：m相当于是缩小之后的图片的宽，n相当于是缩小之后的图片的高
*/           
int show_shortbmp(int x,int y,int w,int h,int scale,char *bmpname)   
{
	int i,j;
	int m,n;
	int lcdfd;
	int bmpfd;
	int *lcdmem;
	// 定义一个数组用于存放bmp的颜色值
	char bmpbuf[w*h*3];  // char占一个字节    bmpbuf[0] bmpbuf[1] bmpbuf[2]一组
	int lcdbuf[w*h]; // int占4字节
	
	// 打开lcd的驱动
	lcdfd = open("/dev/fb0",O_RDWR);
	// 出错处理
	if(lcdfd == -1)
	{
		printf("open lcd error!\n");
		return -1;
	}

	// 将lcd的地址映射到用户空间
	lcdmem = (int *)mmap(NULL,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcdfd,0);
	if(lcdmem==NULL)
	{
		printf("map lcd error!\n");
		return -1;
	}
	// 打开你要显示的bmp图片
	bmpfd = open(bmpname,O_RDONLY);
	if(bmpfd == -1)
	{
		printf("open bmp error!\n");
		return -1;
	}
	// 在读之前，先跳过前面54字节
	lseek(bmpfd,54,SEEK_SET);
	// 读取bmp图片的颜色值,从55字节位置开始读取了
	read(bmpfd,bmpbuf,w*h*3);
	
	// 将读取的颜色值3字节---》转换成4字节
	for(i=0; i<w*h; i++)
		lcdbuf[i]=bmpbuf[3*i]|bmpbuf[3*i+1]<<8|bmpbuf[3*i+2]<<16|0x00<<24;

	// 将（i,j）---》存放在屏幕（i+x,j+y）的位置
	for(i=0,m=0; i<w; i+=scale,m++)
		for(j=0,n=0; j<h; j+=scale,n++)
			*(lcdmem+(n+y)*800+m+x) = lcdbuf[(h-1-j)*w+i];//将w*h图片上的某个点填充到指定位置
	// 关闭lcd和bmp
	close(bmpfd);
	close(lcdfd);
	// 解除内存映射
	munmap(lcdmem,800*480*4);
	return 0;
}




//自定义函数，通过参数内存映射的首地址显示bmp图片
int show_bmp_bymap(int *lcdmem,char *bmpname)
{
	int y = 0,x = 0;
	int i,j;
	int  w = 800;
	int h = 480;
	
	int bmpfd;
	// 定义一个数组用于存放bmp的颜色值
	char bmpbuf[w*h*3];  // char占一个字节    bmpbuf[0] bmpbuf[1] bmpbuf[2]一组
	int lcdbuf[w*h]; // int占4字节
	
	// 打开你要显示的bmp图片
	bmpfd = open(bmpname,O_RDONLY);
	if(bmpfd == -1)
	{
		printf("open bmp error!\n");
		return -1;
	}
	// 在读之前，先跳过前面54字节
	lseek(bmpfd,54,SEEK_SET);
	// 读取bmp图片的颜色值,从55字节位置开始读取了
	read(bmpfd,bmpbuf,w*h*3);
	
	// 将读取的颜色值3字节---》转换成4字节
	for(i=0; i<w*h; i++)
		lcdbuf[i]=bmpbuf[3*i]|bmpbuf[3*i+1]<<8|bmpbuf[3*i+2]<<16|0x00<<24;

	// 将（i,j）---》存放在屏幕（i+x,j+y）的位置
	for(i=0; i<w; i++)
		for(j=0; j<h; j++)
			*(lcdmem+(j+y)*800+i+x) = lcdbuf[(h-1-j)*w+i];

	return 0;
}

















