// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/gles2/command_buffer_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "gpu/command_buffer/service/sync_point_manager.h"
#include "services/gles2/command_buffer_driver.h"

namespace gles2 {
namespace {
void DestroyDriver(scoped_ptr<CommandBufferDriver> driver) {
  // Just let ~scoped_ptr run.
}

void RunCallback(const mojo::Callback<void()>& callback) {
  callback.Run();
}

class CommandBufferDriverClientImpl : public CommandBufferDriver::Client {
 public:
  CommandBufferDriverClientImpl(
      base::WeakPtr<CommandBufferImpl> command_buffer,
      scoped_refptr<base::SingleThreadTaskRunner> control_task_runner)
      : command_buffer_(command_buffer),
        control_task_runner_(control_task_runner) {}

 private:
  void UpdateVSyncParameters(base::TimeTicks timebase,
                             base::TimeDelta interval) override {
    control_task_runner_->PostTask(
        FROM_HERE, base::Bind(&CommandBufferImpl::UpdateVSyncParameters,
                              command_buffer_, timebase, interval));
  }

  void DidLoseContext() override {
    command_buffer_->DidLoseContext();
  }

  base::WeakPtr<CommandBufferImpl> command_buffer_;
  scoped_refptr<base::SingleThreadTaskRunner> control_task_runner_;
};
}  // namespace

CommandBufferImpl::CommandBufferImpl(
    mojo::InterfaceRequest<mojo::CommandBuffer> request,
    mojo::ViewportParameterListenerPtr listener,
    scoped_refptr<base::SingleThreadTaskRunner> control_task_runner,
    gpu::SyncPointManager* sync_point_manager,
    scoped_ptr<CommandBufferDriver> driver)
    : sync_point_manager_(sync_point_manager),
      control_task_runner_(control_task_runner),
      driver_task_runner_(base::MessageLoop::current()->task_runner()),
      driver_(driver.Pass()),
      viewport_parameter_listener_(listener.Pass()),
      binding_(this),
      observer_(nullptr),
      weak_factory_(this) {
  driver_->set_client(make_scoped_ptr(new CommandBufferDriverClientImpl(
      weak_factory_.GetWeakPtr(), control_task_runner)));

  control_task_runner_->PostTask(
      FROM_HERE, base::Bind(&CommandBufferImpl::BindToRequest,
                            base::Unretained(this), base::Passed(&request)));
}

CommandBufferImpl::~CommandBufferImpl() {
  driver_task_runner_->PostTask(
      FROM_HERE, base::Bind(&DestroyDriver, base::Passed(&driver_)));
}

void CommandBufferImpl::Initialize(
    mojo::InterfaceHandle<mojo::CommandBufferSyncClient> sync_client,
    mojo::InterfaceHandle<mojo::CommandBufferSyncPointClient> sync_point_client,
    mojo::InterfaceHandle<mojo::CommandBufferLostContextObserver> loss_observer,
    mojo::ScopedSharedBufferHandle shared_state) {
  sync_point_client_ = mojo::CommandBufferSyncPointClientPtr::Create(
      std::move(sync_point_client));
  driver_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&CommandBufferDriver::Initialize,
                 base::Unretained(driver_.get()), base::Passed(&sync_client),
                 base::Passed(&loss_observer),
                 base::Passed(&shared_state)));
}

void CommandBufferImpl::SetGetBuffer(int32_t buffer) {
  driver_task_runner_->PostTask(
      FROM_HERE, base::Bind(&CommandBufferDriver::SetGetBuffer,
                            base::Unretained(driver_.get()), buffer));
}

void CommandBufferImpl::Flush(int32_t put_offset) {
  driver_task_runner_->PostTask(
      FROM_HERE, base::Bind(&CommandBufferDriver::Flush,
                            base::Unretained(driver_.get()), put_offset));
}

void CommandBufferImpl::MakeProgress(int32_t last_get_offset) {
  driver_task_runner_->PostTask(
      FROM_HERE, base::Bind(&CommandBufferDriver::MakeProgress,
                            base::Unretained(driver_.get()), last_get_offset));
}

void CommandBufferImpl::RegisterTransferBuffer(
    int32_t id,
    mojo::ScopedSharedBufferHandle transfer_buffer,
    uint32_t size) {
  driver_task_runner_->PostTask(
      FROM_HERE, base::Bind(&CommandBufferDriver::RegisterTransferBuffer,
                            base::Unretained(driver_.get()), id,
                            base::Passed(&transfer_buffer), size));
}

void CommandBufferImpl::DestroyTransferBuffer(int32_t id) {
  driver_task_runner_->PostTask(
      FROM_HERE, base::Bind(&CommandBufferDriver::DestroyTransferBuffer,
                            base::Unretained(driver_.get()), id));
}

void CommandBufferImpl::InsertSyncPoint(bool retire) {
  uint32_t sync_point = sync_point_manager_->GenerateSyncPoint();
  sync_point_client_->DidInsertSyncPoint(sync_point);
  if (retire) {
    driver_task_runner_->PostTask(
        FROM_HERE, base::Bind(&CommandBufferDriver::RetireSyncPointOnGpuThread,
                              base::Unretained(driver_.get()), sync_point));
  }
}

void CommandBufferImpl::RetireSyncPoint(uint32_t sync_point) {
  driver_task_runner_->PostTask(
      FROM_HERE, base::Bind(&CommandBufferDriver::RetireSyncPointOnGpuThread,
                            base::Unretained(driver_.get()), sync_point));
}

void CommandBufferImpl::Echo(const mojo::Callback<void()>& callback) {
  driver_task_runner_->PostTaskAndReply(FROM_HERE, base::Bind(&base::DoNothing),
                                        base::Bind(&RunCallback, callback));
}

void CommandBufferImpl::DidLoseContext() {
  NotifyAndDestroy();
}

void CommandBufferImpl::UpdateVSyncParameters(base::TimeTicks timebase,
                                              base::TimeDelta interval) {
  if (!viewport_parameter_listener_)
    return;
  viewport_parameter_listener_->OnVSyncParametersUpdated(
      timebase.ToInternalValue(), interval.ToInternalValue());
}

void CommandBufferImpl::BindToRequest(
    mojo::InterfaceRequest<mojo::CommandBuffer> request) {
  binding_.Bind(request.Pass());
  binding_.set_connection_error_handler([this]() { OnConnectionError(); });
}

void CommandBufferImpl::OnConnectionError() {
  // Called from the control_task_runner thread as we bound to the message pipe
  // handle on that thread. However, the observer have to be notified on the
  // thread that created this object, so we post on driver_task_runner.
  driver_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&CommandBufferImpl::NotifyAndDestroy, base::Unretained(this)));
}

void CommandBufferImpl::NotifyAndDestroy() {
  if (observer_) {
    observer_->OnCommandBufferImplDestroyed();
  }
  // We have notified the observer on the right thread. However, destruction of
  // this object must happen on control_task_runner_
  control_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&CommandBufferImpl::Destroy, base::Unretained(this)));
}

void CommandBufferImpl::Destroy() {
  delete this;
}

}  // namespace gles2
