/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "html_shell.h"

#include "echarts_bundle.h"

#include <cmath>
#include <format>
#include <fstream>
#include <iostream>
#include <random>
#include <system_error>

namespace fs = std::filesystem;

namespace ultra::wopr
{

namespace
{

constexpr std::string_view common_style = R"CSS(
:root {
  --bg: #0f141a;
  --panel: #161d26;
  --card: #1c2531;
  --border: #2a3646;
  --text: #e6edf3;
  --muted: #8b98a7;
  --current: #4aa8ff;
  --reference: #ff9f43;
  --target: #ff6666;
  --queued: #b3b3b3;
  --running: #4dc9ff;
  --finished: #66e666;
  --failed: #ff6666;
  --cancelled: #ffcc4d;
}
* { box-sizing: border-box; }
body {
  margin: 0;
  background: var(--bg);
  color: var(--text);
  font-family: ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, sans-serif;
  font-size: 14px;
}
header {
  position: sticky;
  top: 0;
  z-index: 50;
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 10px 16px;
  background: var(--panel);
  border-bottom: 1px solid var(--border);
}
.brand {
  display: flex;
  align-items: baseline;
  gap: 10px;
}
.brand h1 {
  margin: 0;
  font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
  font-size: 18px;
  letter-spacing: 0.12em;
  text-shadow: 0 0 10px rgba(74, 168, 255, 0.35);
}
.brand button {
  padding: 0;
  background: transparent;
  color: inherit;
  border: 0;
  border-radius: 0;
  cursor: pointer;
  font: inherit;
  letter-spacing: inherit;
  text-shadow: inherit;
}
.brand button:focus-visible {
  outline: 2px solid var(--current);
  outline-offset: 4px;
}
body.terminal {
  --bg: #050b07;
  --panel: #09150e;
  --card: #0d1d13;
  --border: #284e35;
  --text: #98efad;
  --muted: #77ad88;
  --current: #38c96c;
  --target: #ff6666;
  --cancelled: #ffcc66;
  font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
}
.terminal .brand h1 {
  text-shadow: 0 0 10px rgba(152, 239, 173, 0.4);
}
.terminal button.active {
  background: rgba(56, 201, 108, 0.16);
}
.terminal .panel-title, .terminal .subplot-title {
  text-transform: uppercase;
}
.terminal button, .terminal select, .terminal input {
  font-family: inherit;
}
.terminal .chart-box, .terminal .subplot-chart {
  background-image: linear-gradient(rgba(152, 239, 173, 0.035) 1px, transparent 1px),
                    linear-gradient(90deg, rgba(152, 239, 173, 0.035) 1px, transparent 1px);
  background-size: 24px 24px;
}
.theme-message {
  position: fixed;
  top: 58px;
  left: 50%;
  transform: translateX(-50%);
  z-index: 1000;
  max-width: calc(100% - 32px);
  padding: 12px 20px;
  background: var(--bg);
  color: var(--text);
  border: 1px solid var(--current);
  border-radius: 4px;
  box-shadow: 0 10px 28px rgba(0, 0, 0, 0.72);
  font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
  font-weight: 600;
  letter-spacing: 0.08em;
  text-align: center;
  text-shadow: 0 0 8px rgba(74, 168, 255, 0.35);
  pointer-events: none;
}
.terminal .theme-message {
  border-color: var(--text);
  text-shadow: 0 0 10px rgba(152, 239, 173, 0.45);
}
.theme-message[hidden] {
  display: none;
}
.brand span {
  display: inline-flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px;
  color: var(--muted);
  font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
  font-variant-numeric: tabular-nums;
  font-size: 12px;
}
.hdr-alert {
  padding: 1px 6px;
  border-radius: 3px;
  font-weight: 600;
  font-size: 11px;
  border: 1px solid currentColor;
}
.hdr-alert-failed { color: var(--failed); }
.hdr-alert-warn { color: var(--cancelled); }
.controls {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 14px;
}
.controls label {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  cursor: pointer;
  user-select: none;
}
.wopr-cipher {
  display: inline-flex;
  font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
  font-size: 13px;
  font-weight: 600;
  letter-spacing: 0.14em;
  color: rgba(255, 72, 72, 0.45);
  text-shadow: 0 0 8px rgba(255, 72, 72, 0.35);
  user-select: none;
}
.wopr-cipher .locked {
  color: rgba(255, 96, 96, 0.9);
}
.wopr-cipher[hidden] {
  display: none;
}
.hold-btn, .copy-btn {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
  font-variant-numeric: tabular-nums;
  font-size: 11px;
  font-weight: 600;
  letter-spacing: 0.04em;
}
.hold-btn .live-dot {
  width: 7px;
  height: 7px;
  border-radius: 50%;
  background: var(--finished);
  box-shadow: 0 0 6px var(--finished);
  animation: wopr-pulse 1.4s ease-in-out infinite;
}
.hold-btn.paused {
  border-color: var(--cancelled);
  color: var(--cancelled);
}
.hold-btn.paused .live-dot {
  background: var(--cancelled);
  box-shadow: none;
  animation: none;
}
@keyframes wopr-pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.35; }
}
main {
  padding: 12px;
  display: flex;
  flex-direction: column;
  gap: 12px;
}
.dashboard {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}
.dashboard.single-col {
  grid-template-columns: minmax(0, 1fr);
}
.panel {
  background: var(--panel);
  border: 1px solid var(--border);
  border-radius: 6px;
  display: flex;
  flex-direction: column;
  min-height: 380px;
  overflow: hidden;
}
.panel[hidden] {
  display: none;
}
.panel.maximised {
  grid-column: 1 / -1;
  min-height: 75vh;
}
.panel-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  padding: 8px 12px;
  border-bottom: 1px solid var(--border);
  background: rgba(255, 255, 255, 0.02);
}
.panel-title {
  display: flex;
  align-items: center;
  gap: 10px;
  font-weight: 600;
  letter-spacing: 0.04em;
  font-size: 13px;
}
.panel-actions {
  display: flex;
  align-items: center;
  gap: 10px;
}
button, select, input[type="number"] {
  background: var(--card);
  color: var(--text);
  border: 1px solid var(--border);
  border-radius: 4px;
  padding: 3px 9px;
  font-size: 12px;
}
button { cursor: pointer; }
button:hover, select:hover { border-color: var(--current); }
button.active {
  background: rgba(74, 168, 255, 0.16);
  border-color: var(--current);
}
.panel-body {
  padding: 10px;
  flex: 1 1 auto;
  display: flex;
  flex-direction: column;
  gap: 8px;
}
.note {
  color: var(--muted);
  font-size: 12px;
}
.chart-box {
  width: 100%;
  flex: 1 1 auto;
  min-height: 310px;
}
.subplots {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 10px;
}
.subplot {
  background: var(--card);
  border: 1px solid var(--border);
  border-radius: 5px;
  padding: 8px;
  display: flex;
  flex-direction: column;
}
.subplot.warn-mild {
  background: linear-gradient(rgba(255, 59, 48, 0.14), rgba(255, 59, 48, 0.14)), var(--card);
}
.subplot.warn-strong {
  background: linear-gradient(rgba(255, 59, 48, 0.28), rgba(255, 59, 48, 0.28)), var(--card);
}
.subplot-title {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
  font-size: 12px;
  font-weight: 600;
  margin-bottom: 4px;
}
.subplot-chart {
  width: 100%;
  height: 220px;
}
.badge {
  font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
  font-size: 11px;
  font-weight: 600;
}
.badge-Queued { color: var(--queued); }
.badge-Running { color: var(--running); }
.badge-Finished { color: var(--finished); }
.badge-Failed { color: var(--failed); }
.badge-Cancelled { color: var(--cancelled); }
.badge-NoHit { color: var(--cancelled); }
.run-picker {
  display: inline-flex;
  align-items: center;
  gap: 4px;
}
button:disabled {
  opacity: 0.4;
  cursor: default;
  border-color: var(--border);
}
.tabs {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  margin: 8px 0;
}
.toolbar {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 10px;
  margin-bottom: 6px;
}
.table-wrap { overflow-x: auto; }
table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}
th, td {
  text-align: left;
  padding: 7px 10px;
  border-bottom: 1px solid var(--border);
  vertical-align: top;
}
th {
  color: var(--muted);
  font-weight: 600;
  font-size: 12px;
}
td.num {
  font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
  font-variant-numeric: tabular-nums;
}
.code-wrap {
  position: relative;
}
.copy-btn {
  float: right;
  margin: 0 0 4px 8px;
  padding: 2px 6px;
}
.copy-btn.copied {
  border-color: var(--finished);
  color: var(--finished);
}
pre.code {
  margin: 4px 0 0;
  padding: 6px 8px;
  background: var(--bg);
  border: 1px solid var(--border);
  border-radius: 4px;
  font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
  font-size: 12px;
  white-space: pre-wrap;
  word-break: break-word;
}
@media (max-width: 900px) {
  .dashboard { grid-template-columns: minmax(0, 1fr); }
}
)CSS";

