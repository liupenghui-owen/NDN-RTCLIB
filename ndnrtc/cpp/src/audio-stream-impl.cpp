//
// audio-stream-impl.cpp
//
//  Created by Peter Gusev on 21 April 2016.
//  Copyright 2013-2016 Regents of the University of California
//

#include "audio-stream-impl.hpp"
#include "async.hpp"
#include "clock.hpp"
#include "name-components.hpp"
#include "audio-controller.hpp"

#define BUNDLES_POOL_SIZE 10

using namespace std;
using namespace ndnrtc;
using namespace ndnrtc::statistics;
using namespace ndn;

AudioStreamImpl::AudioStreamImpl(const std::string &basePrefix,
                                 const MediaStreamSettings &settings)
    : MediaStreamBase(basePrefix, settings),
      streamRunning_(false)
{
    if (settings_.params_.type_ == MediaStreamParams::MediaStreamType::MediaStreamTypeVideo)
        throw runtime_error("Wrong media stream parameters type supplied (video instead of audio)");

    PublisherSettings ps;
    ps.sign_ = true;
    ps.keyChain_ = settings_.keyChain_;
    ps.memoryCache_ = cache_.get();
    ps.segmentWireLength_ = settings_.params_.producerParams_.segmentSize_;
    ps.freshnessPeriodMs_ = settings.params_.producerParams_.freshness_.sampleMs_;
    ps.statStorage_ = statStorage_.get();

    samplePublisher_ = boost::make_shared<CommonPacketPublisher>(ps);
    samplePublisher_->setDescription("sample-publisher-" + settings_.params_.streamName_);

    description_ = "astream-" + settings_.params_.streamName_;
    for (int i = 0; i < settings_.params_.getThreadNum(); ++i)
        if (settings_.params_.getAudioThread(i))
            add(settings_.params_.getAudioThread(i));

    for (int i = 0; i < BUNDLES_POOL_SIZE; ++i)
        bundlePool_.push_back(boost::make_shared<AudioBundlePacket>(CommonSegment::payloadLength(settings_.params_.producerParams_.segmentSize_)));
}

AudioStreamImpl::~AudioStreamImpl()
{
    // cleanup
    if (streamRunning_)
        stop();
}

void AudioStreamImpl::start()
{
    boost::lock_guard<boost::mutex> scopedLock(internalMutex_);

    for (auto it : threads_)
        if (!it.second->isRunning())
            it.second->start();

    if (!isPeriodicInvocationSet())
    {
        boost::shared_ptr<AudioStreamImpl> me = boost::dynamic_pointer_cast<AudioStreamImpl>(shared_from_this());
        setupInvocation(MediaStreamBase::MetaCheckIntervalMs,
                        boost::bind(&AudioStreamImpl::periodicInvocation, me));
    }

    streamRunning_ = true;
}

void AudioStreamImpl::stop()
{
    // here, lock should not be acquired as it may lead to deadlock
    // if one of the threads already dispatched onSampleBundle callback
    // BUT has not yet acquired internalMutex_ (see onSampleBundle method)
    // boost::lock_guard<boost::mutex> scopedLock(internalMutex_);
    // to avoid this, we need to dispatch call synchronously on audio thread

    for (auto it : threads_)
        it.second->stop();

    streamRunning_ = false;
}

void AudioStreamImpl::setLogger(boost::shared_ptr<ndnlog::new_api::Logger> logger)
{
    boost::lock_guard<boost::mutex> scopedLock(internalMutex_);
    MediaStreamBase::setLogger(logger);

    for (auto it : threads_)
        it.second->setLogger(logger);
    samplePublisher_->setLogger(logger);
    NdnRtcComponent::setLogger(logger);
}

std::vector<std::string>
AudioStreamImpl::getThreads() const
{
    boost::lock_guard<boost::mutex> scopedLock(internalMutex_);

    std::vector<std::string> threads;
    for (auto it : threads_)
        threads.push_back(it.first);
    return threads;
}

