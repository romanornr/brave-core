/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/brave_sync_worker.h"

#include <string>
//#include <iomanip>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/strings/string_number_conversions.h"

//#include "base/files/file_util.h"
//#include "base/json/json_reader.h"
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

#define DB_FILE_NAME      "brave_sync_db" // need this to clear V1 stuff on migrating

leveldb::DB* g_level_db = nullptr;
static std::mutex* g_pLevel_db_init_mutex = new std::mutex();

// namespace {
//
// std::string ExtractObjectIdFromJson(const std::string& json) {
//   // See java BraveSyncWorker.BraveSySaveObjectId
//   // pack looks as
//   // [{"objectId": "69, 224, 213, 24, 100, 92, 94, 82, 63, 236, 192, 154, 81, 237, 213, 154", "order": "255.255.255", "apiVersion": "0"}]  // NOLINT
//   auto root = base::JSONReader::Read(json);
//   if (root && root->is_list() && root->GetList().size() == 1 &&
//       root->GetList()[0].is_dict()) {
//     const std::string* object_id_string =
//         root->GetList()[0].FindStringKey("objectId");
//     if (object_id_string) {
//       return *object_id_string;
//     }
//   }
//   return std::string();
// }
//
// }  // namespace

BraveSyncWorker::BraveSyncWorker(JNIEnv* env,
  //jobject obj
  const base::android::JavaRef<jobject>& obj
):
  weak_java_brave_sync_worker_(env, obj) {
  DLOG(ERROR) << "[BraveSync] " << __func__ << " CTOR 000";

  //Java_BraveSyncWorker_setNativePtr
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



// void CreateOpenDatabase() {
//     if (!g_pLevel_db_init_mutex) {
//         return;
//     }
//     std::lock_guard<std::mutex> guard(*g_pLevel_db_init_mutex);
//
//     if (nullptr == g_level_db) {
//         base::FilePath app_data_path;
//         base::PathService::Get(base::DIR_ANDROID_APP_DATA, &app_data_path);
//         base::FilePath dbFilePath = app_data_path.Append(DB_FILE_NAME);
//
//         leveldb::Options options;
//         options.create_if_missing = true;
//         leveldb::Status status = leveldb::DB::Open(options,
//             dbFilePath.value().c_str(), &g_level_db);
//         if (!status.ok() || !g_level_db) {
//             if (g_level_db) {
//                 delete g_level_db;
//                 g_level_db = nullptr;
//             }
//
//             LOG(ERROR) << "sync level db open error " << DB_FILE_NAME;
//
//             return;
//         }
//     }
// }

base::android::ScopedJavaLocalRef<jstring>
    JNI_BraveSyncWorker_GetLocalIdByObjectId(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller,
        const base::android::JavaParamRef<jstring>& objectId) {
    // CreateOpenDatabase();
    // if (nullptr == g_level_db) {
    //     return base::android::ConvertUTF8ToJavaString(env, "");
    // }
    //
    // std::string value;
    // g_level_db->Get(leveldb::ReadOptions(),
    //     base::android::ConvertJavaStringToUTF8(objectId), &value);
    //
    // return base::android::ConvertUTF8ToJavaString(env, value);
    return base::android::ConvertUTF8ToJavaString(env, "");
}

base::android::ScopedJavaLocalRef<jstring>
    JNI_BraveSyncWorker_GetObjectIdByLocalId(JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& localId) {
    // CreateOpenDatabase();
    // if (nullptr == g_level_db) {
    //     return base::android::ConvertUTF8ToJavaString(env, "");
    // }
    //
    // std::string value;
    // g_level_db->Get(leveldb::ReadOptions(),
    //     base::android::ConvertJavaStringToUTF8(localId), &value);
    //
    // return base::android::ConvertUTF8ToJavaString(env, value);
    return base::android::ConvertUTF8ToJavaString(env, "");
}

void JNI_BraveSyncWorker_SaveObjectId(JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller,
      const base::android::JavaParamRef<jstring>& localId,
      const base::android::JavaParamRef<jstring>& objectIdJSON,
      const base::android::JavaParamRef<jstring>& objectId) {
    // CreateOpenDatabase();
    // if (nullptr == g_level_db) {
    //     return;
    // }
    // std::string strLocalId = base::android::ConvertJavaStringToUTF8(localId);
    //
    // g_level_db->Put(leveldb::WriteOptions(), strLocalId,
    //     base::android::ConvertJavaStringToUTF8(objectIdJSON));
    // std::string strObjectId = base::android::ConvertJavaStringToUTF8(objectId);
    // if (0 != strObjectId.size()) {
    //     g_level_db->Put(leveldb::WriteOptions(), strObjectId, strLocalId);
    // }
}

void JNI_BraveSyncWorker_DeleteByLocalId(JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& localId) {
    // CreateOpenDatabase();
    // if (nullptr == g_level_db) {
    //     return;
    // }
    //
    // std::string strLocalId = base::android::ConvertJavaStringToUTF8(localId);
    // std::string value;
    // g_level_db->Get(leveldb::ReadOptions(), strLocalId, &value);
    // std::string object_id = ExtractObjectIdFromJson(value);
    // if (object_id.empty()) {
    //   object_id = value;
    // }
    //
    // g_level_db->Delete(leveldb::WriteOptions(), strLocalId);
    // g_level_db->Delete(leveldb::WriteOptions(), object_id);
}

static void JNI_BraveSyncWorker_Clear(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj) {
    // if (g_level_db) {
    //     delete g_level_db;
    //     g_level_db = nullptr;
    // }
    // if (g_pLevel_db_init_mutex) {
    //     delete g_pLevel_db_init_mutex;
    //     g_pLevel_db_init_mutex = nullptr;
    // }
}

static void JNI_BraveSyncWorker_ResetSync(JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& key) {
    // CreateOpenDatabase();
    // if (nullptr == g_level_db) {
    //     return;
    // }
    // g_level_db->Delete(leveldb::WriteOptions(),
    //     base::android::ConvertJavaStringToUTF8(key));
}

//static void JNI_BraveSyncWorker_nativeGetSyncCode
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
    //
    return;
  }

  brave_sync::prefs::Prefs brave_sync_prefs(profile_->GetPrefs());

  brave_sync_prefs.SetSeed(str_passphrase);
}

// see PeopleHandler::GetSyncService()
syncer::SyncService* BraveSyncWorker::GetSyncService() const {
DLOG(ERROR) << "[BraveSync] " << __func__ << " ProfileSyncServiceFactory::IsSyncAllowed(profile_)="<<ProfileSyncServiceFactory::IsSyncAllowed(profile_);
if (ProfileSyncServiceFactory::IsSyncAllowed(profile_)) {
DLOG(ERROR) << "[BraveSync] " << __func__ << " ProfileSyncServiceFactory::GetForProfile(profile_)="<<ProfileSyncServiceFactory::GetForProfile(profile_);
}
  return ProfileSyncServiceFactory::IsSyncAllowed(profile_)
             ? ProfileSyncServiceFactory::GetForProfile(profile_)
             : nullptr;
}

//PeopleHandler::HandleShowSetupUI
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

// PeopleHandler::MarkFirstSetupComplete
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


// static void JNI_BraveSyncWorker_GetSyncCode2(
//   JNIEnv* env,
//   const base::android::JavaParamRef<jobject>& jcaller) {
//   DLOG(ERROR) << "[BraveSync] " << __func__ << " 000";
// }

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

//private native boolean nativeIsCodephraseValid(String codepharse);


}  // namespace android
}  // namespace chrome
