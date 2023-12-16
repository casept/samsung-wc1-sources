/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SourceBuffer.h"

#if ENABLE(MEDIA_SOURCE)

#include "AudioTrackList.h"
#include "Event.h"
#include "ExceptionCodePlaceholder.h"
#include "GenericEventQueue.h"
#include "HTMLMediaElement.h"
#include "InbandTextTrack.h"
#include "Logging.h"
#include "MediaDescription.h"
#include "MediaSample.h"
#include "MediaSource.h"
#include "SampleMap.h"
#include "SourceBufferPrivate.h"
#include "TextTrackList.h"
#include "TimeRanges.h"
#include "VideoTrackList.h"
#include <map>
#include <wtf/CurrentTime.h>
#include <wtf/NeverDestroyed.h>

namespace WebCore {

static double ExponentialMovingAverageCoefficient = 0.1;

// Allow hasCurrentTime() to be off by as much as the length of a 24fps video frame
static double CurrentTimeFudgeFactor = 1. / 24;

#if ENABLE(TIZEN_MSE)
static const size_t AudioTrackBufferSizeLimit = 2 * 1024 * 1024;
static const size_t VideoTrackBufferSizeLimit = 25 * 1024 * 1024;
#endif

struct SourceBuffer::TrackBuffer {
    MediaTime lastDecodeTimestamp;
    MediaTime lastFrameDuration;
    MediaTime highestPresentationTimestamp;
    MediaTime lastEnqueuedPresentationTime;
    bool needRandomAccessFlag;
    bool enabled;
    SampleMap samples;
    SampleMap::MapType decodeQueue;
    RefPtr<MediaDescription> description;

