#pragma once

static const char INDEX_HTML[] = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>ESP32 Control Autito</title>
  <style>
    :root {
      --bg-1: #070707;
      --bg-2: #030303;
      --card: rgba(91, 87, 89, 0.246);
      --card-border: rgba(255,255,255,0.14);
      --text: #e5eefc;
      --muted: #eecdde;
      --accent: #f293bb;
      --accent-2: #e271d8;
      --danger: #eb5c7e;
      --warning: #f2ca37;
      --shadow: 0 20px 50px rgba(0,0,0,0.35);
      --radius: 22px;
    }

    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      color: var(--text);
      background:
        radial-gradient(circle at top left, rgba(19, 20, 20, 0.12), transparent 30%),
        radial-gradient(circle at top right, rgba(17, 17, 17, 0.1), transparent 25%),
        linear-gradient(160deg, var(--bg-1), var(--bg-2));
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }

    .app {
      width: min(980px, 100%);
      display: grid;
      grid-template-columns: 1.15fr 0.85fr;
      gap: 20px;
    }

    .panel {
      background: var(--card);
      border: 1px solid var(--card-border);
      border-radius: var(--radius);
      box-shadow: var(--shadow);
      backdrop-filter: blur(14px);
      overflow: hidden;
    }

    .hero {
      padding: 26px;
      position: relative;
    }

    .badge {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 8px 12px;
      border-radius: 999px;
      background: rgba(56,189,248,0.12);
      color: #bfe9ff;
      font-size: 13px;
      margin-bottom: 14px;
    }

    .dot {
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: var(--accent-2);
      box-shadow: 0 0 14px rgba(34,197,94,0.8);
    }

    h1 {
      margin: 0 0 10px;
      font-size: clamp(28px, 4vw, 42px);
      line-height: 1.05;
      letter-spacing: -0.03em;
    }

    .subtitle {
      margin: 0;
      color: var(--muted);
      font-size: 15px;
      line-height: 1.6;
      max-width: 58ch;
    }

    .stats {
      margin-top: 22px;
      display: grid;
      grid-template-columns: repeat(3, minmax(0,1fr));
      gap: 14px;
    }

    .stat {
      padding: 16px;
      border-radius: 18px;
      background: rgba(255,255,255,0.05);
      border: 1px solid rgba(255,255,255,0.08);
    }

    .stat-label {
      font-size: 12px;
      text-transform: uppercase;
      letter-spacing: 0.12em;
      color: var(--muted);
      margin-bottom: 8px;
    }

    .stat-value {
      font-size: 18px;
      font-weight: 700;
    }

    .control-panel {
      padding: 26px;
      display: flex;
      flex-direction: column;
      gap: 18px;
    }

    .panel-title {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 12px;
    }

    .panel-title h2 {
      margin: 0;
      font-size: 22px;
    }

    .hint {
      color: var(--muted);
      font-size: 13px;
    }

    .dpad {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 14px;
      align-items: center;
      justify-items: center;
      margin-top: 4px;
    }

    .control-btn,
    .stop-btn,
    .quick-btn {
      border: 0;
      border-radius: 18px;
      color: var(--text);
      background: linear-gradient(180deg, rgba(255,255,255,0.12), rgba(255,255,255,0.06));
      border: 1px solid rgba(255,255,255,0.10);
      box-shadow: var(--shadow);
      cursor: pointer;
      transition: transform 0.12s ease, border-color 0.12s ease, background 0.12s ease;
      user-select: none;
      -webkit-tap-highlight-color: transparent;
    }

    .control-btn:hover,
    .quick-btn:hover,
    .stop-btn:hover { transform: translateY(-1px); }

    .control-btn:active,
    .quick-btn:active,
    .stop-btn:active,
    .control-btn.active,
    .quick-btn.active {
      transform: translateY(1px) scale(0.99);
      border-color: rgba(77, 206, 231, 0.9);
      background: linear-gradient(180deg, rgba(56,189,248,0.26), rgba(56,189,248,0.14));
    }

    .control-btn {
      width: min(128px, 26vw);
      height: min(128px, 26vw);
      font-size: 16px;
      font-weight: 700;
      display: flex;
      align-items: center;
      justify-content: center;
      flex-direction: column;
      gap: 10px;
    }

    .control-btn svg {
      width: 28px;
      height: 28px;
      fill: currentColor;
    }

    .empty {
      width: min(128px, 26vw);
      height: min(128px, 26vw);
    }

    .stop-btn {
      width: min(128px, 26vw);
      height: min(128px, 26vw);
      background: linear-gradient(180deg, rgba(239, 105, 141, 0.22), rgba(234, 86, 140, 0.12));
      border-color: rgba(239,68,68,0.35);
      font-size: 18px;
      font-weight: 800;
      color: #fee2e2;
    }

    .stop-btn:active {
      border-color: rgba(232, 15, 84, 0.8);
      background: linear-gradient(180deg, rgba(235, 32, 79, 0.32), rgba(232, 46, 114, 0.18));
    }

    .slider-card {
      padding: 18px;
      border-radius: 20px;
      background: rgba(255,255,255,0.05);
      border: 1px solid rgba(255,255,255,0.08);
    }

    .slider-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 12px;
    }

    .speed-pill {
      min-width: 78px;
      text-align: center;
      padding: 8px 12px;
      border-radius: 999px;
      background: rgba(90, 176, 237, 0.6);
      color: #ecdcfc;
      font-weight: 700;
    }

    input[type="range"] {
      width: 100%;
      accent-color: var(--accent);
    }

    .quick-actions {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 12px;
    }

    .quick-btn {
      padding: 14px 12px;
      font-weight: 700;
      font-size: 14px;
    }

    .footer-note {
      font-size: 12px;
      color: var(--muted);
      line-height: 1.6;
      text-align: center;
    }

    .kbd {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      min-width: 28px;
      padding: 4px 8px;
      border-radius: 10px;
      background: rgba(255,255,255,0.08);
      border: 1px solid rgba(255,255,255,0.12);
      font-size: 12px;
      color: var(--text);
      margin: 0 3px;
    }

    @media (max-width: 860px) {
      .app {
        grid-template-columns: 1fr;
      }
      .stats {
        grid-template-columns: 1fr;
      }
      .quick-actions {
        grid-template-columns: 1fr;
      }
    }
  </style>