constexpr std::string_view common_js = R"JS(
function createDashboard(cfg) {
  const data = JSON.parse(document.getElementById("wopr-data").textContent);
  const storageKey = cfg.storagePrefix + location.pathname;
  const saved = (() => {
    try { return JSON.parse(sessionStorage.getItem(storageKey) || "{}"); }
    catch { return {}; }
  })();

  const CIPHER_LEN = 10;
  const CIPHER_CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  function makeCipherOrder() {
    const order = Array.from({ length: CIPHER_LEN }, (_, i) => i);
    for (let i = order.length - 1; i > 0; --i) {
      const j = Math.floor(Math.random() * (i + 1));
      [order[i], order[j]] = [order[j], order[i]];
    }
    return order;
  }

  const savedCipher = saved.cipher && Array.isArray(saved.cipher.order) && saved.cipher.order.length === CIPHER_LEN &&
    Array.isArray(saved.cipher.locked) && saved.cipher.locked.length === CIPHER_LEN
    ? { order: saved.cipher.order.slice(), locked: saved.cipher.locked.slice() }
    : { order: makeCipherOrder(), locked: Array(CIPHER_LEN).fill(null) };

  const hasLiveRefresh = Boolean(data.live) && data.refresh_seconds > 0;
  let refreshRemaining = data.refresh_seconds || 0;
  let refreshTimer = null;
  let holdBtn = null;

  const savedThemeMsg = saved.themeMsg && typeof saved.themeMsg.text === "string" &&
    typeof saved.themeMsg.until === "number" && saved.themeMsg.until > Date.now()
    ? { text: saved.themeMsg.text, until: saved.themeMsg.until }
    : null;

  const state = Object.assign({}, cfg.initState ? cfg.initState(data, saved) : {}, {
    terminal: Boolean(saved.terminal),
    maximised: saved.maximised || null,
    hold: hasLiveRefresh ? Boolean(saved.hold) : false,
    visible: Object.assign({}, cfg.defaultVisible(data), saved.visible || {}),
    themeMsg: savedThemeMsg
  });

  function saveState() {
    try {
      const extra = cfg.serializeState ? cfg.serializeState(state) : {};
      const activeMsg = state.themeMsg && state.themeMsg.until > Date.now()
        ? state.themeMsg : null;
      sessionStorage.setItem(storageKey, JSON.stringify(Object.assign({}, extra, {
        terminal: state.terminal,
        maximised: state.maximised,
        hold: hasLiveRefresh ? state.hold : false,
        visible: state.visible,
        scrollY: window.scrollY,
        cipher: savedCipher,
        themeMsg: activeMsg
      })));
    } catch {}
  }

  let charts = [];
  function disposeCharts(list = charts) {
    for (const c of list) {
      c.dispose();
      if (list !== charts) {
        const idx = charts.indexOf(c);
        if (idx >= 0) charts.splice(idx, 1);
      }
    }
    list.length = 0;
  }

  function initChart(dom, list = null) {
    const c = echarts.init(dom, "dark", { renderer: "canvas" });
    charts.push(c);
    if (list) list.push(c);
    return c;
  }

  function h(tag, attrs = {}, text = null) {
    const node = document.createElement(tag);
    for (const [k, v] of Object.entries(attrs)) {
      if (k === "className") node.className = v;
      else if (v !== null && v !== undefined) node.setAttribute(k, String(v));
    }
    if (text !== null) node.textContent = text;
    return node;
  }

  function makeCheckbox(label, checked, onChange) {
    const lbl = h("label");
    const cb = h("input", { type: "checkbox" });
    cb.checked = checked;
    cb.addEventListener("change", () => onChange(cb.checked));
    lbl.appendChild(cb);
    lbl.appendChild(document.createTextNode(label));
    return lbl;
  }

  function makeCodeBlock(text) {
    const wrap = h("div", { className: "code-wrap" });
    const btn = h("button", { type: "button", className: "copy-btn" }, "COPY");
    let timer = null;
    btn.addEventListener("click", () => {
      const finish = () => {
        btn.textContent = "COPIED";
        btn.classList.add("copied");
        if (timer) clearTimeout(timer);
        timer = setTimeout(() => {
          btn.textContent = "COPY";
          btn.classList.remove("copied");
        }, 1500);
      };
      if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(text).then(finish).catch(() => fallbackCopy(text, finish));
      } else {
        fallbackCopy(text, finish);
      }
    });
    wrap.appendChild(btn);
    wrap.appendChild(h("pre", { className: "code" }, text));
    return wrap;
  }

  function fallbackCopy(text, done) {
    const ta = h("textarea");
    ta.value = text;
    ta.style.position = "fixed";
    ta.style.opacity = "0";
    document.body.appendChild(ta);
    ta.select();
    try { if (document.execCommand && document.execCommand("copy")) done(); }
    catch {}
    document.body.removeChild(ta);
  }

  function baseGrid(extra = {}) {
    return Object.assign({ left: 52, right: 20, top: 32, bottom: 42, containLabel: true }, extra);
  }

  function splitAxis(extra = {}) {
    return Object.assign({ type: "value", splitLine: { lineStyle: { color: "#273241" } } }, extra);
  }

  function primaryColor() {
    return state.terminal ? "#ffd43b" : "#4aa8ff";
  }

  function barColor() {
    return state.terminal ? "#38c96c" : "#4aa8ff";
  }

  function primaryFill(alpha) {
    return state.terminal
      ? `rgba(255, 212, 59, ${alpha})`
      : `rgba(74, 168, 255, ${alpha})`;
  }

  let cipherTimer = null;
  let cipherWrap = null;
  let cipherSlots = [];

  function randomCipherChar() {
    return CIPHER_CHARS[Math.floor(Math.random() * CIPHER_CHARS.length)];
  }

  function stopCipherTimer() {
    if (cipherTimer) {
      clearInterval(cipherTimer);
      cipherTimer = null;
    }
  }

  function tickCipher() {
    for (let i = 0; i < CIPHER_LEN; ++i) {
      const slot = cipherSlots[i];
      if (!slot) continue;
      const fixed = savedCipher.locked[i];
      if (fixed !== null) {
        slot.textContent = fixed;
        slot.classList.add("locked");
      } else {
        slot.textContent = randomCipherChar();
        slot.classList.remove("locked");
      }
    }
  }

  function syncCipher() {
    if (!cipherWrap) return;
    const visible = Boolean(state.terminal);
    cipherWrap.hidden = !visible;
    if (!visible) {
      stopCipherTimer();
      return;
    }
    tickCipher();
    if (!cipherTimer) cipherTimer = setInterval(tickCipher, 50);
  }

  function initCipher(controls) {
    cipherWrap = null;
    stopCipherTimer();
    if (!cfg.getCipherState) return;
    const cs = cfg.getCipherState();
    if (!cs || !cs.active) {
      savedCipher.locked = Array(CIPHER_LEN).fill(null);
      return;
    }

    const ratio = Number.isFinite(cs.progress) ? Math.max(0, Math.min(1, cs.progress)) : 0;
    const targetLocked = Math.floor(ratio * CIPHER_LEN);
    for (let k = 0; k < CIPHER_LEN; ++k) {
      const idx = savedCipher.order[k];
      if (k < targetLocked) {
        if (savedCipher.locked[idx] === null) savedCipher.locked[idx] = randomCipherChar();
      } else {
        savedCipher.locked[idx] = null;
      }
    }

    cipherWrap = h("span", { className: "wopr-cipher", "aria-hidden": "true" });
    cipherSlots = [];
    for (let i = 0; i < CIPHER_LEN; ++i) {
      const slot = h("span");
      cipherSlots.push(slot);
      cipherWrap.appendChild(slot);
    }
    controls.appendChild(cipherWrap);
    syncCipher();
  }

  function updateHoldButton() {
    if (!holdBtn) return;
    holdBtn.className = "hold-btn " + (state.hold ? "paused" : "live");
    holdBtn.innerHTML = "";
    holdBtn.appendChild(h("span", { className: "live-dot", "aria-hidden": "true" }));
    holdBtn.appendChild(document.createTextNode(
      state.hold ? "HOLD (P)" : `LIVE \u00b7 ${refreshRemaining}s`
    ));
    holdBtn.title = state.hold ? "Resume auto-refresh (P)" : "Pause auto-refresh (P)";
  }

  function startRefreshTimer() {
    if (refreshTimer) {
      clearInterval(refreshTimer);
      refreshTimer = null;
    }
    if (!hasLiveRefresh || state.hold) {
      updateHoldButton();
      return;
    }
    refreshRemaining = data.refresh_seconds;
    updateHoldButton();
    refreshTimer = setInterval(() => {
      if (state.hold) return;
      --refreshRemaining;
      if (refreshRemaining <= 0) {
        clearInterval(refreshTimer);
        refreshTimer = null;
        saveState();
        location.reload();
        return;
      }
      updateHoldButton();
    }, 1000);
  }

  function toggleHold() {
    if (!hasLiveRefresh) return;
    state.hold = !state.hold;
    saveState();
    startRefreshTimer();
  }

  function renderControls(items, appendExtra) {
    const controls = document.getElementById("controls");
    controls.innerHTML = "";
    for (const [key, label] of items) {
      controls.appendChild(makeCheckbox(label, state.visible[key], (checked) => {
        state.visible[key] = checked;
        if (state.maximised === key && !checked) state.maximised = null;
        renderAll();
      }));
    }
    if (appendExtra) appendExtra(controls);
    if (hasLiveRefresh) {
      holdBtn = h("button", { type: "button", className: "hold-btn" });
      holdBtn.addEventListener("click", toggleHold);
      updateHoldButton();
      controls.appendChild(holdBtn);
    }
    initCipher(controls);
  }

  function visiblePanels() {
    return cfg.getPanels().filter(
      (p) => (p.available === undefined || p.available) && state.visible[p.key]
    );
  }

  function renderAll() {
    disposeCharts();
    if (cfg.renderHeader) cfg.renderHeader();

    const dash = document.getElementById("dashboard");
    dash.innerHTML = "";

    const active = visiblePanels().filter((p) => !state.maximised || state.maximised === p.key);
    dash.classList.toggle("single-col", active.length <= 1);

    for (const p of active) {
      const isMax = state.maximised === p.key;
      const panel = h("article", { className: "panel" + (isMax ? " maximised" : "") });
      const header = h("div", { className: "panel-header" });
      header.appendChild(h("div", { className: "panel-title" }, p.title));
      const actions = h("div", { className: "panel-actions" });
      const btn = h("button", { type: "button" }, isMax ? "Minimise" : "Maximise");
      btn.addEventListener("click", () => {
        state.maximised = isMax ? null : p.key;
        renderAll();
      });
      actions.appendChild(btn);
      header.appendChild(actions);

      const body = h("div", { className: "panel-body" });
      panel.appendChild(header);
      panel.appendChild(body);
      dash.appendChild(panel);
      p.render(body);
    }

    if (cfg.afterRender) cfg.afterRender();
    saveState();
  }

  function initTheme() {
    const themeButton = document.getElementById("theme-toggle");
    const themeMessage = h("div", { className: "theme-message", role: "status" });
    themeMessage.hidden = true;
    document.body.appendChild(themeMessage);
    let messageTimer = null;

    const showThemeMessage = (text, durationMs) => {
      clearTimeout(messageTimer);
      themeMessage.textContent = text;
      themeMessage.hidden = false;
      state.themeMsg = { text, until: Date.now() + durationMs };
      saveState();
      messageTimer = setTimeout(() => {
        themeMessage.hidden = true;
        state.themeMsg = null;
        saveState();
      }, durationMs);
    };

    const applyTheme = () => {
      document.body.classList.toggle("terminal", state.terminal);
      if (themeButton) {
        themeButton.setAttribute("aria-pressed", String(state.terminal));
        themeButton.title = state.terminal ? "Switch to standard theme" : "Switch to WOPR Terminal theme";
      }
      syncCipher();
    };

    applyTheme();
    if (state.themeMsg) {
      const remaining = state.themeMsg.until - Date.now();
      if (remaining > 0) showThemeMessage(state.themeMsg.text, remaining);
      else state.themeMsg = null;
    }
    if (themeButton) {
      themeButton.onclick = () => {
        state.terminal = !state.terminal;
        applyTheme();
        renderAll();
        showThemeMessage(
          state.terminal
            ? "GREETINGS PROFESSOR FALKEN"
            : "THE ONLY WINNING MOVE IS NOT TO PLAY",
          4000
        );
      };
    }
  }

  function handleKeydown(ev) {
    if (!ev || ev.ctrlKey || ev.metaKey || ev.altKey) return;
    const t = ev.target;
    const tag = t && t.tagName ? t.tagName.toUpperCase() : "";
    if (tag === "INPUT" || tag === "TEXTAREA" || tag === "SELECT" || (t && t.isContentEditable)) return;

    if (ev.key === "Escape" && state.maximised) {
      state.maximised = null;
      renderAll();
    } else if ((ev.key === "p" || ev.key === "P") && hasLiveRefresh) {
      if (ev.preventDefault) ev.preventDefault();
      toggleHold();
    } else if (ev.key >= "1" && ev.key <= "9") {
      const target = visiblePanels()[Number(ev.key) - 1];
      if (target) {
        state.maximised = state.maximised === target.key ? null : target.key;
        renderAll();
      }
    }
  }

  function start() {
    initTheme();
    renderControls(cfg.getControls(), cfg.appendControls);
    startRefreshTimer();
    renderAll();
    if (typeof saved.scrollY === "number") window.scrollTo(0, saved.scrollY);

    window.addEventListener("resize", () => {
      for (const c of charts) c.resize();
    });
    document.addEventListener("keydown", handleKeydown);
  }

  return {
    data, state, saveState, charts, disposeCharts, initChart, h,
    makeCheckbox, makeCodeBlock, baseGrid, splitAxis,
    primaryColor, barColor, primaryFill, renderAll, start
  };
}
)JS";

}  // namespace

