/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ANDROID_BRAVE_SYNC_WORKER_H_
#define BRAVE_BROWSER_ANDROID_BRAVE_SYNC_WORKER_H_

#include <jni.h>

#include "base/android/jni_weak_ref.h"
#include "base/scoped_observer.h"
#include "components/sync_device_info/device_info_tracker.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_service_observer.h"

class Profile;

namespace chrome {
namespace android {

class BraveSyncWorker : public syncer::SyncServiceObserver,
                        public syncer::DeviceInfoTracker::Observer {
 public:
  BraveSyncWorker(JNIEnv* env,
    //jobject obj
    const base::android::JavaRef<jobject>& obj
  );
  ~BraveSyncWorker() override;

  void Destroy(JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller);

  base::android::ScopedJavaLocalRef<jstring> GetSyncCodeWords(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller);

  void SaveCodeWords(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    const base::android::JavaParamRef<jstring>& passphrase);

  void HandleShowSetupUI(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller);

  void OnDidClosePage(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller);

  bool IsFirstSetupComplete(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller);

  void HandleReset(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller);

  base::android::ScopedJavaLocalRef<jstring>
      GetSyncDeviceListJson(JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller);

 private:
  syncer::SyncService* GetSyncService() const;
  void MarkFirstSetupComplete();
  base::Value GetSyncDeviceList();

  // syncer::SyncServiceObserver implementation.
  void OnStateChanged(syncer::SyncService* sync) override;

  // syncer::DeviceInfoTracker::Observer
  void OnDeviceInfoChange() override;

  JavaObjectWeakGlobalRef weak_java_brave_sync_worker_;
  Profile* profile_ = nullptr;

  std::string passphrase_;

  ScopedObserver<syncer::SyncService, syncer::SyncServiceObserver>
      sync_service_observer_{this};

  ScopedObserver<syncer::DeviceInfoTracker, syncer::DeviceInfoTracker::Observer>
    device_info_tracker_observer_{this};

  DISALLOW_COPY_AND_ASSIGN(BraveSyncWorker);
};

}  // namespace android
}  // namespace chrome

#endif  // BRAVE_BROWSER_ANDROID_BRAVE_SYNC_WORKER_H_