</head>
<body>
  <main class="app">
    <section class="panel hero">
      <h1>Control Autito ESP32</h1>
      <div class="stats">
        <div class="stat">
          <div class="stat-label">Comando actual</div>
          <div class="stat-value" id="status-command">STOP</div>
        </div>
        <div class="stat">
          <div class="stat-label">Network</div>
          <div class="stat-value" id="status-ssid">ESP32-Autito</div>
        </div>
        <div class="stat">
          <div class="stat-label">Velocidad</div>
          <div class="stat-value" id="status-speed">70%</div>
        </div>
      </div>
    </section>

    <section class="panel control-panel">
      <div class="panel-title">
        <h2>Controlar</h2>
      </div>

      <div class="dpad">
        <div class="empty"></div>
        <button class="control-btn" data-command="forward" aria-label="Forward">
          <svg viewBox="0 0 24 24"><path d="M12 4 3 15h6v5h6v-5h6z"/></svg>
          Adelante
        </button>
        <div class="empty"></div>

        <button class="control-btn" data-command="left" aria-label="Left">
          <svg viewBox="0 0 24 24"><path d="M4 12 15 3v6h5v6h-5v6z"/></svg>
          Izquierda
        </button>
        <button class="stop-btn" id="stop-btn" aria-label="Stop">STOP</button>
        <button class="control-btn" data-command="right" aria-label="Right">
          <svg viewBox="0 0 24 24"><path d="m20 12-11 9v-6H4V9h5V3z"/></svg>
          Derecha
        </button>

        <div class="empty"></div>
        <button class="control-btn" data-command="backward" aria-label="Backward">
          <svg viewBox="0 0 24 24"><path d="M12 20 21 9h-6V4H9v5H3z"/></svg>
          Atrás
        </button>
        <div class="empty"></div>
      </div>

      <div class="slider-card">
        <div class="slider-row">
          <strong>Velocidad del motor</strong>
          <div class="speed-pill" id="speed-pill">70%</div>
        </div>
        <input id="speed-slider" type="range" min="30" max="100" value="70" />
      </div>

      <div class="quick-actions">
        <button class="quick-btn" data-speed="70">Normal</button>
        <button class="quick-btn" data-speed="100">Rápido</button>
      </div>
    </section>
  </main>

  <script>
    const commandEl = document.getElementById('status-command');
    const speedEl = document.getElementById('status-speed');
    const speedPill = document.getElementById('speed-pill');
    const slider = document.getElementById('speed-slider');
    const stopBtn = document.getElementById('stop-btn');
    const controlButtons = [...document.querySelectorAll('.control-btn')];
    const quickButtons = [...document.querySelectorAll('.quick-btn')];

    let activeCommand = 'stop';
    let isSending = false;

    async function api(path) {
      if (isSending && path !== '/api/status') {
        return;
      }
      try {
        if (path !== '/api/status') isSending = true;
        const res = await fetch(path, { method: 'POST' });
        const data = await res.json();
        updateStatus(data);
      } catch (err) {
        console.error(err);
      } finally {
        if (path !== '/api/status') isSending = false;
      }
    }

    function updateStatus(data) {
      if (!data) return;
      activeCommand = data.command || activeCommand;
      commandEl.textContent = (data.command || 'STOP').toUpperCase();
      const speed = Number(data.speed_percent || slider.value);
      slider.value = speed;
      speedEl.textContent = `${speed}%`;
      speedPill.textContent = `${speed}%`;

      controlButtons.forEach(btn => {
        btn.classList.toggle('active', btn.dataset.command === activeCommand);
      });
      quickButtons.forEach(btn => {
        btn.classList.toggle('active', Number(btn.dataset.speed) === speed);
      });
    }

    function bindHold(button, command) {
      const start = (e) => {
        e.preventDefault();
        api(`/api/${command}`);
      };
      const stop = (e) => {
        e.preventDefault();
        api('/api/stop');
      };

      ['pointerdown', 'mousedown', 'touchstart'].forEach(evt => button.addEventListener(evt, start, { passive: false }));
      ['pointerup', 'pointerleave', 'mouseup', 'touchend', 'touchcancel'].forEach(evt => button.addEventListener(evt, stop, { passive: false }));
    }

    controlButtons.forEach(btn => bindHold(btn, btn.dataset.command));
    bindHold(stopBtn, 'stop');

    stopBtn.addEventListener('click', (e) => {
      e.preventDefault();
      api('/api/stop');
    });

    let speedTimer = null;
    slider.addEventListener('input', () => {
      const value = slider.value;
      speedEl.textContent = `${value}%`;
      speedPill.textContent = `${value}%`;
      clearTimeout(speedTimer);
      speedTimer = setTimeout(() => api(`/api/speed/${value}`), 120);
    });

    quickButtons.forEach(btn => {
      btn.addEventListener('click', () => {
        slider.value = btn.dataset.speed;
        speedEl.textContent = `${btn.dataset.speed}%`;
        speedPill.textContent = `${btn.dataset.speed}%`;
        api(`/api/speed/${btn.dataset.speed}`);
      });
    });

    const keyMap = {
      ArrowUp: 'forward',
      ArrowDown: 'backward',
      ArrowLeft: 'left',
      ArrowRight: 'right'
    };
    const pressed = new Set();

    window.addEventListener('keydown', (e) => {
      if (e.code === 'Space') {
        e.preventDefault();
        api('/api/stop');
        return;
      }
      const command = keyMap[e.key];
      if (!command || pressed.has(e.key)) return;
      pressed.add(e.key);
      api(`/api/${command}`);
    });

    window.addEventListener('keyup', (e) => {
      if (keyMap[e.key]) {
        pressed.delete(e.key);
        api('/api/stop');
      }
    });

    setInterval(() => api('/api/status'), 1000);
    api('/api/status');
  </script>
</body>
</html>
)HTML";