std::string json_escape(std::string_view input)
{
  std::string out;
  out.reserve(input.size() + 8);

  for (const unsigned char ch : input)
  {
    switch (ch)
    {
    case '\\': out += "\\\\"; break;
    case '"':  out += "\\\""; break;
    case '\b': out += "\\b";  break;
    case '\f': out += "\\f";  break;
    case '\n': out += "\\n";  break;
    case '\r': out += "\\r";  break;
    case '\t': out += "\\t";  break;
    case '<':  out += "\\u003c"; break;
    case '>':  out += "\\u003e"; break;
    case '&':  out += "\\u0026"; break;
    default:
      if (ch < 0x20)
        out += std::format("\\u{:04x}", static_cast<unsigned>(ch));
      else
        out.push_back(static_cast<char>(ch));
      break;
    }
  }

  return out;
}

void append_json_string(std::string &out, std::string_view value)
{
  out.push_back('"');
  out += json_escape(value);
  out.push_back('"');
}

void append_json_number(std::string &out, double value)
{
  if (!std::isfinite(value))
    out += "null";
  else if (value == 0.0)
    out += "0";
  else
    out += std::format("{}", value);
}

namespace
{

[[nodiscard]] bool is_unreserved_url_char(unsigned char ch) noexcept
{
  return (ch >= '0' && ch <= '9')
      || (ch >= 'A' && ch <= 'Z')
      || (ch >= 'a' && ch <= 'z')
      || ch == '-' || ch == '.' || ch == '_' || ch == '~';
}

[[nodiscard]] std::string encode_url_component(std::string_view input)
{
  std::string out;
  out.reserve(input.size());

  for (const unsigned char ch : input)
  {
    if (is_unreserved_url_char(ch))
      out.push_back(static_cast<char>(ch));
    else
      out += std::format("%{:02X}", static_cast<unsigned>(ch));
  }

  return out;
}

[[nodiscard]] std::string escape_html_attribute(std::string_view input)
{
  std::string out;
  out.reserve(input.size());

  for (const char ch : input)
  {
    switch (ch)
    {
    case '&':  out += "&amp;";  break;
    case '"':  out += "&quot;"; break;
    case '\'': out += "&#39;";  break;
    case '<':  out += "&lt;";   break;
    case '>':  out += "&gt;";   break;
    default:   out.push_back(ch); break;
    }
  }

  return out;
}

}  // namespace

