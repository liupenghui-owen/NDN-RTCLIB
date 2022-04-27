# NDN-RTC是一个NDN网络环境下的实时视频通信库。
基本原理：视频源将原始图像VP9编码后进行切片，通过NDN流控方式传输到接收端后，组合成VP9视频帧，再解码成原始图。语音通过WebRTC接口编码成RTC/RTCP包，再通过NDN直接传输到接收端后，通过WebRTC接口解码并播放。

主要技术：NDN(NDNCPP)、OpenFEC、WebRTC和VP9。

研究目的：研究ND网络环境下的安全视频聊天，及网络媒体流交互控制。

# Overview
NDN-RTC is a videoconferencing library that employs Named Data Networking (NDN), a proposed future Internet architecture. It was designed to provide a platform for experimental research in low-latency, real-time multimedia communication over NDN. It aims to provide an end-user experience similar to Skype or Google Hangouts, while implementing a receiver-driven approach that takes advantage of NDN's name-based forwarding, data signatures, caching, and request aggregation. As implemented, NDN-RTC employs widely used open source components, including the WebRTC library, VP9 codec, and OpenFEC for forward error correction. This paper presents the design, implementation in C++, and testing of NDN-RTC on the NDN testbed using a demonstration GUI conferencing application, ndncon, which provides HD videoconferencing over NDN to end-users.

# ndnrtc.zip, 移植到UBUNTU20.04上的NDN-RTC库代码

**ndnrtc.zip** 为代码，可用于基于VP9的WebRTC的NDN视频直播。

# server.c, 基于V4L2的视频提取服务器程序

**server.c** 从摄像头驱动提取MJPG视频帧并转码成YUV视频帧，同时发送给ndnrtc的producer进行VP9编码并切割打包成Data。

# client.c, 基于V4L2的视频接收客户端程序

**client.c** 从ndnrtc的consumer(从producer接收并解码成YUV)提取出YUV视频帧，并转码成MJPG，输出给VLC串流播放设备。

**NDN-RTC Real-Time Videoconferencing over Named Data Networking.pdf**为原始参考论文

**NDN-RTC搭建与调试.doc**为搭建文档，（用于NDN环境下的安全视频聊天应用研究）
