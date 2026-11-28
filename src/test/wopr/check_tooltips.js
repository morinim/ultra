const Gio = imports.gi.Gio;

const htmlDir = ARGV[0];
if (!htmlDir) {
  throw new Error("Usage: gjs check_tooltips.js <html_dir>");
}

const scriptDir = Gio.File.new_for_path(imports.system.programPath).get_parent().get_path();
imports.searchPath.unshift(scriptDir);
const {
  readHtml, extractScripts, assert, collectText, findAllByClass, createTestEnv
} = imports.test_harness;

const env = createTestEnv("/tooltip-test.html");
const {
  body, themeButton, dash, ctrls, subtitle, detailsBody, dataEl, copiedTexts, dispatchDoc
} = env;

let echartsLoaded = false;
const charts = [];
function runHtml(name, preserveStorage = false) {
  const parts = extractScripts(readHtml(htmlDir + "/" + name + ".html"), htmlDir);
  if (!echartsLoaded) {
    (0, eval)(parts.echarts);
    const origInit = globalThis.echarts.init;
    globalThis.echarts.init = function(dom, theme, opts) {
      const ch = origInit.call(this, dom, theme, opts);
      charts.push(ch);
      return ch;
    };
    echartsLoaded = true;
  }
  charts.length = 0;
  env.reset(preserveStorage);
  dataEl.textContent = parts.data;
  (0, eval)(parts.app);
  return charts[1];  // Panel 0: Runs, Panel 1: Success rate or Time per run
}

function findRefCheckbox() {
  for (const lbl of ctrls.children) {
    if (lbl.children.length === 2 &&
        lbl.children[0].tagName === "INPUT" &&
        lbl.children[0].type === "checkbox" &&
        lbl.children[1].textContent === "reference values") {
      return lbl.children[0];
    }
  }
  return null;
}

// 1. Success rate tooltip comparisons and missing results
{
  assert(runHtml("sr_external").getOption().series[0].data[0] === 20,
    "live reports must load the companion ECharts script");
  const srChart = runHtml("sr_default");
  const fmt = srChart.getOption().tooltip[0].formatter;
  const measured = fmt([{ dataIndex: 0 }]);
  assert(measured.startsWith("Measured<br>"), "measured prefix: " + measured);
  assert(measured.includes("Current: 20.0% (10 completed runs)"), measured);
  assert(measured.includes("Reference: 50.0% (20 completed runs)"), measured);
  assert(measured.includes("Difference: -30.0 percentage points"), measured);

  const pending = fmt([{ dataIndex: 1 }]);
  assert(pending.includes("Current: awaiting results"), pending);
  assert(pending.includes("Reference: no reference"), pending);
  assert(!pending.includes("Difference:"), pending);

  const zero = fmt([{ dataIndex: 2 }]);
  assert(zero.includes("Current: 0.0% (5 completed runs)"), zero);
  assert(zero.includes("Difference: -50.0 percentage points"), zero);

  const refCb = findRefCheckbox();
  assert(refCb !== null && refCb.checked, "expected Reference checkbox");
  refCb.checked = false;
  charts.length = 0;
  refCb.dispatch("change");
  const currentOnly = charts[1].getOption().tooltip[0].formatter([{ dataIndex: 0 }]);
  assert(currentOnly.includes("Current: 20.0% (10 completed runs)"), currentOnly);
  assert(!currentOnly.includes("Reference:"), currentOnly);
  assert(!currentOnly.includes("Difference:"), currentOnly);
}

{
  const imp = runHtml("sr_improved").getOption().tooltip[0].formatter([{ dataIndex: 0 }]);
  assert(imp.includes("Difference: +25.0 percentage points"), imp);
}

{
  const awaitTxt = runHtml("sr_awaiting").getOption().tooltip[0].formatter([{ dataIndex: 0 }]);
  assert(awaitTxt.includes("Current: awaiting results"), awaitTxt);
  assert(awaitTxt.includes("Reference: 50.0% (20 completed runs)"), awaitTxt);
  assert(!awaitTxt.includes("Difference:"), awaitTxt);
}

{
  const noThresh = runHtml("sr_missing_thresh").getOption().tooltip[0].formatter([{ dataIndex: 2 }]);
  assert(noThresh.includes("No success threshold defined"), noThresh);
}

// 2. Time per run tooltips describe timings, pending and remaining estimates
{
  const tprChart = runHtml("tpr_default");
  const fmt = tprChart.getOption().tooltip[0].formatter;
  const measured = fmt([{ dataIndex: 0 }]);
  assert(measured.startsWith("Measured<br>"), measured);
  assert(measured.includes("Current: 1.250 s/run (10 completed runs)"), measured);
  assert(measured.includes("Reference: 4.500 s/run (20 completed runs)"), measured);

  const pending = fmt([{ dataIndex: 1 }]);
  assert(pending.includes("Current: awaiting results"), pending);
  assert(pending.includes("Reference: no reference"), pending);

  const refCb = findRefCheckbox();
  assert(refCb !== null && refCb.checked, "expected Reference checkbox");
  refCb.checked = false;
  charts.length = 0;
  refCb.dispatch("change");
  const noRefFmt = charts[1].getOption().tooltip[0].formatter;
  assert(noRefFmt([{ dataIndex: 2 }]).includes("Current: 0.000 s/run (5 completed runs)"), "zero tpr");
  assert(noRefFmt([{ dataIndex: 0 }]).includes("Current: 1.250 s/run (10 completed runs)"), "measured no ref tpr");
  assert(!noRefFmt([{ dataIndex: 0 }]).includes("Reference:"), "no ref tpr");
}

