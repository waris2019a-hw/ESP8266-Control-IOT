#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char *ssid = "ESP8266-Control-IoT";
const char *password = "12345678";
//192.168.4.1
ESP8266WebServer server(80);

const int PIN_D5 = 14; 
const int PIN_D6 = 12; 
const int PIN_D7 = 13; 

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
    
    /* Header การ์ดแบบมีปุ่ม Export */
    .card-title-bar { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
    .card-title { font-size: 11px; color: #6b7280; font-weight: bold; letter-spacing: 1px; margin: 0; text-align: left; }
    
    .card-gauge { padding-bottom: 25px; }

    /* ปุ่ม Export CSV */
    .btn-export { background: #00e5ff; color: #15171c; border: none; padding: 4px 10px; border-radius: 4px; font-size: 10px; font-weight: bold; cursor: pointer; transition: 0.2s; outline: none; }
    .btn-export:active { transform: scale(0.95); background: #00b3cc; }

    /* ปุ่มกด Blynk */
    .btn-container { display: flex; justify-content: space-around; align-items: center; }
    .btn-wrap { display: flex; flex-direction: column; align-items: center; }
    .btn { width: 65px; height: 65px; border-radius: 50%; background: #15171c; border: 3px solid #2d333f; color: #5f6675; font-size: 15px; font-weight: bold; cursor: pointer; transition: all 0.2s ease; outline: none; }
    .btn-sub { font-size: 11px; color: #5f6675; margin-top: 8px; }
    
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

    /* กราฟเส้น HTML5 Canvas */
    canvas { background: #15171c; border-radius: 8px; border: 1px solid #2a2e39; display: block; margin: 0 auto; }

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
        <button id="btnD5" class="btn d5" onclick="togglePin('d5')">D5</button>
        <div class="btn-sub">SWITCH</div>
      </div>
      <div class="btn-wrap">
        <button id="btnD6" class="btn d6" onclick="togglePin('d6')">D6</button>
        <div class="btn-sub">SWITCH</div>
      </div>
      <div class="btn-wrap">
        <button id="btnD7" class="btn d7" onclick="togglePin('d7')">D7</button>
        <div class="btn-sub">SWITCH</div>
      </div>
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
  let historyData = new Array(30).fill(0); // ข้อมูลสำหรับวาดกราฟ 30 จุด
  let csvLogArray = [];                   // อาร์เรย์สะสมข้อมูลสำหรับ Export CSV

  function togglePin(pin) {
    fetch('/toggle?pin=' + pin)
      .then(res => res.json())
      .then(data => {
        setBtnState(pin, data.state);
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
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(w, y);
      ctx.stroke();
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

    // สะสมข้อมูลไว้ใน Array สำหรับ Export CSV (จำกัดไว้ไม่เกิน 500 แถวเพื่อไม่ให้หนักเบราว์เซอร์)
    csvLogArray.push({ time: now, raw: val, volt: voltage });
    if (csvLogArray.length > 500) csvLogArray.shift();

    const line = document.createElement('div');
    line.className = 'terminal-line';
    line.innerHTML = `<span class="terminal-time">[${now}]</span> A0 Raw: <b>${val}</b> (${voltage}V)`;
    
    term.appendChild(line);

    if (term.childNodes.length > 25) {
      term.removeChild(term.firstChild);
    }
    term.scrollTop = term.scrollHeight;
  }

  // ฟังก์ชันดาวน์โหลดไฟล์ CSV
  function exportCSV() {
    if (csvLogArray.length === 0) {
      alert("ยังไม่มีข้อมูล A0 สำหรับ Export");
      return;
    }

    let csvContent = "\uFEFFTime,A0_Raw,Voltage_V\n"; // \uFEFF ช่วยให้เปิดภาษาไทยใน Excel ได้ถูกต้อง
    csvLogArray.forEach(row => {
      csvContent += `"${row.time}",${row.raw},${row.volt}\n`;
    });

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

  fetch('/status')
    .then(res => res.json())
    .then(data => {
      setBtnState('d5', data.d5);
      setBtnState('d6', data.d6);
      setBtnState('d7', data.d7);
    });

  setInterval(updateData, 500);
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
  
  if (pin == "d5") targetPin = PIN_D5;
  else if (pin == "d6") targetPin = PIN_D6;
  else if (pin == "d7") targetPin = PIN_D7;

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

  server.begin();
}

void loop() {
  server.handleClient();
}
