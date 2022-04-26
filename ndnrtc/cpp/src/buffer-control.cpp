//
// buffer-control.cpp
//
//  Created by Peter Gusev on 07 June 2016.
//  Copyright 2013-2016 Regents of the University of California
//

#include "buffer-control.hpp"
#include "name-components.hpp"
#include "drd-estimator.hpp"
#include "frame-data.hpp"
#include "statistics.hpp"

using namespace ndnrtc;
using namespace ndnrtc::statistics;

BufferControl::BufferControl(const boost::shared_ptr<DrdEstimator> &drdEstimator,
                             const boost::shared_ptr<IBuffer> &buffer,
                             const boost::shared_ptr<statistics::StatisticsStorage> &storage) : drdEstimator_(drdEstimator), buffer_(buffer), sstorage_(storage)
{
    description_ = "buffer-control";
}

void BufferControl::attach(IBufferControlObserver *o)
{
    if (o)
        observers_.push_back(o);
}

void BufferControl::detach(IBufferControlObserver *o)
{
    std::vector<IBufferControlObserver *>::iterator it = std::find(observers_.begin(), observers_.end(), o);
    if (it != observers_.end())
        observers_.erase(it);
}

void BufferControl::segmentArrived(const boost::shared_ptr<WireSegment> &segment)
{
    if ((segment->getSampleClass() == SampleClass::Key ||
         segment->getSampleClass() == SampleClass::Delta) &&
        buffer_->isRequested(segment)) //segment->getStreamType() == MediaStreamParams::MediaStreamType::MediaStreamTypeAudio
    {
        BufferReceipt receipt = buffer_->received(segment);
        drdEstimator_->newValue(receipt.segment_->getDrdUsec() / 1000,
                                receipt.segment_->isOriginal(),
                                receipt.segment_->getDgen());
        LogTraceC << "segment RTT "
                  << receipt.segment_->getRoundTripDelayUsec() / 1000
                  << " orig " << receipt.segment_->isOriginal()
                  << " dgen " << receipt.segment_->getDgen()
                  << " drd " << receipt.segment_->getDrdUsec() / 1000
                  << " " << receipt.segment_-> getInfo().getSuffix(suffix_filter::Thread)
                  << std::endl;

        (*sstorage_)[Indicator::DrdOriginalEstimation] = drdEstimator_->getOriginalEstimation();
        (*sstorage_)[Indicator::DrdCachedEstimation] = drdEstimator_->getCachedEstimation();

        if (segment->isPacketHeaderSegment())
        {
            double rate = receipt.segment_->getData()->packetHeader().sampleRate_;
            for (auto &o : observers_)
                o->targetRateUpdate(rate);

            (*sstorage_)[Indicator::CurrentProducerFramerate] = rate;
        }

        // since we're receiving new segment, check previous slot state
        // if it was New (no segments previously received), then it means
        // that new sample is arriving and we need to notify observers
        // if (receipt.slot_->getFetchedNum() == 1)
        if (receipt.oldState_ == BufferSlot::New)
            for (auto &o : observers_)
                o->sampleArrived(segment->getPlaybackNo());
    }
    else
        LogTraceC << "data is not in the buffer: " << segment->getData()->getName() << std::endl;
}
