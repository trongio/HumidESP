#pragma once
// Single-file web UI. No external resources: the hotspot has no internet.
static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>HumidESP</title>
<link rel="icon" href="/icon-180.png"><link rel="manifest" href="/manifest.json"><link rel="apple-touch-icon" href="/icon-180.png"><meta name="theme-color" content="#0f1418">
<meta name="apple-mobile-web-app-capable" content="yes"><meta name="mobile-web-app-capable" content="yes"><meta name="apple-mobile-web-app-title" content="HumidESP">
<style>
:root{--bg:#0f1418;--card:#182027;--line:#26313a;--fg:#e6edf2;--mut:#8b9aa6;--acc:#4cc2ff;--ok:#3ccf8e;--warn:#ffb547;--bad:#ff5d6c}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.4 system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
main{max-width:760px;margin:0 auto;padding:12px 16px 40px}
header{display:flex;align-items:center;justify-content:space-between;gap:8px;padding:6px 0 12px}
h1{font-size:20px;margin:0}h1 span{color:var(--acc)}
h2{font-size:13px;text-transform:uppercase;letter-spacing:1px;color:var(--mut);margin:0;font-weight:600}
.hd{display:flex;justify-content:space-between;align-items:center;gap:8px;margin-bottom:10px;flex-wrap:wrap}
.pill{font-size:12px;padding:4px 10px;border-radius:99px;background:var(--card);border:1px solid var(--line);color:var(--mut);white-space:nowrap;overflow:hidden;text-overflow:ellipsis;max-width:60%}
.pill.on{color:var(--ok);border-color:#2a5a45}
.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:14px;margin-bottom:12px}
.alert{background:#3a1d22;border:1px solid #6b2a33;color:#ffd5da;border-radius:12px;padding:10px 12px;margin-bottom:8px;display:flex;justify-content:space-between;align-items:center;gap:8px}
.hero{display:flex;align-items:center;gap:18px}
.gauge{position:relative;width:150px;height:150px;flex:none}.gauge svg{transform:rotate(135deg)}
.gauge .v{position:absolute;inset:0;display:flex;flex-direction:column;align-items:center;justify-content:center;text-align:center}
.gauge .big{font-size:44px;font-weight:700;line-height:1}.gauge .big small{font-size:18px;color:var(--mut)}
.stats{flex:1;display:grid;gap:8px;width:100%}
.stat{display:flex;justify-content:space-between;border-bottom:1px solid var(--line);padding-bottom:6px}
.stat b{font-weight:600}.mut{color:var(--mut)}.sm{font-size:13px}
.target{display:flex;align-items:center;gap:10px;margin-top:14px}.target input{flex:1;accent-color:var(--acc)}
.tv{font-size:22px;font-weight:700;min-width:58px;text-align:center}
button,select{font:inherit;color:var(--fg);background:#22303a;border:1px solid var(--line);border-radius:10px;padding:8px 14px;cursor:pointer}
button:hover{border-color:var(--acc)}button.pri{background:var(--acc);color:#04202f;border:0;font-weight:600}
button.danger{color:var(--bad)}button.sq{width:40px;padding:8px 0;font-size:18px}button.s{padding:4px 10px;font-size:13px}
.ch{display:flex;align-items:center;justify-content:space-between;gap:10px;flex-wrap:wrap;padding:6px 0}
.dot{display:inline-block;width:10px;height:10px;border-radius:50%;background:#44515c;margin-right:8px}
.dot.on{background:var(--acc);box-shadow:0 0 10px var(--acc)}.dot.lock{background:var(--bad)}
.seg{display:flex;border:1px solid var(--line);border-radius:10px;overflow:hidden}
.seg button{border:0;border-radius:0;background:transparent;padding:6px 12px;color:var(--mut)}
.seg button.act{background:var(--acc);color:#04202f;font-weight:600}
canvas{width:100%;height:220px;display:block}
.log{max-height:220px;overflow:auto;font-size:13px}.log div{padding:4px 0;border-bottom:1px solid var(--line)}.log span{color:var(--mut);margin-right:8px}
details>summary{cursor:pointer;list-style:none;font-weight:600}details>summary::after{content:" \25BE";color:var(--mut)}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:12px}
label{display:flex;flex-direction:column;gap:4px;font-size:13px;color:var(--mut)}
label.chk{flex-direction:row;align-items:center;gap:8px;color:var(--fg)}
input[type=number],input[type=text],input[type=password],input[type=url],select{font:inherit;color:var(--fg);background:#0f1418;border:1px solid var(--line);border-radius:8px;padding:8px;width:100%}
.full{grid-column:1/-1}.row{display:flex;gap:8px;flex-wrap:wrap;margin-top:12px;align-items:center}
.net{display:flex;justify-content:space-between;padding:8px;border-radius:8px;cursor:pointer}.net:hover,.net.sel{background:#22303a}
table{width:100%;border-collapse:collapse;font-size:14px}td,th{padding:7px 4px;border-bottom:1px solid var(--line);text-align:left}th{color:var(--mut);font-weight:500;font-size:12px}
.sw{display:inline-block;width:10px;height:10px;border-radius:3px;margin-right:6px}
svg.room{width:100%;display:block;touch-action:none;background:#11181d;border-radius:10px}
svg.room .drag{cursor:grab}
.edit{border:1px dashed var(--line);border-radius:12px;padding:12px;margin-top:12px}
#toast{position:fixed;left:50%;bottom:20px;transform:translateX(-50%);background:#22303a;border:1px solid var(--line);padding:10px 16px;border-radius:10px;opacity:0;transition:opacity .3s;pointer-events:none;z-index:9}
#login{position:fixed;inset:0;background:var(--bg);display:none;align-items:center;justify-content:center;z-index:8;padding:16px}
#login form{width:100%;max-width:320px;display:grid;gap:12px}
@media(max-width:520px){.hero{flex-direction:column}.grid{grid-template-columns:1fr}.hide-m{display:none}}
</style></head><body><main>
<header><h1>Humid<span>ESP</span></h1><div class="pill" id="net">...</div></header>
<div id="alerts"></div>

<section class="card">
 <div class="hero">
  <div class="gauge"><svg width="150" height="150" viewBox="0 0 150 150">
   <circle cx="75" cy="75" r="62" fill="none" stroke="#26313a" stroke-width="12" stroke-dasharray="292 390" stroke-linecap="round"/>
   <circle id="arc" cx="75" cy="75" r="62" fill="none" stroke="var(--acc)" stroke-width="12" stroke-dasharray="0 390" stroke-linecap="round"/>
   <circle id="tick" cx="137" cy="75" r="5" fill="#fff"/></svg>
   <div class="v"><div class="big" id="rh">--<small>%</small></div><div class="mut sm" id="ctlLbl">humidity</div></div></div>
  <div class="stats">
   <div class="stat"><span class="mut">Temperature</span><b id="temp">--</b></div>
   <div class="stat"><span class="mut">Dew point</span><b id="dew">--</b></div>
   <div class="stat"><span class="mut">Water tank</span><b id="water">--</b></div>
   <div class="stat"><span class="mut">Uptime</span><b id="up">--</b></div>
  </div>
 </div>
 <div class="target"><span class="mut">Target</span>
  <button class="sq" onclick="nudge(-1)">&minus;</button>
  <input type="range" id="tgt" min="20" max="99" step="1" oninput="tgtInput()">
  <button class="sq" onclick="nudge(1)">+</button>
  <div class="tv" id="tgtv">--%</div></div>
</section>

<section class="card" id="wiz" style="display:none">
 <div class="hd"><h2>Room setup</h2><button class="s" onclick="wizClose(true)">Skip</button></div>
 <div class="mut sm">Enter the room and what you have. The ESP looks for connected sensors and suggests where everything goes.</div>
 <div class="grid" style="grid-template-columns:repeat(3,1fr)">
  <label>Width (m)<input type="number" id="wW" step="0.1" min="1" oninput="wizPlan()"></label>
  <label>Length (m)<input type="number" id="wL" step="0.1" min="1" oninput="wizPlan()"></label>
  <label>Height (m)<input type="number" id="wH" step="0.1" min="1" oninput="wizPlan()"></label>
 </div>
 <div class="grid">
  <label>Cooler (evaporator) on the<select id="wC" onchange="wizPlan()"><option value="0">top wall</option><option value="1">right wall</option><option value="2">bottom wall</option><option value="3">left wall</option></select></label>
  <label>Humidifiers<select id="wN" onchange="wizPlan()"><option>1</option><option>2</option></select></label>
  <label>Sensors<input type="number" id="wS" min="1" max="8" oninput="wizPlan()"></label>
  <label>&nbsp;<button onclick="wizDetect()" id="wDet">Detect sensors</button></label>
 </div>
 <div id="wFound" class="sm mut" style="margin-top:8px"></div>
 <div id="wPlan" class="sm" style="margin-top:8px"></div>
 <div class="row"><button class="pri" onclick="wizApply()">Apply layout</button><button onclick="wizClose()">Cancel</button></div>
</section>

<section class="card"><div class="hd"><h2>Humidifiers</h2></div><div id="chs"></div></section>

<section class="card">
 <div class="hd"><h2>Room</h2><div class="row" style="margin:0"><span class="mut sm" id="roomHint"></span><button class="s" onclick="wizOpen()">Auto layout</button><button class="s" id="editBtn" onclick="toggleEdit()">Edit layout</button></div></div>
 <svg class="room" id="top"></svg>
 <div class="mut sm" style="margin:8px 0 4px">Side view (height)</div>
 <svg class="room" id="side"></svg>
 <div id="roomEdit" class="edit" style="display:none">
  <div class="mut sm">Drag sensors and humidifiers on the top view, and up/down on the side view. Sizes in metres.</div>
  <div class="grid" style="grid-template-columns:repeat(3,1fr)">
   <label>Width<input type="number" id="rw" step="0.1" min="1" oninput="roomDims()"></label>
   <label>Length<input type="number" id="rl" step="0.1" min="1" oninput="roomDims()"></label>
   <label>Height<input type="number" id="rhh" step="0.1" min="1" oninput="roomDims()"></label>
  </div>
  <div class="row"><button class="pri" onclick="saveModel()">Save layout</button><button onclick="toggleEdit(false)">Cancel</button></div>
 </div>
</section>

<section class="card">
 <div class="hd"><h2>Sensors</h2><button class="s" onclick="editSensor(-1)">+ Add sensor</button></div>
 <table><thead><tr><th>Name</th><th class="hide-m">Type</th><th>Height</th><th>RH</th><th>Temp</th><th></th></tr></thead><tbody id="sens"></tbody></table>
 <div id="sensEdit" class="edit" style="display:none">
  <div class="grid">
   <label>Name<input type="text" id="sName" maxlength="24"></label>
   <label>Type<select id="sType" onchange="typeHint()">
    <option value="xymd02">RS485 XY-MD03 / XY-MD02 (FC4, reg 1-2)</option>
    <option value="renke">RS485 Renke RS-WS-N01 (FC3, reg 0-1)</option>
    <option value="modbus">RS485 custom registers (bus settings)</option>
    <option value="sht4x">SHT40/41/45 I2C</option>
    <option value="sht3x">SHT30/31/35 I2C</option>
    <option value="dht22">DHT22 on GPIO4</option>
    <option value="dht11">DHT11 on GPIO4 (testing only)</option></select></label>
   <label>Address<input type="number" id="sAddr" min="0" max="247"><span id="sAddrHint" class="mut sm"></span></label>
   <label id="sBusL">I2C bus<select id="sBus"><option value="0">1: SDA GPIO21, SCL GPIO22</option><option value="128">2: SDA GPIO19, SCL GPIO23</option></select></label>
   <label>Height above floor (m)<input type="number" id="sZ" step="0.1" min="0"></label>
   <label>Humidity offset (%)<input type="number" id="sHOff" step="0.5"></label>
   <label>Temperature offset (&deg;C)<input type="number" id="sTOff" step="0.1"></label>
   <label class="chk full"><input type="checkbox" id="sCtl"> Use for control (otherwise monitor only)</label>
  </div>
  <div class="row"><button class="pri" onclick="saveSensor()">Save sensor</button><button onclick="$('sensEdit').style.display='none'">Cancel</button><button class="danger" id="sDel" onclick="delSensor()">Remove</button></div>
 </div>
 <details style="margin-top:12px"><summary class="sm">RS485 / Modbus tools</summary>
  <div class="grid">
   <label>Bus baud rate<select id="bBaud"><option>2400</option><option>4800</option><option>9600</option><option>19200</option><option>38400</option><option>115200</option></select></label>
   <label>Custom: function code<select id="bFc"><option value="3">3 (holding)</option><option value="4">4 (input)</option></select></label>
   <label>Custom: RH register (x10)<input type="number" id="bHum" min="0"></label>
   <label>Custom: temp register (x10)<input type="number" id="bTemp" min="0"></label>
   <label>Custom: address register<input type="number" id="bAddr" min="0"></label>
  </div>
  <div class="row"><button onclick="saveBus()">Save bus settings</button></div>
  <div class="grid">
   <label>Sensor type<select id="mType"><option value="xymd02">XY-MD03 / MD02</option><option value="renke">Renke</option><option value="modbus">Custom</option></select></label>
   <label>Scan addresses<div style="display:flex;gap:6px"><input type="number" id="mFrom" value="1" min="1" max="247"><input type="number" id="mTo" value="16" min="1" max="247"></div></label>
  </div>
  <div class="row"><button onclick="mbScan()" id="mbScanBtn">Scan bus</button><span id="mbFound" class="sm"></span></div>
  <p class="mut sm">New sensors usually ship with address 1. Connect them <b>one at a time</b>, give each a unique address, then wire them all onto the bus. Some need a power cycle after changing. XY-MD03 default: 9600 baud. Renke default: 4800 baud. All sensors on one bus must use the same baud.</p>
  <div class="grid"><label>Current address<input type="number" id="mOld" value="1" min="1" max="247"></label><label>New address<input type="number" id="mNew" value="2" min="1" max="247"></label></div>
  <div class="row"><button onclick="mbSetAddr()">Change address</button></div>
 </details>
</section>

<section class="card">
 <div class="hd"><h2>History</h2>
  <div class="row" style="margin:0"><div class="seg" id="kind"><button data-k="rh" class="act">RH</button><button data-k="t">Temp</button></div>
  <div class="seg" id="ranges"><button data-m="60" class="act">1h</button><button data-m="360">6h</button><button data-m="1440">24h</button></div></div></div>
 <canvas id="chart"></canvas>
 <div class="sm mut" id="legend" style="margin-top:6px"></div>
</section>

<section class="card"><div class="hd"><h2>Events</h2></div><div class="log" id="log"></div></section>

<section class="card"><details><summary>Settings</summary>
 <div class="grid">
  <label>Control value from<select id="aggMode"><option value="avg">Average of control sensors</option><option value="min">Driest sensor</option><option value="max">Wettest sensor</option></select></label>
  <label>Hysteresis (%RH)<input type="number" id="hyst" min="0.5" max="15" step="0.5"></label>
  <label>Low humidity alarm (%)<input type="number" id="alertLow" min="0" max="100"></label>
  <label>High humidity alarm (%)<input type="number" id="alertHigh" min="0" max="100"></label>
  <label>Low temperature alarm (&deg;C)<input type="number" id="tempLow" step="0.1"></label>
  <label>High temperature alarm (&deg;C)<input type="number" id="tempHigh" step="0.1"></label>
  <label>Max RH spread between sensors (%, 0=off)<input type="number" id="spreadMax" min="0" max="50"></label>
  <label>Min on/off time (s)<input type="number" id="minCycleS" min="5" max="3600"></label>
  <label>Long-run alert (min, 0=off)<input type="number" id="maxRunMin" min="0" max="1440"></label>
  <label>No-rise check (min, 0=off)<input type="number" id="noRiseMin" min="0" max="240"></label>
  <label class="chk full"><input type="checkbox" id="noRiseStop"> Stop auto humidifiers if humidity doesn't rise (protects against an empty tank)</label>
  <label>Name humidifier 1<input type="text" id="name0" maxlength="24"></label>
  <label>Name humidifier 2<input type="text" id="name1" maxlength="24"></label>
  <label>H1 button presses after power on<input type="number" id="press0" min="0" max="5"></label>
  <label>H2 button presses after power on<input type="number" id="press1" min="0" max="5"></label>
  <div class="full mut sm" style="margin-top:-4px">Only if your atomizer board stays idle after power on and an optocoupler is wired across its button (GPIO32 / GPIO13). 0 = not used.</div>
  <label class="chk"><input type="checkbox" id="waterEnabled"> Water float switch on GPIO33</label>
  <label class="chk"><input type="checkbox" id="waterOkWhenLow"> Switch closed = water OK</label>
  <label class="chk full"><input type="checkbox" id="buzzer"> Beep on alerts (buzzer on GPIO25)</label>
  <label class="full">Push notification URL (e.g. https://ntfy.sh/your-secret-topic). Works when connected to home Wi-Fi.<input type="url" id="webhook" placeholder="https://ntfy.sh/..."></label>
  <label class="full">Change hotspot password (leave empty to keep)<input type="password" id="apPass" minlength="8" placeholder="min 8 characters"></label>
 </div>
 <div class="row"><button class="pri" onclick="saveSettings()">Save</button><button onclick="testAlert()">Send test alert</button></div>
 <div class="edit"><b class="sm">Web password</b> <span class="mut sm" id="pwState"></span>
  <div class="grid">
   <label id="pwCurL">Current password<input type="password" id="pwCur" autocomplete="current-password"></label>
   <label>New password<input type="password" id="pwNew" autocomplete="new-password" placeholder="min 4 characters"></label>
   <label>Repeat new password<input type="password" id="pwNew2" autocomplete="new-password"></label>
  </div>
  <div class="row"><button onclick="setPw()" id="pwSet">Set password</button><button class="danger" id="pwDel" onclick="setPw(true)">Remove password</button></div>
  <div class="mut sm">Protects this page and the API. Forgot it? Hold the BOOT button on the board for 10 s.</div>
 </div>
</details></section>

<section class="card"><details><summary>Wi-Fi</summary>
 <div style="margin-top:12px" id="wifiStatus" class="mut"></div>
 <div class="row"><button onclick="scan()" id="scanBtn">Scan networks</button><button class="danger" onclick="forget()">Forget home Wi-Fi</button></div>
 <div id="nets" style="margin-top:8px"></div>
 <div class="grid"><label>Network (SSID)<input type="text" id="ssid"></label><label>Password<input type="password" id="pass"></label></div>
 <div class="row"><button class="pri" onclick="connectWifi()">Connect</button></div>
 <p class="mut sm">The hotspot stays on. When the ESP joins your router the hotspot may switch radio channel, so your phone can drop off for a few seconds. On your home network open the IP shown above or <b id="mdns">humidesp.local</b>.</p>
</details></section>

<section class="card"><details><summary>System</summary>
 <div class="row"><button onclick="sys('reboot')">Reboot</button><button class="danger" onclick="sys('factory-reset')">Factory reset</button><button id="logoutBtn" onclick="logout()" style="display:none">Log out</button></div>
 <div class="row"><input type="file" id="fw" accept=".bin" style="flex:1;color:var(--mut)"><button onclick="upload()" id="fwBtn">Update firmware</button></div>
 <div class="mut sm">Upload firmware.bin built by PlatformIO. Humidifiers switch off during the update.</div>
 <div class="row"><span id="fwInfo" class="sm" style="flex:1"></span><button onclick="otaCheck()">Check for update</button><button class="pri" id="otaBtn" onclick="otaInstall()" style="display:none">Install</button></div>
 <label class="chk"><input type="checkbox" id="autoUpdate" onchange="api('/api/settings',{autoUpdate:this.checked}).catch(e=>toast(e.message))"> Install new releases from GitHub automatically (needs home Wi-Fi)</label>
</details></section>
<div id="toast"></div>
</main>
<div id="login"><form class="card" onsubmit="login(event)"><h1>Humid<span>ESP</span></h1>
 <label>Password<input type="password" id="lPass" autocomplete="current-password" required></label>
 <button class="pri">Log in</button><div class="mut sm" id="lMsg"></div></form></div>
<script>
const $=id=>document.getElementById(id);
const COLORS=['#4cc2ff','#3ccf8e','#ffb547','#c08bff','#ff7aa8','#7ae0e0','#e6e36a','#ff8c5a'];
const TYPE_LBL={dht11:'DHT11',dht22:'DHT22',sht3x:'SHT3x',sht4x:'SHT4x',xymd02:'XY-MD0x',renke:'Renke',modbus:'RS485 custom'};
let locked=false,st=null,hist=null,range=60,kind='rh',tgtTimer=null,editing=false,editLayout=false,model=null,editIdx=-1;
function toast(m){if(locked)return;const t=$('toast');t.textContent=m;t.style.opacity=1;clearTimeout(t._h);t._h=setTimeout(()=>t.style.opacity=0,3000)}
// Central 401 handling: any call without a valid session shows the login screen, polling pauses while it is up.
async function api(u,b){if(locked)throw new Error('login required');const r=await fetch(u,b===undefined?{}:{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)});
 if(r.status==401)showLogin();const j=await r.json().catch(()=>({}));if(!r.ok)throw new Error(j.error||r.status);return j}
function showLogin(){locked=true;$('login').style.display='flex';document.body.style.overflow='hidden';$('lPass').focus()}
async function login(e){e.preventDefault();const r=await fetch('/api/login',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({password:$('lPass').value})}).catch(()=>null);
 const j=r?await r.json().catch(()=>({})):{error:'offline'};if(!r||!r.ok)return $('lMsg').textContent=j.error||r.status;
 locked=false;$('login').style.display='none';document.body.style.overflow='';$('lPass').value=$('lMsg').textContent='';start()}
async function logout(){await api('/api/logout',{}).catch(()=>{});showLogin()}
async function setPw(remove){const n=$('pwNew').value;if(!remove&&(n.length<4||n!=$('pwNew2').value))return toast(n.length<4?'Password needs at least 4 characters':'Passwords do not match');
 if(remove&&!confirm('Remove the web password? Anyone on the network can then use this page.'))return;
 try{await api('/api/password',remove?{current:$('pwCur').value,remove:true}:{current:$('pwCur').value,password:n});['pwCur','pwNew','pwNew2'].forEach(k=>$(k).value='');toast(remove?'Password removed':'Password saved');refresh()}catch(e){toast(e.message)}}
// New firmware on the device reloads the page; new alerts beep and vibrate while the page is open.
let fwSeen,seenA,ac;addEventListener('pointerdown',()=>ac=ac||new AudioContext(),{once:true});
function ding(){navigator.vibrate&&navigator.vibrate([300,150,300]);if(!ac)return;const o=ac.createOscillator(),g=ac.createGain();o.frequency.value=880;g.gain.value=.2;o.connect(g).connect(ac.destination);o.start();o.stop(ac.currentTime+.5)}
function showOta(){const o=st.ota||{};$('fwInfo').innerHTML='Firmware <b>'+esc(st.fw)+'</b>'+(o.latest?' · latest '+esc(o.latest):'')+(o.error?' · <span class="mut">'+esc(o.error)+'</span>':'');$('otaBtn').style.display=o.available?'':'none';$('autoUpdate').checked=st.settings.autoUpdate}
async function otaCheck(){try{st=await api('/api/ota/check',{});showOta();toast(st.ota.error||(st.ota.available?'Update available':'Up to date'))}catch(e){toast(e.message)}}
async function otaInstall(){if(!confirm('Install '+st.ota.latest+'? Humidifiers stop for about a minute.'))return;try{await api('/api/ota/install',{});toast('Installing, the page reloads when done')}catch(e){toast(e.message)}}
function dur(s){if(s<60)return s+'s';if(s<3600)return Math.floor(s/60)+'m';if(s<86400)return Math.floor(s/3600)+'h '+Math.floor(s%3600/60)+'m';return Math.floor(s/86400)+'d '+Math.floor(s%86400/3600)+'h'}
function dew(t,h){const a=17.62,b=243.12,g=Math.log(h/100)+a*t/(b+t);return b*g/(a-g)}
function esc(s){return String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]))}
const clone=o=>JSON.parse(JSON.stringify(o));
// Colour for an RH value relative to the target: orange dry, green on target, blue wet.
function rhColor(rh,a=1){if(rh==null)return `rgba(120,130,140,${a})`;const s=st.settings,d=rh-s.target;
 const h=d<0?Math.max(25,140+d/10*115):Math.min(220,140+d/(Math.max(1,s.alertHigh-s.target))*80);return `hsla(${h},75%,55%,${a})`}

async function refresh(){
 try{st=await api('/api/state')}catch(e){$('net').textContent='offline';$('net').className='pill';return}
 const w=st.wifi,s=st.settings;$('net').textContent=w.staConnected?w.staSsid+' · '+w.staIp:'Hotspot only';$('net').className='pill'+(w.staConnected?' on':'');
 $('alerts').innerHTML=st.alerts.map(a=>`<div class="alert"><span>${esc(a.msg)}</span>${a.id=='norise'&&st.channels.some(c=>c.lockout)?'<button onclick="resume()">Resume</button>':''}</div>`).join('');
 const ok=st.rh!=null,rh=st.rh;$('rh').innerHTML=(ok?rh.toFixed(rh>=99.5?0:1).replace(/\.0$/,''):'--')+'<small>%</small>';
 $('ctlLbl').textContent=st.ctlN>1?({avg:'average',min:'driest',max:'wettest'}[s.aggMode])+' of '+st.ctlN:'humidity';
 $('arc').setAttribute('stroke-dasharray',(ok?rh/100*292:0)+' 390');
 const ang=s.target/100*270*Math.PI/180;$('tick').setAttribute('cx',75+62*Math.cos(ang));$('tick').setAttribute('cy',75+62*Math.sin(ang));
 $('arc').setAttribute('stroke',ok?rhColor(rh):'#44515c');
 $('temp').textContent=ok?st.temp.toFixed(1)+' °C':'--';$('dew').textContent=ok?dew(st.temp,rh).toFixed(1)+' °C':'--';
 $('water').textContent=st.water;$('up').textContent=dur(st.uptime);
 if(fwSeen&&st.fw!=fwSeen)return location.reload();fwSeen=st.fw;showOta();
 const ids=st.alerts.map(a=>a.id);if(seenA&&ids.some(i=>!seenA.includes(i)))ding();seenA=ids;document.title=(ids.length?'⚠ '+ids.length+' · ':'')+'HumidESP';
 if(!tgtTimer){$('tgt').value=s.target;$('tgtv').textContent=s.target+'%'}
 $('chs').innerHTML=st.channels.map((c,i)=>`<div class="ch"><div><span class="dot ${c.lockout?'lock':c.on?'on':''}"></span><b>${esc(c.name)}</b>
  <div class="mut sm" style="margin-left:18px">${c.lockout?'Safety stop':c.on?'Running':'Idle'} for ${dur(c.since)}</div></div>
  <div class="seg">${['auto','on','off'].map(m=>`<button class="${c.mode==m?'act':''}" onclick="setMode(${i},'${m}')">${m[0].toUpperCase()+m.slice(1)}</button>`).join('')}</div></div>`).join('');
 $('sens').innerHTML=st.sensors.map((x,i)=>`<tr><td><span class="sw" style="background:${COLORS[i%8]}"></span>${esc(x.name)}${x.control?'':' <span class="mut sm">(monitor)</span>'}</td>
  <td class="hide-m mut">${TYPE_LBL[x.type]}${x.type.startsWith('sht')?' 0x'+(x.addr&127).toString(16)+(x.addr&128?' bus 2':''):x.addr?' #'+x.addr:''}</td><td>${x.z.toFixed(1)} m</td>
  <td style="color:${x.ok?rhColor(x.rh):'var(--bad)'}">${x.ok?x.rh.toFixed(1)+'%':'offline'}</td><td>${x.ok?x.t.toFixed(1)+'°':''}</td>
  <td><button class="s" onclick="editSensor(${i})">Edit</button></td></tr>`).join('')||'<tr><td colspan="6" class="mut">No sensors yet</td></tr>';
 if(!editing)fillSettings();
 if(!editLayout&&!wiz){model={sensors:clone(st.sensors),room:clone(st.room),channels:st.channels.map(c=>({x:c.x,y:c.y,z:c.z}))};drawRoom()}
 if(!s.setupDone&&!wizSeen){wizSeen=1;wizOpen()}
 $('wifiStatus').innerHTML=(w.staConnected?`Connected to <b>${esc(w.staSsid)}</b> · IP <b>${w.staIp}</b> · ${w.rssi} dBm`:w.staSsid?`Trying to connect to <b>${esc(w.staSsid)}</b>...`:'Not connected to a home network.')+
  `<br>Hotspot <b>${esc(w.apSsid)}</b> at ${w.apIp} · ${w.apClients} client(s)`;$('mdns').textContent=w.mdns;
 const au=st.auth&&st.auth.enabled;$('pwState').textContent=au?'(on)':'(off)';$('pwSet').textContent=au?'Change password':'Set password';
 for(const k of['pwCurL','pwDel','logoutBtn'])$(k).style.display=au?'':'none';
}
const NUMS=['hyst','alertLow','alertHigh','tempLow','tempHigh','spreadMax','minCycleS','maxRunMin','noRiseMin'],BOOLS=['noRiseStop','waterEnabled','waterOkWhenLow','buzzer'];
function fillSettings(){const s=st.settings;for(const k of NUMS)$(k).value=s[k];for(const k of BOOLS)$(k).checked=s[k];$('webhook').value=s.webhook;$('aggMode').value=s.aggMode;
 st.channels.forEach((c,i)=>{if($('name'+i))$('name'+i).value=c.name;if($('press'+i))$('press'+i).value=c.presses});const b=st.bus;$('bBaud').value=b.baud;$('bFc').value=b.fc;$('bHum').value=b.humReg;$('bTemp').value=b.tempReg;$('bAddr').value=b.addrReg}
document.querySelectorAll('details input,details select').forEach(e=>{e.addEventListener('focus',()=>editing=true);e.addEventListener('input',()=>editing=true)});

// ---------- Room view ----------
function idw(px,py,pts){let n=0,d=0;for(const p of pts){const dd=(p.x-px)**2+(p.y-py)**2;if(dd<1e-6)return p.v;const w=1/dd;n+=w*p.v;d+=w}return n/d}
function drawRoom(){if(!st||!model)return;const R=model.room,S=100,W=R.w*S,Lg=R.l*S,H=R.h*S,fs=Math.max(R.w,R.l)*S/19,r=fs*0.9;
 const live=i=>st.sensors[i]&&st.sensors[i].ok&&!editLayout?st.sensors[i]:(st.sensors[i]&&st.sensors[i].ok?st.sensors[i]:null);
 const pts=model.sensors.map((x,i)=>({x:x.x*W,y:x.y*Lg,v:live(i)?live(i).rh:null})).filter(p=>p.v!=null);
 let g='';if(pts.length){const nx=24,ny=Math.max(4,Math.round(24*Lg/W)),cw=W/nx,chh=Lg/ny;
  for(let i=0;i<nx;i++)for(let j=0;j<ny;j++)g+=`<rect x="${i*cw}" y="${j*chh}" width="${cw+.5}" height="${chh+.5}" fill="${rhColor(idw((i+.5)*cw,(j+.5)*chh,pts),.28)}"/>`}
 let top=`<rect x="0" y="0" width="${W}" height="${Lg}" fill="none" stroke="#3a4752" stroke-width="${fs/5}"/>${g}`;
 for(let m=1;m<R.w;m++)top+=`<line x1="${m*S}" y1="0" x2="${m*S}" y2="${Lg}" stroke="#ffffff10"/>`;for(let m=1;m<R.l;m++)top+=`<line x1="0" y1="${m*S}" x2="${W}" y2="${m*S}" stroke="#ffffff10"/>`;
 const cw=R.cooler|0,cl=[[0,0,W,0],[W,0,W,Lg],[0,Lg,W,Lg],[0,0,0,Lg]][cw];
 top+=`<line x1="${cl[0]}" y1="${cl[1]}" x2="${cl[2]}" y2="${cl[3]}" stroke="#7ae0e0" stroke-width="${fs/2}"/><text x="${(cl[0]+cl[2])/2}" y="${(cl[1]+cl[3])/2+(cw==2?-fs*.5:fs*1.2)}" font-size="${fs*.75}" fill="#7ae0e0" text-anchor="middle">cooler</text>`;
 top+=`<text x="${W-fs*.3}" y="${Lg-fs*.4}" font-size="${fs*.8}" fill="#8b9aa6" text-anchor="end">${R.w} × ${R.l} m</text>`;
 model.channels.forEach((c,i)=>{const on=st.channels[i]&&st.channels[i].on;top+=`<g class="${editLayout?'drag':''}" data-kind="ch" data-i="${i}" transform="translate(${c.x*W},${c.y*Lg})">
  <rect x="${-r}" y="${-r}" width="${2*r}" height="${2*r}" rx="${r/3}" fill="${on?'#4cc2ff':'#22303a'}" stroke="#4cc2ff" stroke-width="${fs/8}"/>
  <text y="${fs*.35}" font-size="${fs}" text-anchor="middle" fill="${on?'#04202f':'#4cc2ff'}">H${i+1}</text></g>`});
 model.sensors.forEach((x,i)=>{const lv=live(i);top+=`<g class="${editLayout?'drag':''}" data-kind="s" data-i="${i}" transform="translate(${x.x*W},${x.y*Lg})">
  <circle r="${r}" fill="${lv?rhColor(lv.rh):'#44515c'}" stroke="${COLORS[i%8]}" stroke-width="${fs/5}"/>
  <text y="${-r-fs*.35}" font-size="${fs*.85}" text-anchor="middle" fill="#e6edf2">${esc(x.name)}</text>
  <text y="${r+fs*1.05}" font-size="${fs*.8}" text-anchor="middle" fill="#8b9aa6">${lv?lv.rh.toFixed(1)+'% '+lv.t.toFixed(1)+'°':'offline'} · ${x.z.toFixed(1)}m</text></g>`});
 const ghost=(x,y,i)=>`<g transform="translate(${x},${y})"><circle r="${r}" fill="none" stroke="#8b9aa6" stroke-dasharray="${fs/3}" stroke-width="${fs/8}"/><text y="${fs*.35}" font-size="${fs*.8}" text-anchor="middle" fill="#8b9aa6">S${i}</text></g>`,pn=model.sensors.length+1;
 (model.plan||[]).forEach((o,i)=>top+=ghost(o.x*W,o.y*Lg,pn+i));
 const pad=fs*3;$('top').setAttribute('viewBox',`${-pad} ${-pad} ${W+2*pad} ${Lg+2*pad}`);$('top').innerHTML=top;
 let side=`<rect x="0" y="0" width="${W}" height="${H}" fill="none" stroke="#3a4752" stroke-width="${fs/5}"/><line x1="0" y1="${H}" x2="${W}" y2="${H}" stroke="#8b9aa6" stroke-width="${fs/4}"/>`;
 for(let m=1;m<R.h;m++)side+=`<line x1="0" y1="${H-m*S}" x2="${W}" y2="${H-m*S}" stroke="#ffffff10"/><text x="${-fs*.3}" y="${H-m*S+fs*.3}" font-size="${fs*.7}" fill="#8b9aa6" text-anchor="end">${m}m</text>`;
 model.channels.forEach((c,i)=>{side+=`<g class="${editLayout?'drag':''}" data-kind="cz" data-i="${i}" transform="translate(${c.x*W},${H-(c.z??2)*S})"><rect x="${-r*.7}" y="${-r*.7}" width="${1.4*r}" height="${1.4*r}" rx="${r/4}" fill="#22303a" stroke="#4cc2ff" stroke-width="${fs/8}"/><text x="${r}" y="${fs*.3}" font-size="${fs*.75}" fill="#4cc2ff">H${i+1}</text></g>`});
 model.sensors.forEach((x,i)=>{const lv=live(i);side+=`<g class="${editLayout?'drag':''}" data-kind="z" data-i="${i}" transform="translate(${x.x*W},${H-x.z*S})">
  <circle r="${r*.75}" fill="${lv?rhColor(lv.rh):'#44515c'}" stroke="${COLORS[i%8]}" stroke-width="${fs/5}"/>
  <text x="${r}" y="${fs*.3}" font-size="${fs*.75}" fill="#e6edf2">${esc(x.name)}</text></g>`});
 (model.plan||[]).forEach((o,i)=>side+=ghost(o.x*W,H-o.z*S,pn+i));
 $('side').setAttribute('viewBox',`${-pad} ${-fs} ${W+2*pad} ${H+fs*2}`);$('side').innerHTML=side;
 $('roomHint').textContent=editLayout?'editing':'';}
function svgPt(svg,e){const p=svg.createSVGPoint();p.x=e.clientX;p.y=e.clientY;return p.matrixTransform(svg.getScreenCTM().inverse())}
let drag=null;
for(const id of['top','side']){const svg=$(id);
 svg.addEventListener('pointerdown',e=>{if(!editLayout)return;const g=e.target.closest('g[data-kind]');if(!g)return;drag={svg,g,kind:g.dataset.kind,i:+g.dataset.i};svg.setPointerCapture(e.pointerId);e.preventDefault()});
 svg.addEventListener('pointermove',e=>{if(!drag||drag.svg!==svg)return;const R=model.room,p=svgPt(svg,e),W=R.w*100,Lg=R.l*100,H=R.h*100;
  const cx=Math.min(W,Math.max(0,p.x));
  if(drag.kind=='z'||drag.kind=='cz'){const z=Math.min(H,Math.max(0,H-p.y)),o=drag.kind=='z'?model.sensors[drag.i]:model.channels[drag.i];o.x=cx/W;o.z=Math.round(z)/100;drag.g.setAttribute('transform',`translate(${cx},${H-z})`)}
  else{const cy=Math.min(Lg,Math.max(0,p.y)),o=drag.kind=='s'?model.sensors[drag.i]:model.channels[drag.i];o.x=cx/W;o.y=cy/Lg;drag.g.setAttribute('transform',`translate(${cx},${cy})`)}});
 svg.addEventListener('pointerup',()=>{if(drag){drag=null;drawRoom()}});}
// Layout wizard. Rules for a cold store: humidifiers high, 0.5 m in front of the cooler so its fan carries the mist;
// sensors in the far 60% of the room (out of the mist and the cold blast) at mixed heights: middle of the stack, low, high.
let wiz=0,wizSeen=0,found=null;
function wizOpen(){if(editLayout)toggleEdit(false);const R=st.room;$('wW').value=R.w;$('wL').value=R.l;$('wH').value=R.h;$('wC').value=R.cooler|0;
 $('wN').value=Math.max(1,st.channels.filter(c=>c.mode!='off').length);$('wS').value=Math.max(1,st.sensors.length);found=null;$('wFound').textContent='';wiz=1;$('wiz').style.display='';$('wiz').scrollIntoView({behavior:'smooth'});wizPlan()}
function wizClose(skip){wiz=0;$('wiz').style.display='none';if(skip)api('/api/settings',{setupDone:true}).catch(()=>{});refresh()}
async function wizDetect(){$('wDet').textContent='Looking...';try{const r=await api('/api/detect',{});found=r.found;
 $('wFound').innerHTML=found.length?'Found: '+found.map(f=>`<b>${TYPE_LBL[f.type]}${f.type.startsWith('sht')?(f.addr&128?' bus 2':''):f.addr?' #'+f.addr:''}</b> ${f.rh.toFixed(0)}%`).join(', '):'No sensors answered. Check wiring, or set the count by hand.';
 if(found.length)$('wS').value=found.length;wizPlan()}catch(e){toast(e.message)}$('wDet').textContent='Detect sensors'}
// Detected sensors keep their existing name, offsets and settings when they were already configured.
function wizSensors(){if(!found||!found.length)return clone(st.sensors);
 return found.map((f,i)=>st.sensors.find(x=>x.type==f.type&&x.addr==f.addr)||{name:'Sensor '+(i+1),type:f.type,addr:f.addr,control:true,hOff:0,tOff:0})}
function wizPlan(){if(!wiz)return;const w=Math.max(1,+$('wW').value||1),l=Math.max(1,+$('wL').value||1),h=Math.max(1,+$('wH').value||1),cw=+$('wC').value,nh=+$('wN').value;
 const sens=wizSensors(),ns=Math.max(1,Math.min(8,+$('wS').value||1),sens.length),depth=cw%2?w:l,r1=v=>Math.round(v*10)/10;
 const P=(u,v)=>cw==0?[u,v]:cw==1?[1-v,u]:cw==2?[u,1-v]:[v,u];
 const chs=st.channels.map((c,i)=>{const[x,y]=P((i+.5)/nh,Math.min(.15,.5/depth));return{x,y,z:r1(Math.max(1,h-.6))}});
 const Z=[r1(Math.min(1.5,h/2)),.4,r1(h-.4)],cols=ns<3?ns:Math.ceil(ns/2),rows=Math.ceil(ns/cols);
 const spots=[...Array(ns)].map((_,i)=>{let u,v;if(ns==1){u=.5;v=.65}else if(ns==2){u=i?.75:.25;v=i?.85:.5}
  else{const rr=Math.floor(i/cols),c=i%cols;u=.12+.76*(c+.5+(rr%2?.25:0))/cols;v=.4+.52*(rr+.5)/rows}const[x,y]=P(Math.min(.9,u),v);return{x,y,z:Z[i%3]}});
 model={room:{w,l,h,cooler:cw},channels:chs,sensors:sens.map((x,i)=>Object.assign(clone(x),spots[i])),plan:spots.slice(sens.length)};drawRoom();
 const m=o=>`${r1(o.x*w)} m from the left, ${r1(o.y*l)} m from the top, <b>${o.z} m high</b>`,Zw=['middle of the stack','near the floor','near the ceiling'];
 $('wPlan').innerHTML='<b>Suggested layout</b> (drawn below in Room):<br>'+chs.slice(0,nh).map((c,i)=>`H${i+1}: ${m(c)}. Aim the mist into the cooler's air stream, not at the crates.`).join('<br>')+
  '<br>'+spots.map((o,i)=>`${sens[i]?esc(sens[i].name):'S'+(i+1)+' (dashed, not connected yet)'}: ${m(o)} (${Zw[i%3]}).`).join('<br>')+
  '<br><span class="mut">Keep sensors at least 1.5 m from the mist and out of the cooler\'s cold blast. Mount them on a post or a crate, not on the outer wall or the door.</span>'}
async function wizApply(){wizPlan();try{await api('/api/sensors',{sensors:model.sensors,room:model.room,channels:model.channels});
 const nh=+$('wN').value;for(let i=nh;i<st.channels.length;i++)await api('/api/channel',{i,mode:'off'});
 await api('/api/settings',{setupDone:true});toast('Layout saved');wizClose();loadHist()}catch(e){toast(e.message)}}
function toggleEdit(on){editLayout=on===undefined?!editLayout:on;$('roomEdit').style.display=editLayout?'':'none';$('editBtn').textContent=editLayout?'Editing...':'Edit layout';
 if(editLayout){$('rw').value=model.room.w;$('rl').value=model.room.l;$('rhh').value=model.room.h}else refresh();drawRoom()}
function roomDims(){const w=+$('rw').value,l=+$('rl').value,h=+$('rhh').value;if(w>=1)model.room.w=w;if(l>=1)model.room.l=l;if(h>=1)model.room.h=h;drawRoom()}
async function saveModel(){try{await api('/api/sensors',model);toast('Saved');editLayout=false;toggleEdit(false)}catch(e){toast(e.message)}}

// ---------- Sensor editor ----------
function typeHint(){const t=$('sType').value,i2c=t.startsWith('sht');$('sBusL').style.display=i2c?'':'none';if(i2c&&!(+$('sAddr').value>0))$('sAddr').value=68;$('sAddrHint').textContent={sht3x:'0x44 = 68, 0x45 = 69',sht4x:'0x44 = 68 (SHT40-AD1B), 0x45 = 69 (BD1B)',dht11:'not used',dht22:'not used'}[t]||'Modbus slave address 1-247'}
function editSensor(i){if(editLayout)toggleEdit(false);if(i<0&&st.sensors.length>=8)return toast('Maximum 8 sensors');editIdx=i;const x=i<0?{name:'Sensor '+(st.sensors.length+1),type:'xymd02',addr:st.sensors.filter(s=>s.type=='xymd02').length+1,z:1.5,control:true,hOff:0,tOff:0}:st.sensors[i];
 $('sName').value=x.name;$('sType').value=x.type;$('sAddr').value=x.addr&127;$('sBus').value=x.type.startsWith('sht')?x.addr&128:0;$('sZ').value=x.z;$('sHOff').value=x.hOff;$('sTOff').value=x.tOff;$('sCtl').checked=x.control;$('sDel').style.display=i<0?'none':'';typeHint();
 $('sensEdit').style.display='';$('sensEdit').scrollIntoView({behavior:'smooth',block:'nearest'})}
async function postSensors(list){await api('/api/sensors',{sensors:list});$('sensEdit').style.display='none';await refresh();loadHist()}
async function saveSensor(){const list=clone(st.sensors),o=editIdx<0?{x:.5,y:.5}:list[editIdx];
 Object.assign(o,{name:$('sName').value.trim()||'Sensor',type:$('sType').value,addr:(+$('sAddr').value||0)|($('sType').value.startsWith('sht')?+$('sBus').value:0),z:+$('sZ').value||0,hOff:+$('sHOff').value||0,tOff:+$('sTOff').value||0,control:$('sCtl').checked});
 if(editIdx<0)list.push(o);try{await postSensors(list);toast('Sensor saved')}catch(e){toast(e.message)}}
async function delSensor(){if(!confirm('Remove this sensor?'))return;const list=clone(st.sensors);list.splice(editIdx,1);try{await postSensors(list)}catch(e){toast(e.message)}}
async function saveBus(){try{await api('/api/bus',{baud:+$('bBaud').value,fc:+$('bFc').value,humReg:+$('bHum').value,tempReg:+$('bTemp').value,addrReg:+$('bAddr').value});editing=false;toast('Bus saved')}catch(e){toast(e.message)}}
async function mbScan(){$('mbScanBtn').textContent='Scanning...';try{const r=await api('/api/modbus/scan',{type:$('mType').value,from:+$('mFrom').value,to:+$('mTo').value});
 $('mbFound').innerHTML=r.found.length?'Found: '+r.found.map(f=>`<b>#${f.addr}</b> ${f.rh.toFixed(1)}% ${f.t.toFixed(1)}°`).join(', '):'<span class="mut">Nothing answered</span>'}catch(e){toast(e.message)}$('mbScanBtn').textContent='Scan bus'}
async function mbSetAddr(){try{await api('/api/modbus/setaddr',{type:$('mType').value,from:+$('mOld').value,to:+$('mNew').value});toast('Address changed. Power cycle the sensor if it does not answer on the new one.')}catch(e){toast(e.message)}}

// ---------- Controls ----------
function tgtInput(){$('tgtv').textContent=$('tgt').value+'%';clearTimeout(tgtTimer);tgtTimer=setTimeout(async()=>{await api('/api/settings',{target:+$('tgt').value}).catch(e=>toast(e.message));tgtTimer=null;refresh().then(drawChart)},500)}
function nudge(d){$('tgt').value=+$('tgt').value+d;tgtInput()}
async function setMode(i,m){try{await api('/api/channel',{i,mode:m});refresh()}catch(e){toast(e.message)}}
async function resume(){await api('/api/resume',{});toast('Resumed');refresh()}
async function saveSettings(){const b={};for(const k of NUMS)b[k]=+$(k).value;for(const k of BOOLS)b[k]=$(k).checked;b.webhook=$('webhook').value.trim();b.aggMode=$('aggMode').value;
 b.names=st.channels.map((c,i)=>$('name'+i)?$('name'+i).value:c.name);b.presses=st.channels.map((c,i)=>$('press'+i)?+$('press'+i).value:c.presses);if($('apPass').value)b.apPass=$('apPass').value;
 try{await api('/api/settings',b);editing=false;$('apPass').value='';toast(b.apPass?'Saved. Reconnect with the new hotspot password.':'Saved');refresh()}catch(e){toast(e.message)}}
async function testAlert(){try{await api('/api/test-alert',{});toast('Test alert queued')}catch(e){toast(e.message)}}
async function scan(){$('scanBtn').textContent='Scanning...';await api('/api/scan?start=1');
 for(let i=0;i<30;i++){await new Promise(r=>setTimeout(r,700));const r=await api('/api/scan');if(!r.scanning){
  r.networks.sort((a,b)=>b.rssi-a.rssi);$('nets').innerHTML=r.networks.map(n=>`<div class="net" data-s="${esc(n.ssid)}"><span>${esc(n.ssid)}${n.open?'':' &#128274;'}</span><span class="mut">${n.rssi} dBm</span></div>`).join('')||'<div class="mut">No networks found</div>';
  document.querySelectorAll('.net').forEach(e=>e.onclick=()=>{document.querySelectorAll('.net').forEach(x=>x.classList.remove('sel'));e.classList.add('sel');$('ssid').value=e.dataset.s;$('pass').focus()});break}}
 $('scanBtn').textContent='Scan networks'}
async function connectWifi(){try{await api('/api/wifi',{ssid:$('ssid').value.trim(),pass:$('pass').value});toast('Connecting... watch the status above');$('pass').value=''}catch(e){toast(e.message)}}
async function forget(){if(!confirm('Disconnect and forget the home Wi-Fi?'))return;await api('/api/wifi/forget',{});toast('Forgotten');refresh()}
async function sys(a){if(!confirm(a=='reboot'?'Reboot the controller?':'Erase all settings, sensors and Wi-Fi?'))return;await api('/api/'+a,{});toast('Restarting...')}

async function upload(){const f=$('fw').files[0];if(!f)return toast('Pick firmware.bin first');const fd=new FormData();fd.append('fw',f);$('fwBtn').textContent='Uploading...';
 try{const r=await fetch('/api/update',{method:'POST',body:fd});if(r.status==401)showLogin();toast(r.ok?'Updated, restarting...':'Update failed')}catch(e){toast('Upload error')}$('fwBtn').textContent='Update firmware'}

// ---------- Log + chart ----------
async function loadLog(){try{const j=await api('/api/log');const now=Date.now();
 $('log').innerHTML=j.log.map(e=>`<div><span>${new Date(now-(j.now-e.t)*1000).toLocaleTimeString([],{hour:'2-digit',minute:'2-digit'})}</span>${esc(e.msg)}</div>`).join('')||'<div class="mut">No events yet</div>'}catch(e){}}
async function loadHist(){try{hist=await api('/api/history?n=1440');drawChart()}catch(e){}}
function drawChart(){if(!st)return;const c=$('chart'),dpr=devicePixelRatio||1,W=c.clientWidth,H=c.clientHeight;c.width=W*dpr;c.height=H*dpr;const x=c.getContext('2d');x.scale(dpr,dpr);
 const L=34,R=8,T=8,B=20,pw=W-L-R,ph=H-T-B;x.font='11px system-ui';x.strokeStyle='#26313a';
 const isRh=kind=='rh',n=1+st.sensors.length,series=[];
 for(let k=0;k<n;k++){let a=hist&&hist[kind]&&hist[kind][k]?hist[kind][k].slice(-range):[];const cur=k==0?(isRh?st.rh:st.temp):(st.sensors[k-1]&&st.sensors[k-1].ok?(isRh?st.sensors[k-1].rh:st.sensors[k-1].t):null);series.push([...a,cur])}
 const on=[...(hist?hist.on.slice(-range):[]),st.channels.some(c=>c.on)?1:0];
 const vals=series.flat().filter(v=>v!=null),ref=isRh?[st.settings.target]:[st.settings.tempLow,st.settings.tempHigh];
 const step=isRh?5:1;let lo=Math.min(...ref,...vals)-step,hi=Math.max(...ref,...vals)+step;lo=Math.floor(lo/step)*step;hi=Math.ceil(hi/step)*step;if(isRh){lo=Math.max(0,lo);hi=Math.min(100,hi)}if(hi-lo<2*step)hi=lo+2*step;
 const len=series[0].length,X=i=>L+pw*(1-(len-1-i)/range),Y=v=>T+ph*(1-(v-lo)/(hi-lo));
 const gs=(hi-lo)/step>8?step*2:step;x.fillStyle='#8b9aa6';for(let v=lo;v<=hi+1e-9;v+=gs){x.beginPath();x.moveTo(L,Y(v));x.lineTo(W-R,Y(v));x.stroke();x.fillText(+v.toFixed(1),4,Y(v)+4)}
 (range==60?[60,30,0]:range==360?[360,240,120,0]:[1440,1080,720,360,0]).forEach(m=>{const px=L+pw*(1-m/range);x.fillText(m?'-'+(m>=120?m/60+'h':m+'m'):'now',Math.min(px-8,W-R-22),H-4)});
 x.fillStyle='#1f3a4a';const bw=pw/range+.5;on.forEach((o,i)=>{if(o)x.fillRect(X(i)-bw/2,T,bw,ph)});
 x.setLineDash([5,4]);x.strokeStyle='#ffb547';x.lineWidth=1;ref.forEach(v=>{x.beginPath();x.moveTo(L,Y(v));x.lineTo(W-R,Y(v));x.stroke()});x.setLineDash([]);
 series.forEach((s,k)=>{if(k>0&&n==2)return;x.strokeStyle=k?COLORS[(k-1)%8]:'#ffffff';x.lineWidth=k?1.3:2.4;x.beginPath();let pen=false;s.forEach((v,i)=>{if(v==null){pen=false;return}pen?x.lineTo(X(i),Y(v)):x.moveTo(X(i),Y(v));pen=true});x.stroke()});
 if(!vals.length){x.fillStyle='#8b9aa6';x.fillText('Collecting data...',L+pw/2-45,T+ph/2)}
 $('legend').innerHTML=`<span style="color:#fff">▬</span> control value `+(n>2?st.sensors.map((s,i)=>`&nbsp; <span style="color:${COLORS[i%8]}">▬</span> ${esc(s.name)}`).join(''):'')+
  ` &nbsp; <span style="color:var(--warn)">- -</span> ${isRh?'target':'alarm limits'} &nbsp; <span style="color:#2b5a75">█</span> humidifier on`}
document.querySelectorAll('#ranges button').forEach(b=>b.onclick=()=>{document.querySelectorAll('#ranges button').forEach(x=>x.classList.remove('act'));b.classList.add('act');range=+b.dataset.m;drawChart()});
document.querySelectorAll('#kind button').forEach(b=>b.onclick=()=>{document.querySelectorAll('#kind button').forEach(x=>x.classList.remove('act'));b.classList.add('act');kind=b.dataset.k;drawChart()});
addEventListener('resize',drawChart);
function start(){refresh().then(()=>{loadHist();loadLog()})}
fetch('/api/auth').then(r=>r.json()).then(a=>a.loggedIn?start():showLogin()).catch(start);
setInterval(()=>locked||refresh().then(drawChart),3000);setInterval(()=>locked||loadHist(),60000);setInterval(()=>locked||loadLog(),5000);
</script></body></html>)HTML";
