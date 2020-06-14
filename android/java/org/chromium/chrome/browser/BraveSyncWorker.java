/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.os.Build;
import android.os.Looper;
import android.util.Base64;
import android.util.JsonReader;
import android.util.JsonToken;
import android.webkit.JavascriptInterface;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.components.url_formatter.UrlFormatter;
import org.chromium.chrome.browser.partnerbookmarks.PartnerBookmarksShim;
import org.chromium.chrome.browser.preferences.BraveSyncScreensObserver;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.components.embedder_support.view.ContentView;
import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui.base.WindowAndroid;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.lang.IllegalArgumentException;
import java.lang.Runnable;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Date;
//import java.util.HashSet;
//import java.util.HashMap;
//import java.util.Random;
//import java.util.Scanner;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.UnsupportedEncodingException;


@JNINamespace("chrome::android")
public class BraveSyncWorker {
    public static final String TAG = "SYNC";
    public static final int BIP39_WORD_COUNT = 24;

    private Context mContext;
    private String mDebug = "true";
    private BraveSyncScreensObserver mSyncScreensObserver;

    private long mNativeBraveSyncWorker;

    @CalledByNative
    private void setNativePtr(long nativePtr) {
Log.e(TAG, "[BraveSync] BraveSyncWorker.Init nativePtr=" + nativePtr);
        assert mNativeBraveSyncWorker == 0;
        mNativeBraveSyncWorker = nativePtr;
    }

    private void Init() {
Log.e(TAG, "[BraveSync] BraveSyncWorker.Init mNativeBraveSyncWorker=" + mNativeBraveSyncWorker);
      if (mNativeBraveSyncWorker == 0) {
          nativeInit();
      }
    }

    @Override
    protected void finalize() {
        Destroy();
    }

    private void Destroy() {
        if (mNativeBraveSyncWorker != 0) {
            nativeDestroy(mNativeBraveSyncWorker);
            mNativeBraveSyncWorker = 0;
        }
    }

    public BraveSyncWorker(Context context) {
Log.e(TAG, "[BraveSync] BraveSyncWorker.CTOR");
        mContext = context;
        Init();
        (new MigrationFromV1()).MigrateFromSyncV1();
    }

    private class MigrationFromV1 {
        // Deprecated
        public static final String PREF_NAME = "SyncPreferences";
        private static final String PREF_LAST_FETCH_NAME = "TimeLastFetch";
        private static final String PREF_LATEST_DEVICE_RECORD_TIMESTAMPT_NAME = "LatestDeviceRecordTime";
        private static final String PREF_LAST_TIME_SEND_NOT_SYNCED_NAME = "TimeLastSendNotSynced";
        public static final String PREF_DEVICE_ID = "DeviceId";
        public static final String PREF_BASE_ORDER = "BaseOrder";
        public static final String PREF_LAST_ORDER = "LastOrder";
        public static final String PREF_SEED = "Seed";
        public static final String PREF_SYNC_DEVICE_NAME = "SyncDeviceName";
        private static final String PREF_SYNC_SWITCH = "sync_switch";
        private static final String PREF_SYNC_BOOKMARKS = "brave_sync_bookmarks";
        public static final String PREF_SYNC_TABS = "brave_sync_tabs"; // never used
        public static final String PREF_SYNC_HISTORY = "brave_sync_history"; // never used
        public static final String PREF_SYNC_AUTOFILL_PASSWORDS = "brave_sync_autofill_passwords"; // never used
        public static final String PREF_SYNC_PAYMENT_SETTINGS = "brave_sync_payment_settings"; // never used

        private boolean HaveSyncV1Prefs() {
            SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);

            String deviceId = sharedPref.getString(PREF_DEVICE_ID, null);
            if (null == deviceId) {
                return false;
            }
            return true;
        }

