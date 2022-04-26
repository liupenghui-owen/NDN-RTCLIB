//
//  audio-renderer.h
//  ndnrtc
//
//  Copyright 2013 Regents of the University of California
//  For licensing details see the LICENSE file.
//
//  Author:  Peter Gusev
//

#ifndef __ndnrtc__audio_renderer__
#define __ndnrtc__audio_renderer__

#include <webrtc/modules/audio_device/include/audio_device.h>
#include <webrtc/modules/audio_device/include/audio_device_defines.h>
#include <webrtc/voice_engine/include/voe_network.h>
#include <webrtc/voice_engine/include/voe_base.h>
#include <boost/thread/lock_guard.hpp>
#include <boost/make_shared.hpp>

#include "audio-capturer.hpp"
#include "audio-controller.hpp"
#include "webrtc.hpp"

#include <webrtc/api/call/transport.h>
#include "ndnrtc-object.hpp"
#include "webrtc-audio-channel.hpp"
#include "audio-capturer.hpp"

using namespace webrtc;
namespace webrtc {
    class VoEHardware;
}

namespace ndnrtc {
    class IAudioFrameConsumer {
    public:
        virtual void onDeliverRtpFrame(unsigned int len, unsigned char *data) = 0;
        virtual void onDeliverRtcpFrame(unsigned int len, unsigned char *data) = 0;
    };

    class AudioRenderer : public NdnRtcComponent,
                          public IAudioFrameConsumer,
                          public WebrtcAudioChannel,
                          public webrtc::Transport
    {
    public: 
        AudioRenderer(const unsigned int deviceIdx, 
            const WebrtcAudioChannel::Codec& codec);
        ~AudioRenderer();
        
        void startRendering();
        void stopRendering();
        
        // IAudioFrameConsumer interface conformance
        void onDeliverRtpFrame(unsigned int len, unsigned char *data);
        void onDeliverRtcpFrame(unsigned int len, unsigned char *data);
        
    private:
        AudioRenderer(const AudioRenderer&) = delete;
        
        boost::atomic<bool> rendering_;
        webrtc::VoEHardware* voeHardware_;
        WebrtcAudioChannel::Codec codec_;
        
        WebRtcSmartPtr<AudioDeviceModule> audioDeviceModule_;

        bool
        SendRtp(const uint8_t* packet, size_t length, const webrtc::PacketOptions& options)
        { assert(false); }

        bool
        SendRtcp(const uint8_t* packet, size_t length)
        { return false; }
    };
}

#endif /* defined(__ndnrtc__audio_renderer__) */
