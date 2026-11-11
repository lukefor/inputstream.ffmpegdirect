/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "FFmpegStream.h"

#include <map>
#include "../utils/HttpProxy.h"
#include "../utils/TimeUtils.h"

namespace ffmpegdirect
{

static const int VIDEO_PLAYER_BUFFER_SECONDS = 10;
static const int TERMINATING_SECOND_STREAM_MIN_SEEK_FROM_LIVE_TIME = 60;
static const int TERMINATING_MINUTE_STREAM_MIN_SEEK_FROM_LIVE_TIME = 120;
// How far from the requested position a reopened stream may start and still be accepted as the
// same source timeline. Servers can start at whole minutes or larger archive chunks, earlier or,
// near live, later than asked. Unrelated timelines (e.g. a live stream) are hours apart.
static constexpr double MAX_REOPEN_START_OFFSET_SECS = 300.0;
// Additional allowance on top of how far back a continued stream is expected to start
static constexpr double REOPEN_START_TOLERANCE_SECS = 30.0;
// How far ahead of the data already read a seek is served by reading on instead of reopening
static constexpr double LOCAL_SEEK_MAX_FORWARD_SECS = 60.0;
// How much earlier the second attempt to refetch missing data asks for
static constexpr double GAP_REFETCH_REWIND_SECS = 60.0;

class FFmpegCatchupStream : public FFmpegStream
{
public:
  FFmpegCatchupStream(IManageDemuxPacket* demuxPacketManager,
                      const Properties& props,
                      const HttpProxy& httpProxy);
  ~FFmpegCatchupStream();

  virtual bool Open(const std::string& streamUrl, const std::string& mimeType, bool isRealTimeStream, const std::string& programProperty) override;
  virtual bool DemuxSeekTime(double timeMs, bool backwards, double& startpts) override;
  virtual DEMUX_PACKET* DemuxRead() override;
  virtual void DemuxSetSpeed(int speed) override;
  virtual void GetCapabilities(kodi::addon::InputstreamCapabilities& caps) override;
  bool DemuxSeekTime(double timeMs)
  {
    double temp = 0;
    return DemuxSeekTime(timeMs, false, temp);
  }

  int64_t SeekCatchupStream(double timeMs, bool backwards);
  virtual int64_t LengthStream() override;
  virtual bool GetTimes(kodi::addon::InputstreamTimes& times) override;
  virtual bool IsRealTimeStream() override;

protected:
  void CurrentPTSUpdated() override;
  bool CheckReturnEmptyOnPacketResult(int result) override;

  long long GetCurrentLiveOffset() { return std::time(nullptr) - m_catchupBufferStartTime; }
  bool SeekDistanceSupported(int64_t seekBufferOffset);
  bool TargetDistanceFromLiveSupported(long long secondsFromLive);
  const std::string GetDateTime(time_t time)
  {
    std::tm timeStruct = SafeLocaltime(time);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d.%X", &timeStruct);

    return buffer;
  }
  std::string GetUpdatedCatchupUrl() const;

  bool m_playbackAsLive = false;
  std::string m_defaultUrl;
  time_t m_programmeStartTime = 0;
  time_t m_programmeEndTime = 0;
  std::string m_catchupUrlFormatString;
  std::string m_catchupUrlNearLiveFormatString;
  time_t m_catchupBufferStartTime = 0;
  time_t m_catchupBufferEndTime = 0;
  long long m_catchupBufferOffset = 0;
  bool m_catchupTerminates = false;
  int m_catchupGranularity = 1;
  int m_catchupGranularityLowWaterMark = 1;
  int m_timezoneShift = 0;
  int m_defaultProgrammeDuration = 0;
  std::string m_programmeCatchupId;

  bool m_isOpeningStream;
  double m_seekOffset;
  double m_pauseStartTime = 0;
  double m_currentDemuxTime = 0;

  long long m_previousLiveBufferOffset = 0;
  bool m_lastSeekWasLive = false;
  bool m_lastPacketWasAvoidedEOF = false;
  bool m_seekCorrectsEOF = false;

  // When the stream is reopened internally to continue from the current demux position
  // (EOF on a terminating catchup stream, or resuming after pause) the new stream can
  // start earlier than where we were, e.g. due to minute granularity or the server
  // starting at a segment boundary. Remember the last timestamp delivered per stream so
  // the overlap can be dropped instead of being played again.
  bool m_continuationSeek = false;
  std::map<int, double> m_lastDemuxTimestamps;
  std::map<int, double> m_continuationTimestamps;
  int m_continuationDroppedPackets = 0;
  // Offset of the new URL, used only if the continued stream does not carry the source timestamps
  double m_pendingContinuationOffset = 0;
  bool m_hasPendingContinuationOffset = false;
  // The position a seek asked for, before aligning it to the catchup granularity
  long long m_seekTargetBufferOffset = -1;
};

} //namespace ffmpegdirect
