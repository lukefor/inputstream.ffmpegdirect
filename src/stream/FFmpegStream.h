/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "../utils/HttpProxy.h"
#include "../utils/Properties.h"
#include "BaseStream.h"
#include "DemuxStream.h"
#include "CurlInput.h"

#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <sstream>
#include <unordered_set>

#include <kodi/addon-instance/Inputstream.h>
#include <kodi/tools/EndTime.h>

#ifndef __GNUC__
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mastering_display_metadata.h>
#include <libavutil/version.h>
}

#ifndef __GNUC__
#pragma warning(pop)
#endif

#define FFMPEG_DVDNAV_BUFFER_SIZE 2048  // for dvd's

struct StereoModeConversionMap;

namespace ffmpegdirect
{

enum class TRANSPORT_STREAM_STATE
{
  NONE,
  READY,
  NOTREADY,
};

class FFmpegStream
  : public BaseStream
{
public:
  FFmpegStream(IManageDemuxPacket* demuxPacketManager, const Properties& props, const HttpProxy& httpProxy);
  FFmpegStream(IManageDemuxPacket* demuxPacketManager, const Properties& props, std::shared_ptr<CurlInput> curlInput, const HttpProxy& httpProxy);
  ~FFmpegStream();

  virtual bool Open(const std::string& streamUrl, const std::string& mimeType, bool isRealTimeStream, const std::string& programProperty) override;
  virtual void Close() override;
  virtual void GetCapabilities(kodi::addon::InputstreamCapabilities& caps) override;
  virtual bool GetStreamIds(std::vector<unsigned int>& ids) override;
  virtual bool GetStream(int streamid, kodi::addon::InputstreamInfo& info) override;
  virtual void EnableStream(int streamid, bool enable) override;
  virtual bool OpenStream(int streamid) override;

  virtual void DemuxReset() override;
  virtual void DemuxAbort() override;
  virtual void DemuxFlush() override;
  virtual DEMUX_PACKET* DemuxRead() override;
  virtual bool DemuxSeekTime(double time, bool backwards, double& startpts) override;
  virtual void DemuxSetSpeed(int speed) override;
  virtual void SetVideoResolution(unsigned int width, unsigned int height) override;

  virtual int GetTotalTime() override;// { return 20; }
  virtual int GetTime() override;// { return m_displayTime; }
  virtual bool GetTimes(kodi::addon::InputstreamTimes& times) override;
  virtual bool PosTime(int ms) override;

  virtual int GetChapter() override;
  virtual int GetChapterCount() override;
  virtual const char* GetChapterName(int ch) override;
  virtual int64_t GetChapterPos(int ch) override;
  virtual bool SeekChapter(int ch) override;

  virtual int ReadStream(uint8_t* buffer, unsigned int bufferSize) override;
  virtual int64_t SeekStream(int64_t position, int whence = SEEK_SET) override;
  virtual int64_t PositionStream() override;
  virtual int64_t LengthStream() override;
  virtual bool IsRealTimeStream() override; // { return true; }

  void Dispose();
  void DisposeStreams();
  bool Aborted();

  AVFormatContext* m_pFormatContext;
  std::shared_ptr<CurlInput> m_curlInput;

  // How a stream reopened after PrepareReopen() maps onto the previous timeline
  enum class ContinuationState
  {
    NONE, // not a reopen
    PENDING, // reopened, waiting for the first audio/video packet to decide
    SOURCE_TIMELINE, // new stream carries the same source timestamps, previous mapping kept
    FALLBACK, // unrelated timestamps, the caller has to supply a new mapping
  };

protected:
  virtual std::string GetStreamCodecName(int iStreamId);

  /*!
   * Call before reopening the stream with a new URL for another position of the same source
   * (e.g. a catchup seek or continuing a terminating catchup stream). Archives of a live
   * channel keep the channel's original timestamps, so if the first packets of the new stream
   * land near the expected position using the current mapping, that mapping is kept: the real
   * position is then known exactly, wherever the server actually starts the new stream.
   * Otherwise GetContinuationState() reports FALLBACK and the caller must supply a new mapping.
   * All times are in the timestamps produced by this class (STREAM_TIME_BASE units).
   * @param expectedTs where the new stream should start
   * @param maxEarlierSecs how much earlier than expected the new stream may start
   * @param maxLaterSecs how much later than expected the new stream may start
   * @param keepReplayHistory keep dropping data that was already delivered (continuing), or
   *                          not (seeking, where earlier data may be wanted again)
   * @param skipTargetSource drop data before this position if the mapping is kept
   * @param skipTargetFallback drop data before this position with the new stream's own mapping
   */
  void PrepareReopen(double expectedTs, double maxEarlierSecs, double maxLaterSecs,
                     bool keepReplayHistory, double skipTargetSource, double skipTargetFallback);
  ContinuationState GetContinuationState() const { return m_continuationState; }

  /*!
   * When enabled, a hole in the source timestamps (data the server skipped, typically after an
   * HTTP reconnect that resumed at the wrong place) is not passed on. Instead
   * ConsumeGapRepairRequest() returns true once, so the caller can reopen the stream from the
   * last delivered position and get the missing data. A hole that is still there after that is
   * accepted as missing from the source.
   */
  void EnableGapRepair(bool enable) { m_gapRepairEnabled = enable; }
  bool ConsumeGapRepairRequest()
  {
    const bool requested = m_gapRepairRequested;
    m_gapRepairRequested = false;
    return requested;
  }
  virtual void CurrentPTSUpdated();
  bool IsPaused() { return m_speed == STREAM_PLAYSPEED_PAUSE; }
  virtual bool CheckReturnEmptyOnPacketResult(int result);

  FFmpegExtraData GetPacketExtradata(const AVPacket* pkt, const AVCodecParameters* codecPar);

  int64_t m_demuxerId;
  mutable std::recursive_mutex m_mutex;
  double m_currentPts; // used for stream length estimation
  bool m_demuxResetOpenSuccess = false;
  std::string m_streamUrl;
  int m_lastPacketResult;
  bool m_isRealTimeStream;

private:
  bool Open(bool fileinfo);
  bool OpenWithFFmpeg(const AVInputFormat* iformat, const AVIOInterruptCB& int_cb);
  bool OpenWithCURL(const AVInputFormat* iformat);
  AVDictionary* GetFFMpegOptionsFromInput();
  void ResetVideoStreams();
  double ConvertTimestamp(int64_t pts, int den, int num);
  unsigned int HLSSelectProgram();
  int GetNrOfStreams() const;
  int GetNrOfStreams(INPUTSTREAM_TYPE streamType);
  int GetNrOfSubtitleStreams();
  std::vector<DemuxStream*> GetDemuxStreams() const;
  DemuxStream* GetDemuxStream(int iStreamId) const;
  void CreateStreams(unsigned int program);
  void AddStream(int streamIdx, DemuxStream* stream);
  DemuxStream* AddStream(int streamIdx);
  void GetL16Parameters(int& channels, int& samplerate);
  double SelectAspect(AVStream* st, bool& forced);
  StreamHdrType DetermineHdrType(AVStream* pStream);
  std::string GetStereoModeFromMetadata(AVDictionary* pMetadata);
  std::string ConvertCodecToInternalStereoMode(const std::string &mode, const StereoModeConversionMap* conversionMap);
  bool SeekTime(double time, bool backwards = false, double* startpts = nullptr);
  void ParsePacket(AVPacket* pkt);
  TRANSPORT_STREAM_STATE TransportStreamAudioState();
  TRANSPORT_STREAM_STATE TransportStreamVideoState();
  bool IsTransportStreamReady();
  bool IsProgramChange();
  void StoreSideData(DEMUX_PACKET *pkt, AVPacket *src);
  void FlushDemuxer();
  bool IsReplayedLivePacket(const AVPacket& pkt, const AVStream* stream);
  void ResetReplayDetection();
  double GetTimestampStartTime() const;
  bool CheckContinuationTimeline(const AVPacket& pkt, const AVStream* stream);
  void SkipUntil(double targetTs);
  bool IsBeforeSkipTarget(const DEMUX_PACKET* packet, const AVStream* stream, bool keyFrame);

  bool StreamsOpened() { return m_streams.size() > 0; }

  int64_t NewGuid()
  {
    static int64_t guid = 0;
    return guid++;
  }

  bool m_paused;

  std::map<int, DemuxStream*> m_streams;
  std::map<int, std::unique_ptr<DemuxParserFFmpeg>> m_parsers;

  AVIOContext* m_ioContext;

  bool     m_bMatroska;
  bool     m_bAVI;
  bool     m_bSup;
  int      m_speed;
  unsigned int m_program;
  unsigned int m_streamsInProgram;
  unsigned int m_newProgram;
  unsigned int m_initialProgramNumber;
  int m_seekStream;

  kodi::tools::CEndTime  m_timeout;

  // Due to limitations of ffmpeg, we only can detect a program change
  // with a packet. This struct saves the packet for the next read and
  // signals STREAMCHANGE to player
  struct
  {
    AVPacket pkt;       // packet ffmpeg returned
    int      result;    // result from av_read_packet
  }m_pkt;

  bool m_streaminfo;
  bool m_reopen = false;
  bool m_checkTransportStream;
  int m_displayTime = 0;
  double m_dtsAtDisplayTime;
  bool m_seekToKeyFrame = false;
  double m_startTime = 0;

  std::string m_mimeType;
  std::string m_programProperty;
  std::string m_manifestType;
  bool m_opened;

  // Live sources can resend data we have already delivered, e.g. when the HTTP
  // connection is transparently re-established and the server restarts from the
  // beginning of its buffer. Track recently delivered timestamps per stream so
  // that such replays can be dropped instead of being played a second time.
  struct ReplayGuard
  {
    int64_t highWaterTs = AV_NOPTS_VALUE;
    AVRational timeBase = {0, 1};
    int64_t refetchedGapAt = AV_NOPTS_VALUE; // position of the last hole that was refetched
    bool skipping = false;
    int droppedPackets = 0;
    std::unordered_set<int64_t> deliveredTs;
    std::deque<int64_t> deliveredOrder;
  };
  static const size_t REPLAY_GUARD_HISTORY_SIZE = 16384; // > 5 minutes of packets for typical A/V streams
  std::map<int, ReplayGuard> m_replayGuards;

  ContinuationState m_continuationState = ContinuationState::NONE;

  bool m_gapRepairEnabled = false;
  bool m_gapRepairRequested = false;
  double m_reopenExpectedTs = 0;
  double m_reopenMaxEarlierSecs = 0;
  double m_reopenMaxLaterSecs = 0;
  double m_reopenSkipTargetSource = STREAM_NOPTS_VALUE;
  double m_reopenSkipTargetFallback = STREAM_NOPTS_VALUE;
  double m_pinnedStartTime = 0; // seconds, used instead of the stream's start time when pinned
  bool m_startTimePinned = false;

  double m_skipUntilTs = STREAM_NOPTS_VALUE;
  double m_skipAudioFloorTs = STREAM_NOPTS_VALUE;
  bool m_skipVideoStarted = false;

  HttpProxy m_httpProxy;
  OpenMode m_openMode;
  StreamMode m_streamMode;
};

} //namespace ffmpegdirect
