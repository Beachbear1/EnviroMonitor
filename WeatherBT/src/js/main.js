import { BleClient } from '@capacitor-community/bluetooth-le';

// Store the scanned devices
let deviceObject = null;

// Function to scan for BLE devices
export async function scan() {
    try {
        // Initialize BLE client
        await BleClient.initialize();
        await BleClient.initialize({ androidNeverForLocation: true });

        // Clear the status and device list before scanning
        document.getElementById('deviceList').innerHTML = ''; // Clear previous results
        document.getElementById('status').innerText = 'Status: Scanning...';

        // Show the modal for scanning devices
        openDeviceModal();

        // Start scanning for devices
        await BleClient.requestLEScan({}, (result) => {
            console.log('Received new scan result:', result);
            displayDevice(result); // Display each device in the modal
        });

        // Stop scanning after 5 seconds only if no connection is made
        setTimeout(async () => {
            if (!deviceObject) { // Only show "Scanning complete" if not connected to any device
                await BleClient.stopLEScan();
                console.log('Stopped scanning');
                if (!deviceObject) { // Double-check that we're still not connected
                    document.getElementById('status').innerText = 'Status: Scanning complete';
                }
            }
        }, 5000);

    } catch (error) {
        console.error('Error during scan:', error);
        document.getElementById('status').innerText = `Error: ${error.message}`;
    }
}

  

// Display scanned devices in the modal (Only shows deviceName and deviceId)
function displayDevice(scanResult) {
  const device = scanResult.device; // Extract the device info from ScanResult

  // Try to get the device name, localName, or fallback to 'Unnamed device'
  const deviceName = device.name || device.localName || 'Unnamed device';
  const deviceId = device.deviceId || 'Unknown ID';

  // Log the device result for debugging
  console.log('Device result:', scanResult);

  const deviceListElement = document.getElementById('deviceList');
  const listItem = document.createElement('li');
  
  listItem.innerHTML = `
    <div class="device-entry">
      <span class="device-name"><strong>${deviceName}</strong></span><br>
      <span class="device-id">Device ID: ${deviceId}</span><br>
      <button onclick="connect('${deviceId}')">Connect</button>
    </div>
  `;
  deviceListElement.appendChild(listItem);
}

// Function to connect to a BLE device and start receiving data
export async function connect(deviceId) {
    try {
        // Stop scanning immediately upon connection to avoid status conflicts
        await BleClient.stopLEScan();  // Stop scanning before connecting
        
        await BleClient.connect(deviceId, () => onDisconnect(deviceId));
        console.log('Connected to device:', deviceId);
        
        // Update status to "Connected"
        document.getElementById('status').innerText = `Connected to device: ${deviceId}`;
        document.getElementById('disconnectButton').disabled = false; // Enable the disconnect button
        deviceObject = { deviceId };

        // Fetch services and characteristics
        const services = await BleClient.getServices(deviceId);
        console.log('Device services:', services);

        // Start reading data from the Bluetooth device
        await startReceivingData(deviceId, services);

        // Close the modal after connecting
        closeDeviceModal();

    } catch (error) {
        console.error('Error connecting to device:', error);
        document.getElementById('status').innerText = `Error: ${error.message}`;
    }
}

  

// Function to disconnect from a BLE device
export async function disconnect() {
  try {
    if (deviceObject && deviceObject.deviceId) {
      await BleClient.disconnect(deviceObject.deviceId);
      console.log('Disconnected from device:', deviceObject.deviceId);
      document.getElementById('status').innerText = 'Disconnected';
      document.getElementById('disconnectButton').disabled = true; // Disable the disconnect button
      deviceObject = null;
    }
  } catch (error) {
    console.error('Error disconnecting from device:', error);
    document.getElementById('status').innerText = `Error: ${error.message}`;
  }
}

