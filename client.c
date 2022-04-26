/**
 * File              : client.c
 * Last Modified Date: 30.08.2021
 * Last Modified By  : liupenghui <lph@pcl.ac.cn>
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <netdb.h>
#include <time.h>

#include "libyuv.h"
#include <jpeglib.h>

//#incude <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include </usr/include/linux/fcntl.h>
#ifdef __cplusplus
extern "C" {
#endif

/* add for compile warnning */
extern int fcntl(int fd, int cmd, ... /* arg */ );
extern int open( const char * pathname, int flags);

#define MYCFIFO   "/home/customerFIFO"
#ifdef __cplusplus
#define FOURCC(a, b, c, d)                                        \
  ((static_cast<uint32_t>(a)) | (static_cast<uint32_t>(b) << 8) | \
   (static_cast<uint32_t>(c) << 16) | /* NOLINT */                \
   (static_cast<uint32_t>(d) << 24))  /* NOLINT */
#else
#define FOURCC(a, b, c, d)                                     \
  (((uint32_t)(a)) | ((uint32_t)(b) << 8) |       /* NOLINT */ \
   ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24)) /* NOLINT */
#endif


struct v4l2_buffer buf_info;
int fd;
void* buff;

/*
 * \brief Function definition to initialize and set parameters
 * */
void start_device(void)
{
	struct v4l2_format format;
	struct v4l2_capability caps;
	struct v4l2_requestbuffers buf_req;

    memset(&caps, 0, sizeof(caps));
    /* ioctl for querying capabilities of connected device     */
    if(ioctl(fd, VIDIOC_QUERYCAP, &caps) < 0) {
        perror("VIDIOC_QUERYCAP");
        exit(1);
    }
    /* Checking for capture capability */
    if(!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "Device does support video capture\n");
        exit(1);
    }
    if(!(caps.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "Device does support video streaming\n");
        exit(1);
    }
    /* Setting up the formats required */
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    format.fmt.pix.width = 320;
    format.fmt.pix.height = 240;
    /* ioctl for setting the required format */
    if(ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        perror("VIDIOC_S_FMT");
        exit(1);
    }

    memset(&buf_req, 0, sizeof(buf_req));
    buf_req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    buf_req.memory = V4L2_MEMORY_MMAP;
    buf_req.count = 5;
    /* Structure for requesting buffers */
    if(ioctl(fd, VIDIOC_REQBUFS, &buf_req) < 0) {
        perror("VIDIOC_REQBUFS");
        exit(1);
    }

    memset(&buf_info, 0, sizeof(buf_info));
    /* Structure to store status of buffer */
    buf_info.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    buf_info.memory = V4L2_MEMORY_MMAP;
    buf_info.index = 0;
    if(ioctl(fd, VIDIOC_QUERYBUF, &buf_info) < 0){
        perror("VIDIOC_QUERYBUF");
        exit(1);
    }
	printf("start buf_info.length = %d\n", buf_info.length);
//102400
	buf_info.length = 307200; //320*240*4
    /*Mapping descriptor with buffer */
    buff = mmap(NULL, buf_info.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf_info.m.offset);
    if(buff == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
}

unsigned long rgb_to_jpeg(unsigned char *rgb, unsigned char *jpeg)
{
    unsigned long jpeg_size;
    struct jpeg_compress_struct jcs;
    struct jpeg_error_mgr jem;
    JSAMPROW row_pointer[1];
    int row_stride;
    
    jcs.err = jpeg_std_error(&jem);
    jpeg_create_compress(&jcs);
    
    jpeg_mem_dest(&jcs, &jpeg, &jpeg_size);
    
    jcs.image_width = 320;
    jcs.image_height = 240;
 
    jcs.input_components = 3;//1;
    jcs.in_color_space = JCS_RGB;//JCS_GRAYSCALE;
 
    jpeg_set_defaults(&jcs);
    jpeg_set_quality(&jcs, 180, TRUE);
    
    jpeg_start_compress(&jcs, TRUE);
    row_stride =jcs.image_width * 3;
    
    while(jcs.next_scanline < jcs.image_height){//对每一行进行压缩
        row_pointer[0] = &rgb[jcs.next_scanline * row_stride];
        (void)jpeg_write_scanlines(&jcs, row_pointer, 1);
    }
    jpeg_finish_compress(&jcs);
    jpeg_destroy_compress(&jcs);
 
#if 0    //jpeg 保存，测试用
    FILE *jpeg_fd;
    
    sprintf(path3, "./jpeg%d.jpg", numb);
    jpeg_fd = fopen(path3,"w");
    if(jpeg_fd < 0 ){
        perror("open jpeg.jpg failed!\n");
        exit(-1);
    }
    
    fwrite(jpeg, jpeg_size, 1, jpeg_fd);
    close(jpeg_fd);
#endif 
    return jpeg_size;
}

