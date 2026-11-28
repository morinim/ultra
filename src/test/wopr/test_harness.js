const Gio = imports.gi.Gio;

function readHtml(path) {
  const [ok, bytes] = Gio.File.new_for_path(path).load_contents(null);
  if (!ok) throw new Error("Cannot read " + path);
  return new TextDecoder().decode(bytes);
}

function extractScripts(html, directory) {
  const dataMark = '<script id="wopr-data" type="application/json">';
  const dStart = html.indexOf(dataMark);
  const dEnd = html.indexOf("</script>", dStart);
  const s1Tag = html.indexOf("<script", dEnd);
  const s1Start = html.indexOf(">", s1Tag) + 1;
  const s1End = html.indexOf("</script>", s1Start);
  const s2Start = html.indexOf("<script>", s1End) + 8;
  const s2End = html.indexOf("</script>", s2Start);
  const source = html.slice(s1Tag, s1Start).match(/src="([^"]+)"/);
  return {
    data: html.slice(dStart + dataMark.length, dEnd),
    echarts: source ? readHtml(directory + "/" + decodeURIComponent(source[1])) : html.slice(s1Start, s1End),
    app: html.slice(s2Start, s2End)
  };
}

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function collectText(node) {
  let out = node.textContent || "";
  for (const c of (node.children || [])) out += collectText(c);
  return out;
}

function findAllByClass(node, cls, out = []) {
  if (node.classList && node.classList.contains(cls)) out.push(node);
  for (const c of (node.children || [])) findAllByClass(c, cls, out);
  return out;
}

function makeEl(tag, id, elems) {
  const listeners = {};
  const clsSet = new Set();
  let clsName = "";
  const el = {
    tagName: tag.toUpperCase(),
    id: id || "",
    get className() { return clsName; },
    set className(v) {
      clsName = String(v);
      clsSet.clear();
      for (const c of clsName.split(/\s+/)) {
        if (c) clsSet.add(c);
      }
    },
    textContent: "",
    type: "",
    value: "",
    checked: false,
    hidden: false,
    style: {},
    children: [],
    parentNode: null,
    clientWidth: 640,
    clientHeight: 360,
    offsetWidth: 640,
    offsetHeight: 360,
    classList: {
      add(c) { clsSet.add(c); clsName = Array.from(clsSet).join(" "); },
      remove(c) { clsSet.delete(c); clsName = Array.from(clsSet).join(" "); },
      contains(c) { return clsSet.has(c); },
      toggle(c, force) {
        if (force === undefined) {
          if (clsSet.has(c)) clsSet.delete(c);
          else clsSet.add(c);
        } else if (force) {
          clsSet.add(c);
        } else {
          clsSet.delete(c);
        }
        clsName = Array.from(clsSet).join(" ");
      }
    },
    setAttribute(k, v) {
      if (k === "id") {
        this.id = v;
        elems.set(v, this);
      } else {
        this[k] = String(v);
      }
    },
    getAttribute(k) { return this[k] ?? null; },
    appendChild(c) { c.parentNode = this; this.children.push(c); return c; },
    removeChild(c) {
      const idx = this.children.indexOf(c);
      if (idx >= 0) this.children.splice(idx, 1);
      c.parentNode = null;
      return c;
    },
    addEventListener(ev, fn) { (listeners[ev] = listeners[ev] || []).push(fn); },
    removeEventListener() {},
    dispatch(ev, arg) { for (const fn of (listeners[ev] || [])) fn(arg); },
    getBoundingClientRect() {
      return { left: 0, top: 0, width: 640, height: 360, right: 640, bottom: 360 };
    },
    querySelectorAll() { return []; },
    querySelector() { return null; },
    getContext() {
      return {
        measureText: (s) => ({ width: String(s).length * 6 }),
        save() {}, restore() {}, scale() {}, translate() {}, rotate() {}, transform() {}, setTransform() {},
        beginPath() {}, closePath() {}, moveTo() {}, lineTo() {}, arc() {}, rect() {},
        fillRect() {}, strokeRect() {}, clearRect() {},
        fill() {}, stroke() {}, clip() {}, fillText() {}, strokeText() {},
        createLinearGradient() { return { addColorStop() {} }; },
        createRadialGradient() { return { addColorStop() {} }; }
      };
    }
  };
  let ih = "";
  Object.defineProperty(el, "innerHTML", {
    get: () => ih,
    set: (v) => { ih = v; if (v === "") el.children = []; }
  });
  return el;
}

function createTestEnv(pathname) {
  const storage = {
    _d: {},
    getItem(k) { return this._d[k] ?? null; },
    setItem(k, v) { this._d[k] = String(v); }
  };
  const copiedTexts = [];
  const elems = new Map();
  let docListeners = {};

  const body = makeEl("body", "", elems);
  const themeButton = makeEl("button", "theme-toggle", elems); elems.set("theme-toggle", themeButton); body.appendChild(themeButton);
  const dash = makeEl("div", "dashboard", elems); elems.set("dashboard", dash); body.appendChild(dash);
  const ctrls = makeEl("div", "controls", elems); elems.set("controls", ctrls); body.appendChild(ctrls);
  const subtitle = makeEl("div", "subtitle", elems); elems.set("subtitle", subtitle); body.appendChild(subtitle);
  const detailsBody = makeEl("div", "details-body", elems); elems.set("details-body", detailsBody); body.appendChild(detailsBody);
  const detailsPanel = makeEl("div", "details-panel", elems); elems.set("details-panel", detailsPanel); body.appendChild(detailsPanel);
  const dataEl = makeEl("script", "wopr-data", elems); elems.set("wopr-data", dataEl); body.appendChild(dataEl);

  globalThis.window = globalThis;
  globalThis.location = { pathname, reload() {} };
  globalThis.navigator = {
    userAgent: "gjs",
    platform: "Linux",
    language: "en",
    clipboard: {
      writeText(t) {
        copiedTexts.push(String(t));
        return Promise.resolve();
      }
    }
  };
  globalThis.screen = { width: 1920, height: 1080 };
  globalThis.devicePixelRatio = 1;
  globalThis.setTimeout = () => 0;
  globalThis.clearTimeout = () => {};
  globalThis.setInterval = () => 0;
  globalThis.clearInterval = () => {};
  globalThis.requestAnimationFrame = () => 0;
  globalThis.cancelAnimationFrame = () => {};
  globalThis.addEventListener = () => {};
  globalThis.removeEventListener = () => {};
  globalThis.localStorage = storage;
  globalThis.sessionStorage = storage;
  globalThis.document = {
    body,
    documentElement: body,
    createElement: (t) => makeEl(t, "", elems),
    createElementNS: (_ns, t) => makeEl(t, "", elems),
    createTextNode: (t) => {
      const n = makeEl("#text", "", elems);
      n.textContent = String(t);
      return n;
    },
    getElementById: (id) => elems.get(id) || makeEl("div", id, elems),
    addEventListener(ev, fn) { (docListeners[ev] = docListeners[ev] || []).push(fn); }
  };

  return {
    body,
    themeButton,
    dash,
    ctrls,
    subtitle,
    detailsBody,
    detailsPanel,
    dataEl,
    storage,
    copiedTexts,
    dispatchDoc(ev, arg) { for (const fn of (docListeners[ev] || [])) fn(arg); },
    reset(preserveStorage = false) {
      dash.innerHTML = "";
      ctrls.innerHTML = "";
      subtitle.innerHTML = "";
      detailsBody.innerHTML = "";
      docListeners = {};
      copiedTexts.length = 0;
      if (!preserveStorage) storage._d = {};
    }
  };
}
