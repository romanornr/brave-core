/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/brave_sync_worker.h"

#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/strings/string_number_conversions.h"

#include "base/path_service.h"
#include "brave/build/android/jni_headers/BraveSyncWorker_jni.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/crypto/crypto.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"

#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_user_settings.h"

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

  brave_sync::prefs::Prefs brave_sync_prefs(profile_->GetPrefs());
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

  brave_sync::prefs::Prefs brave_sync_prefs(profile_->GetPrefs());

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
  syncer::SyncService* service = GetSyncService();
DLOG(ERROR) << "[BraveSync] " << __func__ << " service=" << service;
  // BRAVE_HANDLE_SHOW_SETUP_UI {
DLOG(ERROR) << "[BraveSync] " << __func__ << " BRAVE_HANDLE_SHOW_SETUP_UI";
  brave_sync::prefs::Prefs brave_sync_prefs(profile_->GetPrefs());
  if (!brave_sync_prefs.IsSyncV2Migrated()) {
    service->StopAndClear();
    brave_sync_prefs.SetSyncV2Migrated(true);
  }
  brave_sync_prefs.SetSyncEnabled(true);
  // } BRAVE_HANDLE_SHOW_SETUP_UI

  if (service)
    service->GetUserSettings()->SetSyncRequested(true);

  // ?? PushSyncPrefs();
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
    sync_service->GetUserSettings()->SetSyncRequested(false);
    sync_service->StopAndClear();
  }
  brave_sync::prefs::Prefs brave_sync_prefs(profile_->GetPrefs());
DLOG(ERROR) << "[BraveSync] " << __func__ << " invoke brave_sync_prefs.Clear()";
  brave_sync_prefs.Clear();

  // Sync prefs will be clear in ProfileSyncService::StopImpl
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