void rgb_reverse(unsigned char *rgb)
{     
	unsigned char tmp;     
   
	for (int i=0; i < 240*320*3; i+=3) {         
		tmp=*(rgb+i);         
		*(rgb+i)=*(rgb+i+2);         
		*(rgb+i+2)=tmp;     
	}     
	return;
}

/*
 * \brief Function definition to recieve & write image
 * */
void start_streaming(void)
{
    int fd_fifo;
	int ret;
	long pipe_size;
	unsigned char*argbbuf = NULL;
	unsigned char*rgbbuf = NULL;
	int c;
	unsigned long jz;

    /* Create the fifo pipe */
	if(mkfifo(MYCFIFO, 0777) != 0) {
		if(errno == EEXIST) {
			printf("File exists\n");
		} else {
			perror("mkfifo fail ");
			close(fd);
			exit(1);
		}
	}

    fd_fifo = open(MYCFIFO, O_RDONLY);//只读方式打开,管道描述符
    if(fd_fifo < 0) {
        perror("open fifo fail: ");
		close(fd);
        exit(1);
    }
    printf("open: fd= %d\n", fd_fifo);
	
    pipe_size = (long)fcntl(fd_fifo, F_GETPIPE_SZ);    
    if (pipe_size == -1) {
        perror("get pipe size failed.");
		close(fd);
       exit(1);
    }
    printf("default pipe size: %ld\n", pipe_size);
    ret = fcntl(fd_fifo, F_SETPIPE_SZ, 320*240*1024);
    if (ret < 0) {
        perror("set pipe size failed.");
		close(fd);
        exit(1);
    }
    pipe_size = (long)fcntl(fd_fifo, F_GETPIPE_SZ);
    if (pipe_size == -1) {
        perror("get pipe size 2 failed.");
		close(fd);
        exit(1);
    }
    printf("new pipe size: %ld\n", pipe_size);
    argbbuf = (unsigned char*) malloc(320*240*8);
	if(argbbuf == NULL) {
        perror("malloc failed.");
		close(fd_fifo);
		close(fd);
        exit(1);		
	}
    rgbbuf = (unsigned char*) malloc(320*240*8);
	if(rgbbuf == NULL) {
        perror("malloc failed.");
		close(fd_fifo);
		close(fd);
        exit(1);		
	}
	//FILE *file_= fopen("/home/my.argb", "wb");
    //rewind(file_);

    while(1) {
        memset(buff, 0, buf_info.length);
        memset(argbbuf, 0, 320*240*8);
        memset(rgbbuf, 0, 320*240*8);
		
		c = 0;
		do { // read frame data until all frame data read
			int r = read(fd_fifo, argbbuf+c, 320*240*4-c);

			if (r > 0)
				c += r;
			else if (r < 0) {
				perror("read error on pipe\n");
				close(fd_fifo);
				close(fd);
				free(argbbuf);
				exit(1);
			}
			else {
				if (fd_fifo > 0) 
					close(fd_fifo);
				
				fd_fifo = open(MYCFIFO, O_RDONLY);
				sleep(0.5);
			}
		} while (c < 320*240*4);	

		//fwrite(argbbuf, sizeof(unsigned char), 320*240*4, file_);
		
		//also can convert to libyuv::ARGBToRGB24, further to RGB24tojpeg.
		/*ret = libyuv::ARGBToJ400((unsigned char*)argbbuf,
               320*4,
               (unsigned char*)buff,
                320,
                320,
                240);*/

		ret = libyuv::ARGBToRGB24((unsigned char*)argbbuf,
               320*4,
               (unsigned char*)rgbbuf,
                320*3,
                320,
                240);
		
		if(ret != 0) {
			perror("ConvertToMJPEG error!\n");
			continue;
		}
        memset(argbbuf, 0, 320*240*8);
		//rgb_reverse(rgbbuf);
		jz = rgb_to_jpeg(rgbbuf, argbbuf);
        printf("jpeg length = %ld\n", jz);
		memcpy(buff, argbbuf, jz);
        printf("buf_info.length = %d\n", buf_info.length);
        ret =  write(fd, buff, jz);
		if (ret == -1) {
			perror("write error on fd\n");
			close(fd_fifo);
			close(fd);
			free(argbbuf);
			exit(1);
		}		
    }
}

/* 
 * \brief Client program for streaming video
 * */
int main(int argc, char *argv[])
{
    char *device = "/dev/video0"; 

    /* Opening virtual node */
    fd = open(device, O_RDWR);
    if(fd < 0) {
        perror("Opening Device");
        exit(1);
    }

    start_device(); /* Function call to initialize device */
    start_streaming(); /* Function call to start capture and send */
    close(fd);
    return 0;
}
#ifdef __cplusplus
}
#endif


