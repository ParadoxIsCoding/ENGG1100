#pragma once

const char kControlPage[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
  <meta name="theme-color" content="#000000">
  <title>Station Keeper</title>
  <style>
    :root {
      color-scheme: dark;
      font-family: system-ui,-apple-system,sans-serif;
      --bg: #000; --panel: #0d0d0d; --panel-alt: #111; --border: #2a2a2a; --border-soft: #1c1c1c;
      --text: #fff; --muted: #8a8a8a; --dim: #555;
      --red: #ef3b3b; --red-strong: #ff2d2d; --red-deep: #7a1414; --red-wash: #1a0a0a;
    }
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; -webkit-touch-callout: none; -webkit-user-select: none; user-select: none; }
    html,body { overscroll-behavior: none; }
    body { margin: 0; min-height: 100dvh; background: var(--bg); color: var(--text); touch-action: manipulation; }
    main { width: min(100%, 30rem); margin: auto; padding: max(.6rem,env(safe-area-inset-top)) .75rem calc(6.7rem + env(safe-area-inset-bottom)); }
    header { display: flex; align-items: center; justify-content: space-between; gap: .55rem; min-height: 2rem; }
    .sr-only { position: absolute; width: 1px; height: 1px; padding: 0; margin: -1px; overflow: hidden; clip: rect(0,0,0,0); white-space: nowrap; border: 0; }
    .status-line { min-width: 0; display: flex; align-items: center; gap: .5rem; }
    .pill { border: 1px solid var(--border); border-radius: 999px; padding: .35rem .65rem; font-size: .8rem; }
    .online { color: var(--text); border-color: var(--dim); }
    .offline { color: var(--red); border-color: var(--red-deep); }
    .panel { margin-top: .65rem; padding: .7rem; background: var(--panel); border: 1px solid var(--border); border-radius: .9rem; }
    #mode { flex: none; color: var(--muted); font-size: .67rem; font-weight: 750; letter-spacing: .04em; }
    #state { min-width: 0; margin: 0; overflow: hidden; font-size: .82rem; font-weight: 800; text-overflow: ellipsis; white-space: nowrap; }
    .attitude-panel { display: grid; grid-template-columns: 7rem 1fr; align-items: center; gap: .7rem; }
    .horizon { position: relative; width: 7rem; aspect-ratio: 1; overflow: hidden; border: 3px solid var(--border); border-radius: 50%; background: #050505; box-shadow: inset 0 0 1.2rem #000b,0 .35rem .8rem #03091288; }
    .horizon-world { position: absolute; left: 50%; top: 50%; width: 320%; height: 320%; background: linear-gradient(to bottom,#1a1a1a 0 49.7%,#fff 49.7% 50.3%,#000 50.3% 100%); transform: translate(-50%,-50%); transition: transform 45ms linear; will-change: transform; }
    .pitch-line { position: absolute; left: 50%; width: 2rem; height: 1px; background: #fff; box-shadow: 0 1px #0008; transform: translateX(-50%); }
    .pitch-line.long { width: 3.2rem; }
    .p-up-10 { top: calc(50% - 20px); } .p-up-5 { top: calc(50% - 10px); }
    .p-down-5 { top: calc(50% + 10px); } .p-down-10 { top: calc(50% + 20px); }
    .bank-marks { position: absolute; inset: .35rem; border-top: 2px solid #fff; border-radius: 50%; pointer-events: none; }
    .bank-pointer { position: absolute; top: .3rem; left: 50%; width: 0; height: 0; border-left: .34rem solid transparent; border-right: .34rem solid transparent; border-top: .55rem solid #fff; transform: translateX(-50%); }
    .aircraft { position: absolute; left: 50%; top: 50%; width: 58%; height: 1.7rem; transform: translate(-50%,-50%); pointer-events: none; }
    .aircraft::before,.aircraft::after { content: ''; position: absolute; top: .72rem; width: 38%; height: .22rem; border: 1px solid #000; background: var(--red); }
    .aircraft::before { left: 0; transform: rotate(8deg); transform-origin: right; }
    .aircraft::after { right: 0; transform: rotate(-8deg); transform-origin: left; }
    .aircraft-centre { position: absolute; left: 50%; top: 50%; width: .65rem; aspect-ratio: 1; border: .18rem solid #000; border-radius: 50%; background: var(--red); transform: translate(-50%,-50%); }
    .attitude-readings { display: grid; width: 100%; grid-template-columns: 1fr 1fr; align-items: center; gap: .45rem; }
    .attitude-value { padding: .5rem .35rem; border: 1px solid var(--border); border-radius: .65rem; background: var(--panel-alt); text-align: center; }
    .attitude-value span { display: block; color: var(--muted); font-size: .68rem; font-weight: 700; letter-spacing: .08em; }
    .attitude-value strong { display: block; margin-top: .08rem; font-size: 1.05rem; font-variant-numeric: tabular-nums; }
    #tilt-status { justify-self: stretch; padding: .35rem .4rem; border: 1px solid currentColor; border-radius: .6rem; font-size: .69rem; font-weight: 800; letter-spacing: .05em; text-align: center; }
    .tilt-green { color: var(--text); } .tilt-amber { color: var(--red); } .tilt-red { color: #fff; background: var(--red); border-color: var(--red); }
    #vertical-motion { grid-column: 1 / -1; padding: .3rem .4rem; border: 1px solid var(--border); border-radius: .6rem; font-size: .72rem; font-weight: 800; letter-spacing: .07em; text-align: center; }
    .motion-steady { color: var(--muted); } .motion-rising { color: var(--text); } .motion-falling { color: var(--red); }
    .level { min-height: 2.3rem; padding: 0 .4rem; white-space: nowrap; }
    .tabs { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: .45rem; margin-top: .65rem; padding: .25rem; border: 1px solid var(--border); border-radius: .85rem; background: var(--panel-alt); }
    .tab { min-height: 2.7rem; padding: 0 .2rem; border: 0; border-radius: .65rem; background: transparent; color: var(--muted); font-size: .85rem; }
    .tab.selected { background: var(--red); color: #fff; }
    .control-view { margin-top: .65rem; }
    .controls { display: flex; flex-direction: column; align-items: center; gap: .6rem; }
    button { min-height: 3.65rem; border: 1px solid var(--border); border-radius: .8rem; background: var(--panel-alt); color: inherit; font: inherit; font-weight: 750; touch-action: manipulation; user-select: none; }
    button:active,.active { background: var(--red); border-color: var(--red); transform: scale(.98); }
    button:focus-visible { outline: 3px solid var(--red); outline-offset: 2px; }
    .joystick { position: relative; width: min(58vw,14.5rem); aspect-ratio: 1; border: 2px solid var(--border); border-radius: 50%; background: radial-gradient(circle at center,#1a1a1a 0 11%,#111 12% 54%,#0a0a0a 55%); box-shadow: inset 0 0 0 1px #000,0 .5rem 1.2rem #00000066; touch-action: none; user-select: none; cursor: grab; }
    .joystick.active { background: radial-gradient(circle at center,#2a1414 0 11%,#1a0d0d 12% 54%,#0a0a0a 55%); transform: none; cursor: grabbing; }
    .joystick:focus-visible { outline: 3px solid var(--red); outline-offset: 4px; }
    .joystick::before,.joystick::after { content: ''; position: absolute; inset: 50% 10%; height: 1px; background: #44444455; pointer-events: none; }
    .joystick::after { inset: 10% 50%; width: 1px; height: auto; }
    .axis { position: absolute; color: var(--muted); font-size: .68rem; font-weight: 750; letter-spacing: .08em; pointer-events: none; }
    .axis-up { top: .7rem; left: 50%; transform: translateX(-50%); }
    .axis-down { bottom: .7rem; left: 50%; transform: translateX(-50%); }
    .axis-left { left: .7rem; top: 50%; transform: translateY(-50%); }
    .axis-right { right: .7rem; top: 50%; transform: translateY(-50%); }
    .stick { position: absolute; left: 50%; top: 50%; width: 4.6rem; aspect-ratio: 1; border: 2px solid var(--red); border-radius: 50%; background: linear-gradient(145deg,var(--red),var(--red-deep)); box-shadow: 0 .35rem .8rem #000a,inset 0 1px 1px #ffffff33; transform: translate(-50%,-50%); pointer-events: none; }
    .secondary { display: grid; width: 100%; grid-template-columns: 1fr 1fr; gap: .55rem; }
    .tether-controls { display: grid; grid-template-columns: 1fr 1fr; gap: .6rem; }
    .tether-controls button { min-height: 5.4rem; font-size: 1rem; }
    .trim-controls { display: grid; grid-template-columns: 1fr 1fr; gap: .6rem; margin-top: .6rem; }
    .trim-controls button { min-height: 4rem; font-size: .88rem; }
    .safety-dock { position: fixed; z-index: 20; left: 0; right: 0; bottom: 0; display: grid; grid-template-columns: .8fr 1.2fr; gap: .55rem; width: min(100%,30rem); margin: auto; padding: .6rem .75rem max(.6rem,env(safe-area-inset-bottom)); border-top: 1px solid var(--border); background: #000000f2; box-shadow: 0 -.4rem 1rem #0007; backdrop-filter: blur(12px); }
    .stop { background: var(--red-wash); border-color: var(--red); color: var(--red); font-size: 1rem; }
    .estop { background: var(--red); border-color: var(--red); color: #fff; font-size: 1rem; }
    .reset { grid-column: 1 / -1; min-height: 2.8rem; background: var(--panel-alt); }
    .speed-row { display: flex; align-items: center; gap: .6rem; margin-top: .55rem; }
    .speed-row label { flex: none; color: var(--muted); font-size: .72rem; font-weight: 750; letter-spacing: .05em; }
    .speed-row input[type=range] { flex: 1; accent-color: var(--red); }
    .speed-row strong { flex: none; min-width: 3.4rem; text-align: right; font-variant-numeric: tabular-nums; }
    .stat-grid { display: grid; grid-template-columns: 1fr 1fr; gap: .5rem; margin-top: .6rem; }
    .stat { padding: .4rem .5rem; border: 1px solid var(--border); border-radius: .6rem; background: var(--panel-alt); overflow: hidden; }
    .stat span { display: block; color: var(--muted); font-size: .64rem; font-weight: 750; letter-spacing: .06em; }
    .stat strong { display: block; margin-top: .05rem; font-size: .82rem; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
    .motor-cards { display: grid; grid-template-columns: 1fr 1fr; gap: .5rem; margin-top: .6rem; }
    .motor-card { padding: .45rem .5rem; border: 1px solid var(--border); border-radius: .6rem; background: var(--panel-alt); text-align: center; }
    .motor-card span { display: block; color: var(--muted); font-size: .64rem; font-weight: 750; letter-spacing: .06em; }
    .motor-card strong { display: block; margin-top: .1rem; font-size: .88rem; }
    .motor-card.motor-in strong, .motor-card.motor-out strong { color: var(--red); }
    .motor-card.motor-stopped strong { color: var(--muted); }
    .cal-hint { margin: 0 0 .6rem; padding: .55rem .6rem; border: 1px solid var(--border); border-radius: .7rem; background: var(--panel-alt); color: var(--muted); font-size: .78rem; line-height: 1.35; }
    .cal-active { display: flex; align-items: center; gap: .5rem; margin: 0 0 .6rem; padding: .5rem .6rem; border: 1px solid var(--border); border-radius: .7rem; background: var(--panel); }
    .cal-active span { flex: none; color: var(--muted); font-size: .64rem; font-weight: 750; letter-spacing: .08em; }
    .cal-active strong { min-width: 0; overflow-x: auto; white-space: nowrap; font-size: .8rem; font-variant-numeric: tabular-nums; }
    .cal-slots { display: flex; flex-direction: column; gap: .6rem; }
    .cal-slot { padding: .55rem; border: 1px solid var(--border); border-radius: .8rem; background: var(--panel); }
    .cal-slot.spinning { border-color: var(--red); }
    .cal-slot-head { display: flex; align-items: center; justify-content: space-between; font-size: .82rem; font-weight: 800; letter-spacing: .04em; }
    .cal-slot-status { color: var(--red); font-size: .68rem; font-weight: 750; letter-spacing: .05em; min-height: 1rem; }
    .cal-spin-row { display: grid; grid-template-columns: 1fr 1fr; gap: .5rem; margin-top: .45rem; }
    .cal-spin-row button { min-height: 3.2rem; font-size: .85rem; }
    .cal-corner-row { display: grid; grid-template-columns: 1fr 1fr 1fr 1fr; gap: .4rem; margin-top: .45rem; }
    .cal-corner-row button { min-height: 2.5rem; padding: 0; font-size: .78rem; background: var(--panel-alt); }
    .cal-corner-row button.selected { background: var(--red); border-color: var(--red); color: #fff; }
    .cal-invert { width: 100%; min-height: 2.4rem; margin-top: .45rem; font-size: .78rem; background: var(--panel-alt); }
    .cal-invert.active { background: var(--red-wash); border-color: var(--red); color: var(--red); }
    .cal-actions { display: grid; grid-template-columns: 1fr 1fr; gap: .55rem; margin-top: .7rem; }
    .cal-actions button { min-height: 3rem; font-size: .85rem; }
    #cal-save:not(:disabled) { border-color: var(--red); color: var(--red); }
    #cal-save:disabled { opacity: .4; }
    #cal-message { min-height: 1.1rem; margin: .5rem 0 0; text-align: center; font-size: .78rem; font-weight: 750; color: var(--muted); }
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
      <span id="vertical-motion" class="motion-steady">● STEADY</span>
    </div>
  </section>

  <section class="panel" aria-label="Telemetry and motor status">
    <div class="speed-row">
      <label for="speed">SPEED</label>
      <input type="range" id="speed" min="20" max="100" value="80" aria-label="Motor speed percent, capped at the configured maximum">
      <strong id="speed-value">80%</strong>
    </div>
    <div class="stat-grid">
      <div class="stat"><span>UPTIME</span><strong id="uptime">0s</strong></div>
      <div class="stat"><span>WI-FI CLIENTS</span><strong id="clients">0</strong></div>
      <div class="stat"><span>LAST COMMAND</span><strong id="last-command">none</strong></div>
      <div class="stat"><span>FAILSAFE</span><strong id="failsafe">none</strong></div>
    </div>
    <div class="motor-cards">
      <div class="motor-card motor-stopped" id="card-fl"><span>FRONT LEFT</span><strong>STOPPED</strong></div>
      <div class="motor-card motor-stopped" id="card-fr"><span>FRONT RIGHT</span><strong>STOPPED</strong></div>
      <div class="motor-card motor-stopped" id="card-rl"><span>REAR LEFT</span><strong>STOPPED</strong></div>
      <div class="motor-card motor-stopped" id="card-rr"><span>REAR RIGHT</span><strong>STOPPED</strong></div>
    </div>
  </section>

  <nav class="tabs" aria-label="Control selection">
    <button class="tab selected" type="button" data-tab="drive" aria-controls="drive-controls" aria-selected="true">DRIVE</button>
    <button class="tab" type="button" data-tab="winches" aria-controls="winch-controls" aria-selected="false">WINCHES</button>
    <button class="tab" type="button" data-tab="calibration" aria-controls="calibration-controls" aria-selected="false">CALIBRATE</button>
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

  <section class="control-view" id="winch-controls" aria-label="Hold-to-run individual corner winch controls" hidden>
    <div class="tether-controls">
      <button data-motion="all-payout" aria-label="Hold to pay all four tethers out">↑<br>ALL OUT</button>
      <button data-motion="all-retrieve" aria-label="Hold to take all four tethers in">↓<br>ALL IN</button>
    </div>
    <div class="trim-controls">
      <button data-motion="front-left-payout" aria-label="Hold to pay the front-left tether out">FL ↑ OUT</button>
      <button data-motion="front-left-retrieve" aria-label="Hold to take the front-left tether in">FL ↓ IN</button>
      <button data-motion="front-right-payout" aria-label="Hold to pay the front-right tether out">FR ↑ OUT</button>
      <button data-motion="front-right-retrieve" aria-label="Hold to take the front-right tether in">FR ↓ IN</button>
      <button data-motion="rear-left-payout" aria-label="Hold to pay the rear-left tether out">RL ↑ OUT</button>
      <button data-motion="rear-left-retrieve" aria-label="Hold to take the rear-left tether in">RL ↓ IN</button>
      <button data-motion="rear-right-payout" aria-label="Hold to pay the rear-right tether out">RR ↑ OUT</button>
      <button data-motion="rear-right-retrieve" aria-label="Hold to take the rear-right tether in">RR ↓ IN</button>
    </div>
  </section>

  <section class="control-view" id="calibration-controls" aria-label="Motor identification and calibration" hidden>
    <p class="cal-hint">Hold OUT or IN on one motor at a time and watch which corner physically moves. Tap that corner below to label it, flip INVERT if it spins the wrong way, then SAVE once all four are labelled. Nothing changes until you press SAVE.</p>
    <div class="cal-active" aria-label="Currently active motor calibration">
      <span>ACTIVE</span>
      <strong id="cal-active-summary">—</strong>
    </div>
    <div class="cal-slots">
      <div class="cal-slot" id="cal-slot-0" data-cal-slot-card="0">
        <div class="cal-slot-head"><span>MOTOR 1</span><span class="cal-slot-status" data-cal-status="0"></span></div>
        <div class="cal-spin-row">
          <button data-cal-slot="0" data-cal-dir="payout" aria-label="Hold to spin motor 1 out">↑ OUT</button>
          <button data-cal-slot="0" data-cal-dir="retrieve" aria-label="Hold to spin motor 1 in">↓ IN</button>
        </div>
        <div class="cal-corner-row" data-cal-corner-group="0">
          <button type="button" data-corner="front-left">FL</button>
          <button type="button" data-corner="front-right">FR</button>
          <button type="button" data-corner="rear-left">RL</button>
          <button type="button" data-corner="rear-right">RR</button>
        </div>
        <button type="button" class="cal-invert" data-cal-invert="0">INVERT: OFF</button>
      </div>
      <div class="cal-slot" id="cal-slot-1" data-cal-slot-card="1">
        <div class="cal-slot-head"><span>MOTOR 2</span><span class="cal-slot-status" data-cal-status="1"></span></div>
        <div class="cal-spin-row">
          <button data-cal-slot="1" data-cal-dir="payout" aria-label="Hold to spin motor 2 out">↑ OUT</button>
          <button data-cal-slot="1" data-cal-dir="retrieve" aria-label="Hold to spin motor 2 in">↓ IN</button>
        </div>
        <div class="cal-corner-row" data-cal-corner-group="1">
          <button type="button" data-corner="front-left">FL</button>
          <button type="button" data-corner="front-right">FR</button>
          <button type="button" data-corner="rear-left">RL</button>
          <button type="button" data-corner="rear-right">RR</button>
        </div>
        <button type="button" class="cal-invert" data-cal-invert="1">INVERT: OFF</button>
      </div>
      <div class="cal-slot" id="cal-slot-2" data-cal-slot-card="2">
        <div class="cal-slot-head"><span>MOTOR 3</span><span class="cal-slot-status" data-cal-status="2"></span></div>
        <div class="cal-spin-row">
          <button data-cal-slot="2" data-cal-dir="payout" aria-label="Hold to spin motor 3 out">↑ OUT</button>
          <button data-cal-slot="2" data-cal-dir="retrieve" aria-label="Hold to spin motor 3 in">↓ IN</button>
        </div>
        <div class="cal-corner-row" data-cal-corner-group="2">
          <button type="button" data-corner="front-left">FL</button>
          <button type="button" data-corner="front-right">FR</button>
          <button type="button" data-corner="rear-left">RL</button>
          <button type="button" data-corner="rear-right">RR</button>
        </div>
        <button type="button" class="cal-invert" data-cal-invert="2">INVERT: OFF</button>
      </div>
      <div class="cal-slot" id="cal-slot-3" data-cal-slot-card="3">
        <div class="cal-slot-head"><span>MOTOR 4</span><span class="cal-slot-status" data-cal-status="3"></span></div>
        <div class="cal-spin-row">
          <button data-cal-slot="3" data-cal-dir="payout" aria-label="Hold to spin motor 4 out">↑ OUT</button>
          <button data-cal-slot="3" data-cal-dir="retrieve" aria-label="Hold to spin motor 4 in">↓ IN</button>
        </div>
        <div class="cal-corner-row" data-cal-corner-group="3">
          <button type="button" data-corner="front-left">FL</button>
          <button type="button" data-corner="front-right">FR</button>
          <button type="button" data-corner="rear-left">RL</button>
          <button type="button" data-corner="rear-right">RR</button>
        </div>
        <button type="button" class="cal-invert" data-cal-invert="3">INVERT: OFF</button>
      </div>
    </div>
    <div class="cal-actions">
      <button type="button" id="cal-save" disabled>SAVE CALIBRATION</button>
      <button type="button" id="cal-reset">RESET TO DEFAULT</button>
    </div>
    <p id="cal-message" role="status"></p>
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
  const verticalMotion = document.querySelector('#vertical-motion');
  const speedInput = document.querySelector('#speed');
  const speedValue = document.querySelector('#speed-value');
  const uptimeValue = document.querySelector('#uptime');
  const clientsValue = document.querySelector('#clients');
  const lastCommandValue = document.querySelector('#last-command');
  const failsafeValue = document.querySelector('#failsafe');
  const motorCards = {
    frontLeft: document.querySelector('#card-fl'),
    frontRight: document.querySelector('#card-fr'),
    rearLeft: document.querySelector('#card-rl'),
    rearRight: document.querySelector('#card-rr'),
  };
  let deadZone = .12; // Overwritten from the server's deadZonePercent once status is known.
  let heldMotion = null;
  let joystickPointer = null;
  let joystickX = 0;
  let joystickY = 0;
  let heartbeat = null;
  let lastDriveSent = 0;
  let requestSequence = 0;
  let statusInFlight = false;
  let speedDragging = false;
  const calCorners = ['front-left', 'front-right', 'rear-left', 'rear-right'];
  const calCorner = [null, null, null, null]; // per motor slot; null = not yet labelled
  const calInverted = [false, false, false, false];
  let calSyncedFromServer = false;
  let audioCtx = null;

  // Confirms a press without needing to look at the screen. navigator.vibrate
  // is unsupported on iOS Safari (Apple has never implemented the Vibration
  // API there), so a very short, quiet click is also played through Web
  // Audio — that part works everywhere, including iPhones. Must be called
  // from inside a user-gesture handler (click/pointerdown) for the audio to
  // be allowed to play.
  function buzz(strong = false) {
    try { navigator.vibrate && navigator.vibrate(strong ? [18, 40, 18] : 12); } catch (error) { /* no-op */ }
    try {
      audioCtx = audioCtx || new (window.AudioContext || window.webkitAudioContext)();
      if (audioCtx.state === 'suspended') audioCtx.resume();
      const duration = strong ? .13 : .05;
      const osc = audioCtx.createOscillator();
      const gain = audioCtx.createGain();
      osc.frequency.value = strong ? 880 : 660;
      gain.gain.setValueAtTime(strong ? .14 : .05, audioCtx.currentTime);
      gain.gain.exponentialRampToValueAtTime(.0001, audioCtx.currentTime + duration);
      osc.connect(gain).connect(audioCtx.destination);
      osc.start();
      osc.stop(audioCtx.currentTime + duration);
    } catch (error) { /* no-op */ }
  }

  // Distinguishes "server responded with an error" (show its message) from
  // a network failure (show the generic disconnected state).
  class ServerError extends Error {}

  async function post(path, body = '') {
    const sequence = ++requestSequence;
    try {
      const response = await fetch(path, {method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body,cache:'no-store'});
      const data = await response.json();
      if (!response.ok) throw new ServerError(data.error || response.statusText);
      if (sequence === requestSequence) render(data);
      return true;
    } catch (error) {
      if (sequence !== requestSequence) return false;
      clearMotionState();
      connection.textContent = 'Disconnected';
      connection.className = 'pill offline';
      state.textContent = error instanceof ServerError
        ? error.message.toUpperCase()
        : 'CONNECTION LOST — AUTO STOP';
      return false;
    }
  }

  function render(data) {
    connection.textContent = 'ONLINE';
    connection.className = 'pill online';
    mode.textContent = data.testMode ? 'TEST MODE' : 'HARDWARE';
    if (Number.isFinite(data.deadZonePercent)) deadZone = data.deadZonePercent / 100;
    state.textContent = data.estop ? 'EMERGENCY STOP LATCHED' :
      data.motion === 'joystick' ? `JOYSTICK  X ${data.x} · Y ${data.y}` :
      data.motion === 'all-payout' ? 'TETHERS: ALL PAYING OUT' :
      data.motion === 'all-retrieve' ? 'TETHERS: ALL TAKING IN' :
      data.motion.replace(/-/g, ' ').toUpperCase();
    reset.hidden = !data.estop;
    renderAttitude(data);
    renderTelemetry(data);
    renderCalibration(data);
  }

  function formatUptime(ms) {
    if (!Number.isFinite(ms)) return '0s';
    const totalSeconds = Math.floor(ms / 1000);
    const hours = Math.floor(totalSeconds / 3600);
    const minutes = Math.floor((totalSeconds % 3600) / 60);
    const seconds = totalSeconds % 60;
    return hours > 0 ? `${hours}h ${minutes}m` : minutes > 0 ? `${minutes}m ${seconds}s` : `${seconds}s`;
  }

  function motorCardText(power) {
    if (!Number.isFinite(power) || power === 0) return ['STOPPED', 'motor-stopped'];
    return power > 0 ? [`IN ${power}%`, 'motor-in'] : [`OUT ${Math.abs(power)}%`, 'motor-out'];
  }

  function renderTelemetry(data) {
    if (Number.isFinite(data.maxSpeedPercent)) speedInput.max = data.maxSpeedPercent;
    if (Number.isFinite(data.minSpeedPercent)) speedInput.min = data.minSpeedPercent;
    if (Number.isFinite(data.speedPercent)) {
      speedValue.textContent = `${data.speedPercent}%`;
      if (!speedDragging) speedInput.value = data.speedPercent;
    }
    uptimeValue.textContent = formatUptime(data.uptimeMs);
    if (Number.isFinite(data.clients)) clientsValue.textContent = data.clients;
    if (data.lastCommand) lastCommandValue.textContent = data.lastCommand.replace(/-/g, ' ').toUpperCase();
    if (data.lastSafetyEvent) failsafeValue.textContent = data.lastSafetyEvent.toUpperCase();
    if (data.motorPower) {
      for (const key of ['frontLeft', 'frontRight', 'rearLeft', 'rearRight']) {
        const card = motorCards[key];
        const [text, cls] = motorCardText(data.motorPower[key]);
        card.querySelector('strong').textContent = text;
        card.classList.remove('motor-in', 'motor-out', 'motor-stopped');
        card.classList.add(cls);
      }
    }
  }

  function updateCalUi() {
    for (let slot = 0; slot < 4; slot++) {
      document.querySelectorAll(`[data-cal-corner-group="${slot}"] [data-corner]`).forEach(button => {
        button.classList.toggle('selected', calCorner[slot] === button.dataset.corner);
      });
      const invertButton = document.querySelector(`[data-cal-invert="${slot}"]`);
      invertButton.textContent = `INVERT: ${calInverted[slot] ? 'ON' : 'OFF'}`;
      invertButton.classList.toggle('active', calInverted[slot]);
    }
    document.querySelector('#cal-save').disabled = calCorner.some(corner => corner === null);
  }

  function setCalCorner(slot, corner) {
    for (let i = 0; i < 4; i++) if (i !== slot && calCorner[i] === corner) calCorner[i] = null;
    calCorner[slot] = corner;
    updateCalUi();
  }

  const cornerAbbrev = {'front-left': 'FL', 'front-right': 'FR', 'rear-left': 'RL', 'rear-right': 'RR'};

  function renderCalibration(data) {
    if (Array.isArray(data.calibration)) {
      if (!calSyncedFromServer) {
        data.calibration.forEach((slot, index) => {
          calCorner[index] = slot.corner;
          calInverted[index] = !!slot.inverted;
        });
        calSyncedFromServer = true;
        updateCalUi();
      }
      // Reflects what's actually active on the controller right now, not
      // the (possibly still unsaved) picks being made below.
      document.querySelector('#cal-active-summary').textContent = data.calibration
        .map((slot, index) => `M${index + 1}→${cornerAbbrev[slot.corner] || '?'}${slot.inverted ? ' (INV)' : ''}`)
        .join('  ');
    }
    const spinningSlot = Number.isInteger(data.calibrationSlot) ? data.calibrationSlot : -1;
    for (let slot = 0; slot < 4; slot++) {
      const card = document.querySelector(`[data-cal-slot-card="${slot}"]`);
      const status = document.querySelector(`[data-cal-status="${slot}"]`);
      const spinning = slot === spinningSlot;
      card.classList.toggle('spinning', spinning);
      status.textContent = spinning ? `SPINNING · ${data.calibrationDirection === 'payout' ? 'OUT' : 'IN'}` : '';
    }
  }

  function renderAttitude(data) {
    if (!Number.isFinite(data.pitch) || !Number.isFinite(data.roll)) return;
    const levelButton = document.querySelector('#set-level');
    levelButton.disabled = !data.attitudeConnected && !data.attitudeDemo;
    if (!data.attitudeConnected && !data.attitudeDemo) {
      tiltStatus.textContent = 'IMU OFFLINE';
      tiltStatus.className = 'tilt-red';
      verticalMotion.textContent = '— MOTION';
      verticalMotion.className = 'motion-steady';
      return;
    }
    const pitch = data.pitch;
    const roll = data.roll;
    // Pitch is positive when the nose rises. Keep the established horizon
    // animation direction while presenting that conventional numeric sign.
    const pitchOffset = pitch * 2;
    horizonWorld.style.transform = `translate(-50%,calc(-50% + ${pitchOffset}px)) rotate(${-roll}deg)`;
    pitchValue.textContent = `${pitch.toFixed(1)}°`;
    rollValue.textContent = `${roll.toFixed(1)}°`;
    const tilt = Math.max(Math.abs(pitch), Math.abs(roll));
    const level = tilt < 5 ? ['LEVEL','tilt-green'] : tilt <= 10 ? ['CAUTION','tilt-amber'] : ['HIGH TILT','tilt-red'];
    tiltStatus.textContent = level[0];
    tiltStatus.className = level[1];
    const motion = data.verticalMotion === 'rising'
      ? ['↑ RISING','motion-rising']
      : data.verticalMotion === 'falling'
        ? ['↓ FALLING','motion-falling']
        : ['● STEADY','motion-steady'];
    verticalMotion.textContent = motion[0];
    verticalMotion.className = motion[1];
    document.querySelector('.horizon').setAttribute('aria-label', `Pitch ${pitch.toFixed(1)} degrees, roll ${roll.toFixed(1)} degrees`);
  }

  async function refreshStatus() {
    if (document.hidden || statusInFlight) return;
    statusInFlight = true;
    try {
      const response = await fetch('/api/status', {cache:'no-store'});
      const data = await response.json();
      if (!response.ok) throw new ServerError(data.error || response.statusText);
      render(data);
    } catch (error) {
      connection.textContent = 'Disconnected';
      connection.className = 'pill offline';
      if (error instanceof ServerError) state.textContent = error.message.toUpperCase();
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

  function bindHold(selector, buildRequest) {
    // Buttons use touch-action: manipulation (not none) so a swipe that
    // starts on one can still scroll the page instead of being trapped by
    // it. That means a scroll swipe's pointerdown looks identical to a hold
    // at first contact, so committing to a motor command is delayed briefly
    // and cancelled if the touch moves — a real hold sits still, a scroll
    // swipe doesn't.
    const armDelayMs = 90;
    const moveCancelPx = 10;
    document.querySelectorAll(selector).forEach(button => {
      const send = () => post(...buildRequest(button));
      let armTimer = null;
      let startX = 0;
      let startY = 0;
      let confirmed = false;

      const disarm = () => {
        if (armTimer) clearTimeout(armTimer);
        armTimer = null;
        confirmed = false;
      };

      button.addEventListener('pointerdown', event => {
        endMotion();
        startX = event.clientX;
        startY = event.clientY;
        button.setPointerCapture(event.pointerId);
        armTimer = setTimeout(() => {
          confirmed = true;
          heldMotion = true;
          button.classList.add('active');
          buzz();
          send();
          heartbeat = setInterval(send, 250);
        }, armDelayMs);
      });
      button.addEventListener('pointermove', event => {
        if (confirmed) return;
        if (Math.hypot(event.clientX - startX, event.clientY - startY) > moveCancelPx) disarm();
      });
      button.addEventListener('pointerup', () => { disarm(); endMotion(); });
      button.addEventListener('pointercancel', () => { disarm(); endMotion(); });
      button.addEventListener('lostpointercapture', () => { disarm(); endMotion(); });
    });
  }

  bindHold('[data-motion]', button => ['/api/move', 'direction=' + encodeURIComponent(button.dataset.motion)]);
  bindHold('[data-cal-dir]', button => ['/api/calibration/spin', `slot=${button.dataset.calSlot}&direction=${button.dataset.calDir}`]);

  document.querySelectorAll('[data-tab]').forEach(tab => {
    tab.addEventListener('click', () => {
      buzz();
      endMotion();
      const selected = tab.dataset.tab;
      document.querySelector('#drive-controls').hidden = selected !== 'drive';
      document.querySelector('#winch-controls').hidden = selected !== 'winches';
      document.querySelector('#calibration-controls').hidden = selected !== 'calibration';
      document.querySelectorAll('[data-tab]').forEach(item => {
        const active = item === tab;
        item.classList.toggle('selected', active);
        item.setAttribute('aria-selected', active ? 'true' : 'false');
      });
    });
  });

  document.querySelectorAll('[data-cal-corner-group]').forEach(group => {
    const slot = Number(group.dataset.calCornerGroup);
    group.querySelectorAll('[data-corner]').forEach(button => {
      button.addEventListener('click', () => { buzz(); setCalCorner(slot, button.dataset.corner); });
    });
  });

  document.querySelectorAll('[data-cal-invert]').forEach(button => {
    const slot = Number(button.dataset.calInvert);
    button.addEventListener('click', () => {
      buzz();
      calInverted[slot] = !calInverted[slot];
      updateCalUi();
    });
  });

  document.querySelector('#cal-save').addEventListener('click', async () => {
    buzz();
    const message = document.querySelector('#cal-message');
    if (calCorner.some(corner => corner === null)) {
      message.textContent = 'Label all four motors before saving.';
      return;
    }
    const params = new URLSearchParams();
    calCorner.forEach((corner, slot) => {
      params.set(`slot${slot}corner`, corner);
      params.set(`slot${slot}inverted`, calInverted[slot] ? '1' : '0');
    });
    message.textContent = 'Saving…';
    const ok = await post('/api/calibration/save', params.toString());
    message.textContent = ok ? 'Calibration saved.' : 'Save failed — each corner must be used exactly once.';
  });

  document.querySelector('#cal-reset').addEventListener('click', async () => {
    buzz();
    const message = document.querySelector('#cal-message');
    calSyncedFromServer = false;
    const ok = await post('/api/calibration/reset');
    message.textContent = ok ? 'Calibration reset to default wiring order.' : 'Reset failed.';
  });

  joystick.addEventListener('pointerdown', event => {
    if (joystickPointer !== null) return;
    event.preventDefault();
    buzz();
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

  document.querySelector('#stop').addEventListener('click', () => { buzz(); endMotion(false); post('/api/stop'); });
  document.querySelector('#estop').addEventListener('click', () => { buzz(true); endMotion(false); post('/api/estop'); });
  reset.addEventListener('click', () => { buzz(); post('/api/estop/clear'); });
  document.querySelector('#set-level').addEventListener('click', () => { buzz(); post('/api/attitude/level'); });
  speedInput.addEventListener('pointerdown', () => { speedDragging = true; });
  speedInput.addEventListener('input', () => { speedValue.textContent = `${speedInput.value}%`; });
  speedInput.addEventListener('change', () => {
    speedDragging = false;
    post('/api/speed', 'value=' + encodeURIComponent(speedInput.value));
  });
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