        private void DeleteSyncV1Prefs() {
            SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);
            SharedPreferences.Editor editor = sharedPref.edit();
            editor.remove(PREF_LAST_FETCH_NAME)
                  .remove(PREF_LATEST_DEVICE_RECORD_TIMESTAMPT_NAME)
                  .remove(PREF_LAST_TIME_SEND_NOT_SYNCED_NAME)
                  .remove(PREF_SYNC_SWITCH)
                  .remove(PREF_SYNC_BOOKMARKS)
                  .remove(PREF_SYNC_TABS)
                  .remove(PREF_SYNC_HISTORY)
                  .remove(PREF_SYNC_AUTOFILL_PASSWORDS)
                  .remove(PREF_SYNC_PAYMENT_SETTINGS)
                  .remove(PREF_DEVICE_ID)
                  .remove(PREF_BASE_ORDER)
                  .remove(PREF_LAST_ORDER)
                  .remove(PREF_SYNC_DEVICE_NAME)
                  .apply();
        }

        private void DeleteSyncV1LevelDb() {
            nativeDestroyV1LevelDb();
        }

        public void MigrateFromSyncV1() {
            // Do all migration work in file IO thread because we may need to
            // read shared preferences and delete level db
            PostTask.postTask(TaskTraits.BEST_EFFORT_MAY_BLOCK, () ->
            {
                if (HaveSyncV1Prefs()) {
                    DeleteSyncV1Prefs();
                    DeleteSyncV1LevelDb();
                }
            });
        }
    };

    public String GetCodephrase2() {
      String codephrase = nativeGetSyncCodeWords(mNativeBraveSyncWorker);
Log.e(TAG, "[BraveSync] GetCodephrase codephrase="+codephrase);
      return codephrase;
    }

    public void SaveCodephrase(String codephrase) {
Log.e(TAG, "[BraveSync] SaveCodephrase codephrase="+codephrase);
      nativeSaveCodeWords(mNativeBraveSyncWorker, codephrase);
    }
    public String GetSeedHex(String codephrase) {
Log.e(TAG, "[BraveSync] GetSeedHex codephrase="+codephrase);
Log.e(TAG, "[BraveSync] GetSeedHex hex="+nativeGetSeedHexFromWords(codephrase));
      return nativeGetSeedHexFromWords(codephrase);
    }

    public void InitScreensObserver(BraveSyncScreensObserver syncScreensObserver) {
      mSyncScreensObserver = syncScreensObserver;
    }

    public void HandleShowSetupUI() {
Log.e(TAG, "[BraveSync] HandleShowSetupUI 000");
      nativeHandleShowSetupUI(mNativeBraveSyncWorker);
    }

    public boolean IsFirstSetupComplete() {
Log.e(TAG, "[BraveSync] IsFirstSetupComplete 000");
      return nativeIsFirstSetupComplete(mNativeBraveSyncWorker);
    }

    // TODO(alexeybarabash): rename
    public void OnDidClosePage() {
Log.e(TAG, "[BraveSync] OnDidClosePage 000");
      nativeOnDidClosePage(mNativeBraveSyncWorker);
    }

    public void HandleReset() {
Log.e(TAG, "[BraveSync] HandleReset 000");
      nativeHandleReset(mNativeBraveSyncWorker);
    }

    private native void nativeInit();
    private native void nativeDestroy(long nativeBraveSyncWorker);

    private native void nativeDestroyV1LevelDb();


    private native String nativeGetSyncCodeWords(long nativeBraveSyncWorker);
    private native void nativeHandleShowSetupUI(long nativeBraveSyncWorker);

    private native String nativeGetSeedHexFromWords(String passphrase);
    private native void nativeSaveCodeWords(long nativeBraveSyncWorker, String passphrase);

    private native void nativeOnDidClosePage(long nativeBraveSyncWorker);

    private native boolean nativeIsFirstSetupComplete(long nativeBraveSyncWorker);

    private native void nativeHandleReset(long nativeBraveSyncWorker);
}
