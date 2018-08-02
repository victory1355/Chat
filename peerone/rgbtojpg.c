#include <stdio.h>
#include <stdlib.h>
#include "jpeglib.h"
//定义一个压缩对象，这个对象用于处理主要的功能
struct jpeg_compress_struct jpeg;
//用于错误信息
struct jpeg_error_mgr jerr;
int jpeginit(int image_width,int image_height,int quality)
{
	//错误输出在绑定
    jpeg.err = jpeg_std_error(&jerr);
    //初始化压缩对象
   jpeg_create_compress(&jpeg);
    //压缩参数设置。具体请到网上找相应的文档吧，参数很多，这里只设置主要的。
    //我设置为一个 24 位的 image_width　X　image_height大小的ＲＧＢ图片
   jpeg.image_width = image_width;
   jpeg.image_height = image_height;
   jpeg.input_components  = 3;
   jpeg.in_color_space = JCS_RGB;
   //参数设置为默认的
   jpeg_set_defaults(&jpeg);
   //还可以设置些其他参数：
   //// 指定亮度及色度质量
   //jpeg.q_scale_factor[0] = jpeg_quality_scaling(100);
   //jpeg.q_scale_factor[1] = jpeg_quality_scaling(100);
    //// 图像采样率，默认为2 * 2
	//jpeg.comp_info[0].v_samp_factor = 1;
    //jpeg.comp_info[0].h_samp_factor = 1;
    //// 设定编码jpeg压缩质量
    jpeg_set_quality(&jpeg, quality, TRUE);
    return 0;
}

int rgb2jpeg(char * filename, unsigned char* rgbData)
{
	int i;
   //定义压缩后的输出，这里输出到一个文件！
    FILE* pFile = fopen( filename,"wb" );
   if( !pFile )
       return 0;
    //绑定输出
    jpeg_stdio_dest(&jpeg, pFile);
    //开始压缩。执行这一行数据后，无法再设置参数了！
   jpeg_start_compress(&jpeg, TRUE);
   JSAMPROW row_pointer[1];
   //从上到下，设置图片中每一行的像素值
   for( i=0;i<jpeg.image_height;i++ )
    {
        row_pointer[0] = rgbData+i*jpeg.image_width*3;
        jpeg_write_scanlines( &jpeg,row_pointer,1 );
    }
   //结束压缩
   jpeg_finish_compress(&jpeg);
    fclose( pFile );
    pFile = NULL;
   return 0;
}

int jpeguninit()
{
   //清空对象
    jpeg_destroy_compress(&jpeg);
    return 0;
}

//把jpg图片解码为RGB数据--》刷在lcd上
int read_JPEG_file (unsigned int *p, char * filename)
{
  //创建解码结构体对象
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  
  /* More stuff */
  FILE * infile;		//要解码的文件
  int row_stride;		//解码一行像素存储所需要的空间

  //打开解码文件--标准文件IO--fopen
  if ((infile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    return 0;
  }

  //初始化jpeg解码对象
  jpeg_create_decompress(&cinfo);

  //把jpeg数据与解码对象关联
  jpeg_stdio_src(&cinfo, infile);

  //获取图片文件头信息
  (void) jpeg_read_header(&cinfo, TRUE);
  /* Step 4: set parameters for decompression */

 
  //开始解码
  (void) jpeg_start_decompress(&cinfo);
  
  //计算一行像素所占用的字节数
  row_stride = cinfo.output_width * cinfo.output_components;
  //分配一行像素所要占用的空间(用来存储RGB数据)
  char *buffer=(char*)malloc(row_stride);	/* Output row buffer */

  //一行一行解码
  int i=0;
  while (cinfo.output_scanline < cinfo.output_height) {
	//解一行读取一行--把解码后RGB数据存储在buffer中
    (void) jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&buffer, 1);
	//把buffer中的数据 （）刷在lcd上   buffer={RGBRGBRGB}
	for(i=110; i<700; i++)
	{
		unsigned int data=buffer[i*3+2];        //data -[][][][B]
		data |= buffer[i*3+1]<<8;   //data -[][][G][B]
		data |= buffer[i*3]<<16; //data -[][R][G][B]
		*(p+i) = data;
	}
	p+=800;
  }

  //解码结束
  (void) jpeg_finish_decompress(&cinfo);
  //释放空间
  jpeg_destroy_decompress(&cinfo);

  //关闭图片文件
  fclose(infile);
  //释放空间
  free(buffer);

  return 1;
}