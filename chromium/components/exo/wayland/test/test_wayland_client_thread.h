// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_WAYLAND_TEST_TEST_WAYLAND_CLIENT_THREAD_H_
#define COMPONENTS_EXO_WAYLAND_TEST_TEST_WAYLAND_CLIENT_THREAD_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/message_loop/message_pump_libevent.h"
#include "base/threading/thread.h"
#include "components/exo/wayland/test/test_client.h"

namespace exo::wayland::test {

// TestWaylandClientThread runs a Wayland client on a dedicated thread for
// testing with WaylandServerTest.
class TestWaylandClientThread : public base::Thread,
                                base::MessagePumpLibevent::FdWatcher {
 public:
  explicit TestWaylandClientThread(const std::string& name);

  TestWaylandClientThread(const TestWaylandClientThread&) = delete;
  TestWaylandClientThread& operator=(const TestWaylandClientThread&) = delete;

  ~TestWaylandClientThread() override;

  // Starts the client thread; initializes `client` by calling its Init() method
  // with `params` on the client thread. This method blocks until the
  // initialization on the client thread is done.
  //
  // Returns false on failure. In that case, the other public APIs of this class
  // are not supposed to be called.
  bool Start(std::unique_ptr<TestClient> client, TestClient::InitParams params);

  // Runs `callback` or `closure` on the client thread; blocks until the
  // callable is run and all pending Wayland requests and events are delivered.
  void RunAndWait(base::OnceCallback<void(TestClient*)> callback);
  void RunAndWait(base::OnceClosure closure);

 private:
  // base::MessagePumpLibevent::FdWatcher:
  void OnFileCanReadWithoutBlocking(int fd) override;
  void OnFileCanWriteWithoutBlocking(int fd) override;

  void DoInit(TestClient::InitParams params);
  void DoRun(base::OnceClosure closure);
  void DoCleanUp();

  base::MessagePumpLibevent::FdWatchController controller_;
  std::unique_ptr<TestClient> client_;
};

}  // namespace exo::wayland::test

#endif  // COMPONENTS_EXO_WAYLAND_TEST_TEST_WAYLAND_CLIENT_THREAD_H_
