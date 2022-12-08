// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (c) 2018 Vivaldi Technologies AS. All rights reserved.
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef PLATFORM_MEDIA_IPC_DEMUXER_GPU_PIPELINE_IPC_MEDIA_PIPELINE_H_
#define PLATFORM_MEDIA_IPC_DEMUXER_GPU_PIPELINE_IPC_MEDIA_PIPELINE_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "mojo/public/cpp/bindings/generic_pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

#include "platform_media/ipc_demuxer/gpu/data_source/ipc_data_source.h"
#include "platform_media/ipc_demuxer/gpu/pipeline/ipc_decoding_buffer.h"
#include "platform_media/ipc_demuxer/platform_media.mojom.h"

namespace media {

class PlatformMediaPipeline;

// The IPC-facing participant of the media decoding implementation in the GPU
// process.  It owns a PlatformMediaPipeline and uses it to handle media
// decoding requests.  It owns an IPCDataSource object that provides the
// PlatformMediaPipeline with raw media data by requesting it from a DataSource
// living in the render process.
class IPCMediaPipeline : public platform_media::mojom::Pipeline {
 public:
  IPCMediaPipeline();
  ~IPCMediaPipeline() override;
  IPCMediaPipeline(const IPCMediaPipeline&) = delete;
  IPCMediaPipeline& operator=(const IPCMediaPipeline&) = delete;

  static MEDIA_EXPORT void CreateFactory(mojo::GenericPendingReceiver receiver);

  using StartNewPipelineCallback =
      platform_media::mojom::PipelineFactory::StartNewPipelineCallback;
  void Initialize(platform_media::mojom::PipelineParamsPtr params,
                  StartNewPipelineCallback callback);

 private:
  // See the state diagram below.  Decoding is only allowed in the DECODING
  // state.
  //
  //   CONSTRUCTED
  //       | Initialize()
  //       v
  //     BUSY ----------------------------------------> STOPPED
  //    |     ^               init failure / OnStop()      ^
  //    v     | OnSeek()                                   | OnStop()
  //   DECODING -------------------------------------------
  enum State { CONSTRUCTED, BUSY, DECODING, STOPPED };

  class Factory;
  friend Factory;

  // The method is static to call the callback even when `pipeline` is null.
  // `callback` here belongs to the factory and stopping and deleting the
  // pipeline instance and its connections during initialization does not close
  // the the factory connection so the callback must still be called.
  static void Initialized(base::WeakPtr<IPCMediaPipeline> pipeline,
                          StartNewPipelineCallback callback,
                          platform_media::mojom::PipelineInitResultPtr result);

  void DecodedDataReady(ReadDecodedDataCallback callback,
                        IPCDecodingBuffer buffer);

  // The method is static so we can call the callback with an error status after
  // the week pointer to the pipeline becomes null.
  static void ReadRawData(base::WeakPtr<IPCMediaPipeline> pipeline,
                          ipc_data_source::Buffer source_buffer);

  void DisconnectHandler();

  void SeekDone(SeekCallback callback, bool success);

  bool has_media_type(PlatformStreamType stream_type) const {
    return GetElem(has_media_type_, stream_type);
  }

  void OnRawDataReady(int read_size);

  // platform_media::mojom::Pipeline
  void ReadDecodedData(PlatformStreamType stream_type,
                       ReadDecodedDataCallback callback) override;
  void WillSeek() override;
  void Seek(base::TimeDelta time, SeekCallback callback) override;
  void Stop() override;

  State state_ = CONSTRUCTED;

  bool has_media_type_[kPlatformStreamTypeCount];

  mojo::Remote<platform_media::mojom::PipelineDataSource> data_source_;
  mojo::Receiver<platform_media::mojom::Pipeline> receiver_{this};
  std::unique_ptr<PlatformMediaPipeline> media_pipeline_;

  SEQUENCE_CHECKER(sequence_checker_);

  // Source buffer for pending raw data request.
  ipc_data_source::Buffer pending_source_buffer_;

  // A buffer for decoded media data, shared with the render process.  Filled in
  // the GPU process, consumed in the renderer process.
  IPCDecodingBuffer ipc_decoding_buffers_[kPlatformStreamTypeCount];

  base::WeakPtrFactory<IPCMediaPipeline> weak_ptr_factory_{this};
};

}  // namespace media

#endif  // PLATFORM_MEDIA_IPC_DEMUXER_GPU_PIPELINE_IPC_MEDIA_PIPELINE_H_
