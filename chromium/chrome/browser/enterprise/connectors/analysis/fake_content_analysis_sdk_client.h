// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ENTERPRISE_CONNECTORS_ANALYSIS_FAKE_CONTENT_ANALYSIS_SDK_CLIENT_H_
#define CHROME_BROWSER_ENTERPRISE_CONNECTORS_ANALYSIS_FAKE_CONTENT_ANALYSIS_SDK_CLIENT_H_

#include "third_party/content_analysis_sdk/src/browser/include/content_analysis/sdk/analysis_client.h"

namespace enterprise_connectors {
// A derivative of content analysis SDK client that creates fake clients
// not dependent on having a real service provider agent running.
class FakeContentAnalysisSdkClient : public content_analysis::sdk::Client {
 public:
  explicit FakeContentAnalysisSdkClient(
      const content_analysis::sdk::Client::Config& config);

  ~FakeContentAnalysisSdkClient() override;

  // content_analysis::sdk::Client:
  const content_analysis::sdk::Client::Config& GetConfig() const override;
  int Send(const content_analysis::sdk::ContentAnalysisRequest& request,
           content_analysis::sdk::ContentAnalysisResponse* response) override;
  int Acknowledge(const content_analysis::sdk::ContentAnalysisAcknowledgement&
                      ack) override;
  int CancelRequests(const content_analysis::sdk::ContentAnalysisCancelRequests&
                         cancel) override;

  // Get the latest request client receives.
  const content_analysis::sdk::ContentAnalysisRequest& GetRequest();

  // Get the latest cancel requests receives.
  const content_analysis::sdk::ContentAnalysisCancelRequests&
  GetCancelRequests();

  // Configure response acknowledgement status.
  void SetAckStatus(int status);

  // Configure analysis request sending status.
  void SetSendStatus(int status);

  // Configure cancel requests status.
  void SetCancelStatus(int status);

  // Configure agent response.
  void SetSendResponse(
      const content_analysis::sdk::ContentAnalysisResponse& response);

 private:
  content_analysis::sdk::Client::Config config_;
  content_analysis::sdk::ContentAnalysisResponse response_;
  content_analysis::sdk::ContentAnalysisRequest request_;
  content_analysis::sdk::ContentAnalysisCancelRequests cancel_;
  int send_status_ = 0;
  int ack_status_ = 0;
  int cancel_status_ = 0;
};

}  // namespace enterprise_connectors

#endif  // CHROME_BROWSER_ENTERPRISE_CONNECTORS_ANALYSIS_FAKE_CONTENT_ANALYSIS_SDK_CLIENT_H_
