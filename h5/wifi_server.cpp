#include "wifi_server.h"
#include "gcode.h"

// ============================================================
// EMBEDDED WEB UI (served from flash)
// ============================================================

static const char indexhtml[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>NanoEls H5</title>
  <link rel="icon" href="data:;base64,">
  <style>
    body { font-family: Roboto, sans-serif; margin: 0 auto; max-width: 800px; padding: 20px; background-color: #f4f4f4; }
    h1, h2 { color: #333; }
    input[type=text], textarea { width: 100%; }
    #log { height: 200px; overflow-y: scroll; border: 1px solid #ccc; padding: 10px; background-color: #fff; margin-bottom: 20px; }
    #log p { padding: 0; margin: 0; }
    #command-container, #gcode-container { display: flex; align-items: center; margin-bottom: 20px; }
    #command { flex: 1; padding: 10px; border: 1px solid #ccc; border-radius: 4px; margin-right: 10px; }
    button { padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; color: #fff; background-color: #218838; }
    button:hover { background-color: #105b21; }
    #gcode-list { margin-top: 20px; background-color: #fff; padding: 0; border: 1px solid #ccc; border-radius: 4px; }
    #gcode-list.empty { padding: 10px; text-align: center; }
    #gcode-name, #gcode-content { box-sizing: border-box; padding: 10px; border: 1px solid #ccc; border-radius: 4px; margin-bottom: 10px; }
    #gcode-content { height: 200px; resize: vertical; }
    .remove-icon { cursor: pointer; color: #dc3545; font-size: 16px; width: 20px; }
    .remove-icon:hover { color: #c82333; }
    button.disabled { background-color: #ccc; cursor: not-allowed; }
    button.disabled:hover { background-color: #ccc; }
    .gcode-row { display: flex; justify-content: space-between; align-items: center; padding: 10px; cursor: pointer; }
    .gcode-row:hover { background-color: #f0f0f0; }
    .gcode-item { flex: 1; }
    .gcode-size { flex-basis: 80px; font-size: 0.9em; color: #666; }
    .checkbox-container { align-items: center; display: flex; margin-bottom: 10px; }
    .checkbox-container input { margin: 10px 10px 10px 20px; }
  </style>
</head>
<body>
  <h1>NanoEls H5</h1>
  <p>This Web UI is served from your NanoEls controller memory. It doesn't need Internet connection. Anyone on your local network has access to it.</p>
  <p>It communicates with NanoEls in 2 ways. For saving, loading and removing stored GCode files it uses HTTP calls. For realtime communication it uses WebSocket.</p>
  <h2>Stored GCode</h2>
  <div id="gcode-list"></div>
  <p id="free-space"></p>
  <h2>Add GCode</h2>
  <p>You can generate suitable GCode using <a href="https://kachurovskiy.com/lathecode/" target="_blank">lathecode</a>.</p>
  <input type="text" id="gcode-name" placeholder="GCode name" required minlength="2">
  <textarea id="gcode-content" placeholder="GCode content" required minlength="2"></textarea>
  <div class="checkbox-container">
    <button id="add-gcode">Save</button>
    <input type="checkbox" id="remove-comments" checked>
    <label for="remove-comments">Remove comments before saving</label>
  </div>
  <h2>WebSocket realtime communication</h2>
  <div id="log"></div>
  <div id="command-container">
    <input type="text" id="command" placeholder="Enter command" value="?" minlength="1" required>
    <button id="send">Send</button>
  </div>
  <p>Supported websocket commands:</p>
  <ul>
    <li><code>?</code> requests controller status</li>
    <li><code>=20</code> send key code 20 as if pressed on keyboard</li>
    <li><code>!</code> turns the controller off</li>
    <li><code>~</code> turns the controller on</li>
    <li><code>""</code> removes all GCode</li>
  </ul>
  <script>
    const log = document.getElementById('log');
    const commandInput = document.getElementById('command');
    const sendButton = document.getElementById('send');
    const gcodeList = document.getElementById('gcode-list');
    const gcodeNameInput = document.getElementById('gcode-name');
    const gcodeContentInput = document.getElementById('gcode-content');
    const addGcodeButton = document.getElementById('add-gcode');
    const removeCommentsCheckbox = document.getElementById('remove-comments');
    const ws = new WebSocket(`ws://${window.location.host.split(':')[0]}:81`);
    ws.onopen = () => logMessage('Connected to server');
    ws.onmessage = (event) => logMessage('Received: ' + event.data);
    ws.onclose = () => logMessage('Disconnected from server');
    function updateButtonStates() {
      sendButton.disabled = commandInput.value.trim().length < 1;
      addGcodeButton.disabled = gcodeNameInput.value.trim().length < 2 || gcodeContentInput.value.trim().length < 2;
      sendButton.classList.toggle('disabled', sendButton.disabled);
      addGcodeButton.classList.toggle('disabled', addGcodeButton.disabled);
    }
    commandInput.addEventListener('input', updateButtonStates);
    gcodeNameInput.addEventListener('input', updateButtonStates);
    gcodeContentInput.addEventListener('input', updateButtonStates);
    document.addEventListener('DOMContentLoaded', () => { updateButtonStates(); });
    function send() {
      const command = commandInput.value.trim();
      if (command) { logMessage('Sent: ' + command); ws.send(command + '\n'); commandInput.value = ''; updateButtonStates(); }
    }
    commandInput.addEventListener('keydown', (event) => { if (event.key === 'Enter') send(); });
    sendButton.addEventListener('click', () => { send(); });
    function removeComments(content) { return content.split('\n').map(line => line.split(';')[0].trim()).filter(line => !!line).join('\n'); }
    addGcodeButton.addEventListener('click', () => {
      const name = gcodeNameInput.value.trim();
      let content = gcodeContentInput.value.trim();
      if (removeCommentsCheckbox.checked) content = removeComments(content);
      if (name && content) {
        fetch('/gcode/add', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: new URLSearchParams({ name, gcode: content }) })
          .then(response => response.text()).then(data => { logMessage(data); listGcodes(); gcodeNameInput.value = ''; gcodeContentInput.value = ''; updateButtonStates(); });
      }
    });
    function listGcodes() {
      fetch('/gcode/list').then(response => response.text()).then(data => {
        gcodeList.innerHTML = '';
        gcodeList.classList.toggle('empty', !data);
        if (data) {
          data.split('\n').map(g => g.trim()).filter(g => !!g).forEach(gcode => {
            const row = document.createElement('div');
            row.className = 'gcode-row';
            row.dataset.name = gcode;
            row.innerHTML = `<span class="gcode-item" data-name="${gcode}">${gcode}</span><span class="gcode-size"></span><span class="remove-icon" data-name="${gcode}">&times;</span>`;
            row.addEventListener('click', (event) => { loadGcode(event.target.dataset.name); });
            row.title = 'Click to load G-code';
            gcodeList.appendChild(row);
            fetch(`/gcode/get?name=${encodeURIComponent(gcode)}`).then(response => response.text()).then(text => { row.querySelector('.gcode-size').textContent = `${(text.length / 1024).toFixed(1)} KB`; });
          });
          document.querySelectorAll('.remove-icon').forEach(icon => {
            icon.title = 'Click to remove G-code';
            icon.addEventListener('click', (event) => { removeGcode(event.target.getAttribute('data-name')); });
          });
        } else { gcodeList.innerHTML = 'No G-code stored'; }
        fetchFreeSpace();
      });
    }
    function loadGcode(name) { fetch(`/gcode/get?name=${encodeURIComponent(name)}`).then(response => response.text()).then(data => { gcodeNameInput.value = name; gcodeContentInput.value = data; updateButtonStates(); }); }
    function removeGcode(name) { fetch('/gcode/remove', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: new URLSearchParams({ name }) }).then(response => response.text()).then(data => { logMessage(data); listGcodes(); }); }
    function fetchFreeSpace() { fetch('/status').then(response => response.text()).then(data => { const freeSpaceBytes = Number(data.split('\n').find(l => l.startsWith('LittleFS.freeSpace=')).substr('LittleFS.freeSpace='.length)); if (freeSpaceBytes) { document.getElementById('free-space').textContent = `Free space: ${Math.floor(freeSpaceBytes / 1024)} KB`; } }); }
    function logMessage(message) { const p = document.createElement('p'); p.textContent = message; log.appendChild(p); log.scrollTop = log.scrollHeight; }
    listGcodes();
  </script>
</body>
</html>
)rawliteral";

// ============================================================
// WEB SERVER AND WEBSOCKET GLOBALS
// ============================================================

WebServer        server(80);
WebSocketsServer webSocket(81);

// ============================================================
// CIRCLE BUFFER HELPERS
// ============================================================

bool bufferAvailable(CircleBuffer* b) { return b->head != b->tail; }

bool writeBuffer(CircleBuffer* b, char c) {
  if ((b->head + 1) % b->size == b->tail) return false;
  b->buffer[b->head] = c;
  b->head = (b->head + 1) % b->size;
  return true;
}

bool writeBuffer(CircleBuffer* b, const char* str) {
  while (*str) { if (!writeBuffer(b, *str++)) return false; }
  return true;
}

bool writeBuffer(CircleBuffer* b, const String& str) { return writeBuffer(b, str.c_str()); }

bool writeBuffer(CircleBuffer* b, float f, int precision) {
  char buffer[16];
  dtostrf(f, 0, precision, buffer);
  return writeBuffer(b, buffer);
}

bool writeBuffer(CircleBuffer* b, long value) {
  return writeBuffer(b, String(value).c_str());
}

char shiftBuffer(CircleBuffer* b) {
  if (b->head == b->tail) return 0;
  char c   = b->buffer[b->tail];
  b->tail  = (b->tail + 1) % b->size;
  return c;
}

void initBuffer(CircleBuffer* b, size_t size) {
  b->size   = size;
  b->buffer = (char*)malloc(size);
  b->head   = 0;
  b->tail   = 0;
}

void clearBuffer(CircleBuffer* b) { b->head = 0; b->tail = 0; }

// ============================================================
// WIFI STATUS
// ============================================================

void setWiFiStatus(const String& status) {
  wifiStatus       = status;
  wifiStatusMillis = millis();
}

// ============================================================
// WEB / WEBSOCKET HANDLERS
// ============================================================

void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_TEXT) {
    for (size_t i = 0; i < length; i++) writeBuffer(&inBuffer, payload[i]);
  }
}

void handleClientRequests() { server.send(200, "text/html", indexhtml); }

void handleGcodeAdd() {
  if (server.hasArg("name") && server.hasArg("gcode")) {
    gcodeSaveName  = server.arg("name");
    gcodeSaveValue = server.arg("gcode");
    if (saveGcode()) server.send(200, "text/plain", "G-code saved successfully");
    else             server.send(500, "text/plain", "Failed to save G-code");
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void handleGcodeList() {
  String response = "";
  File root = LittleFS.open("/");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      String filename = file.name();
      if (filename.endsWith(".gcode")) response += filename.substring(0, filename.length() - 6) + "\n";
      file.close();
      file = root.openNextFile();
    }
  }
  server.send(200, "text/plain", response);
}

void handleGcodeGet() {
  if (server.hasArg("name")) {
    String gcode = readGcodeProgram(server.arg("name"));
    if (gcode != "") server.send(200, "text/plain", gcode);
    else             server.send(404, "text/plain", "G-code file not found");
  } else {
    server.send(400, "text/plain", "Missing parameter: name");
  }
}

void handleGcodeRemove() {
  if (server.hasArg("name")) {
    if (removeGcodeByName(server.arg("name"))) server.send(200, "text/plain", "G-code removed successfully");
    else                                        server.send(500, "text/plain", "Failed to remove G-code");
  } else {
    server.send(400, "text/plain", "Missing parameter: name");
  }
}

void handleStatus() {
  server.send(200, "text/plain",
    "LittleFS.freeSpace=" + String(LittleFS.totalBytes() - LittleFS.usedBytes()) + "\n");
}

// ============================================================
// WIFI TASK
// ============================================================

void taskWiFi(void* param) {
  WiFi.begin(DEFAULT_SSID, DEFAULT_PASSWORD);
  setWiFiStatus("Connecting");
  for (int i = 0; i < 40; i++) {
    if (WiFi.status() == WL_CONNECTED) break;
    vTaskDelay(500 / portTICK_PERIOD_MS);
    taskYIELD();
  }
  if (WiFi.status() != WL_CONNECTED) {
    if (WiFi.status() == WL_NO_SSID_AVAIL)     setWiFiStatus("No SSID");
    else if (WiFi.status() == WL_CONNECT_FAILED) setWiFiStatus("WiFi failed");
    else if (WiFi.status() == WL_CONNECTION_LOST) setWiFiStatus("WiFi lost");
    else if (WiFi.status() == WL_DISCONNECTED)   setWiFiStatus("WiFi disconnected");
    else                                          setWiFiStatus("WiFi error");
    vTaskDelete(NULL);
    return;
  }
  setWiFiStatus("See " + WiFi.localIP().toString());

  initBuffer(&inBuffer,  INCOMING_BUFFER_SIZE);
  initBuffer(&outBuffer, OUTGOING_BUFFER_SIZE);

  server.on("/",              handleClientRequests);
  server.on("/gcode/add",    HTTP_POST, handleGcodeAdd);
  server.on("/gcode/list",   HTTP_GET,  handleGcodeList);
  server.on("/gcode/get",    HTTP_GET,  handleGcodeGet);
  server.on("/gcode/remove", HTTP_POST, handleGcodeRemove);
  server.on("/status",       HTTP_GET,  handleStatus);
  server.begin();

  webSocket.begin();
  webSocket.onEvent(handleWebSocketEvent);

  while (emergencyStop == ESTOP_NONE) {
    server.handleClient();
    webSocket.loop();
    if (bufferAvailable(&outBuffer)) {
      String outData = "";
      while (bufferAvailable(&outBuffer)) outData += shiftBuffer(&outBuffer);
      webSocket.broadcastTXT(outData);
    }
    taskYIELD();
  }
  vTaskDelete(NULL);
}
