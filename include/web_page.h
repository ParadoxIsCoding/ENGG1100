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
    .controls { display: grid; grid-template-columns: repeat(3,1fr); gap: .65rem; margin-top: 1rem; }
    button { min-height: 4.25rem; border: 1px solid #49627f; border-radius: .85rem; background: #172b42; color: inherit; font: inherit; font-weight: 750; touch-action: none; user-select: none; }
    button:active,.active { background: #2073b8; transform: scale(.98); }
    button:focus-visible { outline: 3px solid #75bfff; outline-offset: 2px; }
    .forward { grid-column: 2; }
    .left { grid-column: 1; grid-row: 2; }
    .stop { grid-column: 2; grid-row: 2; background: #7c2731; border-color: #ca5866; }
    .right { grid-column: 3; grid-row: 2; }
    .reverse { grid-column: 2; grid-row: 3; }
    .rotate-left { grid-column: 1; grid-row: 4; }
    .rotate-right { grid-column: 3; grid-row: 4; }
    .estop { width: 100%; min-height: 4.8rem; margin-top: 1rem; background: #c92e3e; border-color: #ff7a86; font-size: 1.15rem; }
    .reset { width: 100%; min-height: 3rem; margin-top: .65rem; background: transparent; }
    .hint { margin: .85rem 0 0; color: #afbed0; font-size: .85rem; line-height: 1.4; }
    [hidden] { display: none; }
    @media (max-height: 650px) { button { min-height: 3.35rem; } .panel { padding: .75rem; } }
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
    <button class="forward" data-motion="forward">▲<br>Forward</button>
    <button class="left" data-motion="left">◀ Left</button>
    <button class="stop" id="stop">STOP</button>
    <button class="right" data-motion="right">Right ▶</button>
    <button class="reverse" data-motion="reverse">▼<br>Reverse</button>
    <button class="rotate-left" data-motion="rotate-left">↺ Rotate</button>
    <button class="rotate-right" data-motion="rotate-right">Rotate ↻</button>
  </section>
  <button class="estop" id="estop">EMERGENCY STOP</button>
  <button class="reset" id="reset" hidden>Clear emergency stop</button>
  <p class="hint">Hold a direction to move. Releasing it stops all motors. A lost connection also stops motion automatically.</p>
</main>
<script>
(() => {
  const connection = document.querySelector('#connection');
  const state = document.querySelector('#state');
  const mode = document.querySelector('#mode');
  const reset = document.querySelector('#reset');
  let held = null;
  let heartbeat = null;

  async function post(path, body = '') {
    try {
      const response = await fetch(path, {method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body,cache:'no-store'});
      const data = await response.json();
      if (!response.ok) throw new Error(data.error || response.statusText);
      render(data);
      return true;
    } catch (error) {
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
    state.textContent = data.estop ? 'EMERGENCY STOP LATCHED' : data.motion.toUpperCase();
    reset.hidden = !data.estop;
  }

  function endMotion() {
    if (heartbeat) clearInterval(heartbeat);
    heartbeat = null;
    document.querySelectorAll('.active').forEach(el => el.classList.remove('active'));
    if (held !== null) {
      held = null;
      post('/api/stop');
    }
  }

  document.querySelectorAll('[data-motion]').forEach(button => {
    button.addEventListener('pointerdown', event => {
      event.preventDefault();
      endMotion();
      held = button.dataset.motion;
      button.classList.add('active');
      button.setPointerCapture(event.pointerId);
      post('/api/move', 'direction=' + encodeURIComponent(held));
      heartbeat = setInterval(() => held && post('/api/move', 'direction=' + encodeURIComponent(held)), 250);
    });
    button.addEventListener('pointerup', endMotion);
    button.addEventListener('pointercancel', endMotion);
    button.addEventListener('lostpointercapture', endMotion);
  });

  document.querySelector('#stop').addEventListener('click', () => { endMotion(); post('/api/stop'); });
  document.querySelector('#estop').addEventListener('click', () => { endMotion(); post('/api/estop'); });
  reset.addEventListener('click', () => post('/api/estop/clear'));
  window.addEventListener('blur', endMotion);
  window.addEventListener('pagehide', endMotion);
  document.addEventListener('visibilitychange', () => document.hidden && endMotion());
  window.addEventListener('contextmenu', event => event.preventDefault());
  fetch('/api/status', {cache:'no-store'}).then(r => r.json()).then(render).catch(() => {});
})();
</script>
</body>
</html>
)HTML";
