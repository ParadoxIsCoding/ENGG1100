#pragma once

const char kControlPage[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
  <meta name="theme-color" content="#07111f">
  <title>Station Keeper</title>
  <style>
    :root { color-scheme: dark; font-family: system-ui,-apple-system,sans-serif; }
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
    html,body { overscroll-behavior: none; }
    body { margin: 0; min-height: 100dvh; background: #07111f; color: #f7fbff; touch-action: manipulation; }
    main { width: min(100%, 30rem); margin: auto; padding: max(.6rem,env(safe-area-inset-top)) .75rem calc(6.7rem + env(safe-area-inset-bottom)); }
    header { display: flex; align-items: center; justify-content: space-between; gap: .55rem; min-height: 2rem; }
    .sr-only { position: absolute; width: 1px; height: 1px; padding: 0; margin: -1px; overflow: hidden; clip: rect(0,0,0,0); white-space: nowrap; border: 0; }
    .status-line { min-width: 0; display: flex; align-items: center; gap: .5rem; }
    .pill { border: 1px solid #3d536c; border-radius: 999px; padding: .35rem .65rem; font-size: .8rem; }
    .online { color: #70f0b0; border-color: #287f59; }
    .offline { color: #ffadad; border-color: #963e45; }
    .panel { margin-top: .65rem; padding: .7rem; background: #101d2d; border: 1px solid #263b52; border-radius: .9rem; }
    #mode { flex: none; color: #9dc7f1; font-size: .67rem; font-weight: 750; letter-spacing: .04em; }
    #state { min-width: 0; margin: 0; overflow: hidden; font-size: .82rem; font-weight: 800; text-overflow: ellipsis; white-space: nowrap; }
    .attitude-panel { display: grid; grid-template-columns: 7rem 1fr; align-items: center; gap: .7rem; }
    .horizon { position: relative; width: 7rem; aspect-ratio: 1; overflow: hidden; border: 3px solid #60758c; border-radius: 50%; background: #132131; box-shadow: inset 0 0 1.2rem #000b,0 .35rem .8rem #03091288; }
    .horizon-world { position: absolute; left: 50%; top: 50%; width: 320%; height: 320%; background: linear-gradient(to bottom,#2378b7 0 49.7%,#f3f4f4 49.7% 50.3%,#8b542e 50.3% 100%); transform: translate(-50%,-50%); transition: transform 45ms linear; will-change: transform; }
    .pitch-line { position: absolute; left: 50%; width: 2rem; height: 1px; background: #fff; box-shadow: 0 1px #0008; transform: translateX(-50%); }
    .pitch-line.long { width: 3.2rem; }
    .p-up-10 { top: calc(50% - 20px); } .p-up-5 { top: calc(50% - 10px); }
    .p-down-5 { top: calc(50% + 10px); } .p-down-10 { top: calc(50% + 20px); }
    .bank-marks { position: absolute; inset: .35rem; border-top: 2px solid #fff; border-radius: 50%; pointer-events: none; }
    .bank-pointer { position: absolute; top: .3rem; left: 50%; width: 0; height: 0; border-left: .34rem solid transparent; border-right: .34rem solid transparent; border-top: .55rem solid #fff; transform: translateX(-50%); }
    .aircraft { position: absolute; left: 50%; top: 50%; width: 58%; height: 1.7rem; transform: translate(-50%,-50%); pointer-events: none; }
    .aircraft::before,.aircraft::after { content: ''; position: absolute; top: .72rem; width: 38%; height: .22rem; border: 1px solid #171717; background: #ffd43b; }
    .aircraft::before { left: 0; transform: rotate(8deg); transform-origin: right; }
    .aircraft::after { right: 0; transform: rotate(-8deg); transform-origin: left; }
    .aircraft-centre { position: absolute; left: 50%; top: 50%; width: .65rem; aspect-ratio: 1; border: .18rem solid #171717; border-radius: 50%; background: #ffd43b; transform: translate(-50%,-50%); }
    .attitude-readings { display: grid; width: 100%; grid-template-columns: 1fr 1fr; align-items: center; gap: .45rem; }
    .attitude-value { padding: .5rem .35rem; border: 1px solid #31475f; border-radius: .65rem; background: #0c1928; text-align: center; }
    .attitude-value span { display: block; color: #9db0c4; font-size: .68rem; font-weight: 700; letter-spacing: .08em; }
    .attitude-value strong { display: block; margin-top: .08rem; font-size: 1.05rem; font-variant-numeric: tabular-nums; }
    #tilt-status { justify-self: stretch; padding: .35rem .4rem; border: 1px solid currentColor; border-radius: .6rem; font-size: .69rem; font-weight: 800; letter-spacing: .05em; text-align: center; }
    .tilt-green { color: #70f0b0; } .tilt-amber { color: #ffc85c; } .tilt-red { color: #ff7a86; }
    .level { min-height: 2.3rem; padding: 0 .4rem; white-space: nowrap; }
    .tabs { display: grid; grid-template-columns: 1fr 1fr; gap: .45rem; margin-top: .65rem; padding: .25rem; border: 1px solid #263b52; border-radius: .85rem; background: #0c1928; }
    .tab { min-height: 2.7rem; border: 0; border-radius: .65rem; background: transparent; color: #9db0c4; }
    .tab.selected { background: #2073b8; color: #fff; }
    .control-view { margin-top: .65rem; }
    .controls { display: flex; flex-direction: column; align-items: center; gap: .6rem; }
    button { min-height: 3.65rem; border: 1px solid #49627f; border-radius: .8rem; background: #172b42; color: inherit; font: inherit; font-weight: 750; touch-action: none; user-select: none; }
    button:active,.active { background: #2073b8; transform: scale(.98); }
    button:focus-visible { outline: 3px solid #75bfff; outline-offset: 2px; }
    .joystick { position: relative; width: min(58vw,14.5rem); aspect-ratio: 1; border: 2px solid #49627f; border-radius: 50%; background: radial-gradient(circle at center,#213850 0 11%,#14273b 12% 54%,#0c1928 55%); box-shadow: inset 0 0 0 1px #0a1522,0 .5rem 1.2rem #03091266; touch-action: none; user-select: none; cursor: grab; }
    .joystick.active { background: radial-gradient(circle at center,#284b69 0 11%,#17324b 12% 54%,#0c1928 55%); transform: none; cursor: grabbing; }
    .joystick:focus-visible { outline: 3px solid #75bfff; outline-offset: 4px; }
    .joystick::before,.joystick::after { content: ''; position: absolute; inset: 50% 10%; height: 1px; background: #63809c55; pointer-events: none; }
    .joystick::after { inset: 10% 50%; width: 1px; height: auto; }
    .axis { position: absolute; color: #8da8c0; font-size: .68rem; font-weight: 750; letter-spacing: .08em; pointer-events: none; }
    .axis-up { top: .7rem; left: 50%; transform: translateX(-50%); }
    .axis-down { bottom: .7rem; left: 50%; transform: translateX(-50%); }
    .axis-left { left: .7rem; top: 50%; transform: translateY(-50%); }
    .axis-right { right: .7rem; top: 50%; transform: translateY(-50%); }
    .stick { position: absolute; left: 50%; top: 50%; width: 4.6rem; aspect-ratio: 1; border: 2px solid #8ac9ff; border-radius: 50%; background: linear-gradient(145deg,#3687c7,#17558a); box-shadow: 0 .35rem .8rem #02070daa,inset 0 1px 1px #b9e2ff99; transform: translate(-50%,-50%); pointer-events: none; }
    .secondary { display: grid; width: 100%; grid-template-columns: 1fr 1fr; gap: .55rem; }
    .tether-controls { display: grid; grid-template-columns: 1fr 1fr; gap: .6rem; }
    .tether-controls button { min-height: 5.4rem; font-size: 1rem; }
    .trim-controls { display: grid; grid-template-columns: 1fr 1fr; gap: .6rem; margin-top: .6rem; }
    .trim-controls button { min-height: 4rem; font-size: .88rem; }
    .payout { background: #174f72; }
    .retrieve { background: #3c315f; }
    .safety-dock { position: fixed; z-index: 20; left: 0; right: 0; bottom: 0; display: grid; grid-template-columns: .8fr 1.2fr; gap: .55rem; width: min(100%,30rem); margin: auto; padding: .6rem .75rem max(.6rem,env(safe-area-inset-bottom)); border-top: 1px solid #31475f; background: #07111ff2; box-shadow: 0 -.4rem 1rem #0007; backdrop-filter: blur(12px); }
    .stop { background: #7c2731; border-color: #ca5866; font-size: 1rem; }
    .estop { background: #c92e3e; border-color: #ff7a86; font-size: 1rem; }
    .reset { grid-column: 1 / -1; min-height: 2.8rem; background: #172b42; }
    [hidden] { display: none; }
    @media (max-height: 700px) { .attitude-panel { grid-template-columns: 5.7rem 1fr; } .horizon { width: 5.7rem; } .joystick { width: min(48vw,11.5rem); } .tether-controls button { min-height: 4.2rem; } .trim-controls button { min-height: 3.4rem; } }
    @media (orientation: landscape) and (max-height: 520px) { main { width: min(100%,50rem); padding-bottom: calc(5.3rem + env(safe-area-inset-bottom)); } header { position: absolute; top: max(.45rem,env(safe-area-inset-top)); left: .75rem; right: .75rem; } .panel { width: 42%; margin-top: 2.5rem; } .tabs { width: 42%; } .control-view { position: absolute; top: max(.45rem,env(safe-area-inset-top)); right: .75rem; width: 53%; } .joystick { width: min(45vh,11rem); } .safety-dock { width: min(100%,50rem); } }
  </style>
</head>
<body>
<main>
  <h1 class="sr-only">Station Keeper controls</h1>
  <header aria-live="polite">
    <div class="status-line"><span id="connection" class="pill offline">Connecting</span><p id="state">STOPPED</p></div>
    <div id="mode">LOADING</div>
  </header>
  <section class="panel attitude-panel" aria-label="Live pitch and roll">
    <div class="horizon" role="img" aria-label="Artificial horizon">
      <div id="horizon-world" class="horizon-world">
        <i class="pitch-line long p-up-10"></i><i class="pitch-line p-up-5"></i>
        <i class="pitch-line p-down-5"></i><i class="pitch-line long p-down-10"></i>
      </div>
      <div class="bank-marks"></div><div class="bank-pointer"></div>
      <div class="aircraft"><i class="aircraft-centre"></i></div>
    </div>
    <div class="attitude-readings">
      <div class="attitude-value"><span>PITCH</span><strong id="pitch">0.0°</strong></div>
      <div class="attitude-value"><span>ROLL</span><strong id="roll">0.0°</strong></div>
      <button class="level" id="set-level">SET LEVEL</button>
      <span id="tilt-status" class="tilt-green">LEVEL</span>
    </div>
  </section>

  <nav class="tabs" aria-label="Control selection">
    <button class="tab selected" type="button" data-tab="drive" aria-controls="drive-controls" aria-selected="true">DRIVE</button>
    <button class="tab" type="button" data-tab="winches" aria-controls="winch-controls" aria-selected="false">WINCHES</button>
  </nav>

  <section class="control-view controls" id="drive-controls" aria-label="Hold-to-run drive controls">
    <div id="joystick" class="joystick" role="application" tabindex="0" aria-label="Movement joystick. Hold and drag in any direction.">
      <span class="axis axis-up">FORWARD</span>
      <span class="axis axis-down">REVERSE</span>
      <span class="axis axis-left">LEFT</span>
      <span class="axis axis-right">RIGHT</span>
      <span id="stick" class="stick"></span>
    </div>
    <div class="secondary">
      <button data-motion="rotate-left" aria-label="Hold to rotate left">↺ ROTATE</button>
      <button data-motion="rotate-right" aria-label="Hold to rotate right">ROTATE ↻</button>
    </div>
  </section>

  <section class="control-view" id="winch-controls" aria-label="Hold-to-run tether winch controls" hidden>
    <div class="tether-controls">
      <button class="payout" data-motion="winches-payout" aria-label="Hold to pay both ropes out">↑<br>BOTH OUT</button>
      <button class="retrieve" data-motion="winches-retrieve" aria-label="Hold to take both ropes in">↓<br>BOTH IN</button>
    </div>
    <div class="trim-controls">
      <button data-motion="left-winch-payout" aria-label="Hold to pay left rope out">L ↑ OUT</button>
      <button data-motion="right-winch-payout" aria-label="Hold to pay right rope out">R ↑ OUT</button>
      <button data-motion="left-winch-retrieve" aria-label="Hold to take left rope in">L ↓ IN</button>
      <button data-motion="right-winch-retrieve" aria-label="Hold to take right rope in">R ↓ IN</button>
    </div>
  </section>
</main>
<div class="safety-dock" aria-label="Safety controls">
  <button class="stop" id="stop">STOP</button>
  <button class="estop" id="estop">EMERGENCY STOP</button>
  <button class="reset" id="reset" hidden>CLEAR E-STOP</button>
</div>
<script>
(() => {
  const connection = document.querySelector('#connection');
  const state = document.querySelector('#state');
  const mode = document.querySelector('#mode');
  const reset = document.querySelector('#reset');
  const joystick = document.querySelector('#joystick');
  const stick = document.querySelector('#stick');
  const horizonWorld = document.querySelector('#horizon-world');
  const pitchValue = document.querySelector('#pitch');
  const rollValue = document.querySelector('#roll');
  const tiltStatus = document.querySelector('#tilt-status');
  const deadZone = .12;
  let heldMotion = null;
  let joystickPointer = null;
  let joystickX = 0;
  let joystickY = 0;
  let heartbeat = null;
  let lastDriveSent = 0;
  let requestSequence = 0;
  let statusInFlight = false;

  async function post(path, body = '') {
    const sequence = ++requestSequence;
    try {
      const response = await fetch(path, {method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body,cache:'no-store'});
      const data = await response.json();
      if (!response.ok) throw new Error(data.error || response.statusText);
      if (sequence === requestSequence) render(data);
      return true;
    } catch (error) {
      if (sequence !== requestSequence) return false;
      clearMotionState();
      connection.textContent = 'Disconnected';
      connection.className = 'pill offline';
      state.textContent = 'CONNECTION LOST — AUTO STOP';
      return false;
    }
  }

  function render(data) {
    connection.textContent = 'ONLINE';
    connection.className = 'pill online';
    mode.textContent = data.testMode ? 'TEST MODE' : 'HARDWARE';
    state.textContent = data.estop ? 'EMERGENCY STOP LATCHED' :
      data.motion === 'joystick' ? `JOYSTICK  X ${data.x} · Y ${data.y}` :
      data.motion === 'winches-payout' ? 'TETHERS: BOTH PAYING OUT' :
      data.motion === 'winches-retrieve' ? 'TETHERS: BOTH TAKING IN' :
      data.motion.replace(/-/g, ' ').toUpperCase();
    reset.hidden = !data.estop;
    renderAttitude(data);
  }

  function renderAttitude(data) {
    if (!Number.isFinite(data.pitch) || !Number.isFinite(data.roll)) return;
    const levelButton = document.querySelector('#set-level');
    levelButton.disabled = !data.attitudeConnected && !data.attitudeDemo;
    if (!data.attitudeConnected && !data.attitudeDemo) {
      tiltStatus.textContent = 'IMU OFFLINE';
      tiltStatus.className = 'tilt-red';
      return;
    }
    const pitch = data.pitch;
    const roll = data.roll;
    const pitchOffset = -pitch * 2;
    horizonWorld.style.transform = `translate(-50%,calc(-50% + ${pitchOffset}px)) rotate(${-roll}deg)`;
    pitchValue.textContent = `${pitch.toFixed(1)}°`;
    rollValue.textContent = `${roll.toFixed(1)}°`;
    const tilt = Math.max(Math.abs(pitch), Math.abs(roll));
    const level = tilt < 5 ? ['LEVEL','tilt-green'] : tilt <= 10 ? ['CAUTION','tilt-amber'] : ['HIGH TILT','tilt-red'];
    tiltStatus.textContent = level[0];
    tiltStatus.className = level[1];
    document.querySelector('.horizon').setAttribute('aria-label', `Pitch ${pitch.toFixed(1)} degrees, roll ${roll.toFixed(1)} degrees`);
  }

  async function refreshStatus() {
    if (statusInFlight) return;
    statusInFlight = true;
    try {
      const response = await fetch('/api/status', {cache:'no-store'});
      if (!response.ok) throw new Error(response.statusText);
      render(await response.json());
    } catch (error) {
      connection.textContent = 'Disconnected';
      connection.className = 'pill offline';
    } finally {
      statusInFlight = false;
    }
  }

  function clearMotionState() {
    if (heartbeat) clearInterval(heartbeat);
    heartbeat = null;
    heldMotion = null;
    joystickPointer = null;
    joystickX = 0;
    joystickY = 0;
    stick.style.transform = 'translate(-50%,-50%)';
    joystick.classList.remove('active');
    document.querySelectorAll('.active').forEach(el => el.classList.remove('active'));
  }

  function endMotion(sendStop = true) {
    const wasActive = heldMotion !== null || joystickPointer !== null || heartbeat !== null;
    clearMotionState();
    if (sendStop && wasActive) post('/api/stop');
  }

  function stopForPageExit() {
    const wasActive = heldMotion !== null || joystickPointer !== null || heartbeat !== null;
    clearMotionState();
    if (!wasActive) return;
    if (!navigator.sendBeacon || !navigator.sendBeacon('/api/stop', '')) {
      fetch('/api/stop', {method:'POST',body:'',keepalive:true,cache:'no-store'}).catch(() => {});
    }
  }

  function driveBody() {
    return `x=${joystickX}&y=${joystickY}`;
  }

  function sendJoystick(force = false) {
    const now = performance.now();
    if (!force && now - lastDriveSent < 60) return;
    lastDriveSent = now;
    post('/api/drive', driveBody());
  }

  function updateJoystick(clientX, clientY) {
    const rect = joystick.getBoundingClientRect();
    const centreX = rect.left + rect.width / 2;
    const centreY = rect.top + rect.height / 2;
    const travel = (rect.width - stick.offsetWidth) / 2;
    let dx = clientX - centreX;
    let dy = clientY - centreY;
    let distance = Math.hypot(dx, dy);
    if (distance > travel) {
      dx *= travel / distance;
      dy *= travel / distance;
      distance = travel;
    }
    stick.style.transform = `translate(calc(-50% + ${dx}px),calc(-50% + ${dy}px))`;

    const magnitude = travel > 0 ? distance / travel : 0;
    if (magnitude <= deadZone) {
      joystickX = 0;
      joystickY = 0;
    } else {
      joystickX = Math.round(dx / travel * 100);
      joystickY = Math.round(-dy / travel * 100);
    }
    state.textContent = `JOYSTICK  X ${joystickX} · Y ${joystickY}`;
  }

  document.querySelectorAll('[data-motion]').forEach(button => {
    button.addEventListener('pointerdown', event => {
      event.preventDefault();
      endMotion();
      heldMotion = button.dataset.motion;
      button.classList.add('active');
      button.setPointerCapture(event.pointerId);
      post('/api/move', 'direction=' + encodeURIComponent(heldMotion));
      heartbeat = setInterval(() => heldMotion && post('/api/move', 'direction=' + encodeURIComponent(heldMotion)), 250);
    });
    button.addEventListener('pointerup', endMotion);
    button.addEventListener('pointercancel', endMotion);
    button.addEventListener('lostpointercapture', endMotion);
  });

  document.querySelectorAll('[data-tab]').forEach(tab => {
    tab.addEventListener('click', () => {
      endMotion();
      const selected = tab.dataset.tab;
      document.querySelector('#drive-controls').hidden = selected !== 'drive';
      document.querySelector('#winch-controls').hidden = selected !== 'winches';
      document.querySelectorAll('[data-tab]').forEach(item => {
        const active = item === tab;
        item.classList.toggle('selected', active);
        item.setAttribute('aria-selected', active ? 'true' : 'false');
      });
    });
  });

  joystick.addEventListener('pointerdown', event => {
    if (joystickPointer !== null) return;
    event.preventDefault();
    endMotion();
    joystickPointer = event.pointerId;
    joystick.classList.add('active');
    joystick.setPointerCapture(event.pointerId);
    updateJoystick(event.clientX, event.clientY);
    sendJoystick(true);
    heartbeat = setInterval(() => joystickPointer !== null && sendJoystick(true), 200);
  });
  joystick.addEventListener('pointermove', event => {
    if (event.pointerId !== joystickPointer) return;
    event.preventDefault();
    updateJoystick(event.clientX, event.clientY);
    sendJoystick();
  });
  joystick.addEventListener('pointerup', endMotion);
  joystick.addEventListener('pointercancel', endMotion);
  joystick.addEventListener('lostpointercapture', endMotion);

  document.querySelector('#stop').addEventListener('click', () => { endMotion(false); post('/api/stop'); });
  document.querySelector('#estop').addEventListener('click', () => { endMotion(false); post('/api/estop'); });
  reset.addEventListener('click', () => post('/api/estop/clear'));
  document.querySelector('#set-level').addEventListener('click', () => post('/api/attitude/level'));
  window.addEventListener('blur', endMotion);
  window.addEventListener('offline', endMotion);
  window.addEventListener('pagehide', stopForPageExit);
  document.addEventListener('visibilitychange', () => document.hidden && stopForPageExit());
  window.addEventListener('contextmenu', event => event.preventDefault());
  refreshStatus();
  setInterval(refreshStatus, 50);
})();
</script>
</body>
</html>
)HTML";
