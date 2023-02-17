// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/devtools/simple_devtools_protocol_client/simple_devtools_protocol_client.h"

#include <string>
#include <vector>

#include "base/json/json_writer.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"
#include "base/values.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;

namespace simple_devtools_protocol_client {

class MockDevToolsAgentHost : public content::DevToolsAgentHost {
 public:
  MockDevToolsAgentHost() = default;

  // content::DevToolsAgentHost
  std::string CreateIOStreamFromData(
      scoped_refptr<base::RefCountedMemory>) override {
    return std::string();
  }
  bool AttachClient(content::DevToolsAgentHostClient* client) override {
    DCHECK(!client_);
    client_ = client;
    return true;
  }
  bool AttachClientWithoutWakeLock(
      content::DevToolsAgentHostClient* client) override {
    return AttachClient(client);
  }
  bool DetachClient(content::DevToolsAgentHostClient* client) override {
    if (client != client_)
      return false;
    client_ = nullptr;
    return true;
  }
  bool IsAttached() override { return client_ != nullptr; }
  void DispatchProtocolMessage(content::DevToolsAgentHostClient* client,
                               base::span<const uint8_t> message) override {
    DCHECK_EQ(client, client_);
    client->DispatchProtocolMessage(this, message);
  }
  void InspectElement(content::RenderFrameHost* frame_host,
                      int x,
                      int y) override {}
  std::string GetId() override { return std::string(); }
  std::string GetParentId() override { return std::string(); }
  std::string GetOpenerId() override { return std::string(); }
  bool CanAccessOpener() override { return true; }
  std::string GetOpenerFrameId() override { return std::string(); }
  content::WebContents* GetWebContents() override { return nullptr; }
  content::BrowserContext* GetBrowserContext() override { return nullptr; }
  void DisconnectWebContents() override {}
  void ConnectWebContents(content::WebContents* web_contents) override {}
  std::string GetType() override { return std::string(); }
  std::string GetTitle() override { return std::string(); }
  std::string GetDescription() override { return std::string(); }
  GURL GetURL() override { return GURL(); }
  GURL GetFaviconURL() override { return GURL(); }
  std::string GetFrontendURL() override { return std::string(); }
  bool Activate() override { return true; }
  void Reload() override {}
  bool Close() override { return true; }
  base::TimeTicks GetLastActivityTime() override { return base::TimeTicks(); }
  content::RenderProcessHost* GetProcessHost() override { return nullptr; }
  void ForceDetachAllSessions() override {}

 protected:
  ~MockDevToolsAgentHost() override = default;

