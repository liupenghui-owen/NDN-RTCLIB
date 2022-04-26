/**
 * File              : server.c
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

#define MYFIFO   "/home/producerFIFO"
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



struct stream 
{
    void *start;
    int length;
};
struct v4l2_capability caps;
struct v4l2_format format;
struct v4l2_requestbuffers buf_req;
struct v4l2_buffer buf_info;
struct v4l2_buffer buffer_info;
int fd;
struct stream *buffer;

/*
 * \brief Function definition to initialize and set parameters
 * */
void start_device(void)
{
	struct v4l2_capability caps;
	struct v4l2_format format;
	int iter;
	
    memset(&caps, 0, sizeof(caps));
    /* ioctl for querying capabilities of connected device */
    if(ioctl(fd, VIDIOC_QUERYCAP, &caps) < 0) {
        perror("VIDIOC_QUERYCAP");
        exit(1);
    }
    /* Checking for capture capability */
    if(!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "Device does support video capture\n");
        exit(1);
    }
    /* Checking for capture capability */
    if(!(caps.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "Device does support video streaming\n");
        exit(1);
    }
	
    memset(&format, 0, sizeof(format));
    /* Checking for capture capability */
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    format.fmt.pix.width = 320;
    format.fmt.pix.height = 240;
    /* Checking for capture capability */
    if(ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        perror("VIDIOC_S_FMT");
        exit(1);
    }
    /* Structure for requesting buffers */
    memset(&buf_req, 0, sizeof(buf_req));
    buf_req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf_req.memory = V4L2_MEMORY_MMAP;
    buf_req.count = 5;
    /* ioctl for requesting buffers */
    if(ioctl(fd, VIDIOC_REQBUFS, &buf_req) < 0) {
        perror("VIDIOC_REQBUFS");
        exit(1);
    }

    memset(&buf_info, 0, sizeof(buf_info));
    /* Allocating memory for user buffer */
   // buffer =  calloc(buf_req.count, sizeof(struct stream));
	buffer = static_cast<struct stream*>(std::calloc(buf_req.count, sizeof(struct stream)));

    for(iter = 0; iter < buf_req.count; iter++) {
        /* Structure to store status of buffer */
        buf_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf_info.memory = V4L2_MEMORY_MMAP;
        buf_info.index = iter;
        /* Structure to store status of buffer */
        if(ioctl(fd, VIDIOC_QUERYBUF, &buf_info) < 0){
            perror("VIDIOC_QUERYBUF");
            exit(1);
        }
        buffer[iter].length = buf_info.length;
        /*Mapping descriptor with buffer */
        buffer[iter].start = mmap(NULL, buf_info.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf_info.m.offset);
        if(buffer[iter].start == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }
    }
}
/*
 * \brief Function definition to capture & send image through socket
 * */
void start_capture(void)
{

    int type;
    int validate;
    int temp;
	int iter;
    int fd_fifo;
	int ret;
	long pipe_size;
	unsigned char*argbbuf = NULL ;
		
    memset(&buffer_info, 0, sizeof(buffer_info));
    /* Loop for iterating buffers */
    for(iter = 0; iter < buf_req.count; iter++) {
        buffer_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer_info.memory = V4L2_MEMORY_MMAP;
        buffer_info.index = iter;

        if(ioctl(fd, VIDIOC_QBUF, &buffer_info) < 0) {
            perror("VIDIOC_QBUF");
            exit(1);
        }
    }
    type = buffer_info.type;
    /* ioctl to start streaming */
    if(ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("VIDIOC_STREAMON");
        exit(1);
    }
    sleep(1);
    /* Create the fifo pipe */
	if(mkfifo(MYFIFO, 0777) != 0) {
		if(errno == EEXIST) {
			printf("File exists\n");
		} else {
			perror("mkfifo fail ");
			exit(1);
		}
	}
	
    fd_fifo = open(MYFIFO, O_WRONLY);//只写方式打开,管道描述符
    if(fd_fifo < 0) {
        perror("open fifo fail: ");
		close(fd);
        exit(1);
    }
    printf("open: fd= %d\n", fd_fifo);
	
    pipe_size = (long)fcntl(fd_fifo, F_GETPIPE_SZ);    
    if (pipe_size == -1) {
        perror("get pipe size failed.");
		close(fd_fifo);
		close(fd);
        exit(1);
    }
    printf("default pipe size: %ld\n", pipe_size);
    ret = fcntl(fd_fifo, F_SETPIPE_SZ, 320*240*1024);
    if (ret < 0) {
        perror("set pipe size failed.");
		close(fd_fifo);
		close(fd);
        exit(1);
    }
    pipe_size = (long)fcntl(fd_fifo, F_GETPIPE_SZ);
    if (pipe_size == -1) {
        perror("get pipe size 2 failed.");
		close(fd_fifo);
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
		
    while(1) {
        for(iter = 0; iter < buf_req.count; iter++) {
            /* ioctl for dequeying filled buffer */
            if(ioctl(fd, VIDIOC_DQBUF, &buffer_info) < 0){
                perror("VIDIOC_QBUF");
				close(fd_fifo);
				close(fd);
				free(argbbuf);
                exit(1);
            }
            printf("buffer_info.length = %d\n", buffer_info.length);
            //covert to ARGB.
            memset(argbbuf, 0, (320*240*8));
			ret = libyuv::ConvertToARGB((unsigned char*)(buffer[iter].start),
								  buffer_info.length,  // input size
								  (unsigned char*)(argbbuf),
								  320*4,  // destination stride
								  0,  // crop_x
								  0,  // crop_y
								  320,  // width
								  240,  // height
								  320,  // crop width
								  240,  // crop height
								  libyuv::kRotate0,  libyuv::FOURCC_MJPG);
			if(ret != 0) {
                perror("ConvertToARGB error!\n");
                goto conti;
            }
			
			ret = write(fd_fifo, argbbuf, 320*240*4);
            if (ret == -1) {
                perror("Write error on pipe\n");
				close(fd_fifo);
				close(fd);
				free(argbbuf);
                exit(1);
            }            
conti:
			printf("send bytes= %d\n",ret);
            memset(buffer[iter].start, 0, buffer_info.length);
        }
        for(iter = 0; iter < buf_req.count; iter++) {
            buffer_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buffer_info.memory = V4L2_MEMORY_MMAP;
            buffer_info.index = iter;

            if(ioctl(fd, VIDIOC_QBUF, &buffer_info) < 0) {
                perror("VIDIOC_QBUF");
				close(fd_fifo);
				close(fd);
				free(argbbuf);
                exit(1);
            }
        }
    }
    /* ioctl to terminate streaming */
    if(ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        perror("VIDIOC_STREAMOFF");
		close(fd_fifo);
		close(fd);
		free(argbbuf);
        exit(1);
    }
}

int main(int argc,char *argv[])
{
    char *device = "/dev/video1"; /* Camera device node */   


    /* Opening camera device node */
    fd = open(device, O_RDWR);
    if(fd < 0) {
        perror("Opening Device");
        return 0;
    }
    printf("Device opened successfully\n");

    start_device();   /* Function call to initialize device */
    start_capture();  /* Function call to start capture and send */
    close(fd);
    return 0;
}
#ifdef __cplusplus
}
#endif