std::string build_echarts_html(std::string_view title,
                               std::string_view json_payload,
                               std::string_view body_html,
                               std::string_view app_js,
                               std::string_view script_source)
{
  const auto echarts(script_source.empty() ? echarts_min_js()
                                          : std::string_view{});

  std::string html;
  html.reserve(2048 + common_style.size() + echarts.size() + json_payload.size()
               + body_html.size() + common_js.size() + app_js.size());

  html += "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
          "<meta charset=\"utf-8\">\n"
          "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
          "<title>";
  html += title;
  html += "</title>\n<style>";
  html += common_style;
  html += "</style>\n</head>\n<body>\n";
  html += body_html;
  html += "\n<script id=\"wopr-data\" type=\"application/json\">";
  html += json_payload;
  html += "</script>\n<script";
  if (!script_source.empty())
  {
    html += " src=\"";
    html += escape_html_attribute(encode_url_component(script_source));
    html += '"';
  }
  html += '>';
  html += echarts;
  html += "</script>\n<script>";
  html += common_js;
  html += app_js;
  html += "</script>\n</body>\n</html>\n";

  return html;
}

[[nodiscard]] std::string companion_script_name(const fs::path &output_path)
{
  const auto stem(output_path.stem().string());
  return (stem.empty() ? "wopr" : stem) + ".echarts.min.js";
}

