/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/brave_sync_worker.h"

#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"

#include "brave/build/android/jni_headers/BraveSyncWorker_jni.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/crypto/crypto.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/sync/device_info_sync_service_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"

#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_user_settings.h"
#include "components/sync_device_info/device_info_sync_service.h"
#include "components/sync_device_info/device_info_tracker.h"
#include "components/sync_device_info/local_device_info_provider.h"
#include "components/unified_consent/unified_consent_metrics.h"

#include "third_party/leveldatabase/src/include/leveldb/db.h"


namespace chrome {
namespace android {

// Keep this to clear V1 stuff on migrating
#define DB_FILE_NAME      "brave_sync_db"

BraveSyncWorker::BraveSyncWorker(JNIEnv* env,
  const base::android::JavaRef<jobject>& obj) :
  weak_java_brave_sync_worker_(env, obj) {
  DLOG(ERROR) << "[BraveSync] " << __func__ << " CTOR 000";

  Java_BraveSyncWorker_setNativePtr(env, obj,
    reinterpret_cast<intptr_t>(this));
  DLOG(ERROR) << "[BraveSync] " << __func__ << " CTOR done setNativePtr";
  profile_ =
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
  DLOG(ERROR) << "[BraveSync] " << __func__ << " profile_="<<profile_;
  DCHECK_NE(profile_, nullptr);
}

BraveSyncWorker::~BraveSyncWorker() {
}

void BraveSyncWorker::Destroy(JNIEnv* env, const
        base::android::JavaParamRef<jobject>& jcaller) {
  delete this;
}

static void JNI_BraveSyncWorker_DestroyV1LevelDb(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj) {
  base::FilePath app_data_path;
  base::PathService::Get(base::DIR_ANDROID_APP_DATA, &app_data_path);
  base::FilePath dbFilePath = app_data_path.Append(DB_FILE_NAME);

  leveldb::Status status = leveldb::DestroyDB(dbFilePath.value().c_str(),
    leveldb::Options());
  DLOG(INFO) << "[BraveSync] " << __func__ << " destroy DB status is "<<status.ToString();
}

base::android::ScopedJavaLocalRef<jstring> BraveSyncWorker::GetSyncCodeWords
      (JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller) {
DLOG(ERROR) << "[BraveSync] " << __func__ << " 000 profile_="<<profile_;

  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
  std::string sync_code = brave_sync_prefs.GetSeed();
DLOG(ERROR) << "[BraveSync] " << __func__ << " sync_code=" << sync_code;
  if (sync_code.empty()) {
    std::vector<uint8_t> seed = brave_sync::crypto::GetSeed();
    sync_code = brave_sync::crypto::PassphraseFromBytes32(seed);
DLOG(ERROR) << "[BraveSync] " << __func__ << " sync_code=" << sync_code;
  }

  return base::android::ConvertUTF8ToJavaString(env, sync_code);
}

void BraveSyncWorker::SaveCodeWords(JNIEnv* env,
  const base::android::JavaParamRef<jobject>& jcaller,
  const base::android::JavaParamRef<jstring>& passphrase) {
  std::string str_passphrase = base::android::ConvertJavaStringToUTF8(passphrase);
DLOG(ERROR) << "[BraveSync] " << __func__ << " 000 str_passphrase="<<str_passphrase;

  std::vector<uint8_t> seed;
  if (!brave_sync::crypto::PassphraseToBytes32(str_passphrase, &seed)) {
    LOG(ERROR) << "invalid sync code:" << str_passphrase;
    return;
  }

  passphrase_ = str_passphrase;

  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());

