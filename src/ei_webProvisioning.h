
#pragma once

static const char PROGMEM webPgSetup[] = R"rawliteral(

<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Device Setup</title>

  <style>
    body {
      font-family: Arial, sans-serif;
      max-width: 700px;
      margin: 0 auto;
      padding: 20px;
      background-color: #f4f4f4;
    }

    h1 {
      text-align: center;
      margin-bottom: 30px;
    }

    section {
      background-color: white;
      padding: 20px;
      margin-bottom: 20px;
      border-radius: 6px;
      box-shadow: 0 2px 5px rgba(0, 0, 0, 0.15);
    }

    h2 {
      margin-top: 0;
      margin-bottom: 20px;
    }

    label {
      display: block;
      margin-top: 12px;
      margin-bottom: 5px;
      font-weight: bold;
    }

    input,
    select {
      width: 100%;
      box-sizing: border-box;
      padding: 8px;
        font-size: 16px;
    }

    button {
      margin-top: 20px;
      padding: 10px 20px;
      font-size: 15px;
      cursor: pointer;
    }

    #status {
      padding: 10px;
      background-color: #eeeeee;
    }

    .statusHeader {
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    .statusHeader h2 {
      margin-bottom: 0;
    }

    .statusHeader button {
      margin-top: 0;
    }
  </style>
</head>

<body>

  <h1 id="pageHeader"></h1>

  <section>
    <h2>WiFi Configuration</h2>

    <label for="wifiSsid">SSID</label>
    <input type="text" id="wifiSsid" autocomplete="off">

    <label for="wifiPassword">Password</label>
    <input type="password" id="wifiPassword" autocomplete="off">

    <button type="button" id="saveWifi">Save WiFi</button>
  </section>

  <section>
    <h2>MQTT Configuration</h2>

    <label for="mqttHost">Host</label>
    <input type="text" id="mqttHost" autocomplete="off">

    <label for="mqttPort">Port</label>
    <input type="number" id="mqttPort" min="1" max="65535">

    <label for="mqttUser">Broker Username</label>
    <input type="text" id="mqttUser" autocomplete="off">

    <label for="mqttPassword">Broker Password</label>
    <input type="password" id="mqttPassword" autocomplete="off">

    <button type="button" id="saveMqtt">Save MQTT</button>
  </section>

  <section>
    <h2>File Provisioning</h2>

    <label for="fileDestination">Destination</label>
    <select id="fileDestination">
        <option value="">Select destination</option>
    </select>

    <label for="fileSelect">File</label>
    <input type="file" id="fileSelect" disabled>
    <button type="button" id="uploadFile" disabled>Upload</button>
  </section>

  <section>
    <div class="statusHeader">
      <h2>Status</h2>
      <button type="button" id="reboot">Reboot</button>
    </div>
    <div id="status">Ready</div>
  </section>

  <script>

let websocket = null;

/*-----  LISTENERS   -----*/
  
document.getElementById("fileDestination").addEventListener("change", updateFileProvisioningState);

document.getElementById("fileSelect").addEventListener("change", updateFileProvisioningState);

document.getElementById("saveWifi").addEventListener("click", saveWifi);

document.getElementById("reboot").addEventListener("click", reboot);

document.getElementById("uploadFile").addEventListener("click", uploadFile);

function updateFileProvisioningState() {
  const destination = document.getElementById("fileDestination").value;
  const file = document.getElementById("fileSelect").files.length > 0;
  document.getElementById("fileSelect").disabled = destination === "";
  document.getElementById("uploadFile").disabled = destination === "" || !file;
}

/*-----  HANDLE THE INCOMING SETUP DATA   -----*/
  
function processSetup(data) {
  document.title = data.pageTitle || "";
  document.querySelector("h1").textContent = data.pageHeader || "";

  if (data.wifi) {
    document.getElementById("wifiSsid").value = data.wifi.ssid || "";
    document.getElementById("wifiPassword").value = data.wifi.password || "";
  }
  if (data.mqtt) {
    document.getElementById("mqttHost").value = data.mqtt.host || "";
    document.getElementById("mqttPort").value = data.mqtt.port || "";
    document.getElementById("mqttUser").value = data.mqtt.brokerUser || "";
    document.getElementById("mqttPassword").value = data.mqtt.brokerPwd || "";
  }
  if (data.fileDestinations) {
    const select = document.getElementById("fileDestination");
    select.innerHTML = '<option value="">Select destination</option>';
    data.fileDestinations.forEach(function(destination) {
        const option = document.createElement("option");
        option.value = destination;
        option.textContent = destination;
        select.appendChild(option);
    });
  }
  if (data.fileDestinations) {
    const select = document.getElementById("fileDestination");
    // Keep the initial placeholder
    select.innerHTML = '<option value="">Select destination</option>';
    data.fileDestinations.forEach(function(destination) {
        const option = document.createElement("option");
        option.value = destination;
        option.textContent = destination;
        select.appendChild(option);
    });
  }
  setStatus("Setup information received.");
}

/*-----  HANDLE THE USER CLICKING ON THE UPLOAD BUTTON  -----*/

async function uploadFile() {

    const destination =
        document.getElementById("fileDestination").value;

    const file =
        document.getElementById("fileSelect").files[0];

    if (!destination || !file) {
        setStatus("Select a destination and file.");
        return;
    }

    if (websocket.readyState !== WebSocket.OPEN) {
        setStatus("WebSocket is not connected.");
        return;
    }

    const path = "/" + destination + "/" + file.name;

    if (!sendMessage(
        "library",
        "storage/file",
        "SET",
        {
            path: path,
            size: file.size
        }
    )) {
        return;
    }

    setStatus("Preparing upload of '" + file.name + "'...");
}

/*-----  TEST BINARY CHUNK TRANSFER TO THE SERVER  -----*/

async function testFileChunks() {
    const file = document.getElementById("fileSelect").files[0];

    if (!file) {
        setStatus("No file selected.");
        return;
    }

    if (websocket.readyState !== WebSocket.OPEN) {
        setStatus("WebSocket is not connected.");
        return;
    }

    const CHUNK_SIZE = 1024;
    let offset = 0;
    let chunkNumber = 0;

    while (offset < file.size) {

        const chunk = file.slice(
            offset,
            offset + CHUNK_SIZE
        );

        const data = await chunk.arrayBuffer();

        websocket.send(data);

        chunkNumber++;
        offset += data.byteLength;

        setStatus(
            "Sent chunk " + chunkNumber +
            " — " + offset + " / " + file.size + " bytes"
        );

        // Give the browser/WebSocket stack a chance to process
        // the send before continuing with the next chunk.
        await new Promise(resolve => setTimeout(resolve, 0));
    }

    setStatus(
        "File transfer test complete: " +
        file.size + " bytes in " +
        chunkNumber + " chunks."
    );
}

/*-----  TEST BINARY TRANSFERS TO THE SERVER  -----*/

function testBinaryUpload() {
    const testData = new TextEncoder().encode("Hello ESP32");

    if (websocket.readyState !== WebSocket.OPEN) {
        setStatus("WebSocket is not connected.");
        return;
    }

    websocket.send(testData);
    setStatus("Binary test sent.");
}

/*-----  REQUEST A REBOOT  -----*/

function reboot() {
    sendMessage(
        "library",
        "system/reboot",
        "SET"
    );
}

/*-----  REQUEST PAGE SETUP INFORMATION  -----*/
  
function getSetup() {
    sendMessage("library", "web/setup", "SETUP");
}

/*-----  REQUEST NETWORK CONFIGURATION DATA  -----*/
  
function getWifiConfig() {
  sendMessage("library", "network/wifi/cfg", "GET");
}

/*-----  SET A STATUS MSG  -----*/
  
function setStatus(message) {
    document.getElementById("status").textContent = message;
}

/*-----  SEND A WS MESSAGE  -----*/
  
function sendMessage(owner, route, command, data = {}) {
  const msg = {owner: owner, route: route, command: command, data: data};
  if (websocket.readyState !== WebSocket.OPEN) {
    setStatus("WebSocket is not connected.");
    return false;
  }
  websocket.send(JSON.stringify(msg));
  setStatus("Sent: " + route + " / " + command);
  return true;
}

/*-----  SEND THE ENTERED WIFI DATA TO THE SERVER  -----*/
  
function saveWifi() {
    const ssid = document.getElementById("wifiSsid").value;
    const password = document.getElementById("wifiPassword").value;

    sendMessage(
        "library",
        "network/wifi/cfg",
        "SET",
        {
            ssid: ssid,
            password: password
        }
    );
}

function connectWebSocket() {
    websocket = new WebSocket("ws://" + window.location.host + "/ws");

    websocket.onopen    = onWebSocketOpen;
    websocket.onmessage = onWebSocketMessage;
    websocket.onclose   = onWebSocketClose;
    websocket.onerror   = onWebSocketError;
}

function onWebSocketMessage(event) {
  const msg = JSON.parse(event.data);
  if (msg.owner !== "library") {
    return;
  }
  if (msg.route === "web/setup" && msg.command === "SETUP") {
    processSetup(msg.data);
    return;
  }
  if (msg.route === "network/wifi/cfg" && msg.command === "GET") {
    processWifiConfig(msg.data);
    return;
  }
  if (msg.route === "network/wifi/cfg" && msg.command === "RESULT") {
    if (msg.data && msg.data.success) {
      setStatus(msg.data.message || "WiFi configuration saved.");
    } else {
      setStatus(msg.data?.message || "WiFi configuration was not saved.");
    }

    return;
  }
  setStatus("Received: " + event.data);
if (
    msg.route === "storage/file" &&
    msg.command === "RESULT"
) {
    if (msg.data && msg.data.success) {

        const file =
            document.getElementById("fileSelect").files[0];

        if (!file) {
            setStatus("Upload file is no longer selected.");
            return;
        }

        uploadFileChunks(file);
    } else {
        setStatus(
            msg.data?.message ||
            "File upload could not be started."
        );
    }

    return;
}
}

function onWebSocketOpen() {
  setStatus("WebSocket connected");
  getSetup();
}

async function uploadFileChunks(file) {

    const CHUNK_SIZE = 1024;
    let offset = 0;
    let chunkNumber = 0;

    while (offset < file.size) {

        const chunk = file.slice(
            offset,
            offset + CHUNK_SIZE
        );

        const data = await chunk.arrayBuffer();

        websocket.send(data);

        chunkNumber++;
        offset += data.byteLength;

        setStatus(
            "Uploading " + file.name +
            " — chunk " + chunkNumber +
            " — " + offset +
            " / " + file.size + " bytes"
        );

        await new Promise(resolve =>
            setTimeout(resolve, 0)
        );
    }

    setStatus(
        "File data transferred: " +
        file.size + " bytes in " +
        chunkNumber + " chunks."
    );

    sendMessage(
        "library",
        "storage/file",
        "COMPLETE"
    );
}

function processWifiConfig(data) {

    document.getElementById("wifiSsid").value =
        data.ssid || "";

    document.getElementById("wifiPassword").value =
        data.password || "";

    setStatus("WiFi configuration received.");
}

function onWebSocketClose() {
    setStatus("WebSocket disconnected.");
}

function onWebSocketError() {
    setStatus("WebSocket error.");
}

connectWebSocket();

  </script>
</body>
</html>

)rawliteral";
