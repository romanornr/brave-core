/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.sync;


import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

@JNINamespace("chrome::android")
public class BraveSyncDevices {
    public static final String TAG = "SYNC";
    private long mNativeBraveSyncDevicesAndroid = 0;

    private static BraveSyncDevices sBraveSyncDevices;
    private static boolean sInitialized;

    public static BraveSyncDevices get() {
        ThreadUtils.assertOnUiThread();
        if (!sInitialized) {
            sBraveSyncDevices = new BraveSyncDevices();
            sInitialized = true;
        }
        return sBraveSyncDevices;
    }

    public BraveSyncDevices() {
        Init();
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
Log.e(TAG, "[BraveSync] BraveSyncDevices.Init nativePtr=" + nativePtr);
        assert mNativeBraveSyncDevicesAndroid == 0;
        mNativeBraveSyncDevicesAndroid = nativePtr;
    }

    private void Init() {
Log.e(TAG, "[BraveSync] BraveSyncDevices.Init mNativeBraveSyncDevicesAndroid=" + mNativeBraveSyncDevicesAndroid);
      if (mNativeBraveSyncDevicesAndroid == 0) {
          nativeInit();
      }
    }

    @Override
    protected void finalize() {
        Destroy();
    }

    private void Destroy() {
        if (mNativeBraveSyncDevicesAndroid != 0) {
            nativeDestroy(mNativeBraveSyncDevicesAndroid);
            mNativeBraveSyncDevicesAndroid = 0;
        }
    }

    /**
     * Listener for the devices syncchain changes.
     */
    public interface DeviceInfoChangedListener {
        // Invoked when the device info has changed.
        public void deviceInfoChanged();
    }

    // Sync state changes more often than listeners are added/removed, so using CopyOnWrite.
    private final List<DeviceInfoChangedListener> mDeviceInfoListeners =
            new CopyOnWriteArrayList<DeviceInfoChangedListener>();

    public void addDeviceInfoChangedListener(DeviceInfoChangedListener listener) {
Log.e(TAG, "[BraveSync] addDeviceInfoChangedListener 000");
        ThreadUtils.assertOnUiThread();
        mDeviceInfoListeners.add(listener);
    }

    public void removeDeviceInfoChangedListener(DeviceInfoChangedListener listener) {
Log.e(TAG, "[BraveSync] removeDeviceInfoChangedListener 000");
        ThreadUtils.assertOnUiThread();
        mDeviceInfoListeners.remove(listener);
    }

    /**
     * Called when the state of the native sync engine has changed, so various
     * UI elements can update themselves.
     */
    @CalledByNative
    protected void deviceInfoChanged() {
Log.e(TAG, "[BraveSync] deviceInfoChanged 000 - invoking listeners");
        for (DeviceInfoChangedListener listener : mDeviceInfoListeners) {
            listener.deviceInfoChanged();
        }
    }

    public class SyncDeviceInfo {
        public String mName;
        public boolean mIsCurrentDevice;
        public String mType;
        public Date mLastUpdatedTimestamp;
    }

    public ArrayList<SyncDeviceInfo> GetSyncDeviceList() {
Log.e(TAG, "[BraveSync] GetSyncDeviceList 000");
        ArrayList<SyncDeviceInfo> deviceList = new ArrayList<SyncDeviceInfo>();
        String json = nativeGetSyncDeviceListJson(mNativeBraveSyncDevicesAndroid);
        // Add root element to make it real JSON, otherwise getJSONArray cannot parse it
        json = "{\"devices\":" + json + "}";
Log.e(TAG, "[BraveSync] GetSyncDeviceList json="+json);
        try {
            JSONObject result = new JSONObject(json);
            JSONArray devices = result.getJSONArray("devices");
Log.e(TAG, "[BraveSync] GetSyncDeviceList devices.length()="+devices.length());
            for (int i = 0; i < devices.length(); i++) {
              SyncDeviceInfo deviceInfo = new SyncDeviceInfo();
              JSONObject device = devices.getJSONObject(i);
              deviceInfo.mName = device.getString("name");
              deviceInfo.mIsCurrentDevice = device.getBoolean("isCurrentDevice");
              deviceInfo.mType = device.getString("type");
              long lastUpdatedTimestamp = device.getLong("lastUpdatedTimestamp");
              deviceInfo.mLastUpdatedTimestamp = new Date(lastUpdatedTimestamp);
              deviceList.add(deviceInfo);
            }
        } catch (JSONException e) {
            Log.e(TAG, "GetDeviceNameByObjectId JSONException error " + e);
        } catch (IllegalStateException e) {
            Log.e(TAG, "GetDeviceNameByObjectId IllegalStateException error " + e);
        }
Log.e(TAG, "[BraveSync] GetSyncDeviceList deviceList.size()="+deviceList.size());
        return deviceList;
    }

    private native void nativeInit();
    private native void nativeDestroy(long nativeBraveSyncDevicesAndroid);

    private native String nativeGetSyncDeviceListJson(long nativeBraveSyncDevicesAndroid);


/*
// syncer::SyncServiceObserver implementation.
void OnStateChanged(syncer::SyncService* sync) override;

// syncer::DeviceInfoTracker::Observer
void OnDeviceInfoChange() override;

*/

}