    TrackBuffer()
        : lastDecodeTimestamp(MediaTime::invalidTime())
        , lastFrameDuration(MediaTime::invalidTime())
        , highestPresentationTimestamp(MediaTime::invalidTime())
        , lastEnqueuedPresentationTime(MediaTime::invalidTime())
        , needRandomAccessFlag(true)
        , enabled(false)
    {
    }

#if ENABLE(TIZEN_MSE)
    bool hasRoomForData(const size_t newData);
#endif
};

#if ENABLE(TIZEN_MSE)
bool SourceBuffer::TrackBuffer::hasRoomForData(const size_t newData)
{
    if ((description->isVideo() && samples.size() + newData > VideoTrackBufferSizeLimit)
        || (description->isAudio() && samples.size() + newData > AudioTrackBufferSizeLimit)) {
        return false;
    };
    return true;
}
#endif

PassRefPtr<SourceBuffer> SourceBuffer::create(PassRefPtr<SourceBufferPrivate> sourceBufferPrivate, MediaSource* source)
{
    RefPtr<SourceBuffer> sourceBuffer(adoptRef(new SourceBuffer(std::move(sourceBufferPrivate), source)));
    sourceBuffer->suspendIfNeeded();
    return sourceBuffer.release();
}

SourceBuffer::SourceBuffer(PassRefPtr<SourceBufferPrivate> sourceBufferPrivate, MediaSource* source)
    : ActiveDOMObject(source->scriptExecutionContext())
    , m_private(std::move(sourceBufferPrivate))
    , m_source(source)
    , m_asyncEventQueue(this)
    , m_updating(false)
    , m_appendBufferTimer(this, &SourceBuffer::appendBufferTimerFired)
    , m_highestPresentationEndTimestamp(MediaTime::invalidTime())
    , m_receivedFirstInitializationSegment(false)
    , m_buffered(TimeRanges::create())
    , m_active(false)
    , m_appendState(WaitingForSegment)
    , m_timeOfBufferingMonitor(monotonicallyIncreasingTime())
    , m_bufferedSinceLastMonitor(0)
    , m_averageBufferRate(0)
    , m_pendingRemoveStart(MediaTime::invalidTime())
    , m_pendingRemoveEnd(MediaTime::invalidTime())
    , m_removeTimer(this, &SourceBuffer::removeTimerFired)
{
    ASSERT(m_private);
    ASSERT(m_source);

    m_private->setClient(this);
}

SourceBuffer::~SourceBuffer()
{
    ASSERT(isRemoved());

    m_private->setClient(0);
}

PassRefPtr<TimeRanges> SourceBuffer::buffered(ExceptionCode& ec) const
{
    // Section 3.1 buffered attribute steps.
    // https://dvcs.w3.org/hg/html-media/raw-file/default/media-source/media-source.html#attributes-1
    // 1. If this object has been removed from the sourceBuffers attribute of the parent media source then throw an
    //    INVALID_STATE_ERR exception and abort these steps.
    if (isRemoved()) {
        ec = INVALID_STATE_ERR;
        return 0;
    }

    // 2. Return a new static normalized TimeRanges object for the media segments buffered.
    return m_buffered->copy();
}

const RefPtr<TimeRanges>& SourceBuffer::buffered() const
{
    return m_buffered;
}

double SourceBuffer::timestampOffset() const
{
    return m_timestampOffset.toDouble();
}

void SourceBuffer::setTimestampOffset(double offset, ExceptionCode& ec)
{
    // Section 3.1 timestampOffset attribute setter steps.
    // https://dvcs.w3.org/hg/html-media/raw-file/default/media-source/media-source.html#attributes-1
    // 1. Let new timestamp offset equal the new value being assigned to this attribute.
    // 2. If this object has been removed from the sourceBuffers attribute of the parent media source, then throw an
    //    INVALID_STATE_ERR exception and abort these steps.
    // 3. If the updating attribute equals true, then throw an INVALID_STATE_ERR exception and abort these steps.
    if (isRemoved() || m_updating) {
        ec = INVALID_STATE_ERR;
        return;
    }

    // 4. If the readyState attribute of the parent media source is in the "ended" state then run the following steps:
    // 4.1 Set the readyState attribute of the parent media source to "open"
    // 4.2 Queue a task to fire a simple event named sourceopen at the parent media source.
    m_source->openIfInEndedState();

    // 5. If the append state equals PARSING_MEDIA_SEGMENT, then throw an INVALID_STATE_ERR and abort these steps.
    if (m_appendState == ParsingMediaSegment) {
        ec = INVALID_STATE_ERR;
        return;
    }

    // FIXME: Add step 6 text when mode attribute is implemented.
    // 7. Update the attribute to the new value.
#if ENABLE(TIZEN_MSE)
    // FIXME: this is arbitraly chosen number, can't be 1, as it's casted to int, so we would lose precision from 0.5 for example
    // if absolute precision is needed it should be changed to increase each MediaTime by it's own timescale
    const int offsetScale = 100000;
    m_timestampOffset = MediaTime::createWithDouble(offset, offsetScale);
#else
    m_timestampOffset = offset;
#endif
}

void SourceBuffer::appendBuffer(PassRefPtr<ArrayBuffer> data, ExceptionCode& ec)
{
    // Section 3.2 appendBuffer()
    // https://dvcs.w3.org/hg/html-media/raw-file/default/media-source/media-source.html#widl-SourceBuffer-appendBuffer-void-ArrayBufferView-data
    // 1. If data is null then throw an INVALID_ACCESS_ERR exception and abort these steps.
    if (!data) {
        ec = INVALID_ACCESS_ERR;
        return;
    }

    appendBufferInternal(static_cast<unsigned char*>(data->data()), data->byteLength(), ec);
}

void SourceBuffer::appendBuffer(PassRefPtr<ArrayBufferView> data, ExceptionCode& ec)
{
    // Section 3.2 appendBuffer()
    // https://dvcs.w3.org/hg/html-media/raw-file/default/media-source/media-source.html#widl-SourceBuffer-appendBuffer-void-ArrayBufferView-data
    // 1. If data is null then throw an INVALID_ACCESS_ERR exception and abort these steps.
    if (!data) {
        ec = INVALID_ACCESS_ERR;
        return;
    }
    appendBufferInternal(static_cast<unsigned char*>(data->baseAddress()), data->byteLength(), ec);
}

void SourceBuffer::abort(ExceptionCode& ec)
{
    // Section 3.2 abort() method steps.
    // https://dvcs.w3.org/hg/html-media/raw-file/default/media-source/media-source.html#widl-SourceBuffer-abort-void
    // 1. If this object has been removed from the sourceBuffers attribute of the parent media source
    //    then throw an INVALID_STATE_ERR exception and abort these steps.
    // 2. If the readyState attribute of the parent media source is not in the "open" state
    //    then throw an INVALID_STATE_ERR exception and abort these steps.
    if (isRemoved() || !m_source->isOpen()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    // 3. If the sourceBuffer.updating attribute equals true, then run the following steps: ...
    abortIfUpdating();

    // 4. Run the reset parser state algorithm.
    m_private->abort();
#if ENABLE(TIZEN_MSE)
    for (auto i = m_trackBufferMap.values().begin(); i != m_trackBufferMap.values().end(); ++i) {
        i->lastDecodeTimestamp = MediaTime::invalidTime();
        i->lastFrameDuration = MediaTime::invalidTime();
        i->highestPresentationTimestamp = MediaTime::invalidTime();
        i->needRandomAccessFlag = true;
    }
    m_timestampOffset = MediaTime::zeroTime();
    m_highestPresentationEndTimestamp = MediaTime::invalidTime();
#endif
    // FIXME(229408) Add steps 5-6 update appendWindowStart & appendWindowEnd.
}

void SourceBuffer::remove(double start, double end, ExceptionCode& ec)
{
    removeInternal(start, end, ec, false);
}

void SourceBuffer::removeInternal(double start, double end, ExceptionCode& ec, bool synchronous )
{
    // Section 3.2 remove() method steps.
    // 1. If start is negative or greater than duration, then throw an InvalidAccessError exception and abort these steps.
    // 2. If end is less than or equal to start, then throw an InvalidAccessError exception and abort these steps.
    if (start < 0 || (m_source && (std::isnan(m_source->duration()) || start > m_source->duration())) || end <= start) {
        ec = INVALID_ACCESS_ERR;
        return;
    }

    // 3. If this object has been removed from the sourceBuffers attribute of the parent media source then throw an
    //    InvalidStateError exception and abort these steps.
    // 4. If the updating attribute equals true, then throw an InvalidStateError exception and abort these steps.
    if (isRemoved() || m_updating) {
        ec = INVALID_ACCESS_ERR;
        return;
    }

    // 5. If the readyState attribute of the parent media source is in the "ended" state then run the following steps:
    // 5.1. Set the readyState attribute of the parent media source to "open"
    // 5.2. Queue a task to fire a simple event named sourceopen at the parent media source .
    m_source->openIfInEndedState();

    // 6. Set the updating attribute to true.
    m_updating = true;

    // 7. Queue a task to fire a simple event named updatestart at this SourceBuffer object.
    scheduleEvent(eventNames().updatestartEvent);

    // 8. Return control to the caller and run the rest of the steps asynchronously.
    m_pendingRemoveStart = MediaTime::createWithDouble(start);
    m_pendingRemoveEnd = MediaTime::createWithDouble(end);

    if (synchronous)
        removeTimerFired(0);
    else
        m_removeTimer.startOneShot(0);
}

void SourceBuffer::abortIfUpdating()
{
    // Section 3.2 abort() method step 3 substeps.
    // https://dvcs.w3.org/hg/html-media/raw-file/default/media-source/media-source.html#widl-SourceBuffer-abort-void

    if (!m_updating)
        return;

    // 3.1. Abort the buffer append and stream append loop algorithms if they are running.
    m_appendBufferTimer.stop();
    m_pendingAppendData.clear();

    m_removeTimer.stop();
    m_pendingRemoveStart = MediaTime::invalidTime();
    m_pendingRemoveEnd = MediaTime::invalidTime();

    // 3.2. Set the updating attribute to false.
    m_updating = false;

    // 3.3. Queue a task to fire a simple event named abort at this SourceBuffer object.
    scheduleEvent(eventNames().abortEvent);

    // 3.4. Queue a task to fire a simple event named updateend at this SourceBuffer object.
    scheduleEvent(eventNames().updateendEvent);
}

void SourceBuffer::removedFromMediaSource()
{
    if (isRemoved())
        return;

    abortIfUpdating();

    m_private->removedFromMediaSource();
    m_source = 0;
    m_asyncEventQueue.close();
#if ENABLE(TIZEN_MSE)
    if (m_appendBufferTimer.isActive())
        m_appendBufferTimer.stop();
#endif // ENABLE(TIZEN_MSE)
}

void SourceBuffer::sourceBufferPrivateSeekToTime(SourceBufferPrivate*, const MediaTime& time)
{
    LOG(Media, "SourceBuffer::sourceBufferPrivateSeekToTime(%p)", this);

    for (auto trackBufferIterator = m_trackBufferMap.begin(); trackBufferIterator != m_trackBufferMap.end(); ++trackBufferIterator) {
        TrackBuffer& trackBuffer = trackBufferIterator->value;
        AtomicString trackID = trackBufferIterator->key;

        // Find the sample which contains the current presentation time.
        auto currentSamplePTSIterator = trackBuffer.samples.findSampleContainingPresentationTime(time);

        if (currentSamplePTSIterator == trackBuffer.samples.presentationEnd()) {
            trackBuffer.decodeQueue.clear();
            m_private->flushAndEnqueueNonDisplayingSamples(Vector<RefPtr<MediaSample>>(), trackID);
            continue;
        }

        // Seach backward for the previous sync sample.
        MediaTime currentSampleDecodeTime = currentSamplePTSIterator->second->decodeTime();
        auto currentSampleDTSIterator = trackBuffer.samples.findSampleWithDecodeTime(currentSampleDecodeTime);
        ASSERT(currentSampleDTSIterator != trackBuffer.samples.decodeEnd());

        auto reverseCurrentSampleIter = --SampleMap::reverse_iterator(currentSampleDTSIterator);
        auto reverseLastSyncSampleIter = trackBuffer.samples.findSyncSamplePriorToDecodeIterator(reverseCurrentSampleIter);
        if (reverseLastSyncSampleIter == trackBuffer.samples.reverseDecodeEnd()) {
            trackBuffer.decodeQueue.clear();
            m_private->flushAndEnqueueNonDisplayingSamples(Vector<RefPtr<MediaSample>>(), trackID);
            continue;
        }

        Vector<RefPtr<MediaSample>> nonDisplayingSamples;
        for (auto iter = reverseLastSyncSampleIter; iter != reverseCurrentSampleIter; --iter)
            nonDisplayingSamples.append(iter->second);

        m_private->flushAndEnqueueNonDisplayingSamples(nonDisplayingSamples, trackID);

        // Fill the decode queue with the remaining samples.
        trackBuffer.decodeQueue.clear();
        for (auto iter = currentSampleDTSIterator; iter != trackBuffer.samples.decodeEnd(); ++iter)
            trackBuffer.decodeQueue.insert(*iter);

        provideMediaData(trackBuffer, trackID);
    }

    m_source->monitorSourceBuffers();
}

MediaTime SourceBuffer::sourceBufferPrivateFastSeekTimeForMediaTime(SourceBufferPrivate*, const MediaTime& targetTime, const MediaTime& negativeThreshold, const MediaTime& positiveThreshold)
{
    MediaTime seekTime = targetTime;
    MediaTime lowerBoundTime = targetTime - negativeThreshold;
    MediaTime upperBoundTime = targetTime + positiveThreshold;

    for (auto trackBufferIterator = m_trackBufferMap.begin(); trackBufferIterator != m_trackBufferMap.end(); ++trackBufferIterator) {
        TrackBuffer& trackBuffer = trackBufferIterator->value;

        // Find the sample which contains the target time time.
        auto futureSyncSampleIterator = trackBuffer.samples.findSyncSampleAfterPresentationTime(targetTime, positiveThreshold);
        auto pastSyncSampleIterator = trackBuffer.samples.findSyncSamplePriorToPresentationTime(targetTime, negativeThreshold);
        auto upperBound = trackBuffer.samples.decodeEnd();
        auto lowerBound = trackBuffer.samples.reverseDecodeEnd();

        if (futureSyncSampleIterator == upperBound && pastSyncSampleIterator == lowerBound)
            continue;

        MediaTime futureSeekTime = MediaTime::positiveInfiniteTime();
        if (futureSyncSampleIterator != upperBound) {
            RefPtr<MediaSample>& sample = futureSyncSampleIterator->second;
            futureSeekTime = sample->presentationTime();
        }

        MediaTime pastSeekTime = MediaTime::negativeInfiniteTime();
        if (pastSyncSampleIterator != lowerBound) {
            RefPtr<MediaSample>& sample = pastSyncSampleIterator->second;
            pastSeekTime = sample->presentationTime();
        }

        MediaTime trackSeekTime = abs(targetTime - futureSeekTime) < abs(targetTime - pastSeekTime) ? futureSeekTime : pastSeekTime;
        if (abs(targetTime - trackSeekTime) > abs(targetTime - seekTime))
            seekTime = trackSeekTime;
    }

    return seekTime;
}

bool SourceBuffer::hasPendingActivity() const
{
    return m_source;
}

void SourceBuffer::stop()
{
    m_appendBufferTimer.stop();
    m_removeTimer.stop();
}

ScriptExecutionContext* SourceBuffer::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

const AtomicString& SourceBuffer::interfaceName() const
{
    return eventNames().interfaceForSourceBuffer;
}

EventTargetData* SourceBuffer::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* SourceBuffer::ensureEventTargetData()
{
    return &m_eventTargetData;
}

bool SourceBuffer::isRemoved() const
{
    return !m_source;
}

void SourceBuffer::scheduleEvent(const AtomicString& eventName)
{
    RefPtr<Event> event = Event::create(eventName, false, false);
    event->setTarget(this);

    m_asyncEventQueue.enqueueEvent(event.release());
}

#if ENABLE(TIZEN_MSE)
void SourceBuffer::sendEvent(const AtomicString& eventName) {
    RefPtr<Event> event = Event::create(eventName, false, false);
    event->setTarget(this);
    PassRefPtr<Event> passEvent = event.release();
    fireEventListeners(passEvent.get());
}

#endif

void SourceBuffer::appendBufferInternal(unsigned char* data, unsigned size, ExceptionCode& ec)
{
    // Section 3.2 appendBuffer()
    // https://dvcs.w3.org/hg/html-media/raw-file/default/media-source/media-source.html#widl-SourceBuffer-appendBuffer-void-ArrayBufferView-data

    // Step 1 is enforced by the caller.
    // 2. Run the prepare append algorithm.
    // Section 3.5.4 Prepare AppendAlgorithm

    // 1. If the SourceBuffer has been removed from the sourceBuffers attribute of the parent media source
    // then throw an INVALID_STATE_ERR exception and abort these steps.
    // 2. If the updating attribute equals true, then throw an INVALID_STATE_ERR exception and abort these steps.
    if (isRemoved() || m_updating) {
        ec = INVALID_STATE_ERR;
        return;
    }

    // 3. If the readyState attribute of the parent media source is in the "ended" state then run the following steps:
    // 3.1. Set the readyState attribute of the parent media source to "open"
    // 3.2. Queue a task to fire a simple event named sourceopen at the parent media source .
    m_source->openIfInEndedState();

    // 4. Run the coded frame eviction algorithm.
#if ENABLE(TIZEN_MSE)
    evictCodedFrames(size);
#else
    m_private->evictCodedFrames();
#endif

    // 5. If the buffer full flag equals true, then throw a QUOTA_EXCEEDED_ERR exception and abort these step.
#if ENABLE(TIZEN_MSE)
    if (isFull()) {
#else
    if (m_private->isFull()) {
#endif
        ec = QUOTA_EXCEEDED_ERR;
        return;
    }

    // NOTE: Return to 3.2 appendBuffer()
    // 3. Add data to the end of the input buffer.
    m_pendingAppendData.append(data, size);

    // 4. Set the updating attribute to true.
    m_updating = true;

    // 5. Queue a task to fire a simple event named updatestart at this SourceBuffer object.
    scheduleEvent(eventNames().updatestartEvent);

    // 6. Asynchronously run the buffer append algorithm.
    m_appendBufferTimer.startOneShot(0);
}

void SourceBuffer::appendBufferTimerFired(Timer<SourceBuffer>*)
{
    ASSERT(m_updating);
#if ENABLE(TIZEN_MSE)
    ASSERT(m_source);
#endif // ENABLE(TIZEN_MSE)

    // Section 3.5.5 Buffer Append Algorithm
    // https://dvcs.w3.org/hg/html-media/raw-file/default/media-source/media-source.html#sourcebuffer-buffer-append

    // 1. Run the segment parser loop algorithm.
    size_t appendSize = m_pendingAppendData.size();
    if (!appendSize) {
        // Resize buffer for 0 byte appends so we always have a valid pointer.
        // We need to convey all appends, even 0 byte ones to |m_private| so
        // that it can clear its end of stream state if necessary.
        m_pendingAppendData.resize(1);
    }

    // Section 3.5.1 Segment Parser Loop
    // https://dvcs.w3.org/hg/html-media/raw-file/tip/media-source/media-source.html#sourcebuffer-segment-parser-loop
    // When the segment parser loop algorithm is invoked, run the following steps:

    SourceBufferPrivate::AppendResult result = SourceBufferPrivate::AppendSucceeded;
    do {
        // 1. Loop Top: If the input buffer is empty, then jump to the need more data step below.
        if (!m_pendingAppendData.size())
            break;

        result = m_private->append(m_pendingAppendData.data(), appendSize);
        m_pendingAppendData.clear();

        // 2. If the input buffer contains bytes that violate the SourceBuffer byte stream format specification,
        // then run the end of stream algorithm with the error parameter set to "decode" and abort this algorithm.
        if (result == SourceBufferPrivate::ParsingFailed) {
            m_source->streamEndedWithError(decodeError(), IgnorableExceptionCode());
            break;
        }

        // NOTE: Steps 3 - 6 enforced by sourceBufferPrivateDidReceiveInitializationSegment() and
        // sourceBufferPrivateDidReceiveSample below.

        // 7. Need more data: Return control to the calling algorithm.
    } while (0);

    // NOTE: return to Section 3.5.5
    // 2.If the segment parser loop algorithm in the previous step was aborted, then abort this algorithm.
    if (result != SourceBufferPrivate::AppendSucceeded)
        return;

    // 3. Set the updating attribute to false.
    // FIXME: This is work-around code for passing MSE/EME TC. 'm_updating' has to be set 'false' just before 'updateEvent' is sent.
    m_updating = false;
#if !ENABLE(TIZEN_MSE)
    // 4. Queue a task to fire a simple event named update at this SourceBuffer object.
    scheduleEvent(eventNames().updateEvent);

    // 5. Queue a task to fire a simple event named updateend at this SourceBuffer object.
    scheduleEvent(eventNames().updateendEvent);

    m_source->monitorSourceBuffers();

    for (auto iter = m_trackBufferMap.begin(), end = m_trackBufferMap.end(); iter != end; ++iter)
        provideMediaData(iter->value, iter->key);
#endif
}

#if ENABLE(TIZEN_MSE)
void SourceBuffer::parseComplete()
{
    // 4. Queue a task to fire a simple event named update at this SourceBuffer object.
    sendEvent(eventNames().updateEvent);

    // 5. Queue a task to fire a simple event named updateend at this SourceBuffer object.
    sendEvent(eventNames().updateendEvent);

    if (!isRemoved())
        m_source->monitorSourceBuffers();

    for (auto iter = m_trackBufferMap.begin(), end = m_trackBufferMap.end(); iter != end; ++iter)
        provideMediaData(iter->value, iter->key);
}
#endif

void SourceBuffer::removeCodedFrames(const MediaTime& start, const MediaTime& end)
{
    // 3.5.9 Coded Frame Removal Algorithm
    // https://dvcs.w3.org/hg/html-media/raw-file/tip/media-source/media-source.html#sourcebuffer-coded-frame-removal

    // 1. Let start be the starting presentation timestamp for the removal range.
    MediaTime durationMediaTime = MediaTime::createWithDouble(m_source->duration());
    MediaTime currentMediaTime = MediaTime::createWithDouble(m_source->currentTime());

    // 2. Let end be the end presentation timestamp for the removal range.
    // 3. For each track buffer in this source buffer, run the following steps:
    for (auto iter = m_trackBufferMap.begin(), iter_end = m_trackBufferMap.end(); iter != iter_end; ++iter) {
        TrackBuffer& trackBuffer = iter->value;
        // 3.1. Let remove end timestamp be the current value of duration
        SampleMap::iterator removeEnd = trackBuffer.samples.presentationEnd();

        // 3.2 If this track buffer has a random access point timestamp that is greater than or equal to end, then update
        // remove end timestamp to that random access point timestamp.
        SampleMap::iterator nextRandomAccessPoint = trackBuffer.samples.findSyncSampleAfterPresentationTime(end);
#if ENABLE(TIZEN_MSE)
        if ((nextRandomAccessPoint != trackBuffer.samples.decodeEnd()) && nextRandomAccessPoint->first >= end)
#else
        if (nextRandomAccessPoint->first >= end)
#endif
            removeEnd = trackBuffer.samples.findSampleContainingPresentationTime(nextRandomAccessPoint->first);

        // 3.3 Remove all media data, from this track buffer, that contain starting timestamps greater than or equal to
        // start and less than the remove end timestamp.
        SampleMap::iterator removeStart = trackBuffer.samples.findSampleAfterPresentationTime(start);

#if ENABLE(TIZEN_MSE)
        if (removeEnd == trackBuffer.samples.presentationEnd()) { // if we remove from the end of trackbuffer we should update
            SampleMap::iterator lastNotErasedSample = removeStart;

            if (lastNotErasedSample == trackBuffer.samples.presentationBegin()) {
                trackBuffer.highestPresentationTimestamp = MediaTime::invalidTime();
            } else {
                lastNotErasedSample--;
                trackBuffer.highestPresentationTimestamp = lastNotErasedSample->second->presentationTime() + lastNotErasedSample->second->duration();
            }
        }
#endif
        SampleMap::MapType erasedSamples(removeStart, removeEnd);
        RefPtr<TimeRanges> erasedRanges = TimeRanges::create();
        MediaTime microsecond(1, 1000000);
        for (auto erasedIt = erasedSamples.begin(), erasedIt_end = erasedSamples.end(); erasedIt != erasedIt_end; ++erasedIt) {
            trackBuffer.samples.removeSample(erasedIt->second.get());
            double startTime = erasedIt->first.toDouble();
            double endTime = ((erasedIt->first + erasedIt->second->duration()) + microsecond).toDouble();
            erasedRanges->add(startTime, endTime);
#if ENABLE(TIZEN_MSE)
        if (erasedIt->first < m_highestPresentationEndTimestamp && m_highestPresentationEndTimestamp <= erasedIt_end->first)
            m_highestPresentationEndTimestamp = erasedIt->first;
#endif
        }

        erasedRanges->invert();
        m_buffered->intersectWith(erasedRanges.get());

        // 3.4 If this object is in activeSourceBuffers, the current playback position is greater than or equal to start
        // and less than the remove end timestamp, and HTMLMediaElement.readyState is greater than HAVE_METADATA, then set
        // the HTMLMediaElement.readyState attribute to HAVE_METADATA and stall playback.
        if (m_active && currentMediaTime >= start && currentMediaTime < end && m_private->readyState() > MediaPlayer::HaveMetadata)
#if ENABLE(TIZEN_MSE)
            setReadyState(MediaPlayer::HaveMetadata);
#else
            m_private->setReadyState(MediaPlayer::HaveMetadata);
#endif
    }

    // 4. If buffer full flag equals true and this object is ready to accept more bytes, then set the buffer full flag to false.
    // No-op
}

void SourceBuffer::removeTimerFired(Timer<SourceBuffer>*)
{
    ASSERT(m_updating);
    ASSERT(m_pendingRemoveStart.isValid());
    ASSERT(m_pendingRemoveStart < m_pendingRemoveEnd);

    // Section 3.2 remove() method steps
    // https://dvcs.w3.org/hg/html-media/raw-file/default/media-source/media-source.html#widl-SourceBuffer-remove-void-double-start-double-end

    // 9. Run the coded frame removal algorithm with start and end as the start and end of the removal range.
    removeCodedFrames(m_pendingRemoveStart, m_pendingRemoveEnd);

    // 10. Set the updating attribute to false.
    m_updating = false;
    m_pendingRemoveStart = MediaTime::invalidTime();
    m_pendingRemoveEnd = MediaTime::invalidTime();

    // 11. Queue a task to fire a simple event named update at this SourceBuffer object.
    scheduleEvent(eventNames().updateEvent);

    // 12. Queue a task to fire a simple event named updateend at this SourceBuffer object.
    scheduleEvent(eventNames().updateendEvent);
}

#if ENABLE(TIZEN_MSE)
void SourceBuffer::evictCodedFrames(const size_t newData)
{
    // 3.5.10 Coded Frame Eviction Algorithm
    // 1. Let new data equal the data that is about to be appended to this SourceBuffer.
    // 2. If the buffer full flag equals false, then abort these steps.

    for (auto iter = m_trackBufferMap.begin(), end = m_trackBufferMap.end(); iter != end; ++iter) {
        TrackBuffer& trackBuffer = iter->value;
        if (trackBuffer.samples.size() > 0 && !trackBuffer.hasRoomForData(newData)) {
            // 3. Let removal ranges equal a list of presentation time ranges that can be evicted from the presentation to make room for the new data.
            MediaTime currentMediaTime = MediaTime::createWithDouble(m_source->currentTime());
            MediaTime startTime = MediaTime::createWithDouble(0);

            // remove all samples from the beginning to currentTime
            SampleMap::reverse_iterator sample = trackBuffer.samples.findSyncSamplePriorToPresentationTime(currentMediaTime);
            // 4. For each range in removal ranges, run the coded frame removal algorithm with start and end equal to the removal range start and end timestamp respectively.

            if (sample != trackBuffer.samples.reverseDecodeEnd()) {
                TIZEN_LOGI("(1): remove frames from %f to %f", startTime.toDouble(), sample->first.toDouble());
                removeCodedFrames(startTime, sample->first);
            }

            // remove samples from the end until having enough space for new data
            size_t size = trackBuffer.samples.size();

            if (size == 0)
                return;

            sample = trackBuffer.samples.reversePresentationBegin();
            MediaTime removeStart = sample->first, removeEnd = sample->first + sample->second->duration();

            size_t limit = trackBuffer.description->isAudio() ? AudioTrackBufferSizeLimit : VideoTrackBufferSizeLimit;

            for (auto it = sample, end = trackBuffer.samples.reversePresentationEnd();
                (it != end) && (size + newData >= limit); ++it) {
                removeStart = it->first;
                size -= it->second->size();
            };

            // 4. For each range in removal ranges, run the coded frame removal algorithm with start and end equal to the removal range start and end timestamp respectively.
            TIZEN_LOGI("(2): remove frames from %f to %f", removeStart.toDouble(), removeEnd.toDouble());
            removeCodedFrames(removeStart, removeEnd);
        };
    };
}

bool SourceBuffer::hasRoomForData(const size_t newData)
{
    for (auto iter = m_trackBufferMap.begin(), end = m_trackBufferMap.end(); iter != end; ++iter) {
        TrackBuffer& trackBuffer = iter->value;
        if (!trackBuffer.hasRoomForData(newData))
            return false;
    };
    return true;
}

bool SourceBuffer::isFull()
{
    return !hasRoomForData(0);
}
#endif

const AtomicString& SourceBuffer::decodeError()
{
    static NeverDestroyed<AtomicString> decode("decode", AtomicString::ConstructFromLiteral);
    return decode;
}

const AtomicString& SourceBuffer::networkError()
{
    static NeverDestroyed<AtomicString> network("network", AtomicString::ConstructFromLiteral);
    return network;
}

VideoTrackList* SourceBuffer::videoTracks()
{
    if (!m_source->mediaElement())
        return 0;

    if (!m_videoTracks)
        m_videoTracks = VideoTrackList::create(m_source->mediaElement(), ActiveDOMObject::scriptExecutionContext());

    return m_videoTracks.get();
}

AudioTrackList* SourceBuffer::audioTracks()
{
    if (!m_source->mediaElement())
        return 0;

    if (!m_audioTracks)
        m_audioTracks = AudioTrackList::create(m_source->mediaElement(), ActiveDOMObject::scriptExecutionContext());

    return m_audioTracks.get();
}

TextTrackList* SourceBuffer::textTracks()
{
    if (!m_source->mediaElement())
        return 0;

    if (!m_textTracks)
        m_textTracks = TextTrackList::create(m_source->mediaElement(), ActiveDOMObject::scriptExecutionContext());

    return m_textTracks.get();
}

void SourceBuffer::setActive(bool active)
{
    if (m_active == active)
        return;

    m_active = active;
    m_private->setActive(active);
    m_source->sourceBufferDidChangeAcitveState(this, active);
}

#if ENABLE(TIZEN_MSE)
MediaPlayer::ReadyState SourceBuffer::readyState()
{
    if (m_private)
        return m_private->readyState();

    return MediaPlayer::HaveNothing;
}

void SourceBuffer::setReadyState(MediaPlayer::ReadyState readyState)
{
    if (m_private)
        m_private->setReadyState(readyState);

    if (m_source)
        m_source->monitorSourceBuffers();
}

MediaTime SourceBuffer::fastSeekTimeForMediaTime(const MediaTime& time, const MediaTime& negativeThreshold, const MediaTime& positiveThreshold)
{
    return sourceBufferPrivateFastSeekTimeForMediaTime(0, time, negativeThreshold, positiveThreshold);
}

void SourceBuffer::prepareSeeking(const MediaTime& time)
{
    m_private->prepareSeeking(time);
}

void SourceBuffer::didSeek(const MediaTime& time)
{
    m_private->didSeek(time);
}
#endif // ENABLE(TIZEN_MSE)

void SourceBuffer::sourceBufferPrivateDidEndStream(SourceBufferPrivate*, const WTF::AtomicString& error)
{
    m_source->endOfStream(error, IgnorableExceptionCode());
}

void SourceBuffer::sourceBufferPrivateDidReceiveInitializationSegment(SourceBufferPrivate*, const InitializationSegment& segment)
{
    if (isRemoved())
        return;

#if ENABLE(TIZEN_MSE)
    m_source->setSize(IntSize(segment.m_width, segment.m_height));
#endif

    // 3.5.7 Initialization Segment Received
    // https://dvcs.w3.org/hg/html-media/raw-file/default/media-source/media-source.html#sourcebuffer-init-segment-received
    // 1. Update the duration attribute if it currently equals NaN:
    if (std::isnan(m_source->duration())) {
        // ↳ If the initialization segment contains a duration:
        //   Run the duration change algorithm with new duration set to the duration in the initialization segment.
        // ↳ Otherwise:
        //   Run the duration change algorithm with new duration set to positive Infinity.
        MediaTime newDuration = segment.duration.isValid() ? segment.duration : MediaTime::positiveInfiniteTime();
        m_source->setDuration(newDuration.toDouble(), IGNORE_EXCEPTION);
    }

    // 2. If the initialization segment has no audio, video, or text tracks, then run the end of stream
    // algorithm with the error parameter set to "decode" and abort these steps.
    if (!segment.audioTracks.size() && !segment.videoTracks.size() && !segment.textTracks.size())
        m_source->endOfStream(decodeError(), IgnorableExceptionCode());


    // 3. If the first initialization segment flag is true, then run the following steps:
    if (m_receivedFirstInitializationSegment) {
        if (!validateInitializationSegment(segment)) {
            m_source->endOfStream(decodeError(), IgnorableExceptionCode());
            return;
        }
        // 3.2 Add the appropriate track descriptions from this initialization segment to each of the track buffers.
        // NOTE: No changes to make
    }

    // 4. Let active track flag equal false.
    bool activeTrackFlag = false;

    // 5. If the first initialization segment flag is false, then run the following steps:
    if (!m_receivedFirstInitializationSegment) {
        // 5.1 If the initialization segment contains tracks with codecs the user agent does not support,
        // then run the end of stream algorithm with the error parameter set to "decode" and abort these steps.
        // NOTE: This check is the responsibility of the SourceBufferPrivate.

        // 5.2 For each audio track in the initialization segment, run following steps:
        for (auto it = segment.audioTracks.begin(); it != segment.audioTracks.end(); ++it) {
            AudioTrackPrivate* audioTrackPrivate = it->track.get();

            // 5.2.1 Let new audio track be a new AudioTrack object.
            // 5.2.2 Generate a unique ID and assign it to the id property on new video track.
            RefPtr<AudioTrack> newAudioTrack = AudioTrack::create(this, audioTrackPrivate);
            newAudioTrack->setSourceBuffer(this);

            // 5.2.3 If audioTracks.length equals 0, then run the following steps:
            if (!audioTracks()->length()) {
                // 5.2.3.1 Set the enabled property on new audio track to true.
                newAudioTrack->setEnabled(true);

                // 5.2.3.2 Set active track flag to true.
                activeTrackFlag = true;
            }

            // 5.2.4 Add new audio track to the audioTracks attribute on this SourceBuffer object.
            // 5.2.5 Queue a task to fire a trusted event named addtrack, that does not bubble and is
            // not cancelable, and that uses the TrackEvent interface, at the AudioTrackList object
            // referenced by the audioTracks attribute on this SourceBuffer object.
            audioTracks()->append(newAudioTrack);

            // 5.2.6 Add new audio track to the audioTracks attribute on the HTMLMediaElement.
            // 5.2.7 Queue a task to fire a trusted event named addtrack, that does not bubble and is
            // not cancelable, and that uses the TrackEvent interface, at the AudioTrackList object
            // referenced by the audioTracks attribute on the HTMLMediaElement.
            m_source->mediaElement()->audioTracks()->append(newAudioTrack);

            // 5.2.8 Create a new track buffer to store coded frames for this track.
            ASSERT(!m_trackBufferMap.contains(newAudioTrack->id()));
            TrackBuffer& trackBuffer = m_trackBufferMap.add(newAudioTrack->id(), TrackBuffer()).iterator->value;

            // 5.2.9 Add the track description for this track to the track buffer.
            trackBuffer.description = it->description;
        }

        // 5.3 For each video track in the initialization segment, run following steps:
        for (auto it = segment.videoTracks.begin(); it != segment.videoTracks.end(); ++it) {
            VideoTrackPrivate* videoTrackPrivate = it->track.get();

            // 5.3.1 Let new video track be a new VideoTrack object.
            // 5.3.2 Generate a unique ID and assign it to the id property on new video track.
            RefPtr<VideoTrack> newVideoTrack = VideoTrack::create(this, videoTrackPrivate);
            newVideoTrack->setSourceBuffer(this);

            // 5.3.3 If videoTracks.length equals 0, then run the following steps:
            if (!videoTracks()->length()) {
                // 5.3.3.1 Set the selected property on new video track to true.
                newVideoTrack->setSelected(true);

                // 5.3.3.2 Set active track flag to true.
                activeTrackFlag = true;
            }

            // 5.3.4 Add new video track to the videoTracks attribute on this SourceBuffer object.
            // 5.3.5 Queue a task to fire a trusted event named addtrack, that does not bubble and is
            // not cancelable, and that uses the TrackEvent interface, at the VideoTrackList object
            // referenced by the videoTracks attribute on this SourceBuffer object.
            videoTracks()->append(newVideoTrack);

            // 5.3.6 Add new video track to the videoTracks attribute on the HTMLMediaElement.
            // 5.3.7 Queue a task to fire a trusted event named addtrack, that does not bubble and is
            // not cancelable, and that uses the TrackEvent interface, at the VideoTrackList object
            // referenced by the videoTracks attribute on the HTMLMediaElement.
            m_source->mediaElement()->videoTracks()->append(newVideoTrack);

            // 5.3.8 Create a new track buffer to store coded frames for this track.
            ASSERT(!m_trackBufferMap.contains(newVideoTrack->id()));
            TrackBuffer& trackBuffer = m_trackBufferMap.add(newVideoTrack->id(), TrackBuffer()).iterator->value;

            // 5.3.9 Add the track description for this track to the track buffer.
            trackBuffer.description = it->description;
        }

        // 5.4 For each text track in the initialization segment, run following steps:
        for (auto it = segment.textTracks.begin(); it != segment.textTracks.end(); ++it) {
            InbandTextTrackPrivate* textTrackPrivate = it->track.get();

            // 5.4.1 Let new text track be a new TextTrack object with its properties populated with the
            // appropriate information from the initialization segment.
            RefPtr<InbandTextTrack> newTextTrack = InbandTextTrack::create(scriptExecutionContext(), this, textTrackPrivate);

            // 5.4.2 If the mode property on new text track equals "showing" or "hidden", then set active
            // track flag to true.
            if (textTrackPrivate->mode() != InbandTextTrackPrivate::Disabled)
                activeTrackFlag = true;

            // 5.4.3 Add new text track to the textTracks attribute on this SourceBuffer object.
            // 5.4.4 Queue a task to fire a trusted event named addtrack, that does not bubble and is
            // not cancelable, and that uses the TrackEvent interface, at textTracks attribute on this
            // SourceBuffer object.
            textTracks()->append(newTextTrack);

            // 5.4.5 Add new text track to the textTracks attribute on the HTMLMediaElement.
            // 5.4.6 Queue a task to fire a trusted event named addtrack, that does not bubble and is
            // not cancelable, and that uses the TrackEvent interface, at the TextTrackList object
            // referenced by the textTracks attribute on the HTMLMediaElement.
            m_source->mediaElement()->textTracks()->append(newTextTrack);

            // 5.4.7 Create a new track buffer to store coded frames for this track.
            ASSERT(!m_trackBufferMap.contains(textTrackPrivate->id()));
            TrackBuffer& trackBuffer = m_trackBufferMap.add(textTrackPrivate->id(), TrackBuffer()).iterator->value;

            // 5.4.8 Add the track description for this track to the track buffer.
            trackBuffer.description = it->description;
        }

        // 5.5 If active track flag equals true, then run the following steps:
        if (activeTrackFlag) {
            // 5.5.1 Add this SourceBuffer to activeSourceBuffers.
            setActive(true);
        }

        // 5.6 Set first initialization segment flag to true.
        m_receivedFirstInitializationSegment = true;
    }

    // 6. If the HTMLMediaElement.readyState attribute is HAVE_NOTHING, then run the following steps:
    if (m_private->readyState() == MediaPlayer::HaveNothing) {
        // 6.1 If one or more objects in sourceBuffers have first initialization segment flag set to false, then abort these steps.
        for (unsigned long i = 0; i < m_source->sourceBuffers()->length(); ++i) {
            if (!m_source->sourceBuffers()->item(i)->m_receivedFirstInitializationSegment)
                return;
        }

        // 6.2 Set the HTMLMediaElement.readyState attribute to HAVE_METADATA.
        // 6.3 Queue a task to fire a simple event named loadedmetadata at the media element.
#if ENABLE(TIZEN_MSE)
            setReadyState(MediaPlayer::HaveMetadata);
#else
            m_private->setReadyState(MediaPlayer::HaveMetadata);
#endif

    }

    // 7. If the active track flag equals true and the HTMLMediaElement.readyState
    // attribute is greater than HAVE_CURRENT_DATA, then set the HTMLMediaElement.readyState
    // attribute to HAVE_METADATA.
    if (activeTrackFlag && m_private->readyState() > MediaPlayer::HaveCurrentData)
#if ENABLE(TIZEN_MSE)
            setReadyState(MediaPlayer::HaveMetadata);
#else
            m_private->setReadyState(MediaPlayer::HaveMetadata);
#endif

}

bool SourceBuffer::validateInitializationSegment(const InitializationSegment& segment)
{
    // 3.5.7 Initialization Segment Received (ctd)
    // https://dvcs.w3.org/hg/html-media/raw-file/default/media-source/media-source.html#sourcebuffer-init-segment-received

    // 3.1. Verify the following properties. If any of the checks fail then run the end of stream
    // algorithm with the error parameter set to "decode" and abort these steps.
    //   * The number of audio, video, and text tracks match what was in the first initialization segment.
    if (segment.audioTracks.size() != audioTracks()->length()
        || segment.videoTracks.size() != videoTracks()->length()
        || segment.textTracks.size() != textTracks()->length())
        return false;

#if !ENABLE(TIZEN_MSE)
    //   * The codecs for each track, match what was specified in the first initialization segment.
    for (auto it = segment.audioTracks.begin(); it != segment.audioTracks.end(); ++it) {
        if (!m_videoCodecs.contains(it->description->codec()))
            return false;
    }

    for (auto it = segment.videoTracks.begin(); it != segment.videoTracks.end(); ++it) {
        if (!m_audioCodecs.contains(it->description->codec()))
            return false;
    }

    for (auto it = segment.textTracks.begin(); it != segment.textTracks.end(); ++it) {
        if (!m_textCodecs.contains(it->description->codec()))
            return false;
    }
#endif

    //   * If more than one track for a single type are present (ie 2 audio tracks), then the Track
    //   IDs match the ones in the first initialization segment.
    if (segment.audioTracks.size() >= 2) {
        for (auto it = segment.audioTracks.begin(); it != segment.audioTracks.end(); ++it) {
            if (!m_trackBufferMap.contains(it->track->id()))
                return false;
        }
    }

    if (segment.videoTracks.size() >= 2) {
        for (auto it = segment.videoTracks.begin(); it != segment.videoTracks.end(); ++it) {
            if (!m_trackBufferMap.contains(it->track->id()))
                return false;
        }
    }

    if (segment.textTracks.size() >= 2) {
        for (auto it = segment.videoTracks.begin(); it != segment.videoTracks.end(); ++it) {
            if (!m_trackBufferMap.contains(it->track->id()))
                return false;
        }
    }

    return true;
}

class SampleLessThanComparator {
public:
    bool operator()(std::pair<MediaTime, RefPtr<MediaSample>> value1, std::pair<MediaTime, RefPtr<MediaSample>> value2)
    {
        return value1.first < value2.first;
    }

    bool operator()(MediaTime value1, std::pair<MediaTime, RefPtr<MediaSample>> value2)
    {
        return value1 < value2.first;
    }

    bool operator()(std::pair<MediaTime, RefPtr<MediaSample>> value1, MediaTime value2)
    {
        return value1.first < value2;
    }
};

void SourceBuffer::sourceBufferPrivateDidReceiveSample(SourceBufferPrivate*, PassRefPtr<MediaSample> prpSample)
{
    if (isRemoved())
        return;

    RefPtr<MediaSample> sample = prpSample;

    // 3.5.8 Coded Frame Processing
    // When complete coded frames have been parsed by the segment parser loop then the following steps
    // are run:
    // 1. For each coded frame in the media segment run the following steps:
    // 1.1. Loop Top
    do {
        // 1.1 (ctd) Let presentation timestamp be a double precision floating point representation of
        // the coded frame's presentation timestamp in seconds.
        MediaTime presentationTimestamp = sample->presentationTime();

        // 1.2 Let decode timestamp be a double precision floating point representation of the coded frame's
        // decode timestamp in seconds.
        MediaTime decodeTimestamp = sample->decodeTime();

        // 1.3 Let frame duration be a double precision floating point representation of the coded frame's
        // duration in seconds.
        MediaTime frameDuration = sample->duration();

        // 1.4 If mode equals "sequence" and group start timestamp is set, then run the following steps:
        // FIXME: add support for "sequence" mode

        // 1.5 If timestampOffset is not 0, then run the following steps:
        if (m_timestampOffset != MediaTime::zeroTime()) {
            // 1.5.1 Add timestampOffset to the presentation timestamp.
            presentationTimestamp += m_timestampOffset;

            // 1.5.2 Add timestampOffset to the decode timestamp.
            decodeTimestamp += m_timestampOffset;

            // 1.5.3 If the presentation timestamp or decode timestamp is less than the presentation start
            // time, then run the end of stream algorithm with the error parameter set to "decode", and
            // abort these steps.
            MediaTime presentationStartTime = MediaTime::zeroTime();
            if (presentationTimestamp < presentationStartTime || decodeTimestamp < presentationStartTime) {
                m_source->streamEndedWithError(decodeError(), IgnorableExceptionCode());
                return;
            }
        }

        // 1.6 Let track buffer equal the track buffer that the coded frame will be added to.
        AtomicString trackID = sample->trackID();
        auto it = m_trackBufferMap.find(trackID);
        if (it == m_trackBufferMap.end())
            it = m_trackBufferMap.add(trackID, TrackBuffer()).iterator;
        TrackBuffer& trackBuffer = it->value;

#if TIZEN_MSE_FRAME_DURATION_WORKAROUND
        // FIX ME : This should be fixed and GST-Parser should provide any support for duration.
        if (frameDuration.toDouble()>1.0)
            frameDuration = trackBuffer.lastFrameDuration;
#endif

        // 1.7 If last decode timestamp for track buffer is set and decode timestamp is less than last
        // decode timestamp:
        // OR
        // If last decode timestamp for track buffer is set and the difference between decode timestamp and
        // last decode timestamp is greater than 2 times last frame duration:
        if (trackBuffer.lastDecodeTimestamp.isValid() && (decodeTimestamp < trackBuffer.lastDecodeTimestamp
            || abs(decodeTimestamp - trackBuffer.lastDecodeTimestamp) > (trackBuffer.lastFrameDuration * 2))) {
            // 1.7.1 If mode equals "segments":
            // Set highest presentation end timestamp to presentation timestamp.
            m_highestPresentationEndTimestamp = presentationTimestamp;

            // If mode equals "sequence":
            // Set group start timestamp equal to the highest presentation end timestamp.
            // FIXME: Add support for "sequence" mode.

            for (auto i = m_trackBufferMap.values().begin(); i != m_trackBufferMap.values().end(); ++i) {
                // 1.7.2 Unset the last decode timestamp on all track buffers.
                i->lastDecodeTimestamp = MediaTime::invalidTime();
                // 1.7.3 Unset the last frame duration on all track buffers.
                i->lastFrameDuration = MediaTime::invalidTime();
                // 1.7.4 Unset the highest presentation timestamp on all track buffers.
                i->highestPresentationTimestamp = MediaTime::invalidTime();
                // 1.7.5 Set the need random access point flag on all track buffers to true.
                i->needRandomAccessFlag = true;
            }

            // 1.7.6 Jump to the Loop Top step above to restart processing of the current coded frame.
            continue;
        }

        // 1.8 Let frame end timestamp equal the sum of presentation timestamp and frame duration.
        MediaTime frameEndTimestamp = presentationTimestamp + frameDuration;

        // 1.9 If presentation timestamp is less than appendWindowStart, then set the need random access
        // point flag to true, drop the coded frame, and jump to the top of the loop to start processing
        // the next coded frame.
        // 1.10 If frame end timestamp is greater than appendWindowEnd, then set the need random access
        // point flag to true, drop the coded frame, and jump to the top of the loop to start processing
        // the next coded frame.
        // FIXME: implement append windows

        // 1.11 If the need random access point flag on track buffer equals true, then run the following steps:
        if (trackBuffer.needRandomAccessFlag) {
            // 1.11.1 If the coded frame is not a random access point, then drop the coded frame and jump
            // to the top of the loop to start processing the next coded frame.
            if (!sample->isSync()) {
                didDropSample();
                return;
            }

            // 1.11.2 Set the need random access point flag on track buffer to false.
            trackBuffer.needRandomAccessFlag = false;
        }

        // 1.12 Let spliced audio frame be an unset variable for holding audio splice information
        // 1.13 Let spliced timed text frame be an unset variable for holding timed text splice information
        // FIXME: Add support for sample splicing.

        SampleMap::MapType erasedSamples;
        MediaTime microsecond(1, 1000000);

        // 1.14 If last decode timestamp for track buffer is unset and there is a coded frame in
        // track buffer with a presentation timestamp less than or equal to presentation timestamp
        // and presentation timestamp is less than this coded frame's presentation timestamp plus
        // its frame duration, then run the following steps:
        if (trackBuffer.lastDecodeTimestamp.isInvalid()) {
            auto iter = trackBuffer.samples.findSampleContainingPresentationTime(presentationTimestamp);
            if (iter != trackBuffer.samples.presentationEnd()) {
                // 1.14.1 Let overlapped frame be the coded frame in track buffer that matches the condition above.
                RefPtr<MediaSample> overlappedFrame = iter->second;

                // 1.14.2 If track buffer contains audio coded frames:
                // Run the audio splice frame algorithm and if a splice frame is returned, assign it to
                // spliced audio frame.
                // FIXME: Add support for sample splicing.

                // If track buffer contains video coded frames:
                if (trackBuffer.description->isVideo()) {
                    // 1.14.2.1 Let overlapped frame presentation timestamp equal the presentation timestamp
                    // of overlapped frame.
                    MediaTime overlappedFramePresentationTimestamp = overlappedFrame->presentationTime();

                    // 1.14.2.2 Let remove window timestamp equal overlapped frame presentation timestamp
                    // plus 1 microsecond.
                    MediaTime removeWindowTimestamp = overlappedFramePresentationTimestamp + microsecond;

                    // 1.14.2.3 If the presentation timestamp is less than the remove window timestamp,
                    // then remove overlapped frame and any coded frames that depend on it from track buffer.
                    if (presentationTimestamp < removeWindowTimestamp)
                        erasedSamples.insert(*iter);
                }

                // If track buffer contains timed text coded frames:
                // Run the text splice frame algorithm and if a splice frame is returned, assign it to spliced timed text frame.
                // FIXME: Add support for sample splicing.
            }
        }

        // 1.15 Remove existing coded frames in track buffer:
        // If highest presentation timestamp for track buffer is not set:
        if (trackBuffer.highestPresentationTimestamp.isInvalid()) {
            // Remove all coded frames from track buffer that have a presentation timestamp greater than or
            // equal to presentation timestamp and less than frame end timestamp.
            auto iter_pair = trackBuffer.samples.findSamplesBetweenPresentationTimes(presentationTimestamp, frameEndTimestamp);
            if (iter_pair.first != trackBuffer.samples.presentationEnd())
                erasedSamples.insert(iter_pair.first, iter_pair.second);
        }

        // If highest presentation timestamp for track buffer is set and less than presentation timestamp
        if (trackBuffer.highestPresentationTimestamp.isValid() && trackBuffer.highestPresentationTimestamp < presentationTimestamp) {
            // Remove all coded frames from track buffer that have a presentation timestamp greater than highest
            // presentation timestamp and less than or equal to frame end timestamp.
            auto iter_pair = trackBuffer.samples.findSamplesBetweenPresentationTimes(trackBuffer.highestPresentationTimestamp, frameEndTimestamp);
            if (iter_pair.first != trackBuffer.samples.presentationEnd())
                erasedSamples.insert(iter_pair.first, iter_pair.second);
        }

        // 1.16 Remove decoding dependencies of the coded frames removed in the previous step:
        SampleMap::MapType dependentSamples;
        if (!erasedSamples.empty()) {
            // If detailed information about decoding dependencies is available:
            // FIXME: Add support for detailed dependency information

            // Otherwise: Remove all coded frames between the coded frames removed in the previous step
            // and the next random access point after those removed frames.
            for (auto erasedIt = erasedSamples.begin(), end = erasedSamples.end(); erasedIt != end; ++erasedIt) {
                auto currentDecodeIter = trackBuffer.samples.findSampleWithDecodeTime(erasedIt->second->decodeTime());
                auto nextSyncIter = trackBuffer.samples.findSyncSampleAfterDecodeIterator(currentDecodeIter);
                dependentSamples.insert(currentDecodeIter, nextSyncIter);
            }


            RefPtr<TimeRanges> erasedRanges = TimeRanges::create();
            for (auto erasedIt = erasedSamples.begin(), end = erasedSamples.end(); erasedIt != end; ++erasedIt) {
                double startTime = erasedIt->first.toDouble();
                double endTime = ((erasedIt->first + erasedIt->second->duration()) + microsecond).toDouble();
                erasedRanges->add(startTime, endTime);
                trackBuffer.samples.removeSample(erasedIt->second.get());
            }

            for (auto dependentIt = dependentSamples.begin(), end = dependentSamples.end(); dependentIt != end; ++dependentIt) {
                double startTime = dependentIt->first.toDouble();
                double endTime = ((dependentIt->first + dependentIt->second->duration()) + microsecond).toDouble();
                erasedRanges->add(startTime, endTime);
                trackBuffer.samples.removeSample(dependentIt->second.get());
            }

            erasedRanges->invert();
            m_buffered->intersectWith(erasedRanges.get());
        }

        // 1.17 If spliced audio frame is set:
        // Add spliced audio frame to the track buffer.
        // If spliced timed text frame is set:
        // Add spliced timed text frame to the track buffer.
        // FIXME: Add support for sample splicing.

#if ENABLE(TIZEN_MSE)
        if (m_timestampOffset != MediaTime::zeroTime())
            sample->addTimestamp(m_timestampOffset);
#endif
        // Otherwise:
        // Add the coded frame with the presentation timestamp, decode timestamp, and frame duration to the track buffer.
        trackBuffer.samples.addSample(sample);
        trackBuffer.decodeQueue.insert(SampleMap::MapType::value_type(decodeTimestamp, sample));
        // 1.18 Set last decode timestamp for track buffer to decode timestamp.
        trackBuffer.lastDecodeTimestamp = decodeTimestamp;

        // 1.19 Set last frame duration for track buffer to frame duration.
        trackBuffer.lastFrameDuration = frameDuration;

        // 1.20 If highest presentation timestamp for track buffer is unset or frame end timestamp is greater
        // than highest presentation timestamp, then set highest presentation timestamp for track buffer
        // to frame end timestamp.
        if (trackBuffer.highestPresentationTimestamp.isInvalid() || frameEndTimestamp > trackBuffer.highestPresentationTimestamp)
            trackBuffer.highestPresentationTimestamp = frameEndTimestamp;

        // 1.21 If highest presentation end timestamp is unset or frame end timestamp is greater than highest
        // presentation end timestamp, then set highest presentation end timestamp equal to frame end timestamp.
        if (m_highestPresentationEndTimestamp.isInvalid() || frameEndTimestamp > m_highestPresentationEndTimestamp)
            m_highestPresentationEndTimestamp = frameEndTimestamp;

        m_buffered->add(presentationTimestamp.toDouble(), (presentationTimestamp + frameDuration + microsecond).toDouble());
        m_bufferedSinceLastMonitor += frameDuration.toDouble();

#if ENABLE(TIZEN_MSE)
        // MSE SPEC 12. Byte Stream Format
        // Support seamless playback of media segments having a timestamp gap smaller than the audio frame size.
        // User agent *must not* reflect these gaps in the buffered attribute.
        if (trackBuffer.description->isAudio())
        {
            SampleMap::iterator samplePositon = trackBuffer.samples.findSampleWithDecodeTime(decodeTimestamp);
            // check gap before current segment if exist
            if (samplePositon != trackBuffer.samples.decodeBegin()) {
                samplePositon--;
                double gapFromPrevSample = decodeTimestamp.toDouble() - samplePositon->first.toDouble() - frameDuration.toDouble();
                if (gapFromPrevSample > microsecond.toDouble() && gapFromPrevSample < frameDuration.toDouble() )
                    m_buffered->add(samplePositon->first.toDouble(), presentationTimestamp.toDouble() + microsecond.toDouble());
                samplePositon++;
            }
            // check gap after current segment if exist
            if (samplePositon != trackBuffer.samples.decodeEnd()) {
                samplePositon++;
                double gaptoNextSample = samplePositon->first.toDouble() - decodeTimestamp.toDouble() - frameDuration.toDouble();
                if (gaptoNextSample > microsecond.toDouble() && gaptoNextSample < frameDuration.toDouble() )
                    m_buffered->add(presentationTimestamp.toDouble(), samplePositon->first.toDouble()  + microsecond.toDouble());
            }
        }
#endif
        break;
    } while (1);

    // Steps 2-4 will be handled by MediaSource::monitorSourceBuffers()

    // 5. If the media segment contains data beyond the current duration, then run the duration change algorithm with new
    // duration set to the maximum of the current duration and the highest end timestamp reported by HTMLMediaElement.buffered.
    if (highestPresentationEndTimestamp().toDouble() > m_source->duration())
        m_source->setDuration(highestPresentationEndTimestamp().toDouble(), IgnorableExceptionCode());
}

bool SourceBuffer::sourceBufferPrivateHasAudio(const SourceBufferPrivate*) const
{
    return m_audioTracks && m_audioTracks->length();
}

bool SourceBuffer::sourceBufferPrivateHasVideo(const SourceBufferPrivate*) const
{
    return m_videoTracks && m_videoTracks->length();
}

void SourceBuffer::videoTrackSelectedChanged(VideoTrack* track)
{
    // 2.4.5 Changes to selected/enabled track state
    // If the selected video track changes, then run the following steps:
    // 1. If the SourceBuffer associated with the previously selected video track is not associated with
    // any other enabled tracks, run the following steps:
    if (track->selected()
        && (!m_videoTracks || !m_videoTracks->isAnyTrackEnabled())
        && (!m_audioTracks || !m_audioTracks->isAnyTrackEnabled())
        && (!m_textTracks || !m_textTracks->isAnyTrackEnabled())) {
        // 1.1 Remove the SourceBuffer from activeSourceBuffers.
        // 1.2 Queue a task to fire a simple event named removesourcebuffer at activeSourceBuffers
        setActive(false);
    } else if (!track->selected()) {
        // 2. If the SourceBuffer associated with the newly selected video track is not already in activeSourceBuffers,
        // run the following steps:
        // 2.1 Add the SourceBuffer to activeSourceBuffers.
        // 2.2 Queue a task to fire a simple event named addsourcebuffer at activeSourceBuffers
        setActive(true);
    }

    if (!isRemoved())
        m_source->mediaElement()->videoTrackSelectedChanged(track);
}

void SourceBuffer::audioTrackEnabledChanged(AudioTrack* track)
{
    // 2.4.5 Changes to selected/enabled track state
    // If an audio track becomes disabled and the SourceBuffer associated with this track is not
    // associated with any other enabled or selected track, then run the following steps:
    if (track->enabled()
        && (!m_videoTracks || !m_videoTracks->isAnyTrackEnabled())
        && (!m_audioTracks || !m_audioTracks->isAnyTrackEnabled())
        && (!m_textTracks || !m_textTracks->isAnyTrackEnabled())) {
        // 1. Remove the SourceBuffer associated with the audio track from activeSourceBuffers
        // 2. Queue a task to fire a simple event named removesourcebuffer at activeSourceBuffers
        setActive(false);
    } else if (!track->enabled()) {
        // If an audio track becomes enabled and the SourceBuffer associated with this track is
        // not already in activeSourceBuffers, then run the following steps:
        // 1. Add the SourceBuffer associated with the audio track to activeSourceBuffers
        // 2. Queue a task to fire a simple event named addsourcebuffer at activeSourceBuffers
        setActive(true);
    }

    if (!isRemoved())
        m_source->mediaElement()->audioTrackEnabledChanged(track);
}

void SourceBuffer::textTrackModeChanged(TextTrack* track)
{
    // 2.4.5 Changes to selected/enabled track state
    // If a text track mode becomes "disabled" and the SourceBuffer associated with this track is not
    // associated with any other enabled or selected track, then run the following steps:
    if (track->mode() == TextTrack::disabledKeyword()
        && (!m_videoTracks || !m_videoTracks->isAnyTrackEnabled())
        && (!m_audioTracks || !m_audioTracks->isAnyTrackEnabled())
        && (!m_textTracks || !m_textTracks->isAnyTrackEnabled())) {
        // 1. Remove the SourceBuffer associated with the audio track from activeSourceBuffers
        // 2. Queue a task to fire a simple event named removesourcebuffer at activeSourceBuffers
        setActive(false);
    } else {
        // If a text track mode becomes "showing" or "hidden" and the SourceBuffer associated with this
        // track is not already in activeSourceBuffers, then run the following steps:
        // 1. Add the SourceBuffer associated with the text track to activeSourceBuffers
        // 2. Queue a task to fire a simple event named addsourcebuffer at activeSourceBuffers
        setActive(true);
    }

    if (!isRemoved())
        m_source->mediaElement()->textTrackModeChanged(track);
}

void SourceBuffer::textTrackAddCue(TextTrack* track, WTF::PassRefPtr<TextTrackCue> cue)
{
    if (!isRemoved())
        m_source->mediaElement()->textTrackAddCue(track, cue);
}

void SourceBuffer::textTrackAddCues(TextTrack* track, TextTrackCueList const* cueList)
{
    if (!isRemoved())
        m_source->mediaElement()->textTrackAddCues(track, cueList);
}

void SourceBuffer::textTrackRemoveCue(TextTrack* track, WTF::PassRefPtr<TextTrackCue> cue)
{
    if (!isRemoved())
        m_source->mediaElement()->textTrackRemoveCue(track, cue);
}

void SourceBuffer::textTrackRemoveCues(TextTrack* track, TextTrackCueList const* cueList)
{
    if (!isRemoved())
        m_source->mediaElement()->textTrackRemoveCues(track, cueList);
}

void SourceBuffer::textTrackKindChanged(TextTrack* track)
{
    if (!isRemoved())
        m_source->mediaElement()->textTrackKindChanged(track);
}

void SourceBuffer::sourceBufferPrivateDidBecomeReadyForMoreSamples(SourceBufferPrivate*, AtomicString trackID)
{
    LOG(Media, "SourceBuffer::sourceBufferPrivateDidBecomeReadyForMoreSamples(%p)", this);
    auto it = m_trackBufferMap.find(trackID);
    if (it == m_trackBufferMap.end())
        return;

    provideMediaData(it->value, trackID);
}

void SourceBuffer::provideMediaData(TrackBuffer& trackBuffer, AtomicString trackID)
{
#if !LOG_DISABLED
    unsigned enqueuedSamples = 0;
#endif

    auto sampleIt = trackBuffer.decodeQueue.begin();
    for (auto sampleEnd = trackBuffer.decodeQueue.end(); sampleIt != sampleEnd; ++sampleIt) {
        if (!m_private->isReadyForMoreSamples(trackID)) {
            m_private->notifyClientWhenReadyForMoreSamples(trackID);
            break;
        }

        RefPtr<MediaSample> sample = sampleIt->second;
        trackBuffer.lastEnqueuedPresentationTime = sample->presentationTime();
        m_private->enqueueSample(sample.release(), trackID);
#if !LOG_DISABLED
        ++enqueuedSamples;
#endif

    }
    trackBuffer.decodeQueue.erase(trackBuffer.decodeQueue.begin(), sampleIt);

    LOG(Media, "SourceBuffer::provideMediaData(%p) - Enqueued %u samples", this, enqueuedSamples);
}

void SourceBuffer::didDropSample()
{
    if (!isRemoved())
        m_source->mediaElement()->incrementDroppedFrameCount();
}

void SourceBuffer::monitorBufferingRate()
{
    if (!m_bufferedSinceLastMonitor)
        return;

    double now = monotonicallyIncreasingTime();
    double interval = now - m_timeOfBufferingMonitor;
    double rateSinceLastMonitor = m_bufferedSinceLastMonitor / interval;

    m_timeOfBufferingMonitor = now;
    m_bufferedSinceLastMonitor = 0;

    m_averageBufferRate = m_averageBufferRate * (1 - ExponentialMovingAverageCoefficient) + rateSinceLastMonitor * ExponentialMovingAverageCoefficient;

    LOG(Media, "SourceBuffer::monitorBufferingRate(%p) - m_avegareBufferRate: %lf", this, m_averageBufferRate);
}

bool SourceBuffer::hasCurrentTime() const
{
    if (!m_buffered->length())
        return false;

    double currentTime = m_source->currentTime();
    return fabs(m_buffered->nearest(m_source->currentTime()) - currentTime) <= CurrentTimeFudgeFactor;
}

bool SourceBuffer::hasFutureTime() const
{
    double currentTime = m_source->currentTime();
    double nearest = m_buffered->nearest(m_source->currentTime());
    if (fabs(m_buffered->nearest(m_source->currentTime()) - currentTime) > CurrentTimeFudgeFactor)
        return false;

    size_t found = m_buffered->find(nearest);
    ASSERT(found != notFound);

    ExceptionCode ignoredValid = 0;
    return m_buffered->end(found, ignoredValid) - currentTime > CurrentTimeFudgeFactor;
}

bool SourceBuffer::canPlayThrough()
{
    monitorBufferingRate();

    // Assuming no fluctuations in the buffering rate, loading 1 second per second or greater
    // means indefinite playback. This could be improved by taking jitter into account.
    if (m_averageBufferRate > 1)
        return true;

    // Add up all the time yet to be buffered.
    double unbufferedTime = 0;
    double currentTime = m_source->currentTime();
    double duration = m_source->duration();

    PassRefPtr<TimeRanges> unbufferedRanges = m_buffered->copy();
    unbufferedRanges->invert();
    unbufferedRanges->intersectWith(TimeRanges::create(currentTime, std::max(currentTime, duration)).get());
    ExceptionCode valid = 0;

    for (size_t i = 0, end = unbufferedRanges->length(); i < end; ++i)
        unbufferedTime += unbufferedRanges->end(i, valid) - unbufferedRanges->start(i, valid);

    double timeRemaining = duration - currentTime;
    return unbufferedTime / m_averageBufferRate < timeRemaining;
}

#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
const String SourceBuffer::getMimeType()
{
    return m_private->getMimeType();
}

void SourceBuffer::appendNeedKeySourceBuffer()
{
    m_source->appendNeedKeySourceBuffer(this);
}

PassRefPtr<Uint8Array> SourceBuffer::generateKeyRequest(const String& keySystem, Uint8Array* initData)
{
    return m_private->generateKeyRequest(keySystem, initData);
}

bool SourceBuffer::updateKey(const String& keySystem, Uint8Array* key, Uint8Array* initData)
{
    return m_private->updateKey(keySystem, key, initData);
}
#endif

} // namespace WebCore
#endif
