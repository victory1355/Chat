#ifndef _VIDEO_H_
#define _VIDEO_H_

int rgb(int y,int u,int v);
int yuyvtorgb(char *yuyvbuf,char *rgbbuf);
int lcd_init();
int camera_init();
int get_rgb();

int scan_lcd_input(void);

int show_bmp_bymap(int *lcdmem,char *bmpname);
int show_fullbmp(char *bmpname);
int show_shapebmp(int x,int y,int w,int h,char *bmpname);
int show_shortbmp(int x,int y,int w,int h,int scale,char *bmpname);
int recv_sound_message(void);

int jpeginit(int image_width,int image_height,int quality);
int rgb2jpeg(char * filename, unsigned char* rgbData);
int jpeguninit();
int read_JPEG_file (unsigned int *p, char * filename);

void *recv_routine(void *arg);
void *send_routine(void *arg);
void *recv_sound_routine(void *arg);


//建立语音的TCP连接
int setup_sound_tcp();
//发送语音消息给对方
void send_sound(int fd);


//定义一个结构体，将4个缓冲块的首地址跟长度封装起来
struct usrbuf
{
	void *someaddr;
	int length;
};
#endif