//******************************************************************************
void AudioStreamImpl::add(const MediaThreadParams *mp)
{
    const AudioThreadParams *params = static_cast<const AudioThreadParams *>(mp);
    if (threads_.find(params->threadName_) != threads_.end())
    {
        stringstream ss;
        ss << "Thread " << params->threadName_ << " has been added already";
        throw runtime_error(ss.str());
    }
    else
    {
        AudioCaptureParams p;
        p.deviceId_ = settings_.params_.captureDevice_.deviceId_;
        boost::shared_ptr<AudioThread> thread = 
            boost::make_shared<AudioThread>(*params,
                                            p, this,
                                            CommonSegment::payloadLength(settings_.params_.producerParams_.segmentSize_));

        // before adding new thread entry, acquire exclusive access
        {
            boost::lock_guard<boost::mutex> scopedLock(internalMutex_);
            threads_[params->threadName_] = thread;
            metaKeepers_[params->threadName_] = boost::make_shared<MetaKeeper>(params);
        }

        if (streamRunning_)
            threads_[params->threadName_]->start();
        LogDebugC << "added thread " << params->threadName_ << std::endl;
    }
}

void AudioStreamImpl::remove(const std::string &threadName)
{
    if (threads_.find(threadName) != threads_.end())
    {
        boost::shared_ptr<AudioThread> thread = threads_[threadName];

        {
            boost::lock_guard<boost::mutex> scopedLock(internalMutex_);
            threads_.erase(threadName);
            metaKeepers_.erase(threadName);
        }

        if (thread->isRunning())
            thread->stop();

        if (!threads_.size())
            streamRunning_ = false;
        LogDebugC << "removed thread " << threadName << std::endl;
    }
}

//******************************************************************************
void AudioStreamImpl::onSampleBundle(std::string threadName, uint64_t bundleNo,
                                     boost::shared_ptr<AudioBundlePacket> packet)
{
    if (!bundlePool_.size())
    {
        LogWarnC << "Audio bundle pool is drained. This may happen due to fast capturing "
                    "and slow publishing or too small segment size"
                 << std::endl;
        return;
    }

    Name n(streamPrefix_);
    n.append(threadName).appendSequenceNumber(bundleNo);
    boost::shared_ptr<AudioStreamImpl> me = boost::static_pointer_cast<AudioStreamImpl>(shared_from_this());
    boost::shared_ptr<AudioBundlePacket> bundle = bundlePool_.back();

    double packetRate = 0;
    bool threadRemoved = false;
    {
        boost::lock_guard<boost::mutex> scopedLock(internalMutex_);
        threadRemoved = (metaKeepers_.find(threadName) == metaKeepers_.end());
        bundle->swap(*packet);
        packetRate = metaKeepers_[threadName]->getMeta().getRate();
        bundlePool_.pop_back();
    }

    if (!threadRemoved)
        async::dispatchAsync(settings_.faceIo_, [this, packetRate, n, bundle, me]() {
            CommonHeader packetHdr;

            packetHdr.sampleRate_ = packetRate;
            packetHdr.publishTimestampMs_ = clock::millisecondTimestamp();
            packetHdr.publishUnixTimestamp_ = clock::unixTimestamp();
            bundle->setHeader(packetHdr);

            me->samplePublisher_->publish(n, *bundle);
            (*statStorage_)[Indicator::PublishedNum]++;

            {
				//liupenghui, bundle->clear();  for audio sample fetching... 	
                boost::lock_guard<boost::mutex> scopedLock(me->internalMutex_);
                bundle->clear();
                me->bundlePool_.push_back(bundle);
            }
        });
}

bool AudioStreamImpl::updateMeta()
{
    if (streamRunning_)
    {
        boost::lock_guard<boost::mutex> scopedLock(internalMutex_);
        for (auto it : metaKeepers_)
        {
            it.second->updateMeta(threads_[it.first]->getRate(),
                                  threads_[it.first]->getBundleNo());

            Name metaName(streamPrefix_);
            // TODO: appendVersion() should probably be gone once SegemntFetcher
            // is updated to work without version number
            metaName.append(it.first).append(NameComponents::NameComponentMeta).appendVersion(0);
            metadataPublisher_->publish(metaName, it.second->getMeta());
        }
    }
    return streamRunning_;
}

//******************************************************************************
void AudioStreamImpl::MetaKeeper::updateMeta(double rate, uint64_t bundleNo)
{
    rate_ = rate;
    bundleNo_ = bundleNo;
}

AudioThreadMeta
AudioStreamImpl::MetaKeeper::getMeta() const
{
    return boost::move(AudioThreadMeta(rate_, bundleNo_, ((AudioThreadParams *)params_)->codec_));
}
