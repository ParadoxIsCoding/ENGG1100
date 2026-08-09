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
    body { margin: 0; min-height: 100dvh; background: #07111f; color: #f7fbff; touch-action: manipulation; }
    main { width: min(100%, 30rem); margin: auto; padding: max(1rem,env(safe-area-inset-top)) 1rem max(1rem,env(safe-area-inset-bottom)); }
    header { display: flex; align-items: center; justify-content: space-between; gap: 1rem; }
    h1 { margin: 0; font-size: clamp(1.4rem,7vw,2rem); }
    .pill { border: 1px solid #3d536c; border-radius: 999px; padding: .35rem .65rem; font-size: .8rem; }
    .online { color: #70f0b0; border-color: #287f59; }
    .offline { color: #ffadad; border-color: #963e45; }
    .panel { margin-top: 1rem; padding: 1rem; background: #101d2d; border: 1px solid #263b52; border-radius: 1rem; }
    #mode { color: #9dc7f1; font-size: .9rem; }
    #state { margin: .35rem 0 0; font-size: 1.15rem; font-weight: 700; }
    .controls { display: flex; flex-direction: column; align-items: center; gap: 1rem; margin-top: 1rem; }
    button { min-height: 4.25rem; border: 1px solid #49627f; border-radius: .85rem; background: #172b42; color: inherit; font: inherit; font-weight: 750; touch-action: none; user-select: none; }
    button:active,.active { background: #2073b8; transform: scale(.98); }
    button:focus-visible { outline: 3px solid #75bfff; outline-offset: 2px; }
    .joystick { position: relative; width: min(72vw,17rem); aspect-ratio: 1; border: 2px solid #49627f; border-radius: 50%; background: radial-gradient(circle at center,#213850 0 11%,#14273b 12% 54%,#0c1928 55%); box-shadow: inset 0 0 0 1px #0a1522,0 .6rem 1.5rem #03091266; touch-action: none; user-select: none; cursor: grab; }
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
    .secondary { display: grid; width: 100%; grid-template-columns: 1fr 1.1fr 1fr; gap: .65rem; }
    .stop { background: #7c2731; border-color: #ca5866; }
    .estop { width: 100%; min-height: 4.8rem; margin-top: 1rem; background: #c92e3e; border-color: #ff7a86; font-size: 1.15rem; }
    .reset { width: 100%; min-height: 3rem; margin-top: .65rem; background: transparent; }
    .hint { margin: .85rem 0 0; color: #afbed0; font-size: .85rem; line-height: 1.4; }
    [hidden] { display: none; }
    @media (max-height: 700px) { button { min-height: 3.35rem; } .panel { padding: .75rem; } .joystick { width: min(58vw,13rem); } }
  </style>
</head>
<body>
<main>
  <header><h1>Station Keeper</h1><span id="connection" class="pill offline">Connecting</span></header>
  <section class="panel" aria-live="polite">
    <div id="mode">Loading controller status…</div>
    <p id="state">STOPPED</p>
  </section>
  <section class="controls" aria-label="Hold-to-run motion controls">
    <div id="joystick" class="joystick" role="application" tabindex="0" aria-label="Movement joystick. Hold and drag in any direction." aria-describedby="control-hint">
      <span class="axis axis-up">FORWARD</span>
      <span class="axis axis-down">REVERSE</span>
      <span class="axis axis-left">LEFT</span>
      <span class="axis axis-right">RIGHT</span>
      <span id="stick" class="stick"></span>
    </div>
    <div class="secondary">
      <button data-motion="rotate-left">↺ Rotate</button>
      <button class="stop" id="stop">STOP</button>
      <button data-motion="rotate-right">Rotate ↻</button>
    </div>
  </section>
  <button class="estop" id="estop">EMERGENCY STOP</button>
  <button class="reset" id="reset" hidden>Clear emergency stop</button>
  <p class="hint" id="control-hint">Hold and drag the joystick to move proportionally. Its centre is a dead zone. Releasing it stops all motors. A lost connection also stops motion automatically.</p>
</main>
<script>
(() => {
  const connection = document.querySelector('#connection');
  const state = document.querySelector('#state');
  const mode = document.querySelector('#mode');
  const reset = document.querySelector('#reset');
  const joystick = document.querySelector('#joystick');
  const stick = document.querySelector('#stick');
  const deadZone = .12;
  let heldMotion = null;
  let joystickPointer = null;
  let joystickX = 0;
  let joystickY = 0;
  let heartbeat = null;
  let lastDriveSent = 0;
  let requestSequence = 0;

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
    connection.textContent = 'Connected';
    connection.className = 'pill online';
    mode.textContent = data.testMode ? 'TEST MODE — motors simulated' : 'HARDWARE MODE';
    state.textContent = data.estop ? 'EMERGENCY STOP LATCHED' :
      data.motion === 'joystick' ? `JOYSTICK  X ${data.x} · Y ${data.y}` : data.motion.toUpperCase();
    reset.hidden = !data.estop;
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
  window.addEventListener('blur', endMotion);
  window.addEventListener('offline', endMotion);
  window.addEventListener('pagehide', stopForPageExit);
  document.addEventListener('visibilitychange', () => document.hidden && stopForPageExit());
  window.addEventListener('contextmenu', event => event.preventDefault());
  fetch('/api/status', {cache:'no-store'}).then(r => r.json()).then(render).catch(() => {});
})();
</script>
</body>
</html>
)HTML";