  brave_sync_prefs.SetSeed(str_passphrase);
}

// See PeopleHandler::GetSyncService()
syncer::SyncService* BraveSyncWorker::GetSyncService() const {
DLOG(ERROR) << "[BraveSync] " << __func__ << " ProfileSyncServiceFactory::IsSyncAllowed(profile_)="<<ProfileSyncServiceFactory::IsSyncAllowed(profile_);
if (ProfileSyncServiceFactory::IsSyncAllowed(profile_)) {
DLOG(ERROR) << "[BraveSync] " << __func__ << " ProfileSyncServiceFactory::GetForProfile(profile_)="<<ProfileSyncServiceFactory::GetForProfile(profile_);
}
  return ProfileSyncServiceFactory::IsSyncAllowed(profile_)
             ? ProfileSyncServiceFactory::GetForProfile(profile_)
             : nullptr;
}

// See PeopleHandler::HandleShowSetupUI
void BraveSyncWorker::HandleShowSetupUI(JNIEnv* env,
  const base::android::JavaParamRef<jobject>& jcaller) {
DLOG(ERROR) << "[BraveSync] " << __func__ << " 000";
  syncer::SyncService* service =
        ProfileSyncServiceFactory::GetForProfile(profile_);

  if (service && !sync_service_observer_.IsObserving(service)) {
    sync_service_observer_.Add(service);
  }

  syncer::DeviceInfoTracker* tracker =
    DeviceInfoSyncServiceFactory::GetForProfile(profile_)
       ->GetDeviceInfoTracker();
  DCHECK(tracker);
  if (tracker && !device_info_tracker_observer_.IsObserving(tracker) ) {
    device_info_tracker_observer_.Add(tracker);
  }

  // TODO, AB: call SetSyncRequested(true) and sync_service_observer_ must be set and only if sync is not enabled
  // The point when it is invoked when sync is enabled is above on the stack

  // Mark Sync as requested by the user. It might already be requested, but
  // it's not if this is either the first time the user is setting up Sync, or
  // Sync was set up but then was reset via the dashboard. This also pokes the
  // SyncService to start up immediately, i.e. bypass deferred startup.
  if (service) {
    service->GetUserSettings()->SetSyncRequested(true);
  }

  /* TODO: look on
  void ProfileSyncServiceAndroid::RequestStart(JNIEnv* env,
                                               const JavaParamRef<jobject>&) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    sync_service_->GetUserSettings()->SetSyncRequested(true);
  }
  */
}

// See PeopleHandler::MarkFirstSetupComplete
void BraveSyncWorker::MarkFirstSetupComplete() {
DLOG(ERROR) << "[BraveSync] " << __func__ << " 000";
  syncer::SyncService* service = GetSyncService();
DLOG(ERROR) << "[BraveSync] " << __func__ << " service="<<service;
  // The sync service may be nullptr if it has been just disabled by policy.
  if (!service)
    return;
DLOG(ERROR) << "[BraveSync] " << __func__ << " 001";
  service->GetUserSettings()->SetSyncRequested(true);

  // If the first-time setup is already complete, there's nothing else to do.
  if (service->GetUserSettings()->IsFirstSetupComplete())
    return;
DLOG(ERROR) << "[BraveSync] " << __func__ << " 002";
  unified_consent::metrics::RecordSyncSetupDataTypesHistrogam(
      service->GetUserSettings(), profile_->GetPrefs());
DLOG(ERROR) << "[BraveSync] " << __func__ << " 003";
  // We're done configuring, so notify SyncService that it is OK to start
  // syncing.
  service->GetUserSettings()->SetFirstSetupComplete(
      syncer::SyncFirstSetupCompleteSource::ADVANCED_FLOW_CONFIRM);
  //FireWebUIListener("sync-settings-saved");
DLOG(ERROR) << "[BraveSync] " << __func__ << " 004";
}

// PeopleHandler::OnDidClosePage // TODO, AB: rename
void BraveSyncWorker::OnDidClosePage(JNIEnv* env,
  const base::android::JavaParamRef<jobject>& jcaller) {
DLOG(ERROR) << "[BraveSync] " << __func__ << " 000";
  MarkFirstSetupComplete();
}

bool BraveSyncWorker::IsFirstSetupComplete(JNIEnv* env,
  const base::android::JavaParamRef<jobject>& jcaller) {
/* TODO: look on
jboolean ProfileSyncServiceAndroid::IsFirstSetupComplete(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return sync_service_->GetUserSettings()->IsFirstSetupComplete();
}
*/
  syncer::SyncService* sync_service = GetSyncService();
  DLOG(ERROR) << "[BraveSync] " << __func__ << " will ret " <<
      (sync_service && sync_service->GetUserSettings()->IsFirstSetupComplete());
  return sync_service &&
      sync_service->GetUserSettings()->IsFirstSetupComplete();
}

void BraveSyncWorker::HandleReset(JNIEnv* env,
  const base::android::JavaParamRef<jobject>& jcaller) {
  // PeopleHandler::HandleReset
DLOG(ERROR) << "[BraveSync] " << __func__ << " 000";
  syncer::SyncService* sync_service = GetSyncService();
DLOG(ERROR) << "[BraveSync] " << __func__ << " sync_service=" << sync_service;
  if (sync_service) {
/* TODO: look on
void ProfileSyncServiceAndroid::RequestStop(JNIEnv* env,
                                            const JavaParamRef<jobject>&) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  sync_service_->GetUserSettings()->SetSyncRequested(false);
}
*/
    sync_service->GetUserSettings()->SetSyncRequested(false);
    sync_service->StopAndClear();
  }
  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
DLOG(ERROR) << "[BraveSync] " << __func__ << " invoke brave_sync_prefs.Clear()";
  brave_sync_prefs.Clear();

  // Sync prefs will be clear in ProfileSyncService::StopImpl
}

namespace {

// A structure which contains all the configuration information for sync.
struct SyncConfigInfo {
  SyncConfigInfo();
  ~SyncConfigInfo();