{
  const rem = runHtml("tpr_remaining").getOption().tooltip[0].formatter([{ dataIndex: 0 }]);
  assert(rem.includes("Runs left: 10<br>Estimated time remaining: ~13s"), rem);
}

for (const name of ["tpr_done20", "tpr_done21"]) {
  const done = runHtml(name).getOption().tooltip[0].formatter([{ dataIndex: 0 }]);
  assert(done.includes("Runs left: 0"), done);
  assert(done.includes("Estimated time remaining: Complete"), done);
}

// Equal run counts keep the shifted reference fitness points inside the axis.
{
  runHtml("fitness_single_run");
  const model = charts[2].getModel();
  const axis = model.getComponent("xAxis").axis;
  const reference = model.getSeries().filter((s) => s.name.startsWith("Reference"));
  assert(reference.length === 3, "expected reference range, mean and best fitness");
  for (const series of reference) {
    const x = series.getData().get("x", 0);
    assert(x > 1, "reference points should be separated from current points");
    assert(axis.containData(x), series.name + " lies outside the fitness axis: " + x);
  }
}

// 3. RUNS legend visibility & Target runs color, ELITE 10-bar cap & rotated inside labels
{
  runHtml("runs_no_ref");
  const runsOpt = charts[0].getOption();
  assert(runsOpt.legend[0].show === false, "runs without ref must hide legend");
  assert(runsOpt.series.some((s) => s.name === "Target runs"), "expected Target runs series");
  assert(findAllByClass(ctrls, "wopr-cipher").length === 0, "non-live report must hide cipher");
  assert(findAllByClass(ctrls, "hold-btn").length === 0, "non-live report must hide hold btn");

  runHtml("elite_cap");
  const refRunsOpt = charts[0].getOption();
  assert(refRunsOpt.legend[0].show === true, "runs with ref must show legend");
  const targetSeries = refRunsOpt.series.find((s) => s.name === "Target runs");
  assert(targetSeries && targetSeries.itemStyle && targetSeries.itemStyle.color === "#ff6666",
    "Target runs legend color must match #ff6666 marker");

  const eliteChart = charts[charts.length - 1];
  const eliteOpt = eliteChart.getOption();
  assert(eliteOpt.series[0].data.length === 10, "expected Current elite capped at 10");
  assert(eliteOpt.series[1].data.length === 10, "expected Reference elite capped at 10");
  assert(eliteOpt.xAxis[0].data.length === 10, "expected 10 elite rank labels");
  assert(eliteOpt.series[0].label.position === "inside" && eliteOpt.series[0].label.rotate === 90,
    "expected rotated inside accuracy label on elite bars");
  const gap = eliteOpt.yAxis[0].boundaryGap;
  assert(Array.isArray(gap) && gap[0] === "10%" && gap[1] === "10%",
    "expected 10% yAxis boundaryGap on elite chart");
}

