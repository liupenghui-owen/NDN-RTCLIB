//
//  audio-renderer.cpp
//  ndnrtc
//
//  Copyright 2013 Regents of the University of California
//  For licensing details see the LICENSE file.
//
//  Author:  Peter Gusev
//

#include <webrtc/voice_engine/include/voe_network.h>
#include <webrtc/voice_engine/include/voe_base.h>
#include <webrtc/modules/audio_device/include/audio_device.h>
#include <webrtc/modules/audio_device/include/audio_device_defines.h>

#include "audio-renderer.hpp"
#include "audio-controller.hpp"

using namespace std;
using namespace ndnrtc;
using namespace webrtc;
#include <iostream>
//******************************************************************************
#pragma mark - construction/destruction
AudioRenderer::AudioRenderer(const unsigned int deviceIdx, 
  const WebrtcAudioChannel::Codec& codec):
WebrtcAudioChannel(WebrtcAudioChannel::Codec::G722),
rendering_(false)
{
    description_ = "arenderer";
    AudioController::getSharedInstance()->initVoiceEngine();

    int res = 0;
    // AudioController::getSharedInstance()->performOnAudioThread([this, deviceIdx, &res]{
     // audioDeviceModule_ = WebRtcSmartPtr<AudioDeviceModule>(AudioDeviceModule::Create(0, AudioDeviceModule::kPlatformDefaultAudio));
     // audioDeviceModule_->Init();
      audioDeviceModule_ = voeBase_->audio_device_module();
      
      unsigned int tmpdeviceIdx = 0;
      int nDevices = audioDeviceModule_->PlayoutDevices();
      std::cout<<"tmpdeviceIdx "<<tmpdeviceIdx<<"  nDevices"<<nDevices<<std::endl;
      if (tmpdeviceIdx > nDevices)
        res = 1;
      else
        audioDeviceModule_->SetPlayoutDevice(tmpdeviceIdx);
	  
	  //liupenghui,  for audio sample fetching...	  
        unsigned int maxvolume = 0;
        audioDeviceModule_->MaxSpeakerVolume(&maxvolume);
        audioDeviceModule_->SetSpeakerVolume(maxvolume);
        bool enable=0;
        audioDeviceModule_->SpeakerMute(&enable); 
        bool mute=0;  
        audioDeviceModule_->SetSpeakerMute(mute); 
    // });
    std::cout<<"Audio AudioRenderer setup, codec:"<< (int)codec <<"   nDevices:"<<nDevices<<"  maxvolume:"<<maxvolume<<"  enable:"<<enable<<std::endl;
    if (res != 0)
      throw std::runtime_error("Can't initialize audio renderer");
}

AudioRenderer::~AudioRenderer()
{
  if (rendering_)
    stopRendering();

}

//******************************************************************************
#pragma mark - public
void
AudioRenderer::startRendering()
{
  if (rendering_)
    throw std::runtime_error("Audio rendering has already started");

  int res = RESULT_OK;
  
  //liupenghui,  for audio sample fetching...	  
  AudioController::getSharedInstance()->performOnAudioThread([this, &res](){
   // register external transport in order to playback. however, we are not
   // going to set this channel for sending and should not be getting callback
   // on webrtc::Transport callbacks
    if (voeNetwork_->RegisterExternalTransport(webrtcChannelId_, *this) < 0)
     res = voeBase_->LastError();
    if (voeBase_->StartReceive(webrtcChannelId_) < 0)
     res = voeBase_->LastError();
    if (voeBase_->StartPlayout(webrtcChannelId_) < 0)
     res = voeBase_->LastError();
 });
  std::cout<<"Audio start Rendering"<<std::endl;
  if (RESULT_GOOD(res))
  {
    rendering_ = true;
    LogInfoC << "started" << endl;
    std::cout<<"Audio startRendering"<<std::endl;
  }
  else
  {
    stringstream ss;
    ss << "Can't start capturing due to WebRTC error " << res;
    throw std::runtime_error(ss.str());
  }
}

void
AudioRenderer::stopRendering()
{
  if (!rendering_) return;

  AudioController::getSharedInstance()->performOnAudioThread([this](){
    voeBase_->StopPlayout(webrtcChannelId_);
    voeBase_->StopReceive(webrtcChannelId_);
    voeNetwork_->DeRegisterExternalTransport(webrtcChannelId_);
  });
    
  rendering_ = false;
  LogInfoC << "stopped" << endl;
  std::cout<<"Audio startRendering stop"<<std::endl;
}

void
AudioRenderer::onDeliverRtpFrame(unsigned int len, unsigned char *data)
{
  int res = RESULT_OK;
  #if 0
  AudioController::getSharedInstance()->performOnAudioThread([this, &res](){
   // register external transport in order to playback. however, we are not
   // going to set this channel for sending and should not be getting callback
   // on webrtc::Transport callbacks
    if (voeNetwork_->RegisterExternalTransport(webrtcChannelId_, *this) < 0)
     res = voeBase_->LastError();
    if (voeBase_->StartReceive(webrtcChannelId_) < 0)
     res = voeBase_->LastError();
    if (voeBase_->StartPlayout(webrtcChannelId_) < 0)
     res = voeBase_->LastError();
 });
 #endif
  std::cout<<"Audio onDeliverRtpFrame"<<std::endl;
  if (rendering_)
    AudioController::getSharedInstance()->performOnAudioThread([this, len, data](){
        voeNetwork_->ReceivedRTPPacket(webrtcChannelId_, data, len);
    });
}

void
AudioRenderer::onDeliverRtcpFrame(unsigned int len, unsigned char *data)
{
  std::cout<<"Audio onDeliverRtcpFrame"<<std::endl;
  if (rendering_)
    AudioController::getSharedInstance()->performOnAudioThread([this, len, data](){
        voeNetwork_->ReceivedRTCPPacket(webrtcChannelId_, data, len);
    });
}