// Function to start receiving data (notifications or read)
async function startReceivingData(deviceId, services) {
    const serviceUUID = '0000ffe0-0000-1000-8000-00805f9b34fb';  // HM-10 Service UUID
    const characteristicUUID = '0000ffe1-0000-1000-8000-00805f9b34fb';  // HM-10 Characteristic UUID

    let messageBuffer = '';  // Buffer to store partial messages

    try {
        await BleClient.startNotifications(deviceId, serviceUUID, characteristicUUID, (value) => {
            console.log('Raw value:', value);  // Log the raw DataView
            const receivedData = new TextDecoder().decode(value);
            console.log('Received data:', receivedData);  // Debug the received data

            // Append the received data to the buffer
            messageBuffer += receivedData;

            // Check if we have complete messages (delimited by '!')
            while (messageBuffer.includes('!')) {
                // Extract the complete message up to the first '!'
                const fullMessage = messageBuffer.substring(0, messageBuffer.indexOf('!'));
                messageBuffer = messageBuffer.substring(messageBuffer.indexOf('!') + 1);  // Remove the processed message from the buffer

                console.log('Parsed message:', fullMessage);  // Debug the parsed message
                updateSensorDataUI(fullMessage);  // Process the complete message
            }
        });
    } catch (error) {
        console.error('Error starting notifications:', error);
    }
}

// Function to update the UI with received sensor data
function updateSensorDataUI(receivedData) {
    try {
        console.log('Updating UI with:', receivedData);  // Log received data for debugging
        
        // Assuming the received data format is "Temp:XX.XX,Humidity:XX.XX,Pressure:XXXX.XX"
        const dataParts = receivedData.split(',');

        // If the length of data is less than 3, it's incomplete; we skip updating the UI
        if (dataParts.length < 3) {
            console.error('Incomplete data:', receivedData);
            return;
        }

        const tempPart = dataParts[0]?.split(':')[1];
        const humidityPart = dataParts[1]?.split(':')[1];
        const pressurePart = dataParts[2]?.split(':')[1];

        // Log the parsed temperature, humidity, and pressure for debugging
        console.log('Parsed temperature:', tempPart);
        console.log('Parsed humidity:', humidityPart);
        console.log('Parsed pressure:', pressurePart);

        // Check for valid numeric values before updating the UI
        if (!isNaN(tempPart) && !isNaN(humidityPart) && !isNaN(pressurePart)) {
            document.getElementById('temperature').innerText = `Temperature: ${parseFloat(tempPart).toFixed(2)} F`;
            document.getElementById('humidity').innerText = `Humidity: ${parseFloat(humidityPart).toFixed(2)} %`;
            document.getElementById('pressure').innerText = `Pressure: ${parseFloat(pressurePart).toFixed(2)} hPa`;
        } else {
            console.error("Non-numeric sensor values received:", tempPart, humidityPart, pressurePart);
            // Display error if parsing fails
            document.getElementById('temperature').innerText = 'Temperature: N/A';
            document.getElementById('humidity').innerText = 'Humidity: N/A';
            document.getElementById('pressure').innerText = 'Pressure: N/A';
        }
    } catch (error) {
        console.error('Error parsing sensor data:', error);
        document.getElementById('temperature').innerText = 'Temperature: N/A';
        document.getElementById('humidity').innerText = 'Humidity: N/A';
        document.getElementById('pressure').innerText = 'Pressure: N/A';
    }
}


// Handle disconnection
function onDisconnect(deviceId) {
  console.log(`Device ${deviceId} disconnected`);
  document.getElementById('status').innerText = 'Disconnected';
  document.getElementById('disconnectButton').disabled = true; // Disable the disconnect button
}

// Open the modal to display devices
function openDeviceModal() {
  document.getElementById('deviceModal').style.display = 'block';
}

// Close the modal
function closeDeviceModal() {
  document.getElementById('deviceModal').style.display = 'none';
}

// Expose functions globally so they can be called from the HTML
window.scan = scan;
window.connect = connect;
window.disconnect = disconnect;
window.closeDeviceModal = closeDeviceModal;
