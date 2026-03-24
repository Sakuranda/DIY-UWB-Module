/*
 * Copyright 2022 NXP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.jetpackexample.managers;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import android.util.Log;

import androidx.core.app.ActivityCompat;

import com.jetpackexample.utils.Utils;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class BluetoothManagerImpl {

    private static final String TAG = BluetoothManagerImpl.class.getName();

    /// Nordic UART Service (NUS) - Primary service for both NXP and Qorvo devices
    protected static UUID serviceUUID = UUID.fromString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
    protected static UUID rxCharacteristicUUID = UUID.fromString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
    protected static UUID txCharacteristicUUID = UUID.fromString("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
    protected static UUID descriptorUUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

    /// Qorvo NI Service - Alternative service for Qorvo DW3220 devices
    protected static UUID qorvoServiceUUID = UUID.fromString("2E938FD0-6A61-11ED-A1EB-0242AC120002");
    protected static UUID qorvoRxCharacteristicUUID = UUID.fromString("2E93998A-6A61-11ED-A1EB-0242AC120002");
    protected static UUID qorvoTxCharacteristicUUID = UUID.fromString("2E939AF2-6A61-11ED-A1EB-0242AC120002");

    private Context context;
    private BluetoothManager bluetoothManager = null;
    private BluetoothAdapter bluetoothAdapter = null;
    private BluetoothLeScanner bluetoothLeScanner = null;
    private BluetoothScanListener bluetoothScanListener = null;
    private BluetoothConnectionListener bluetoothConnectionListener = null;
    private BluetoothDataReceivedListener bluetoothDataReceivedListener = null;

    private BluetoothGatt bluetoothGatt;
    private BluetoothGattCharacteristic txCharacteristic;
    private BluetoothGattCharacteristic rxCharacteristic;

    private static BluetoothManagerImpl mInstance = null;

    public interface BluetoothScanListener {
        void onDeviceScanned(BluetoothDevice device);
    }

    public interface BluetoothConnectionListener {
        void onConnect(String remoteDeviceName);

        void onDisconnect();
    }

    public interface BluetoothDataReceivedListener {
        void onDataReceived(byte[] data);
    }

    /**
     * This class is the entry point for Bluetooth LE Communication.
     *
     * @param context Application context
     */
    private BluetoothManagerImpl(final Context context) {
        this.context = context;

        this.bluetoothManager = (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
        this.bluetoothAdapter = bluetoothManager.getAdapter();
        this.bluetoothLeScanner = bluetoothAdapter.getBluetoothLeScanner();
        this.bluetoothConnectionListener = (BluetoothConnectionListener) context;
        this.bluetoothDataReceivedListener = (BluetoothDataReceivedListener) context;
    }

    public static synchronized BluetoothManagerImpl getInstance(final Context context) {
        if (mInstance == null) {
            mInstance = new BluetoothManagerImpl(context);
        }

        return mInstance;
    }

    public boolean isSupported() {
        return bluetoothAdapter != null;
    }

    public boolean isEnabled() {
        return bluetoothAdapter != null && bluetoothAdapter.isEnabled();
    }

    public boolean isDiscovering() {
        return bluetoothAdapter != null && bluetoothAdapter.isDiscovering();
    }

    public boolean isConnected() {
        return bluetoothGatt != null && bluetoothGatt.getDevice() != null;
    }

    public BluetoothDevice getRemoteDevice() {
        return bluetoothGatt.getDevice();
    }

    /**
     * Start scanning for BLE devices
     *
     * @param bluetoothScanListener Listener where new detected devices will be informed
     */
    public void startLeDeviceScan(BluetoothScanListener bluetoothScanListener) {
        Log.d(TAG, "Bluetooth starting LE Scanning");
        this.bluetoothScanListener = bluetoothScanListener;

        List<ScanFilter> filters = new ArrayList<>();
        // Add filter for Nordic UART Service (NUS)
        ScanFilter nusFilter = new ScanFilter.Builder().setServiceUuid(new ParcelUuid(serviceUUID)).build();
        filters.add(nusFilter);
        // Add filter for Qorvo NI Service
        ScanFilter qorvoFilter = new ScanFilter.Builder().setServiceUuid(new ParcelUuid(qorvoServiceUUID)).build();
        filters.add(qorvoFilter);

        ScanSettings.Builder settings = new ScanSettings.Builder();
        settings.setScanMode(ScanSettings.SCAN_MODE_BALANCED);
        settings.setReportDelay(0);

        if (bluetoothLeScanner != null) {
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED) {
                Log.d(TAG, "Bluetooth SCAN successfully started");
                bluetoothLeScanner.startScan(filters, settings.build(), scanCallback);
            } else {
                Log.d(TAG, "Missing required permission to scan for BLE devices");
            }
        }
    }

    /**
     * Stop scanning for BLE devices
     */
    public void stopLeDeviceScan() {
        Log.d(TAG, "Bluetooth stopping LE Scanning");

        if (bluetoothLeScanner != null) {
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED) {
                bluetoothLeScanner.flushPendingScanResults(scanCallback);
                bluetoothLeScanner.stopScan(scanCallback);
            }
        }
    }

    // Device scan callback.
    private ScanCallback scanCallback = new ScanCallback() {

        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            Log.d(TAG, "New device discovered");
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED) {
                onScan(result.getDevice());
            }
        }
    };

    private final BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {

        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {

            Log.d(TAG, "BluetoothGattCallback onConnectionStateChange. Status: " + status + " State: " + newState);

            if (status == BluetoothGatt.GATT_SUCCESS) {
                if (newState == BluetoothProfile.STATE_CONNECTED) {

                    // Look for target Service
                    bluetoothGatt.discoverServices();

                } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                    onDisconnect();

                    if (bluetoothGatt != null) {
                        bluetoothGatt.close();
                        bluetoothGatt = null;
                    }
                }
            } else {
                //If wearable devices showdown or removed from plug
                onDisconnect();

                if (bluetoothGatt != null) {
                    bluetoothGatt.close();
                    bluetoothGatt = null;
                }
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {

            Log.d(TAG, "BluetoothGattCallback onServicesDiscovered status: " + status);

            // Try Nordic UART Service first
            BluetoothGattService service = gatt.getService(serviceUUID);
            // If not found, try Qorvo NI Service
            if (service == null) {
                service = gatt.getService(qorvoServiceUUID);
                if (service != null) {
                    Log.d(TAG, "Using Qorvo NI Service");
                }
            } else {
                Log.d(TAG, "Using Nordic UART Service");
            }
            if (service == null) {
                Log.d(TAG, "No compatible UWB service found");
                return;
            }

            // Determine which characteristic UUIDs to use based on service
            UUID rxUUID = service.getUuid().equals(qorvoServiceUUID) ? qorvoRxCharacteristicUUID : rxCharacteristicUUID;
            UUID txUUID = service.getUuid().equals(qorvoServiceUUID) ? qorvoTxCharacteristicUUID : txCharacteristicUUID;

            List<BluetoothGattCharacteristic> bluetoothGattCharacteristics = service.getCharacteristics();
            for (int j = 0; j < bluetoothGattCharacteristics.size(); j++) {
                BluetoothGattCharacteristic bluetoothGattCharacteristic = bluetoothGattCharacteristics.get(j);
                if (bluetoothGattCharacteristic.getUuid().equals(rxUUID)) {
                    Log.i(TAG, "Write characteristic found, UUID is: " + bluetoothGattCharacteristic.getUuid().toString());
                    rxCharacteristic = bluetoothGattCharacteristic;
                } else if (bluetoothGattCharacteristic.getUuid().equals(txUUID)) {
                    Log.i(TAG, "Notify characteristic found, UUID is " + bluetoothGattCharacteristic.getUuid().toString());
                    txCharacteristic = bluetoothGattCharacteristic;
                }
            }

            if (!gatt.setCharacteristicNotification(txCharacteristic, true)) {
                Log.d(TAG, "Failed setCharacteristicNotification txCharacteristic");
            }

            try {
                BluetoothGattDescriptor descriptor = txCharacteristic.getDescriptor(descriptorUUID);
                if (descriptor != null) {
                    descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                    bluetoothGatt.writeDescriptor(descriptor);
                    Log.d(TAG, "descriptor written");
                } else {
                    Log.d(TAG, "descriptor is null");
                }
            } catch (NullPointerException e) {
                Log.d(TAG, "NullPointerException!" + e);
            } catch (IllegalArgumentException e) {
                Log.d(TAG, "IllegalArgumentException!" + e);
            }
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            Log.d(TAG, "onCharacteristicChanged");

            if (gatt == null || characteristic == null) {
                Log.d(TAG, "invalid arguments");
                return;
            }

            final byte[] data = characteristic.getValue();
            if (data != null && data.length > 0) {
                Log.d(TAG, "Bluetooth LE Data received: " + Utils.byteArrayToHexString(data));
                bluetoothDataReceivedListener.onDataReceived(data);
            }
        }

        @Override
        public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
            Log.d(TAG, "onDescriptorWrite status: " + status);

            // Request MTU update
            gatt.requestMtu(84);
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {

            Log.d(TAG, "onCharacteristicWrite status: " + status);
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic, int status) {
            Log.d(TAG, "onCharacteristicRead status: " + status);
        }

        @Override
        public void onDescriptorRead(BluetoothGatt gatt,
                                     BluetoothGattDescriptor descriptor, int status) {
            Log.d(TAG, "onDescriptorRead status: " + status);
        }

        @Override
        public void onMtuChanged(BluetoothGatt gatt,
                                 int mtu,
                                 int status) {
            Log.d(TAG, "onMtuChanged status: " + status + " mtu: " + mtu);

            // We are done establishing the connection
            onConnect(gatt.getDevice().getName());
        }
    };

    /**
     * Connects to a Bluetooth device given by its Bluetooth MAC Address
     *
     * @param address Bluetooth device MAC address
     * @return true if connection was launched, else false
     */
    public boolean connect(final String address) {

        if (bluetoothAdapter == null || address == null) {
            Log.d(TAG, "BluetoothAdapter not initialized or unspecified address.");
            return false;
        }

        final BluetoothDevice device = bluetoothAdapter.getRemoteDevice(address);
        if (device == null) {
            Log.d(TAG, "Device not found. Unable to connect.");
            return false;
        } else {
            bluetoothGatt = device.connectGatt(context, false, mGattCallback);
            // bluetoothGatt.requestMtu(84);
            return true;
        }
    }

    public void transmit(byte[] data) {

        if (bluetoothGatt == null || rxCharacteristic == null) {
            return;
        }

        Log.d(TAG, "Bluetooth LE Data to transmit: " + Utils.byteArrayToHexString(data));

        rxCharacteristic.setValue(data);
        rxCharacteristic.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
        bluetoothGatt.writeCharacteristic(rxCharacteristic);
    }

    /**
     * Closes bluetooth managed
     */
    public void close() {

        if (bluetoothGatt != null) {
            bluetoothGatt.close();
        }

        bluetoothGatt = null;
    }

    /**
     * Calls onScan method on SEScanListener object
     * Call is done on the Main UI Thread to get out of the Bluetooth Binder thread
     *
     * @param device Bluetooth device
     */
    private void onScan(final BluetoothDevice device) {
        // Send callback to app on the UI Thread
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                // Send callback to app on the UI Thread
                bluetoothScanListener.onDeviceScanned(device);
            }
        });
    }

    /**
     * Calls onConnect method on SEConnectionListener object
     * Call is done on the Main UI Thread to get out of the Bluetooth Binder thread
     */
    private void onConnect(final String name) {
        // Send callback to app on the UI Thread
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                // Send callback to app on the UI Thread
                bluetoothConnectionListener.onConnect(name);
            }
        });
    }

    /**
     * Calls onDisconnect method on SEConnectionListener object
     * Call is done on the Main UI Thread to get out of the Bluetooth Binder thread
     */
    private void onDisconnect() {
        // Send callback to app on the UI Thread
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                // Send callback to app on the UI Thread
                bluetoothConnectionListener.onDisconnect();
            }
        });
    }
}
