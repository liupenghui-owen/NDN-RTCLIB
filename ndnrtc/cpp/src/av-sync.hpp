//
//  av-sync.h
//  ndnrtc
//
//  Copyright 2013 Regents of the University of California
//  For licensing details see the LICENSE file.
//
//  Author:  Peter Gusev

#ifndef __ndnrtc__av_sync__
#define __ndnrtc__av_sync__

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#if 0
#include "simple-log.hpp"

namespace ndnrtc
{
    namespace new_api
    {
        class Consumer;
    }
    
    class IMediaReceiverCallback
    {
    public:
        // called whenever receiver encounters rebuffering
        virtual void onRebuffer(new_api::Consumer *caller) = 0;
    };
    
    class AudioVideoSynchronizer : public ndnlog::new_api::ILoggingObject,
    public IMediaReceiverCallback
    {
    public:
        static const int64_t TolerableLeadingDriftMs; // audio should not lead video by more than this value
        static const int64_t TolerableLaggingDriftMs; // audio should not lag video by more than this value
        static const int64_t MaxAllowableAvSyncAdjustment; // this value should be used when synchronizing stream for sync adjustment control
        
        AudioVideoSynchronizer(const boost::shared_ptr<new_api::Consumer>& masterConsumer,
                               const boost::shared_ptr<new_api::Consumer>& slaveConsumer);
        ~AudioVideoSynchronizer();
        
        /**
         * Main method which should be called for synchronization. It should
         * be called once the packet was released from the playout buffer (i.e. 
         * releaseAcquiredSlot was called) BUT right before running playout 
         * timer (i.e. runPlayoutTimer call of JitterTiming class). Upon 
         * return, it provides delay by which playout time should be 
         * incremented in order to be synchronized with the other stream.
         * @param slot Slot which contains packet to be played out. By its 
         * content (audio sample or video frame) it also determines whether 
         * audio or video stream should be synchronized.
         * @return Playout duration adjustment (in ms) needed for media streams 
         * to be synchronized or 0 if no adjustment is required.
         */
        int synchronizePacket(int64_t remoteTimestamp, int64_t localTimestamp,
                              new_api::Consumer *consumer);
        
        /**
         * Resets synchronization data (used for rebufferings).
         */
        void reset();
        
        /**
         * IMediaReceiverCallback interface
         */
        void onRebuffer(new_api::Consumer *consumer);
        
    protected:
        // those attributes which have suffix "remote" relate to remote producer's
        // timestamps, whereas suffixed with "local" relate to local timestamps
        class SyncStruct {
        public:
            SyncStruct(const char *name):
            name_(name),
            initialized_(false),
            lastPacketTsLocal_(-1),
            lastPacketTsRemote_(-1)
            {}
            
            ~SyncStruct()
            {
            }
            
            boost::mutex mutex_;
            new_api::Consumer *consumer_;
            const char *name_;
            bool initialized_;
            
            int64_t lastPacketTsLocal_; // local timestamp of last packet
                                        // playout time, i.e. when packet was
                                        // acquired for playback
            int64_t lastPacketTsRemote_;    // remote timestamp of last packet
                                            // playout time, i.e. packet
                                            // timestamp published by producer
            
            // resets sync data strcuture
            void reset(){
                boost::lock_guard<boost::mutex> scopedLock(mutex_);
                initialized_ = false;
                lastPacketTsLocal_ = -1;
                lastPacketTsRemote_ = -1;
            }
        };
        
        bool initialized_;  // indicates, whether synchronizer was initialized
                       // (when both streams has started)
        boost::mutex syncMutex_;
        SyncStruct masterSyncData_, slaveSyncData_;
        
        int syncPacket(SyncStruct& syncData,
                       SyncStruct& pairedSyncData,
                       int64_t packetTsRemote,
                       int64_t packetTsLocal,
                       new_api::Consumer *consumer);
        
        void initialize(SyncStruct& syncData,
                        int64_t firstPacketTsRemote,
                        int64_t packetTsLocal,
                        new_api::Consumer *consumer);
    };
}
#endif
#endif /* defined(__ndnrtc__av_sync__) */
