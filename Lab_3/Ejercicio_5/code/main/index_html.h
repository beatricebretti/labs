#pragma once

static const char INDEX_HTML[] = R"HTML(
<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Dashboard Robot Sumo ESP32</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;800&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg-1: #0a0a14;
      --bg-2: #050508;
      --card: rgba(22, 22, 35, 0.55);
      --card-border: rgba(255, 255, 255, 0.06);
      --text: #f0f2f5;
      --muted: #a0a5b5;
      --cyan: #00f0ff;
      --magenta: #ff007f;
      --green: #39ff14;
      --red: #ff3131;
      --shadow: 0 15px 35px rgba(0, 0, 0, 0.5);
      --radius: 20px;
    }

    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      font-family: 'Outfit', sans-serif;
      color: var(--text);
      background: 
        radial-gradient(circle at 10% 20%, rgba(255, 0, 127, 0.08), transparent 40%),
        radial-gradient(circle at 90% 80%, rgba(0, 240, 255, 0.08), transparent 40%),
        var(--bg-1);
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 24px;
    }

    header {
      width: min(1000px, 100%);
      margin-bottom: 24px;
      text-align: center;
    }

    .title-area {
      display: inline-flex;
      align-items: center;
      gap: 12px;
    }

    .pulsing-dot {
      width: 14px;
      height: 14px;
      border-radius: 50%;
      background: var(--green);
      box-shadow: 0 0 15px var(--green);
      animation: pulse 2s infinite;
    }

    @keyframes pulse {
      0% { transform: scale(0.9); opacity: 0.6; }
      50% { transform: scale(1.15); opacity: 1; box-shadow: 0 0 25px var(--green); }
      100% { transform: scale(0.9); opacity: 0.6; }
    }

    h1 {
      margin: 0;
      font-size: clamp(24px, 5vw, 36px);
      font-weight: 800;
      letter-spacing: -0.02em;
      background: linear-gradient(135deg, #ffffff 30%, var(--cyan) 100%);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
    }

    .subtitle {
      margin: 4px 0 0;
      color: var(--muted);
      font-size: 14px;
    }

    .app-grid {
      width: min(1000px, 100%);
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 24px;
    }

    @media (max-width: 860px) {
      .app-grid {
        grid-template-columns: 1fr;
      }
    }

    .panel {
      background: var(--card);
      border: 1px solid var(--card-border);
      border-radius: var(--radius);
      box-shadow: var(--shadow);
      backdrop-filter: blur(16px);
      padding: 24px;
      display: flex;
      flex-direction: column;
      gap: 20px;
    }

    .panel h2 {
      margin: 0;
      font-size: 20px;
      font-weight: 600;
      border-bottom: 1px solid rgba(255, 255, 255, 0.08);
      padding-bottom: 12px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    /* Stats bar */
    .stats-container {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 16px;
    }

    .stat-box {
      background: rgba(255, 255, 255, 0.03);
      border: 1px solid rgba(255, 255, 255, 0.05);
      border-radius: 14px;
      padding: 16px;
      text-align: center;
      transition: all 0.3s ease;
    }

    .stat-box.active-mode {
      border-color: var(--cyan);
      box-shadow: 0 0 10px rgba(0, 240, 255, 0.15);
    }

    .stat-box.danger-active {
      border-color: var(--red);
      box-shadow: 0 0 15px rgba(255, 49, 49, 0.25);
      animation: flash-border 1s infinite alternate;
    }

    @keyframes flash-border {
      from { border-color: rgba(255, 49, 49, 0.3); }
      to { border-color: var(--red); }
    }

    .stat-title {
      font-size: 11px;
      text-transform: uppercase;
      color: var(--muted);
      margin-bottom: 6px;
      letter-spacing: 0.1em;
    }

    .stat-val {
      font-size: 20px;
      font-weight: 800;
    }

    /* Mode Selection */
    .mode-switch-container {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
      background: rgba(0, 0, 0, 0.2);
      padding: 6px;
      border-radius: 14px;
      border: 1px solid rgba(255, 255, 255, 0.04);
    }

    .mode-btn {
      border: 0;
      padding: 12px 8px;
      border-radius: 10px;
      font-family: inherit;
      font-weight: 600;
      font-size: 14px;
      cursor: pointer;
      color: var(--muted);
      background: transparent;
      transition: all 0.2s ease;
    }

    .mode-btn.active {
      color: #fff;
      box-shadow: 0 4px 12px rgba(0,0,0,0.35);
    }

    .mode-btn.active.manual {
      background: linear-gradient(135deg, rgba(0, 240, 255, 0.3) 0%, rgba(0, 240, 255, 0.15) 100%);
      border: 1px solid var(--cyan);
      text-shadow: 0 0 5px var(--cyan);
    }

    .mode-btn.active.auto {
      background: linear-gradient(135deg, rgba(255, 0, 127, 0.3) 0%, rgba(255, 0, 127, 0.15) 100%);
      border: 1px solid var(--magenta);
      text-shadow: 0 0 5px var(--magenta);
    }

    /* D-PAD Manual controls */
    .controls-wrapper {
      display: flex;
      flex-direction: column;
      gap: 16px;
      transition: opacity 0.3s ease, filter 0.3s ease;
    }

    .controls-disabled {
      opacity: 0.25;
      pointer-events: none;
      filter: blur(1px);
    }

    .dpad {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 12px;
      align-items: center;
      justify-items: center;
      margin: 10px auto;
      max-width: 300px;
    }

    .control-btn, .stop-btn {
      border: 0;
      border-radius: 16px;
      color: var(--text);
      background: linear-gradient(180deg, rgba(255,255,255,0.08), rgba(255,255,255,0.03));
      border: 1px solid rgba(255,255,255,0.06);
      cursor: pointer;
      transition: all 0.1s ease;
      user-select: none;
      -webkit-tap-highlight-color: transparent;
    }

    .control-btn {
      width: 84px;
      height: 84px;
      font-size: 13px;
      font-weight: 600;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      gap: 6px;
    }

    .control-btn svg {
      width: 24px;
      height: 24px;
      fill: currentColor;
    }

    .control-btn:hover {
      background: rgba(255,255,255,0.1);
    }

    .control-btn:active, .control-btn.active {
      transform: scale(0.96);
      border-color: var(--cyan);
      background: linear-gradient(180deg, rgba(0, 240, 255, 0.2), rgba(0, 240, 255, 0.05));
      box-shadow: 0 0 15px rgba(0, 240, 255, 0.3);
    }

    .stop-btn {
      width: 84px;
      height: 84px;
      background: linear-gradient(180deg, rgba(255, 49, 49, 0.15), rgba(255, 49, 49, 0.05));
      border-color: rgba(255, 49, 49, 0.3);
      font-weight: 800;
      color: #ffcccc;
      font-size: 15px;
      box-shadow: 0 4px 10px rgba(0,0,0,0.2);
    }

    .stop-btn:active {
      transform: scale(0.96);
      border-color: var(--red);
      background: linear-gradient(180deg, rgba(255, 49, 49, 0.35), rgba(255, 49, 49, 0.15));
      box-shadow: 0 0 15px rgba(255, 49, 49, 0.4);
    }

    .dpad-empty {
      width: 84px;
      height: 84px;
    }

    /* Speed Slider */
    .slider-card {
      padding: 16px;
      border-radius: 16px;
      background: rgba(255,255,255,0.02);
      border: 1px solid rgba(255,255,255,0.05);
    }

    .slider-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-bottom: 10px;
    }

    .speed-pill {
      background: rgba(0, 240, 255, 0.1);
      color: var(--cyan);
      border: 1px solid rgba(0, 240, 255, 0.3);
      padding: 4px 10px;
      border-radius: 12px;
      font-weight: 800;
      font-size: 13px;
    }

    input[type="range"] {
      width: 100%;
      height: 6px;
      background: rgba(255,255,255,0.1);
      outline: none;
      border-radius: 99px;
      accent-color: var(--cyan);
      cursor: pointer;
    }

    /* Camera Simulation Panel */
    .sim-buttons {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 10px;
    }

    .sim-btn {
      padding: 12px;
      border-radius: 12px;
      font-family: inherit;
      font-weight: 600;
      font-size: 13px;
      border: 1px solid rgba(255,255,255,0.08);
      background: rgba(255,255,255,0.03);
      color: var(--text);
      cursor: pointer;
      transition: all 0.2s ease;
    }

    .sim-btn:hover {
      background: rgba(255,255,255,0.08);
    }

    .sim-btn.active {
      background: rgba(255, 255, 255, 0.08);
      border-color: var(--magenta);
      box-shadow: 0 0 10px rgba(255, 0, 127, 0.2);
      color: #fff;
    }

    .sim-btn.active[data-border="none"] { border-color: var(--green); box-shadow: 0 0 10px rgba(57, 255, 20, 0.2); }

    /* Grayscale Camera Canvas Visualizer */
    .cam-viewer-wrapper {
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      margin-top: 10px;
    }

    .cam-screen-border {
      padding: 8px;
      background: #11111a;
      border: 3px solid rgba(255, 255, 255, 0.15);
      border-radius: 12px;
      box-shadow: 0 8px 25px rgba(0,0,0,0.6);
      transition: border-color 0.2s ease, box-shadow 0.2s ease;
      position: relative;
    }

    .cam-screen-border.alert {
      border-color: var(--red);
      box-shadow: 0 0 25px rgba(255, 49, 49, 0.5);
      animation: alert-flash 0.5s infinite alternate;
    }

    @keyframes alert-flash {
      from { border-color: rgba(255, 49, 49, 0.3); }
      to { border-color: var(--red); }
    }

    .cam-screen {
      width: 192px;  /* 96 * 2 for visibility */
      height: 192px;
      background: #ffffff;
      border-radius: 4px;
      position: relative;
      overflow: hidden;
      transition: background 0.3s ease;
    }

    .camera-overlay-line {
      position: absolute;
      background: #000;
      transition: all 0.2s ease;
    }

    .overlay-front {
      top: 0; left: 0; width: 100%; height: 48px; /* 24px * 2 */
      opacity: 0;
    }
    .overlay-left {
      top: 0; left: 0; width: 48px; height: 100%;
      opacity: 0;
    }
    .overlay-right {
      top: 0; right: 0; width: 48px; height: 100%;
      opacity: 0;
    }

    .cam-screen.front .overlay-front { opacity: 1; }
    .cam-screen.left .overlay-left { opacity: 1; }
    .cam-screen.right .overlay-right { opacity: 1; }
    .cam-screen.none { background: #fff; }

    .cam-label {
      font-size: 11px;
      text-transform: uppercase;
      letter-spacing: 0.1em;
      color: var(--muted);
      margin-top: 10px;
      text-align: center;
      display: flex;
      align-items: center;
      gap: 6px;
    }

    .cam-indicator {
      display: inline-block;
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: var(--muted);
    }
    .cam-indicator.active {
      background: var(--red);
      box-shadow: 0 0 8px var(--red);
      animation: pulse 1s infinite;
    }

    /* Key bindings cheat sheet */
    .cheat-sheet {
      text-align: center;
      font-size: 12px;
      color: var(--muted);
      margin-top: 24px;
      width: min(1000px, 100%);
      padding: 12px;
      background: rgba(255,255,255,0.02);
      border-radius: 12px;
      border: 1px solid rgba(255,255,255,0.04);
    }
    .kbd {
      background: rgba(255,255,255,0.08);
      border: 1px solid rgba(255,255,255,0.12);
      border-radius: 5px;
      padding: 2px 6px;
      font-size: 11px;
      margin: 0 2px;
    }
  </style>
</head>
<body>

  <header>
    <div class="title-area">
      <div class="pulsing-dot"></div>
      <h1>Sumo Robot ESP32</h1>
    </div>
    <p class="subtitle">Laboratorio 3 - Ejercicio 5 (Simulación de Evasión de Bordes)</p>
  </header>

  <main class="app-grid">
    
    <!-- LEFT PANEL: State, Modes, and D-Pad -->
    <div class="panel">
      <h2>Estado del Robot</h2>
      
      <div class="stats-container">
        <div class="stat-box" id="mode-box">
          <div class="stat-title">Modo de Operación</div>
          <div class="stat-val" id="status-mode" style="color: var(--cyan);">MANUAL</div>
        </div>
        <div class="stat-box" id="command-box">
          <div class="stat-title">Motores</div>
          <div class="stat-val" id="status-command" style="font-weight: 800;">STOP</div>
        </div>
        <div class="stat-box" id="speed-box">
          <div class="stat-title">Velocidad Base</div>
          <div class="stat-val" id="status-speed">70%</div>
        </div>
        <div class="stat-box" id="border-box">
          <div class="stat-title">Borde Ring</div>
          <div class="stat-val" id="status-border" style="color: var(--green);">SEGURO</div>
        </div>
      </div>

      <div class="mode-switch-container">
        <button class="mode-btn manual active" id="btn-manual-mode" onclick="setMode('manual')">Modo Manual</button>
        <button class="mode-btn auto" id="btn-auto-mode" onclick="setMode('auto')">Modo Sumo (Auto)</button>
      </div>

      <div class="controls-wrapper" id="manual-controls">
        <h3 style="margin: 5px 0 0; font-size: 14px; font-weight: 600; color: var(--muted); text-align: center;">Control de Conducción Manual</h3>
        
        <div class="dpad">
          <div class="dpad-empty"></div>
          <button class="control-btn" data-command="forward" aria-label="Avanzar">
            <svg viewBox="0 0 24 24"><path d="M12 4 3 15h6v5h6v-5h6z"/></svg>
            Adelante
          </button>
          <div class="dpad-empty"></div>

          <button class="control-btn" data-command="left" aria-label="Girar Izquierda">
            <svg viewBox="0 0 24 24"><path d="M4 12 15 3v6h5v6h-5v6z"/></svg>
            Izquierda
          </button>
          <button class="stop-btn" id="stop-btn" aria-label="Detener">STOP</button>
          <button class="control-btn" data-command="right" aria-label="Girar Derecha">
            <svg viewBox="0 0 24 24"><path d="m20 12-11 9v-6H4V9h5V3z"/></svg>
            Derecha
          </button>

          <div class="dpad-empty"></div>
          <button class="control-btn" data-command="backward" aria-label="Retroceder">
            <svg viewBox="0 0 24 24"><path d="M12 20 21 9h-6V4H9v5H3z"/></svg>
            Atrás
          </button>
          <div class="dpad-empty"></div>
        </div>

        <div class="slider-card">
          <div class="slider-row">
            <strong style="font-size: 14px;">Potencia Motores</strong>
            <div class="speed-pill" id="speed-pill">70%</div>
          </div>
          <input id="speed-slider" type="range" min="30" max="100" value="70" />
        </div>
      </div>
    </div>

    <!-- RIGHT PANEL: Sensor Simulation & Visualizer -->
    <div class="panel">
      <h2>Simulador de Cámara (Capa de Sensores)</h2>
      
      <p style="margin: 0; color: var(--muted); font-size: 13.5px; line-height: 1.5;">
        Haz clic en los botones para simular qué ve la cámara del robot en tiempo real. 
        En <strong>Modo Sumo (Auto)</strong>, el robot reaccionará automáticamente evadiendo el borde simulado.
      </p>

      <div class="sim-buttons">
        <button class="sim-btn active" id="sim-none" data-border="none" onclick="injectSimulation('none')">✓ Piso Blanco (Seguro)</button>
        <button class="sim-btn" id="sim-front" data-border="front" onclick="injectSimulation('front')">⚠ Borde al Frente</button>
        <button class="sim-btn" id="sim-left" data-border="left" onclick="injectSimulation('left')">⚠ Borde a la Izquierda</button>
        <button class="sim-btn" id="sim-right" data-border="right" onclick="injectSimulation('right')">⚠ Borde a la Derecha</button>
      </div>

      <div class="cam-viewer-wrapper">
        <div class="cam-screen-border" id="cam-border-element">
          <!-- virtual 96x96 camera buffer visualizer -->
          <div class="cam-screen none" id="cam-screen-element">
            <div class="camera-overlay-line overlay-front"></div>
            <div class="camera-overlay-line overlay-left"></div>
            <div class="camera-overlay-line overlay-right"></div>
          </div>
        </div>
        <div class="cam-label">
          <span class="cam-indicator" id="cam-alert-indicator"></span>
          <span>Buffer Cámara Simulado (96x96 Gray)</span>
        </div>
      </div>
      
      <div style="background: rgba(0,0,0,0.15); padding: 12px; border-radius: 12px; border: 1px solid rgba(255,255,255,0.03); font-size: 13px;">
        <span style="font-weight: 600; color: var(--magenta);">Lógica de Detección:</span> 
        Si el algoritmo detecta que en alguna de las zonas de monitoreo de la matriz se reduce el brillo promedio (píxeles oscuros &lt; umbral), se activa la secuencia de seguridad en los motores.
      </div>
    </div>

  </main>

  <div class="cheat-sheet">
    <span>💡 <strong>Teclas rápidas (Modo Manual):</strong> Usa las flechas <kbd>▲</kbd> <kbd>▼</kbd> <kbd>◀</kbd> <kbd>▶</kbd> para conducir. Suelta la tecla o presiona <kbd>Espacio</kbd> para detener el vehículo.</span>
  </div>

  <script>
    const commandEl = document.getElementById('status-command');
    const speedEl = document.getElementById('status-speed');
    const speedPill = document.getElementById('speed-pill');
    const modeEl = document.getElementById('status-mode');
    const borderEl = document.getElementById('status-border');
    const slider = document.getElementById('speed-slider');
    const stopBtn = document.getElementById('stop-btn');
    const modeBox = document.getElementById('mode-box');
    const borderBox = document.getElementById('border-box');
    
    const manualControlsWrapper = document.getElementById('manual-controls');
    const btnManualMode = document.getElementById('btn-manual-mode');
    const btnAutoMode = document.getElementById('btn-auto-mode');

    const camScreen = document.getElementById('cam-screen-element');
    const camBorder = document.getElementById('cam-border-element');
    const camIndicator = document.getElementById('cam-alert-indicator');

    const controlButtons = [...document.querySelectorAll('.control-btn')];
    const simButtons = [...document.querySelectorAll('.sim-btn')];

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
        updateUI(data);
      } catch (err) {
        console.error("API Error:", err);
      } finally {
        if (path !== '/api/status') isSending = false;
      }
    }

    function updateUI(data) {
      if (!data) return;

      // Update motor command
      activeCommand = data.command || 'stop';
      commandEl.textContent = activeCommand.toUpperCase();
      if (activeCommand === 'stop') {
        commandEl.style.color = '#fff';
      } else {
        commandEl.style.color = 'var(--cyan)';
      }

      // Update speed
      const speed = Number(data.speed_percent || slider.value);
      slider.value = speed;
      speedEl.textContent = `${speed}%`;
      speedPill.textContent = `${speed}%`;

      // Update mode
      if (data.mode === 'auto') {
        modeEl.textContent = 'SUMO (AUTO)';
        modeEl.style.color = 'var(--magenta)';
        modeBox.classList.add('active-mode');
        
        // Disable manual inputs visually
        manualControlsWrapper.classList.add('controls-disabled');
        btnManualMode.classList.remove('active');
        btnAutoMode.classList.add('active');
      } else {
        modeEl.textContent = 'MANUAL';
        modeEl.style.color = 'var(--cyan)';
        modeBox.classList.remove('active-mode');

        // Enable manual inputs
        manualControlsWrapper.classList.remove('controls-disabled');
        btnManualMode.classList.add('active');
        btnAutoMode.classList.remove('active');
      }

      // Update simulated screen and border flags
      const simState = data.simulation || 'none';
      simButtons.forEach(btn => {
        btn.classList.toggle('active', btn.dataset.border === simState);
      });

      // Update virtual camera view class
      camScreen.className = `cam-screen ${simState}`;

      // Update border detection state
      if (data.border_detected) {
        borderEl.textContent = `EVASIÓN (${data.detected_zone.toUpperCase()})`;
        borderEl.style.color = 'var(--red)';
        borderBox.classList.add('danger-active');
        camBorder.classList.add('alert');
        camIndicator.classList.add('active');
      } else {
        borderEl.textContent = 'SEGURO';
        borderEl.style.color = 'var(--green)';
        borderBox.classList.remove('danger-active');
        camBorder.classList.remove('alert');
        camIndicator.classList.remove('active');
      }

      // Update button active states
      controlButtons.forEach(btn => {
        btn.classList.toggle('active', btn.dataset.command === activeCommand);
      });
    }

    async function setMode(mode) {
      await api(`/api/mode/${mode}`);
    }

    async function injectSimulation(border) {
      await api(`/api/simulate/${border}`);
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
      speedPill.textContent = `${value}%`;
      clearTimeout(speedTimer);
      speedTimer = setTimeout(() => api(`/api/speed/${value}`), 120);
    });

    // Keyboard bindings for manual driving
    const keyMap = {
      ArrowUp: 'forward',
      ArrowDown: 'backward',
      ArrowLeft: 'left',
      ArrowRight: 'right'
    };
    const pressed = new Set();

    window.addEventListener('keydown', (e) => {
      // Ignore if auto mode
      if (modeEl.textContent.includes('SUMO')) return;
      
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
      if (modeEl.textContent.includes('SUMO')) return;
      if (keyMap[e.key]) {
        pressed.delete(e.key);
        api('/api/stop');
      }
    });

    // Poll status every 400ms for responsiveness
    setInterval(() => api('/api/status'), 400);
    api('/api/status');
  </script>
</body>
</html>
)HTML";