bool write_echarts_html(const fs::path &output_path, std::string_view title,
                        std::string_view json_payload,
                        std::string_view body_html, std::string_view app_js,
                        bool live)
{
  const auto script_name(companion_script_name(output_path));
  const auto script_path(output_path.parent_path() / script_name);

  if (live)
  {
    std::error_code ec;
    if ((!fs::is_regular_file(script_path, ec)
         || fs::file_size(script_path, ec) != echarts_min_js().size())
        && !write_file_atomically(script_path, echarts_min_js()))
      return false;

    return write_file_atomically(output_path, build_echarts_html(
      title, json_payload, body_html, app_js, script_name));
  }

  if (!write_file_atomically(output_path, build_echarts_html(
        title, json_payload, body_html, app_js)))
    return false;

  std::error_code ec;
  fs::remove(script_path, ec);
  return true;
}

bool write_file_atomically(const fs::path &output_path,
                           std::string_view content)
{
  // Separate temporary files allow reports to publish a shared script safely.
  std::random_device random;
  auto temp_path(output_path);
  temp_path += std::format(".{:08x}{:08x}.tmp", random(), random());

  {
    std::ofstream out(temp_path, std::ios::binary | std::ios::noreplace);
    if (!out)
    {
      std::cerr << "Cannot open file `" << temp_path << "` for writing.\n";
      return false;
    }

    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!out)
    {
      std::cerr << "Failed while writing file `" << temp_path << "`.\n";
      std::error_code ec;
      fs::remove(temp_path, ec);
      return false;
    }
  }

  std::error_code ec;
  fs::rename(temp_path, output_path, ec);
  if (ec)
  {
    std::cerr << "Cannot rename `" << temp_path << "` to `" << output_path
              << "`: " << ec.message() << '\n';
    fs::remove(temp_path, ec);
    return false;
  }

  return true;
}

}  // namespace ultra::wopr
