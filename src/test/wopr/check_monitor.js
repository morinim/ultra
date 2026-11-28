const Gio = imports.gi.Gio;

const htmlDir = ARGV[0];
if (!htmlDir) {
  throw new Error("Usage: gjs check_monitor.js <html_dir>");
}

const scriptDir = Gio.File.new_for_path(imports.system.programPath).get_parent().get_path();
imports.searchPath.unshift(scriptDir);
const {
  readHtml, extractScripts, assert, collectText, findAllByClass, createTestEnv
} = imports.test_harness;

const env = createTestEnv("/monitor-test.html");
const { body, themeButton, dash, ctrls, subtitle, dataEl, dispatchDoc } = env;

const options = [];
globalThis.echarts = {
  init() {
    return {
      setOption(o) { options.push(o); },
      resize() {},
      dispose() {}
    };
  }
};

function runMonitorHtml(name) {
  const parts = extractScripts(readHtml(htmlDir + "/" + name + ".html"), htmlDir);
  options.length = 0;
  env.reset(false);
  dataEl.textContent = parts.data;
  (0, eval)(parts.app);
}

// 1. One-shot monitor HTML: single run view (no accordions), histogram baseline, dynamic tabs, hidden cipher/hold
{
  runMonitorHtml("monitor_oneshot");
  assert(findAllByClass(dash, "run-section").length === 0, "monitor must not render stacked run accordions");
  assert(options.length === 4, "expected exactly 1 chart per active panel (4 total), got " + options.length);
  assert(findAllByClass(ctrls, "wopr-cipher").length === 0, "one-shot monitor must hide cipher");
  assert(findAllByClass(ctrls, "hold-btn").length === 0, "one-shot monitor must hide hold btn");
  assert(collectText(subtitle).includes("Run 1 (2 runs)"), "expected Latest Run 1 in subtitle");

  // Run 1 (latest) population has Generation 1? No, pop1 had gen 0 and pop0 gen 0; lay1 had gen 1.
  assert(options[2].title.text.includes("Generation 1"), "expected Run 1 layer fit Generation 1");

  const prevBtn = findAllByClass(ctrls, "run-prev")[0];
  const nextBtn = findAllByClass(ctrls, "run-next")[0];
  const runSel = findAllByClass(ctrls, "run-select")[0];
  assert(prevBtn && nextBtn && runSel, "expected run picker controls");
  assert(!prevBtn.disabled && nextBtn.disabled, "latest run should enable prev and disable next");

  options.length = 0;
  prevBtn.dispatch("click");
  assert(collectText(subtitle).includes("Run 0 (2 runs)"), "prev must select Run 0");
  assert(prevBtn.disabled && !nextBtn.disabled, "Run 0 should disable prev and enable next");
  assert(options[2].title.text.includes("Generation 0"), "expected Run 0 layer fit Generation 0");

  assert(options[0].yAxis.name === "Fit" && options[0].series.length === 3,
    "expected initial dynamic fitness tab");
  assert(options[0].series[0].name === "StdDev" && options[0].series[0].itemStyle.color === "#9aa8b8",
    "expected #9aa8b8 legend itemStyle color on StdDev custom series");
  assert(options[1].yAxis.name === "Individuals" && options[1].yAxis.min === 0 && options[1].yAxis.scale === undefined,
    "expected histogram count axis with min: 0 baseline");

  const tabs = findAllByClass(dash, "tabs");
  assert(tabs.length === 2, "expected dynamic and population tab bars");

  options.length = 0;
  tabs[1].children[1].dispatch("click");
  assert(options[0].yAxis.name === "Entropy" && options[0].yAxis.scale === true,
    "expected entropy yAxis scale: true");

  options.length = 0;
  tabs[0].children[1].dispatch("click");
  assert(options[0].yAxis.name === "Length" && options[0].series.length === 3,
    "expected length tab with Longest series");

  const toolbars = findAllByClass(dash, "toolbar");
  const longestCb = toolbars[0].children[0].children[0];
  longestCb.checked = false;
  options.length = 0;
  longestCb.dispatch("change");
  assert(options[0].yAxis.name === "Length" && options[0].series.length === 2,
    "expected Longest toggle to remove Longest series");

  options.length = 0;
  tabs[0].children[2].dispatch("click");
  assert(options[0].yAxis.name === "Crossover spreading" && options[0].series.length === 2,
    "expected crossover tab with CT0 and CT1");

  runSel.value = "latest";
  options.length = 0;
  runSel.dispatch("change");
  assert(collectText(subtitle).includes("Run 1 (2 runs)"), "select Latest must return to Run 1");
  assert(options[2].title.text.includes("Generation 1"), "select Latest must show Run 1 layer fit");
}

// 2. Live monitor HTML: spinning cipher (0 locked) & HOLD/P
{
  runMonitorHtml("monitor_live");
  const ciphers = findAllByClass(ctrls, "wopr-cipher");
  assert(ciphers.length === 1 && ciphers[0].children.length === 10, "live monitor must show cipher");
  assert(ciphers[0].hidden, "live monitor cipher must stay hidden in standard theme");

  const holds = findAllByClass(ctrls, "hold-btn");
  assert(holds.length === 1 && collectText(holds[0]).includes("LIVE"), "live monitor must show LIVE btn");
  dispatchDoc("keydown", { key: "p", target: body });
  assert(collectText(holds[0]).includes("HOLD"), "P shortcut must toggle monitor HOLD");

  // Terminal theme shows cipher (0 locked) and switches monitor diagram colors
  assert(options[1].series[0].itemStyle.color === "#4aa8ff", "expected standard blue histogram");
  options.length = 0;
  themeButton.onclick();
  assert(!ciphers[0].hidden, "live monitor cipher must show in WOPR terminal mode");
  const locked = ciphers[0].children.filter((s) => s.classList.contains("locked")).length;
  assert(locked === 0, "live monitor cipher must have 0 locked chars");
  assert(options[0].series[1].itemStyle.color === "#ffd43b", "expected yellow dynamic line in terminal mode");
  assert(options[1].series[0].itemStyle.color === "#38c96c", "expected phosphor green histogram in terminal mode");
  themeButton.onclick();
  assert(ciphers[0].hidden, "live monitor cipher must hide when leaving WOPR terminal mode");
}
