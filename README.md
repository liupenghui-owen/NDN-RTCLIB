# ndnrtc.zip, 移植到UBUNTU20.04上的NDN-RTC库代码

**ndnrtc.zip** 为NDN-RTC库代码是官网已存档的代码，存在很多BUG, 本网版本已经调试通过，并可用于基于VP9的WEB-RTC的NDN视频直播。

# server.c, 基于V4L2的视频提取服务器程序

**server.c** 从摄像头驱动提取MJPG视频帧并转码成YUV视频帧，同时发送给ndnrtc的producer进行VP9编码并切割打包成Data。

# client.c, 基于V4L2的视频接收客户端程序

**client.c** 从ndnrtc的consumer(从producer接收并解码成YUV)提取出YUV视频帧，并转码成MJPG，输出给VLC串流播放设备。

**NDN-RTC Real-Time Videoconferencing over Named Data Networking.pdf**为原始参考论文

**NDN-RTC搭建与调试**为搭建文档