// 4. Live dashboard controls: header alerts, cipher locking, HOLD/P, 1-4/Esc, [NO HIT] & COPY
{
  runHtml("live_ui");
  const subText = collectText(subtitle);
  assert(subText.includes("1 FAILED"), "expected failed alert in subtitle: " + subText);
  assert(subText.includes("1 REGRESSION"), "expected regression alert in subtitle: " + subText);

  const ciphers = findAllByClass(ctrls, "wopr-cipher");
  assert(ciphers.length === 1 && ciphers[0].children.length === 10, "expected 10-char cipher");
  assert(ciphers[0].hidden, "cipher must stay hidden in standard theme");
  themeButton.onclick();
  assert(!ciphers[0].hidden, "cipher must become visible in WOPR terminal mode");
  const lockedCount = ciphers[0].children.filter((s) => s.classList.contains("locked")).length;
  // bad: failed (10/10) + nohit: 5/10 => 15/20 => floor(0.75 * 10) = 7 locked
  assert(lockedCount === 7, "expected 7 locked cipher chars, got " + lockedCount);
  themeButton.onclick();
  assert(ciphers[0].hidden, "cipher must hide again when leaving WOPR terminal mode");

  const holds = findAllByClass(ctrls, "hold-btn");
  assert(holds.length === 1 && collectText(holds[0]).includes("LIVE"), "expected LIVE hold btn");
  holds[0].dispatch("click");
  assert(collectText(holds[0]).includes("HOLD"), "click must pause live refresh");
  dispatchDoc("keydown", { key: "p", ctrlKey: true, target: body });
  assert(collectText(holds[0]).includes("HOLD"), "Ctrl+P must not toggle HOLD");
  dispatchDoc("keydown", { key: "p", target: { tagName: "INPUT" } });
  assert(collectText(holds[0]).includes("HOLD"), "input P must not toggle HOLD");
  dispatchDoc("keydown", { key: "p", target: body });
  assert(collectText(holds[0]).includes("LIVE"), "P shortcut must resume LIVE");

  dispatchDoc("keydown", { key: "2", target: body });
  assert(dash.children.length === 1 && dash.children[0].classList.contains("maximised"),
    "key 2 must maximise second panel");
  dispatchDoc("keydown", { key: "Escape", target: body });
  assert(dash.children.length === 4, "Escape must restore 2x2 grid");

  const dashText = collectText(dash);
  assert(dashText.includes("No elite runs recorded."), "expected empty elite note for failed run");

  const copyBtns = findAllByClass(detailsBody, "copy-btn");
  assert(copyBtns.length === 3, "expected 3 copy buttons, got " + copyBtns.length);
  for (const btn of copyBtns) btn.dispatch("click");
  assert(JSON.stringify(copiedTexts) === JSON.stringify(["ref_p", "x + 1", "x"]),
    "unexpected clipboard contents: " + JSON.stringify(copiedTexts));
  assert(findAllByClass(detailsBody, "num").length > 0, "expected tabular .num cells");

  runHtml("queued_only");
  assert(findAllByClass(ctrls, "wopr-cipher").length === 0,
    "queued-only live report must hide cipher");
  themeButton.onclick();
  assert(findAllByClass(ctrls, "wopr-cipher").length === 0,
    "queued-only live report must stay without cipher even in terminal mode");
}

// 5. Theme toggle toast quotes & persistence across reload
{
  const originalSetTimeout = globalThis.setTimeout;
  const originalClearTimeout = globalThis.clearTimeout;
  const timers = new Map();
  let nextTimer = 1;
  globalThis.setTimeout = (callback, delay) => {
    const id = nextTimer++;
    timers.set(id, { callback, delay });
    return id;
  };
  globalThis.clearTimeout = (id) => timers.delete(id);
  runHtml("sr_default");
  const message = findAllByClass(body, "theme-message").slice(-1)[0];
  assert(message.hidden, "theme message must be hidden on page load");
  assert(!body.classList.contains("terminal"), "standard theme must be the default");
  assert(charts[0].getOption().series[0].itemStyle.color === "#4aa8ff",
    "standard theme must use blue diagram color");
  assert(themeButton.getAttribute("aria-pressed") === "false", "theme toggle must start unpressed");
  charts.length = 0;
  themeButton.onclick();
  assert(!message.hidden && message.textContent === "GREETINGS PROFESSOR FALKEN",
    "activation must show the greeting");
  assert(charts[0].getOption().series[0].itemStyle.color === "#38c96c",
    "WOPR terminal theme must use phosphor green bar color");
  assert(charts[1].getOption().series[0].itemStyle.color === "#38c96c",
    "WOPR terminal theme must use phosphor green success-rate bar color");
  const greetingTimer = nextTimer - 1;
  themeButton.onclick();
  assert(!message.hidden && message.textContent === "THE ONLY WINNING MOVE IS NOT TO PLAY",
    "deactivation must show the farewell");
  assert(!timers.has(greetingTimer), "switching themes must cancel the previous message timeout");
  const farewellTimer = timers.get(nextTimer - 1);
  assert(farewellTimer.delay === 4000, "theme message must last four seconds");
  farewellTimer.callback();
  assert(message.hidden, "theme message must disappear after its timeout");
  themeButton.onclick();
  assert(body.classList.contains("terminal"), "clicking WOPR must enable the terminal theme");
  assert(themeButton.getAttribute("aria-pressed") === "true", "theme toggle must reflect the active theme");
  runHtml("sr_default", true);
  assert(body.classList.contains("terminal"), "terminal theme must survive a refresh");
  const persistedMsg = findAllByClass(body, "theme-message").slice(-1)[0];
  assert(persistedMsg && !persistedMsg.hidden &&
    persistedMsg.textContent === "GREETINGS PROFESSOR FALKEN",
    "active theme message must survive a refresh");
  themeButton.onclick();
  assert(!body.classList.contains("terminal"), "clicking WOPR again must restore the standard theme");
  runHtml("sr_default", true);
  assert(!body.classList.contains("terminal"), "standard theme choice must survive a refresh");
  globalThis.setTimeout = originalSetTimeout;
  globalThis.clearTimeout = originalClearTimeout;

  runHtml("fitness_single_run");
  charts.length = 0;
  themeButton.onclick();
  assert(charts[2].getOption().series.find((s) => s.name === "Current mean").itemStyle.color === "#ffd43b",
    "WOPR terminal theme must keep yellow error-bar color");
}