  base::raw_ptr<content::DevToolsAgentHostClient> client_ = nullptr;
};

class SimpleDevToolsProtocolClientTest : public SimpleDevToolsProtocolClient,
                                         public testing::Test {
 public:
  SimpleDevToolsProtocolClientTest() {
    AttachClient(new MockDevToolsAgentHost);
  }

  void RunUntilIdle() { task_environment_.RunUntilIdle(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
};

class SimpleDevToolsProtocolClientSendCommandTest
    : public SimpleDevToolsProtocolClientTest {
 public:
  SimpleDevToolsProtocolClientSendCommandTest() = default;

  void SendCommand1() {
    EXPECT_EQ(pending_response_map_.size(), 0ul);
    SendCommand("command1",
                base::BindOnce(&SimpleDevToolsProtocolClientSendCommandTest::
                                   OnSendCommand1Response,
                               base::Unretained(this)));
    RunUntilIdle();
  }

  void OnSendCommand1Response(base::Value::Dict params) {
    EXPECT_EQ(*params.FindString("method"), "command1");
    EXPECT_EQ(pending_response_map_.size(), 0ul);
    SendCommand("command2",
                base::BindOnce(&SimpleDevToolsProtocolClientSendCommandTest::
                                   OnSendCommand2Response,
                               base::Unretained(this)));
    RunUntilIdle();
  }

  void OnSendCommand2Response(base::Value::Dict params) {
    EXPECT_EQ(*params.FindString("method"), "command2");
    EXPECT_EQ(pending_response_map_.size(), 0ul);
    SendCommand("command3",
                base::BindOnce(
                    [](SimpleDevToolsProtocolClientSendCommandTest* self,
                       base::Value::Dict params) {
                      self->OnSendCommand3Response(std::move(params));
                    },
                    base::Unretained(this)));
    RunUntilIdle();
  }

  void OnSendCommand3Response(base::Value::Dict params) {
    EXPECT_EQ(*params.FindString("method"), "command3");
    EXPECT_EQ(pending_response_map_.size(), 0ul);
  }
};

TEST_F(SimpleDevToolsProtocolClientSendCommandTest, CallbackChain) {
  // Verify command result dispatcher map is empty when calling
  // chained commands.
  SendCommand1();
}

class SimpleDevToolsProtocolClientEventHandlerTest
    : public SimpleDevToolsProtocolClientTest {
 public:
  SimpleDevToolsProtocolClientEventHandlerTest() = default;

  void SendEvent(const std::string& event_name) {
    base::Value::Dict params;
    params.Set("method", event_name);

    std::string json;
    base::JSONWriter::Write(base::Value(std::move(params)), &json);
    DispatchProtocolMessage(agent_host_.get(),
                            base::as_bytes(base::make_span(json)));
    RunUntilIdle();
  }
};

class SimpleDevToolsProtocolClientEventHandlerTraceTest
    : public SimpleDevToolsProtocolClientEventHandlerTest {
 public:
  SimpleDevToolsProtocolClientEventHandlerTraceTest() = default;

  void OnEvent1(const base::Value::Dict& params) {
    received_events_.push_back(*params.FindString("method"));
  }
  void OnEvent2(const base::Value::Dict& params) {
    received_events_.push_back(*params.FindString("method"));
  }

  void OnEventX(const base::Value::Dict& params) {
    received_events_.push_back(*params.FindString("method"));
  }

  std::vector<std::string> received_events_;
};

TEST_F(SimpleDevToolsProtocolClientEventHandlerTraceTest,
       AddRemoveEventHandler) {
  EXPECT_EQ(event_handler_map_.size(), 0ul);

  EventCallback event1_handler1 = base::BindRepeating(
      &SimpleDevToolsProtocolClientEventHandlerTraceTest::OnEvent1,
      base::Unretained(this));
  EventCallback event1_handler2 = base::BindRepeating(
      &SimpleDevToolsProtocolClientEventHandlerTraceTest::OnEvent1,
      base::Unretained(this));
  EventCallback event2_handler = base::BindRepeating(
      &SimpleDevToolsProtocolClientEventHandlerTraceTest::OnEvent2,
      base::Unretained(this));

  AddEventHandler("event1", event1_handler1);
  EXPECT_EQ(event_handler_map_.size(), 1ul);
  AddEventHandler("event1", event1_handler2);
  EXPECT_EQ(event_handler_map_.size(), 1ul);
  AddEventHandler("event2", event2_handler);
  EXPECT_EQ(event_handler_map_.size(), 2ul);

  // Event1 is received by two handlers, and event2 by one.
  SendEvent("event1");
  SendEvent("event2");
  EXPECT_THAT(received_events_, ElementsAre("event1", "event1", "event2"));
  received_events_.clear();

  // Both events are received by their respective handlers once.
  RemoveEventHandler("event1", event1_handler1);
  SendEvent("event1");
  SendEvent("event2");
  EXPECT_EQ(event_handler_map_.size(), 2ul);
  EXPECT_THAT(received_events_, ElementsAre("event1", "event2"));
  received_events_.clear();

  // Only the second event is received as the first one has no handlers.
  RemoveEventHandler("event1", event1_handler2);
  SendEvent("event1");
  SendEvent("event2");
  EXPECT_EQ(event_handler_map_.size(), 1ul);
  EXPECT_THAT(received_events_, ElementsAre("event2"));
  received_events_.clear();

  // No events are received.
  RemoveEventHandler("event2", event2_handler);
  SendEvent("event1");
  SendEvent("event2");
  EXPECT_EQ(event_handler_map_.size(), 0ul);
  EXPECT_TRUE(received_events_.empty());
}

TEST_F(SimpleDevToolsProtocolClientEventHandlerTraceTest,
       AddRemoveAllEventHandlers) {
  EXPECT_EQ(event_handler_map_.size(), 0ul);

  EventCallback event1_handler1 = base::BindRepeating(
      &SimpleDevToolsProtocolClientEventHandlerTraceTest::OnEvent1,
      base::Unretained(this));
  EventCallback event1_handler2 = base::BindRepeating(
      &SimpleDevToolsProtocolClientEventHandlerTraceTest::OnEvent1,
      base::Unretained(this));
  EventCallback event2_handler = base::BindRepeating(
      &SimpleDevToolsProtocolClientEventHandlerTraceTest::OnEvent2,
      base::Unretained(this));

  AddEventHandler("event1", event1_handler1);
  AddEventHandler("event1", event1_handler2);
  AddEventHandler("event2", event2_handler);

  SendEvent("event1");
  SendEvent("event2");
  EXPECT_THAT(received_events_, ElementsAre("event1", "event1", "event2"));
  received_events_.clear();

  RemoveEventHandler("event1", event1_handler1);
  RemoveEventHandler("event1", event1_handler2);
  SendEvent("event1");
  SendEvent("event2");
  EXPECT_THAT(received_events_, ElementsAre("event2"));
  received_events_.clear();

  RemoveEventHandler("event2", event2_handler);
  SendEvent("event1");
  SendEvent("event2");
  EXPECT_TRUE(received_events_.empty());
  received_events_.clear();
}

TEST_F(SimpleDevToolsProtocolClientEventHandlerTraceTest, EventsDispatching) {
  AddEventHandler(
      "event1",
      base::BindRepeating(
          &SimpleDevToolsProtocolClientEventHandlerTraceTest::OnEvent1,
          base::Unretained(this)));
  AddEventHandler(
      "event2",
      base::BindRepeating(
          &SimpleDevToolsProtocolClientEventHandlerTraceTest::OnEvent2,
          base::Unretained(this)));

  SendEvent("event1");
  SendEvent("event2");
  SendEvent("event1");
  SendEvent("event2");
  SendEvent("event3");

  // 'event3' is not expected and should be ignored.
  EXPECT_THAT(received_events_,
              ElementsAre("event1", "event2", "event1", "event2"));
}

class SimpleDevToolsProtocolClientEventHandlerNestedAddTest
    : public SimpleDevToolsProtocolClientEventHandlerTest {
 public:
  SimpleDevToolsProtocolClientEventHandlerNestedAddTest() = default;

  void OnEvent(const base::Value::Dict& params) {
    received_events_.push_back(*params.FindString("method"));
    AddEventHandler(
        "event",
        base::BindRepeating(
            &SimpleDevToolsProtocolClientEventHandlerNestedAddTest::OnEvent,
            base::Unretained(this)));
    AddEventHandler(
        "event2",
        base::BindRepeating(
            &SimpleDevToolsProtocolClientEventHandlerNestedAddTest::OnEvent2,
            base::Unretained(this)));
  }

  void OnEvent2(const base::Value::Dict& params) {
    received_events_.push_back(*params.FindString("method"));
    AddEventHandler(
        "event3",
        base::BindRepeating(
            &SimpleDevToolsProtocolClientEventHandlerNestedAddTest::OnEvent3,
            base::Unretained(this)));
  }

  void OnEvent3(const base::Value::Dict& params) {
    received_events_.push_back(*params.FindString("method"));
  }

  std::vector<std::string> received_events_;
};

TEST_F(SimpleDevToolsProtocolClientEventHandlerNestedAddTest, ChainedAddEvent) {
  AddEventHandler(
      "event",
      base::BindRepeating(
          &SimpleDevToolsProtocolClientEventHandlerNestedAddTest::OnEvent,
          base::Unretained(this)));
  SendEvent("event");
  SendEvent("event2");
  SendEvent("event3");

  EXPECT_THAT(received_events_, ElementsAre("event", "event2", "event3"));
}

class SimpleDevToolsProtocolClientEventHandlerNestedRemoveTest
    : public SimpleDevToolsProtocolClientEventHandlerTest {
 public:
  SimpleDevToolsProtocolClientEventHandlerNestedRemoveTest() = default;

  void OnEvent(const base::Value::Dict& params) {
    received_events_.push_back(*params.FindString("method"));
    RemoveEventHandler("event", event_handler_);
    RemoveEventHandler("event", event_handler1_);
    RemoveEventHandler("event2", event_handler2_);
    RemoveEventHandler("event3", event_handler3_);
  }

  void OnEvent1(const base::Value::Dict& params) {
    received_events_.push_back(*params.FindString("method"));
  }

  void OnEvent2(const base::Value::Dict& params) {
    received_events_.push_back(*params.FindString("method"));
  }

  void OnEvent3(const base::Value::Dict& params) {
    received_events_.push_back(*params.FindString("method"));
  }

  std::vector<std::string> received_events_;

  EventCallback event_handler_;
  EventCallback event_handler1_;
  EventCallback event_handler2_;
  EventCallback event_handler3_;
};

TEST_F(SimpleDevToolsProtocolClientEventHandlerNestedRemoveTest,
       NestedRemoveEvent) {
  AddEventHandler(
      "event",
      event_handler_ = base::BindRepeating(
          &SimpleDevToolsProtocolClientEventHandlerNestedRemoveTest::OnEvent,
          base::Unretained(this)));
  AddEventHandler(
      "event",
      event_handler1_ = base::BindRepeating(
          &SimpleDevToolsProtocolClientEventHandlerNestedRemoveTest::OnEvent1,
          base::Unretained(this)));
  AddEventHandler(
      "event2",
      event_handler2_ = base::BindRepeating(
          &SimpleDevToolsProtocolClientEventHandlerNestedRemoveTest::OnEvent2,
          base::Unretained(this)));
  AddEventHandler(
      "event3",
      event_handler3_ = base::BindRepeating(
          &SimpleDevToolsProtocolClientEventHandlerNestedRemoveTest::OnEvent3,
          base::Unretained(this)));

  SendEvent("event");
  SendEvent("event2");
  SendEvent("event3");
  SendEvent("event");

  // All event handlers were removed by the first event handler so we should
  // only register the very first event.
  EXPECT_THAT(received_events_, ElementsAre("event"));
}

}  // namespace simple_devtools_protocol_client