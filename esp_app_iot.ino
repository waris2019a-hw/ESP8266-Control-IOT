#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char *ssid = "ESP8266-Control-IoT";
const char *password = "12345678";
//192.168.4.1
ESP8266WebServer server(80);

const int PIN_D5 = 14; 
const int PIN_D6 = 12; 
const int PIN_D7 = 13; 

// ตัวแปรสำหรับระบบ Auto
bool autoD5 = false; String condD5 = ">"; int threshD5 = 512;
bool autoD6 = false; String condD6 = ">"; int threshD6 = 512;
bool autoD7 = false; String condD7 = ">"; int threshD7 = 512;
unsigned long lastAutoCheck = 0;

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Blynk ESP8266 Dashboard</title>
  <style>
    body { background-color: #15171c; color: #a0a5b1; font-family: sans-serif; text-align: center; margin: 0; padding: 15px; }
    h2 { font-size: 13px; letter-spacing: 2px; color: #5f6675; text-transform: uppercase; margin-bottom: 15px; }
    
    .card { background: #1e222b; border-radius: 12px; padding: 15px; margin: 12px auto; max-width: 320px; box-shadow: 0 4px 15px rgba(0,0,0,0.4); box-sizing: border-box; }
    
    .card-title-bar { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
    .card-title { font-size: 11px; color: #6b7280; font-weight: bold; letter-spacing: 1px; margin: 0; text-align: left; }
    
    .card-gauge { padding-bottom: 25px; }

    .btn-export { background: #00e5ff; color: #15171c; border: none; padding: 4px 10px; border-radius: 4px; font-size: 10px; font-weight: bold; cursor: pointer; transition: 0.2s; outline: none; }
    .btn-export:active { transform: scale(0.95); background: #00b3cc; }

    /* ปุ่มกด Blynk */
    .btn-container { display: flex; justify-content: space-around; align-items: center; }
    .btn-wrap { display: flex; flex-direction: column; align-items: center; position: relative; }
    .btn { width: 65px; height: 65px; border-radius: 50%; background: #15171c; border: 3px solid #2d333f; color: #5f6675; font-size: 15px; font-weight: bold; cursor: pointer; transition: all 0.2s ease; outline: none; }
    .btn-sub { font-size: 11px; color: #5f6675; margin-top: 8px; }
    .auto-badge { position: absolute; top: -5px; right: -5px; background: #9b59b6; color: white; font-size: 9px; padding: 2px 5px; border-radius: 8px; display: none; }
    
    .btn.d5.active { border-color: #e74c3c; color: #e74c3c; box-shadow: 0 0 15px rgba(231, 76, 60, 0.4); }
    .btn.d6.active { border-color: #f1c40f; color: #f1c40f; box-shadow: 0 0 15px rgba(241, 196, 15, 0.4); }
    .btn.d7.active { border-color: #2ecc71; color: #2ecc71; box-shadow: 0 0 15px rgba(46, 204, 113, 0.4); }

    /* เกจวัดความเร็ว A0 */
    .gauge-box { position: relative; width: 180px; height: 135px; margin: 0 auto; }
    svg { transform: rotate(135deg); }
    circle { fill: none; stroke-width: 10; stroke-linecap: round; }
    .bg-arc { stroke: #2a2e39; }
    .val-arc { stroke: #2ecc71; stroke-dasharray: 330; stroke-dashoffset: 330; transition: stroke-dashoffset 0.3s ease; }
    .gauge-num { position: absolute; top: 42%; left: 50%; transform: translate(-50%, -50%); font-size: 32px; font-weight: bold; color: #ffffff; }
    .gauge-minmax { display: flex; justify-content: space-between; width: 140px; margin: 15px auto 0; font-size: 11px; color: #5f6675; }

    /* กราฟ */
    canvas { background: #15171c; border-radius: 8px; border: 1px solid #2a2e39; display: block; margin: 0 auto; }

    /* Automation Panel */
    .auto-row { display: flex; justify-content: space-between; align-items: center; margin-top: 10px; font-size: 12px; background: #15171c; padding: 8px; border-radius: 8px; }
    select, input[type="number"] { background: #2a2e39; color: #fff; border: 1px solid #5f6675; border-radius: 4px; padding: 4px; font-size: 11px; outline: none; }
    .btn-save { background: #2ecc71; color: #15171c; border: none; padding: 4px 8px; border-radius: 4px; cursor: pointer; font-weight: bold; font-size: 10px; }
    .btn-save:active { transform: scale(0.95); }

    /* Terminal Console */
    .terminal { background: #0d0f14; border: 1px solid #2a2e39; border-radius: 8px; height: 120px; overflow-y: auto; font-family: 'Courier New', monospace; font-size: 11px; color: #00ff66; text-align: left; padding: 10px; box-sizing: border-box; }
    .terminal-line { margin-bottom: 4px; word-break: break-all; }
    .terminal-time { color: #5f6675; }
  </style>
</head>
<body>
  <h2>ESP8266 AP DASHBOARD</h2>

  <!-- BUTTONS CARD -->
  <div class="card">
    <div class="card-title">BUTTONS CONTROL</div>
    <div class="btn-container">
      <div class="btn-wrap">
        <div id="badgeD5" class="auto-badge">AUTO</div>
        <button id="btnD5" class="btn d5" onclick="togglePin('d5')">D5</button>
        <div class="btn-sub">SWITCH</div>
      </div>
      <div class="btn-wrap">
        <div id="badgeD6" class="auto-badge">AUTO</div>
        <button id="btnD6" class="btn d6" onclick="togglePin('d6')">D6</button>
        <div class="btn-sub">SWITCH</div>
      </div>
      <div class="btn-wrap">
        <div id="badgeD7" class="auto-badge">AUTO</div>
        <button id="btnD7" class="btn d7" onclick="togglePin('d7')">D7</button>
        <div class="btn-sub">SWITCH</div>
      </div>
    </div>
  </div>

  <!-- AUTOMATION CARD -->
  <div class="card">
    <div class="card-title">AUTOMATION (A0 TRIGGER)</div>
    
    <div class="auto-row">
      <span style="color:#e74c3c; font-weight:bold;">D5</span>
      <select id="modeD5"><option value="0">Manual</option><option value="1">Auto</option></select>
      <select id="condD5"><option value=">">&gt;</option><option value="<">&lt;</option></select>
      <input type="number" id="threshD5" value="512" style="width: 50px;">
      <button class="btn-save" onclick="saveAuto('d5')">SAVE</button>
    </div>

    <div class="auto-row">
      <span style="color:#f1c40f; font-weight:bold;">D6</span>
      <select id="modeD6"><option value="0">Manual</option><option value="1">Auto</option></select>
      <select id="condD6"><option value=">">&gt;</option><option value="<">&lt;</option></select>
      <input type="number" id="threshD6" value="512" style="width: 50px;">
      <button class="btn-save" onclick="saveAuto('d6')">SAVE</button>
    </div>

    <div class="auto-row">
      <span style="color:#2ecc71; font-weight:bold;">D7</span>
      <select id="modeD7"><option value="0">Manual</option><option value="1">Auto</option></select>
      <select id="condD7"><option value=">">&gt;</option><option value="<">&lt;</option></select>
      <input type="number" id="threshD7" value="512" style="width: 50px;">
      <button class="btn-save" onclick="saveAuto('d7')">SAVE</button>
    </div>
  </div>

  <!-- GAUGE CARD -->
  <div class="card card-gauge">
    <div class="card-title">GAUGE (ANALOG A0)</div>
    <div class="gauge-box">
      <svg width="180" height="180">
        <circle class="bg-arc" cx="90" cy="90" r="70" stroke-dasharray="330 440" />
        <circle id="arcVal" class="val-arc" cx="90" cy="90" r="70" stroke-dasharray="330 440" />
      </svg>
      <div id="txtA0" class="gauge-num">0</div>
    </div>
    <div class="gauge-minmax">
      <span>0</span>
      <span>1023</span>
    </div>
  </div>

  <!-- GRAPH CARD -->
  <div class="card">
    <div class="card-title">REAL-TIME GRAPH (A0)</div>
    <canvas id="chartCanvas" width="290" height="120"></canvas>
  </div>

  <!-- TERMINAL CARD + EXPORT CSV -->
  <div class="card">
    <div class="card-title-bar">
      <span class="card-title">TERMINAL DATA</span>
      <button class="btn-export" onclick="exportCSV()">EXPORT CSV</button>
    </div>
    <div id="terminal" class="terminal">
      <div class="terminal-line"><span class="terminal-time">[SYS]</span> Terminal Initialized...</div>
    </div>
  </div>

<script>
  let historyData = new Array(30).fill(0); 
  let csvLogArray = [];                    

  // โหลดค่า Auto Settings จาก ESP8266 ตอนเปิดเว็บ
  function loadAutoConfig() {
    fetch('/getAuto')
      .then(res => res.json())
      .then(data => {
        ['d5', 'd6', 'd7'].forEach(pin => {
          document.getElementById('mode' + pin.toUpperCase()).value = data[pin].auto ? "1" : "0";
          document.getElementById('cond' + pin.toUpperCase()).value = data[pin].cond;
          document.getElementById('thresh' + pin.toUpperCase()).value = data[pin].thresh;
          document.getElementById('badge' + pin.toUpperCase()).style.display = data[pin].auto ? "block" : "none";
        });
      });
  }

  // ส่งค่า Auto Settings ไปยัง ESP8266
  function saveAuto(pin) {
    const pU = pin.toUpperCase();
    const mode = document.getElementById('mode' + pU).value;
    const cond = document.getElementById('cond' + pU).value;
    const thresh = document.getElementById('thresh' + pU).value;
    
    // แปลง > เป็น %3E และ < เป็น %3C สำหรับ URL
    const condEncoded = encodeURIComponent(cond);

    fetch(`/setAuto?pin=${pin}&auto=${mode}&cond=${condEncoded}&thresh=${thresh}`)
      .then(res => res.text())
      .then(msg => {
        alert(pin.toUpperCase() + " Automation Saved!");
        loadAutoConfig(); // รีเฟรชป้ายแสดงสถานะ
      });
  }

  function togglePin(pin) {
    fetch('/toggle?pin=' + pin)
      .then(res => res.json())
      .then(data => {
        setBtnState(pin, data.state);
        // หากกด Manual เอง ให้ปิดป้าย Auto (ระบบจะตั้ง Auto=false ในฝั่ง C++ ให้อัตโนมัติ)
        document.getElementById('badge' + pin.toUpperCase()).style.display = "none";
        document.getElementById('mode' + pin.toUpperCase()).value = "0"; 
      });
  }

  function setBtnState(pin, state) {
    const el = document.getElementById('btn' + pin.toUpperCase());
    if (state) el.classList.add('active');
    else el.classList.remove('active');
  }

  function drawChart(newVal) {
    historyData.push(newVal);
    historyData.shift();

    const canvas = document.getElementById('chartCanvas');
    const ctx = canvas.getContext('2d');
    const w = canvas.width;
    const h = canvas.height;

    ctx.clearRect(0, 0, w, h);
    ctx.strokeStyle = '#2a2e39';
    ctx.lineWidth = 1;
    for (let y = 0; y <= h; y += h / 4) {
      ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
    }
    ctx.strokeStyle = '#00e5ff';
    ctx.lineWidth = 2;
    ctx.beginPath();
    let step = w / (historyData.length - 1);
    for (let i = 0; i < historyData.length; i++) {
      let x = i * step;
      let y = h - (historyData[i] / 1023.0 * (h - 10)) - 5;
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
  }

  function appendTerminal(val) {
    const term = document.getElementById('terminal');
    const now = new Date().toLocaleTimeString();
    const voltage = (val / 1023.0 * 3.3).toFixed(2);

    csvLogArray.push({ time: now, raw: val, volt: voltage });
    if (csvLogArray.length > 500) csvLogArray.shift();

    const line = document.createElement('div');
    line.className = 'terminal-line';
    line.innerHTML = `<span class="terminal-time">[${now}]</span> A0 Raw: <b>${val}</b> (${voltage}V)`;
    term.appendChild(line);
    if (term.childNodes.length > 25) term.removeChild(term.firstChild);
    term.scrollTop = term.scrollHeight;
  }

  function exportCSV() {
    if (csvLogArray.length === 0) { alert("ยังไม่มีข้อมูล A0 สำหรับ Export"); return; }
    let csvContent = "\uFEFFTime,A0_Raw,Voltage_V\n"; 
    csvLogArray.forEach(row => { csvContent += `"${row.time}",${row.raw},${row.volt}\n`; });
    const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.setAttribute('href', url);
    link.setAttribute('download', `ESP8266_A0_Data_${Date.now()}.csv`);
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
  }

  function updateData() {
    fetch('/getA0')
      .then(res => res.json())
      .then(data => {
        let val = data.val;
        document.getElementById('txtA0').innerText = val;
        let offset = 330 - (val / 1023.0 * 330);
        document.getElementById('arcVal').style.strokeDashoffset = offset;
        drawChart(val);
        appendTerminal(val);
      });
  }

  function checkStatus() {
    fetch('/status')
      .then(res => res.json())
      .then(data => {
        setBtnState('d5', data.d5);
        setBtnState('d6', data.d6);
        setBtnState('d7', data.d7);
      });
  }

  // Initial Load
  loadAutoConfig();
  checkStatus();
  setInterval(updateData, 500);
  setInterval(checkStatus, 500); // อัปเดตสถานะปุ่มตลอดเวลาเผื่อระบบ Auto สั่งงาน
</script>
</body>
</html>
)rawliteral";

void handleRoot() { server.send(200, "text/html", INDEX_HTML); }

void handleGetA0() {
  int val = analogRead(A0);
  String json = "{\"val\":" + String(val) + "}";
  server.send(200, "application/json", json);
}

void handleToggle() {
  if (!server.hasArg("pin")) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }
  String pin = server.arg("pin");
  int targetPin = -1;
  
  if (pin == "d5") { targetPin = PIN_D5; autoD5 = false; } // ยกเลิก Auto หากมีการกดปุ่มด้วยตัวเอง
  else if (pin == "d6") { targetPin = PIN_D6; autoD6 = false; }
  else if (pin == "d7") { targetPin = PIN_D7; autoD7 = false; }

  if (targetPin != -1) {
    int currentState = digitalRead(targetPin);
    digitalWrite(targetPin, !currentState);
    String json = "{\"state\":" + String(!currentState) + "}";
    server.send(200, "application/json", json);
  } else {
    server.send(400, "text/plain", "Invalid Pin");
  }
}

void handleStatus() {
  String json = "{";
  json += "\"d5\":" + String(digitalRead(PIN_D5)) + ",";
  json += "\"d6\":" + String(digitalRead(PIN_D6)) + ",";
  json += "\"d7\":" + String(digitalRead(PIN_D7));
  json += "}";
  server.send(200, "application/json", json);
}

void handleGetAuto() {
  String json = "{";
  json += "\"d5\":{\"auto\":" + String(autoD5 ? "true" : "false") + ",\"cond\":\"" + condD5 + "\",\"thresh\":" + String(threshD5) + "},";
  json += "\"d6\":{\"auto\":" + String(autoD6 ? "true" : "false") + ",\"cond\":\"" + condD6 + "\",\"thresh\":" + String(threshD6) + "},";
  json += "\"d7\":{\"auto\":" + String(autoD7 ? "true" : "false") + ",\"cond\":\"" + condD7 + "\",\"thresh\":" + String(threshD7) + "}";
  json += "}";
  server.send(200, "application/json", json);
}

void handleSetAuto() {
  if (server.hasArg("pin") && server.hasArg("auto") && server.hasArg("cond") && server.hasArg("thresh")) {
    String pin = server.arg("pin");
    bool isAuto = (server.arg("auto") == "1");
    String cond = server.arg("cond");
    int thresh = server.arg("thresh").toInt();

    if (pin == "d5") { autoD5 = isAuto; condD5 = cond; threshD5 = thresh; }
    else if (pin == "d6") { autoD6 = isAuto; condD6 = cond; threshD6 = thresh; }
    else if (pin == "d7") { autoD7 = isAuto; condD7 = cond; threshD7 = thresh; }

    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void checkAutoRules() {
  int a0Value = analogRead(A0);

  // เช็คเงื่อนไข D5
  if (autoD5) {
    if (condD5 == ">" && a0Value > threshD5) digitalWrite(PIN_D5, HIGH);
    else if (condD5 == "<" && a0Value < threshD5) digitalWrite(PIN_D5, HIGH);
    else digitalWrite(PIN_D5, LOW);
  }

  // เช็คเงื่อนไข D6
  if (autoD6) {
    if (condD6 == ">" && a0Value > threshD6) digitalWrite(PIN_D6, HIGH);
    else if (condD6 == "<" && a0Value < threshD6) digitalWrite(PIN_D6, HIGH);
    else digitalWrite(PIN_D6, LOW);
  }

  // เช็คเงื่อนไข D7
  if (autoD7) {
    if (condD7 == ">" && a0Value > threshD7) digitalWrite(PIN_D7, HIGH);
    else if (condD7 == "<" && a0Value < threshD7) digitalWrite(PIN_D7, HIGH);
    else digitalWrite(PIN_D7, LOW);
  }
}

void setup() {
  pinMode(PIN_D5, OUTPUT);
  pinMode(PIN_D6, OUTPUT);
  pinMode(PIN_D7, OUTPUT);

  digitalWrite(PIN_D5, LOW);
  digitalWrite(PIN_D6, LOW);
  digitalWrite(PIN_D7, LOW);

  WiFi.softAP(ssid, password);

  server.on("/", handleRoot);
  server.on("/getA0", handleGetA0);
  server.on("/toggle", handleToggle);
  server.on("/status", handleStatus);
  server.on("/getAuto", handleGetAuto);
  server.on("/setAuto", handleSetAuto);

  server.begin();
}

void loop() {
  server.handleClient();

  // ตรวจสอบเงื่อนไข Auto ทุกๆ 100 มิลลิวินาที โดยไม่ให้โค้ดหยุดการทำงาน (Non-blocking)
  if (millis() - lastAutoCheck > 100) {
    lastAutoCheck = millis();
    checkAutoRules();
  }
}
