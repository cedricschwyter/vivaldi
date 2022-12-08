// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UPDATER_APP_SERVER_LINUX_SERVER_H_
#define CHROME_UPDATER_APP_SERVER_LINUX_SERVER_H_

#include "base/memory/scoped_refptr.h"
#include "chrome/updater/app/app_server.h"

namespace updater {

class App;
class UpdateService;
class UpdateServiceInternal;

class AppServerLinux : public AppServer {
 public:
  AppServerLinux();

 private:
  ~AppServerLinux() override;

  // Overrides for AppServer.
  void ActiveDuty(scoped_refptr<UpdateService> update_service) override;
  void ActiveDutyInternal(
      scoped_refptr<UpdateServiceInternal> update_service_internal) override;
  bool SwapInNewVersion() override;
  bool MigrateLegacyUpdaters(
      base::RepeatingCallback<void(const RegistrationRequest&)>
          register_callback) override;
  void UninstallSelf() override;
};

scoped_refptr<App> MakeAppServer();

}  // namespace updater

#endif  // CHROME_UPDATER_APP_SERVER_LINUX_SERVER_H_
