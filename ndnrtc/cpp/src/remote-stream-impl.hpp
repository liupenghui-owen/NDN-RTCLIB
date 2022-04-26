//
// remote-stream-impl.hpp
//
//  Created by Peter Gusev on 17 June 2016.
//  Copyright 2013-2016 Regents of the University of California
//

#ifndef __remote_stream_impl_h__
#define __remote_stream_impl_h__

#include <ndn-cpp/name.hpp>

#include "remote-stream.hpp"
#include "ndnrtc-object.hpp"
#include "data-validator.hpp"

namespace ndnrtc
{
namespace statistics
{
class StatisticsStorage;
}

class SegmentController;
class BufferControl;
class PipelineControl;
class SampleEstimator;
class DrdEstimator;
class PipelineControlStateMachine;
class PipelineControl;
class ILatencyControl;
class IInterestControl;
class IBuffer;
class IPipeliner;
class IInterestQueue;
class IPlaybackQueue;
class IPlayout;
class IPlayoutControl;
class MediaStreamMeta;
class MetaFetcher;
class RetransmissionController;

// forward delcaration of typedef'ed template class
struct Mutable;
template <typename T>
class NetworkDataT;
typedef NetworkDataT<Mutable> NetworkDataAlias;

/**
 * RemoteStreamImpl is a base class for implementing remote stream functionality
 */
class RemoteStreamImpl : public NdnRtcComponent
{
  public:
	  //liupenghui, add type  MediaStreamParams::MediaStreamType type_
    RemoteStreamImpl(boost::asio::io_service &io,
                     const boost::shared_ptr<ndn::Face> &face,
                     const boost::shared_ptr<ndn::KeyChain> &keyChain,
                     const std::string &streamPrefix, MediaStreamParams::MediaStreamType type_);

    bool isMetaFetched() const;
    std::vector<std::string> getThreads() const;

    void fetchMeta();
    void start(const std::string &threadName);
    void setThread(const std::string &threadName);
    std::string getThread() const { return threadName_; }
    void stop();

    void setInterestLifetime(unsigned int lifetimeMs);
    void setTargetBufferSize(unsigned int bufferSizeMs);
    void setPipelineSize(unsigned int pipelineSizeSamples);
    void setLogger(boost::shared_ptr<ndnlog::new_api::Logger> logger);

    bool isVerified() const;
    bool isRunning() const { return isRunning_; };
    void attach(IRemoteStreamObserver *observer);
    void detach(IRemoteStreamObserver *observer);

    void setNeedsMeta(bool needMeta) { needMeta_ = needMeta; }
    statistics::StatisticsStorage getStatistics() const;
    ndn::Name getStreamPrefix() const;

  protected:
	//liupenghui, add type  MediaStreamParams::MediaStreamType type_
    MediaStreamParams::MediaStreamType type_;
    boost::asio::io_service &io_;
    bool needMeta_, isRunning_, cuedToRun_;
    int64_t metadataRequestedMs_;
    boost::shared_ptr<ndn::Face> face_;
    boost::shared_ptr<ndn::KeyChain> keyChain_;
    ndn::Name streamPrefix_;
    std::string threadName_;
    boost::shared_ptr<statistics::StatisticsStorage> sstorage_;

    std::vector<IRemoteStreamObserver *> observers_;
    boost::shared_ptr<MetaFetcher> metaFetcher_;
    boost::shared_ptr<MediaStreamMeta> streamMeta_;
    std::map<std::string, boost::shared_ptr<NetworkDataAlias>> threadsMeta_;

    boost::shared_ptr<IBuffer> buffer_;
    boost::shared_ptr<DrdEstimator> drdEstimator_;
    boost::shared_ptr<SegmentController> segmentController_;
    boost::shared_ptr<PipelineControl> pipelineControl_;
    boost::shared_ptr<BufferControl> bufferControl_;
    boost::shared_ptr<SampleEstimator> sampleEstimator_;
    boost::shared_ptr<IPlayoutControl> playoutControl_;
    boost::shared_ptr<IInterestQueue> interestQueue_;
    boost::shared_ptr<IPipeliner> pipeliner_;
    boost::shared_ptr<ILatencyControl> latencyControl_;
    boost::shared_ptr<IInterestControl> interestControl_;
    boost::shared_ptr<IPlayout> playout_;
    boost::shared_ptr<IPlaybackQueue> playbackQueue_;
    boost::shared_ptr<RetransmissionController> rtxController_;

    std::vector<ValidationErrorInfo> validationInfo_;

    void construct();

    void fetchThreadMeta(const std::string &threadName, const int64_t& metadataRequestedMs);
    void streamMetaFetched(NetworkDataAlias &);
    void threadMetaFetched(const std::string &thread, NetworkDataAlias &);
    virtual void initiateFetching();
    virtual void stopFetching();
    void addValidationInfo(const std::vector<ValidationErrorInfo> &);
    void notifyObservers(RemoteStream::Event ev);
};
}

#endif