  bool encrypt_all;
  std::string passphrase;
  bool set_new_passphrase;
};

SyncConfigInfo::SyncConfigInfo()
    : encrypt_all(false),
      set_new_passphrase(false) {}

SyncConfigInfo::~SyncConfigInfo() {}

}

void BraveSyncWorker::OnStateChanged(syncer::SyncService* sync) {
  SyncConfigInfo configuration;

  // Inspired by PeopleHandler::HandleSetEncryption
  // Start configuring the SyncService using the configuration passed to us from
  // the JS layer.
  syncer::SyncService* service = GetSyncService();

  // If the sync engine has shutdown for some reason, just close the sync
  // dialog.
  if (!service || !service->IsEngineInitialized()) {
    // CloseSyncSetup();
    // ResolveJavascriptCallback(*callback_id, base::Value(kDonePageStatus));
    return;
  }

  // syncStatus.firstSetupInProgress
  // BRAVE_GET_SYNC_STATUS_DICTIONARY:
  bool firstSetupInProgress = service &&
          !service->GetUserSettings()->IsFirstSetupComplete();

  // this.syncPrefs.encryptAllData
  // PeopleHandler::PushSyncPrefs
  // sync_user_settings->IsEncryptEverythingEnabled()
  configuration.encrypt_all =
      service->GetUserSettings()->IsEncryptEverythingEnabled();

  // this.syncPrefs.passphraseRequired
  // PeopleHandler::PushSyncPrefs
  // sync_user_settings->IsPassphraseRequired()
  bool syncPrefs_passphraseRequired =
      service->GetUserSettings()->IsPassphraseRequired();

  // Inspired by brave_sync_subpage.js:handleSyncPrefsChanged_
  if (!firstSetupInProgress) {
    if (!configuration.encrypt_all) {
      configuration.encrypt_all = true;
      configuration.set_new_passphrase = true;
      DCHECK_NE(this->passphrase_.size(), 0u);
      configuration.passphrase = this->passphrase_;
    } else if (syncPrefs_passphraseRequired) {
      configuration.set_new_passphrase = false;
      DCHECK_NE(this->passphrase_.size(), 0u);
      configuration.passphrase = this->passphrase_;
    } else {
      return;
    }
  }

  // Don't allow "encrypt all" if the SyncService doesn't allow it.
  // The UI is hidden, but the user may have enabled it e.g. by fiddling with
  // the web inspector.
  if (!service->GetUserSettings()->IsEncryptEverythingAllowed()) {
    configuration.encrypt_all = false;
    configuration.set_new_passphrase = false;
  }

  // Note: Data encryption will not occur until configuration is complete
  // (when the PSS receives its CONFIGURE_DONE notification from the sync
  // engine), so the user still has a chance to cancel out of the operation
  // if (for example) some kind of passphrase error is encountered.
  if (configuration.encrypt_all)
    service->GetUserSettings()->EnableEncryptEverything();

  bool passphrase_failed = false;
  if (!configuration.passphrase.empty()) {
    // We call IsPassphraseRequired() here (instead of
    // IsPassphraseRequiredForPreferredDataTypes()) because the user may try to
    // enter a passphrase even though no encrypted data types are enabled.
    if (service->GetUserSettings()->IsPassphraseRequired()) {
      // If we have pending keys, try to decrypt them with the provided
      // passphrase. We track if this succeeds or fails because a failed
      // decryption should result in an error even if there aren't any encrypted
      // data types.
      passphrase_failed = !service->GetUserSettings()->SetDecryptionPassphrase(
          configuration.passphrase);
    } else if (service->GetUserSettings()->IsTrustedVaultKeyRequired()) {
      // There are pending keys due to trusted vault keys being required, likely
      // because something changed since the UI was displayed. A passphrase
      // cannot be set in such circumstances.
      passphrase_failed = true;
    } else {
      // OK, the user sent us a passphrase, but we don't have pending keys. So
      // it either means that the pending keys were resolved somehow since the
      // time the UI was displayed (re-encryption, pending passphrase change,
      // etc) or the user wants to re-encrypt.
      if (configuration.set_new_passphrase &&
          !service->GetUserSettings()->IsUsingSecondaryPassphrase()) {
        service->GetUserSettings()->SetEncryptionPassphrase(
            configuration.passphrase);
      }
    }
  }

  if (passphrase_failed ||
      service->GetUserSettings()->IsPassphraseRequiredForPreferredDataTypes()) {
    // If the user doesn't enter any passphrase, we won't call
    // SetDecryptionPassphrase() (passphrase_failed == false), but we still
    // want to display an error message to let the user know that their blank
    // passphrase entry is not acceptable.

    // TODO(tommycli): Switch this to RejectJavascriptCallback once the
    // Sync page JavaScript has been further refactored.
//    ResolveJavascriptCallback(*callback_id,
//                              base::Value(kPassphraseFailedPageStatus));
  } else {
//    ResolveJavascriptCallback(*callback_id, base::Value(kConfigurePageStatus));
  }

  if (configuration.encrypt_all)
    ProfileMetrics::LogProfileSyncInfo(ProfileMetrics::SYNC_ENCRYPT);
  if (!configuration.set_new_passphrase && !configuration.passphrase.empty())
    ProfileMetrics::LogProfileSyncInfo(ProfileMetrics::SYNC_PASSPHRASE);
}

void BraveSyncWorker::OnDeviceInfoChange() {
  DLOG(ERROR) << "[BraveSync] " << __func__ << " 000";
  // Notify UI page
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveSyncWorker_OnDeviceInfoChangeJava(env,
      weak_java_brave_sync_worker_.get(env));
}

base::Value BraveSyncWorker::GetSyncDeviceList() {
DLOG(ERROR) << "[BraveSync] " << __func__ << " 000";
  auto* device_info_service =
      DeviceInfoSyncServiceFactory::GetForProfile(profile_);
  syncer::DeviceInfoTracker* tracker =
      device_info_service->GetDeviceInfoTracker();
  DCHECK(tracker);
  const syncer::DeviceInfo* local_device_info = device_info_service
     ->GetLocalDeviceInfoProvider()->GetLocalDeviceInfo();

  base::Value device_list(base::Value::Type::LIST);

  for (const auto& device : tracker->GetAllDeviceInfo()) {
    auto device_value = base::Value::FromUniquePtrValue(device->ToValue());
    bool is_current_device = local_device_info
        ? local_device_info->guid() == device->guid()
        : false;
    device_value.SetBoolKey("isCurrentDevice", is_current_device);
    device_list.Append(std::move(device_value));
  }
DLOG(ERROR) << "[BraveSync] " << __func__ << " device_list=" <<device_list;
  return device_list;
}

base::android::ScopedJavaLocalRef<jstring>
    BraveSyncWorker::GetSyncDeviceListJson(JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  base::Value device_list = GetSyncDeviceList();
  std::string json_string;
  if (!base::JSONWriter::Write(device_list, &json_string)) {
    DVLOG(1) << "Writing as JSON failed. Passing empty string to Java code.";
    json_string = std::string();
  }
DLOG(ERROR) << "[BraveSync] " << __func__ << " json_string=" << json_string;
  return base::android::ConvertUTF8ToJavaString(env, json_string);
}


static void JNI_BraveSyncWorker_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
DLOG(ERROR) << "[BraveSync] " << __func__ << " 000";
  new BraveSyncWorker(env, jcaller);
}

base::android::ScopedJavaLocalRef<jstring> JNI_BraveSyncWorker_GetSeedHexFromWords(
  JNIEnv* env,
  const base::android::JavaParamRef<jobject>& jcaller,
  const base::android::JavaParamRef<jstring>& seed_words) {
DLOG(ERROR) << "[BraveSync] " << __func__ << " 000";
  std::string str_seed_words = base::android::ConvertJavaStringToUTF8(
        seed_words);
  DCHECK(!str_seed_words.empty());
DLOG(ERROR) << "[BraveSync] " << __func__ << " str_seed_words="<<str_seed_words;
  std::string sync_code_hex;
  std::vector<uint8_t> bytes;
  if (brave_sync::crypto::PassphraseToBytes32(str_seed_words, &bytes)) {
    DCHECK_EQ(bytes.size(), 32u);
    sync_code_hex = base::HexEncode(&bytes.at(0), bytes.size());
  } else {
    DLOG(WARNING) << "[BraveSync] " << __func__ <<
        " PassphraseToBytes32 failed for " << str_seed_words;
  }
DLOG(ERROR) << "[BraveSync] " << __func__ << " sync_code_hex="<<sync_code_hex;
  return base::android::ConvertUTF8ToJavaString(env, sync_code_hex);
}

}  // namespace android
}  // namespace chrome
