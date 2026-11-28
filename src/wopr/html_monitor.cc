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

#include "html_monitor.h"

#include "html_shell.h"

#include <string>
#include <string_view>

namespace ultra::wopr
{

static void append_json(std::string &out, const dynamic_sequence &dr)
{
  out.push_back('{');
  append_json_field(out, "xs", dr.xs, false);
  append_json_field(out, "fit_best", dr.fit_best);
  append_json_field(out, "fit_mean", dr.fit_mean);
  append_json_field(out, "fit_std_dev", dr.fit_std_dev);
  append_json_field(out, "len_mean", dr.len_mean);
  append_json_field(out, "len_std_dev", dr.len_std_dev);
  append_json_field(out, "len_max", dr.len_max);

  append_json_field(out, "crossover_types", [&](std::string &cs)
  {
    cs.push_back('{');
    bool first_ct(true);
    for (const auto &[ct, nums] : dr.crossover_types)
    {
      append_json_field(cs, "CT" + std::to_string(ct), nums, !first_ct);
      first_ct = false;
    }
    cs.push_back('}');
  });

  append_json_field(out, "best_prg", dr.best_prg);
  out.push_back('}');
}

static void append_json(std::string &out, const population_sequence &pr)
{
  out.push_back('{');
  append_json_field(out, "generation", pr.generation, false);
  append_json_field(out, "fit", pr.fit);
  append_json_field(out, "obs", pr.obs);
  append_json_field(out, "xs", pr.xs);
  append_json_field(out, "fit_entropy", pr.fit_entropy);
  out.push_back('}');
}

static void append_json(std::string &out, const layers_line &lr)
{
  const auto matrix(build_layers_fit_matrix(lr));

  out.push_back('{');
  append_json_field(out, "generation", lr.generation, false);
  append_json_field(out, "rows", matrix.rows);
  append_json_field(out, "cols", matrix.cols);
  append_json_field(out, "ind_max", matrix.ind_max);
  append_json_field(out, "fit_min", matrix.fit_min);
  append_json_field(out, "fit_max", matrix.fit_max);
  append_json_field(out, "heatmap", matrix.values);
  append_json_field(out, "age_sup", lr.age_sup);
  append_json_field(out, "age_mean", lr.age_mean);
  append_json_field(out, "age_min", lr.age_min);
  append_json_field(out, "age_max", lr.age_max);
  out.push_back('}');
}

namespace
{

[[nodiscard]] std::string build_monitor_json(const monitor_state &state,
                                             const monitor::options &options,
                                             bool live)
{
  std::string out;
  out.reserve(4096);

  out.push_back('{');
  append_json_field(out, "live", live, false);
  append_json_field(out, "refresh_seconds", options.interval);
  append_json_field(out, "window", options.window);
  append_json_field(out, "has_dynamic",
                    !options.slog.dynamic_file_path.empty()
                    || !state.dynamic_runs.empty());
  append_json_field(out, "has_population",
                    !options.slog.population_file_path.empty()
                    || !state.population_runs.empty());
  append_json_field(out, "has_layers",
                    !options.slog.layers_file_path.empty()
                    || !state.layers_runs.empty());
  append_json_field(out, "dynamic_runs", state.dynamic_runs);
  append_json_field(out, "population_runs", state.population_runs);
  append_json_field(out, "layers_runs", state.layers_runs);
  out.push_back('}');

  return out;
}

constexpr std::string_view monitor_body = R"HTML(
<header>
  <div class="brand">
    <h1><button id="theme-toggle" type="button" aria-label="WOPR Terminal theme" aria-pressed="false">WOPR</button></h1>
    <span id="subtitle"></span>
  </div>
  <div class="controls" id="controls"></div>
</header>
<main>
  <section class="dashboard" id="dashboard"></section>
</main>
)HTML";

constexpr std::string_view monitor_js = R"JS(
(() => {
  const {
    data, state, saveState, disposeCharts, initChart, h,
    makeCheckbox, baseGrid, splitAxis, primaryColor, barColor, primaryFill,
    renderAll, start
  } = createDashboard({
    storagePrefix: "wopr-monitor:",
    initState: (d, s) => ({
      window: s.window !== undefined ? Number(s.window) : d.window,
      run: s.run !== undefined ? s.run : null,
      tabs: s.tabs || {},
      showBest: s.showBest !== undefined ? s.showBest : true,
      showLongest: s.showLongest !== undefined ? s.showLongest : true,
      bestIdx: s.bestIdx || {}
    }),
    defaultVisible: (d) => ({
      dynamic: d.has_dynamic,
      population: d.has_population,
      layers_fit: d.has_layers,
      layers_age: d.has_layers
    }),
    serializeState: (s) => ({
      window: s.window,
      run: s.run,
      tabs: s.tabs,
      showBest: s.showBest,
      showLongest: s.showLongest,
      bestIdx: s.bestIdx
    }),
    getControls: () => [
      ["dynamic", "Dynamic"],
      ["population", "Population"],
      ["layers_fit", "Layers fit."],
      ["layers_age", "Layers age"]
    ],
    appendControls: (controls) => {
      const total = getTotalRuns();
      if (total > 0) {
        const picker = h("label", { className: "run-picker" });
        picker.appendChild(document.createTextNode("run:"));
        const prevBtn = h("button", { type: "button", className: "run-prev" }, "<");
        const runSel = h("select", { className: "run-select" });
        runSel.appendChild(h("option", { value: "latest" }, `Latest (${total - 1})`));
        for (let r = 0; r < total; ++r) {
          runSel.appendChild(h("option", { value: String(r) }, `Run ${r}`));
        }
        const nextBtn = h("button", { type: "button", className: "run-next" }, ">");

        const syncPicker = () => {
          const idx = selectedRunIndex();
          runSel.value = state.run === null ? "latest" : String(idx);
          prevBtn.disabled = idx <= 0;
          nextBtn.disabled = idx < 0 || idx + 1 >= total;
        };

        prevBtn.addEventListener("click", () => {
          state.run = Math.max(0, selectedRunIndex() - 1);
          syncPicker();
          renderAll();
        });
        nextBtn.addEventListener("click", () => {
          const next = selectedRunIndex() + 1;
          state.run = next >= total - 1 ? null : next;
          syncPicker();
          renderAll();
        });
        runSel.addEventListener("change", () => {
          state.run = runSel.value === "latest" ? null : Number(runSel.value);
          syncPicker();
          renderAll();
        });

        syncPicker();
        picker.appendChild(prevBtn);
        picker.appendChild(runSel);
        picker.appendChild(nextBtn);
        controls.appendChild(picker);
      }

      const winLbl = h("label");
      winLbl.appendChild(document.createTextNode("window:"));
      const winInput = h("input", { type: "number", min: "0", max: "100000", value: String(state.window) });
      winInput.style.width = "82px";
      winInput.addEventListener("change", () => {
        state.window = Math.max(0, Number(winInput.value) || 0);
        renderAll();
      });
      winLbl.appendChild(winInput);
      controls.appendChild(winLbl);
    },
    renderHeader: () => {
      const subtitle = document.getElementById("subtitle");
      const total = getTotalRuns();
      const parts = ["Monitor"];
      if (total > 0) {
        const idx = selectedRunIndex();
        parts.push(total > 1 ? `Run ${idx} (${total} runs)` : `Run ${idx}`);
      }
      if (data.live && data.refresh_seconds > 0) {
        parts.push(`refreshing every ${data.refresh_seconds}s`);
      }
      subtitle.textContent = parts.join(" · ");
    },
    getCipherState: () => ({
      active: Boolean(data.live),
      progress: 0
    }),
    getPanels: () => [
      { key: "dynamic", title: "DYNAMICS", available: data.has_dynamic, render: renderDynamic },
      { key: "population", title: "POPULATION", available: data.has_population, render: renderPopulation },
      { key: "layers_fit", title: "FITNESS BY LAYER", available: data.has_layers, render: renderLayersFit },
      { key: "layers_age", title: "AGE BY LAYER", available: data.has_layers, render: renderLayersAge }
    ]
  });

  function hasRunData(run) {
    const dr = (data.dynamic_runs || [])[run];
    const pr = (data.population_runs || [])[run];
    const lr = (data.layers_runs || [])[run];
    return Boolean(
      (dr && dr.xs && dr.xs.length) ||
      (pr && pr.fit && pr.fit.length) ||
      (lr && ((lr.heatmap && lr.heatmap.length) || (lr.age_mean && lr.age_mean.length)))
    );
  }

  function getTotalRuns() {
    let n = Math.max(
      (data.dynamic_runs || []).length,
      (data.population_runs || []).length,
      (data.layers_runs || []).length
    );
    while (n > 0 && !hasRunData(n - 1)) --n;
    return n;
  }

  function selectedRunIndex() {
    const total = getTotalRuns();
    if (!total) return -1;
    if (state.run === null || state.run === undefined) return total - 1;
    return Math.min(total - 1, Math.max(0, Number(state.run)));
  }

  function sliceWindow(arr) {
    if (!state.window || state.window <= 0 || arr.length <= state.window) return arr;
    return arr.slice(arr.length - state.window);
  }

  function makeTabbedView(container, sectionKey, tabSpecs, renderBody) {
    let activeTab = state.tabs[sectionKey] || tabSpecs[0][0];

    const tabs = h("div", { className: "tabs" });
    const content = h("div");
    const tabCharts = [];

    function drawTab() {
      disposeCharts(tabCharts);
      content.innerHTML = "";
      renderBody(activeTab, content, tabCharts, drawTab);
    }

    for (const [id, title] of tabSpecs) {
      const b = h("button", { type: "button", className: activeTab === id ? "active" : "" }, title);
      b.addEventListener("click", () => {
        activeTab = id;
        state.tabs[sectionKey] = id;
        for (const child of tabs.children) child.classList.remove("active");
        b.classList.add("active");
        saveState();
        drawTab();
      });
      tabs.appendChild(b);
    }

    container.appendChild(tabs);
    container.appendChild(content);
    drawTab();
  }

  function makeLineChartOption(xLabel, yLabel, series) {
    return {
      backgroundColor: "transparent",
      tooltip: { trigger: "axis" },
      legend: { top: 4, textStyle: { color: "#cfd8e3" } },
      grid: baseGrid({ right: 16, top: 34, bottom: 38 }),
      xAxis: splitAxis({ name: xLabel, nameLocation: "middle", nameGap: 24, scale: true }),
      yAxis: splitAxis({ name: yLabel, scale: true }),
      dataZoom: [{ type: "inside" }],
      series
    };
  }

  function makeErrorBarLineOption(xLabel, yLabel, xs, mean, sd, extraName, extraData, showExtra) {
    const errData = [];
    const meanPts = [];
    const extraPts = [];
    for (let i = 0; i < xs.length; ++i) {
      errData.push([xs[i], mean[i] - sd[i], mean[i] + sd[i]]);
      meanPts.push([xs[i], mean[i]]);
      extraPts.push([xs[i], extraData[i]]);
    }

    const series = [
      {
        name: "StdDev",
        type: "custom",
        itemStyle: { color: "#9aa8b8" },
        encode: { x: 0, y: [1, 2] },
        renderItem(_params, api) {
          const x = api.value(0), lo = api.value(1), hi = api.value(2);
          const pLo = api.coord([x, lo]), pHi = api.coord([x, hi]);
          return {
            type: "line",
            shape: { x1: pLo[0], y1: pLo[1], x2: pHi[0], y2: pHi[1] },
            style: { stroke: "#9aa8b8", lineWidth: 1, opacity: 0.7 }
          };
        },
        data: errData,
        silent: true
      },
      {
        name: "Mean",
        type: "line",
        showSymbol: xs.length <= 200,
        symbol: "rect",
        symbolSize: 5,
        itemStyle: { color: primaryColor() },
        lineStyle: { color: primaryColor(), width: 2 },
        data: meanPts
      }
    ];

    if (showExtra) {
      series.push({
        name: extraName,
        type: "line",
        showSymbol: false,
        itemStyle: { color: "#66e666" },
        lineStyle: { color: "#66e666", width: 2 },
        data: extraPts
      });
    }

    return makeLineChartOption(xLabel, yLabel, series);
  }

  const dynamicErrorTabs = {
    fitness: {
      yLabel: "Fit",
      meanKey: "fit_mean",
      sdKey: "fit_std_dev",
      extraKey: "fit_best",
      extraLabel: "Best",
      stateKey: "showBest",
      showPrograms: true
    },
    length: {
      yLabel: "Length",
      meanKey: "len_mean",
      sdKey: "len_std_dev",
      extraKey: "len_max",
      extraLabel: "Longest",
      stateKey: "showLongest",
      showPrograms: false
    }
  };

  function renderDynamic(container) {
    const run = selectedRunIndex();
    const dr = (data.dynamic_runs || [])[run];
    if (!dr || !dr.xs || !dr.xs.length) {
      container.appendChild(h("div", { className: "note" }, "No dynamics data for this run."));
      return;
    }

    makeTabbedView(
      container, "dyn",
      [["fitness", "Fitness dynamic"], ["length", "Length dynamic"], ["crossover", "Crossover dynamics"]],
      (activeTab, content, tabCharts, drawTab) => {
        const xs = sliceWindow(dr.xs);
        const spec = dynamicErrorTabs[activeTab];

        if (spec) {
          const bar = h("div", { className: "toolbar" });
          if (spec.showPrograms && dr.best_prg && dr.best_prg.length) {
            const sel = h("select");
            for (let k = dr.best_prg.length - 1; k >= 0; --k) {
              sel.appendChild(h("option", { value: k }, dr.best_prg[k]));
            }
            const defIdx = dr.best_prg.length - 1;
            const curIdx = state.bestIdx[run] !== undefined
              ? Math.min(defIdx, Math.max(0, state.bestIdx[run])) : defIdx;
            sel.value = String(curIdx);
            sel.addEventListener("change", () => {
              state.bestIdx[run] = Number(sel.value);
              saveState();
            });
            bar.appendChild(h("span", { className: "note" }, "Best programs:"));
            bar.appendChild(sel);
          }

          bar.appendChild(makeCheckbox(spec.extraLabel, state[spec.stateKey], (checked) => {
            state[spec.stateKey] = checked;
            saveState();
            drawTab();
          }));
          content.appendChild(bar);

          const box = h("div", { className: "chart-box" });
          content.appendChild(box);
          initChart(box, tabCharts).setOption(makeErrorBarLineOption(
            "Generation", spec.yLabel, xs,
            sliceWindow(dr[spec.meanKey]), sliceWindow(dr[spec.sdKey]),
            spec.extraLabel, sliceWindow(dr[spec.extraKey]), state[spec.stateKey]
          ));
        } else {
          const box = h("div", { className: "chart-box" });
          content.appendChild(box);
          const series = [];
          for (const [ct, nums] of Object.entries(dr.crossover_types || {})) {
            const sliced = sliceWindow(nums);
            const pts = xs.map((x, idx) => [x, sliced[idx]]);
            series.push({ name: ct, type: "line", showSymbol: false, data: pts });
          }
          initChart(box, tabCharts).setOption(
            makeLineChartOption("Generation", "Crossover spreading", series));
        }
      }
    );
  }

  function renderPopulation(container) {
    const run = selectedRunIndex();
    const pr = (data.population_runs || [])[run];
    if (!pr || !pr.fit || !pr.fit.length) {
      container.appendChild(h("div", { className: "note" }, "No population data for this run."));
      return;
    }

    makeTabbedView(
      container, "pop",
      [["hist", "Fitness histogram"], ["entropy", "Fitness entropy"]],
      (activeTab, content, tabCharts) => {
        const box = h("div", { className: "chart-box" });
        content.appendChild(box);
        const isHist = activeTab === "hist";
        const pts = isHist
          ? pr.fit.map((f, idx) => [f, pr.obs[idx]])
          : pr.xs.map((x, idx) => [x, pr.fit_entropy[idx]]);

        initChart(box, tabCharts).setOption({
          backgroundColor: "transparent",
          title: { text: "Generation " + pr.generation, left: "center", textStyle: { color: "#cfd8e3", fontSize: 13 } },
          tooltip: { trigger: "axis" },
          grid: baseGrid({ right: 16, top: 36, bottom: 38 }),
          xAxis: splitAxis({ name: isHist ? "Fitness" : "Generation", nameLocation: "middle", nameGap: 24, scale: true }),
          yAxis: splitAxis(isHist ? { name: "Individuals", min: 0 } : { name: "Entropy", scale: true }),
          series: [isHist
            ? { type: "bar", itemStyle: { color: barColor() }, data: pts }
            : { type: "line", showSymbol: false, itemStyle: { color: primaryColor() }, areaStyle: { color: primaryFill(0.25) }, data: pts }]
        });
      }
    );
  }

  function renderLayerRun(container, hasData, makeOption) {
    const run = selectedRunIndex();
    const lr = (data.layers_runs || [])[run];
    if (!lr || !hasData(lr)) {
      container.appendChild(h("div", { className: "note" }, "No layer data for this run."));
      return;
    }

    const box = h("div", { className: "chart-box" });
    container.appendChild(box);
    initChart(box).setOption(makeOption(lr));
  }

  function renderLayersFit(container) {
    renderLayerRun(
      container,
      (lr) => lr.heatmap && lr.heatmap.length && lr.rows && lr.cols,
      (lr) => {
        const xLabels = [];
        for (let c = 0; c < lr.cols; ++c) {
          if (c === 0) xLabels.push("1");
          else if (c + 1 === lr.cols) xLabels.push(String(lr.ind_max));
          else xLabels.push("");
        }
        const yLabels = [];
        for (let r = 0; r < lr.rows; ++r) yLabels.push("L" + r);

        const heatData = [];
        for (let r = 0; r < lr.rows; ++r) {
          for (let c = 0; c < lr.cols; ++c) {
            heatData.push([c, r, lr.heatmap[r * lr.cols + c]]);
          }
        }

        const vMin = lr.fit_min;
        const vMax = lr.fit_max > lr.fit_min ? lr.fit_max : lr.fit_min + 1e-9;

        return {
          backgroundColor: "transparent",
          title: { text: "Fitness by layer - Generation " + lr.generation, left: "center", textStyle: { color: "#cfd8e3", fontSize: 13 } },
          tooltip: {
            position: "top",
            formatter(p) {
              return `L${p.value[1]}: ${Number(p.value[2]).toPrecision(5)}`;
            }
          },
          grid: baseGrid({ left: 48, right: 86, top: 36, bottom: 28 }),
          xAxis: { type: "category", data: xLabels, axisTick: { show: false } },
          yAxis: { type: "category", inverse: true, data: yLabels, axisTick: { show: false } },
          visualMap: {
            min: vMin,
            max: vMax,
            calculable: true,
            orient: "vertical",
            right: 8,
            top: "center",
            inRange: { color: ["#1a0000", "#8b0000", "#e64800", "#ffae00", "#ffffcc"] },
            textStyle: { color: "#cfd8e3" }
          },
          series: [{ type: "heatmap", data: heatData }]
        };
      }
    );
  }

  function renderLayersAge(container) {
    renderLayerRun(
      container,
      (lr) => lr.age_mean && lr.age_mean.length,
      (lr) => {
        const errData = [];
        const meanPts = [];
        const markLines = [];
        for (let layer = 0; layer < lr.age_mean.length; ++layer) {
          errData.push([layer, lr.age_min[layer], lr.age_max[layer]]);
          meanPts.push([lr.age_mean[layer], layer]);
          if (lr.age_sup[layer]) {
            markLines.push({
              xAxis: lr.age_sup[layer],
              label: { formatter: `L${layer} <${lr.age_sup[layer]}`, color: "#ffd43b" }
            });
          }
        }

        return {
          backgroundColor: "transparent",
          title: { text: "Age by layer - Generation " + lr.generation, left: "center", textStyle: { color: "#cfd8e3", fontSize: 13 } },
          tooltip: {
            trigger: "item",
            formatter(p) {
              const layer = p.seriesType === "scatter" ? p.value[1] : p.value[0];
              return `Layer ${layer}<br>Mean: ${lr.age_mean[layer].toFixed(2)}<br>Range: [${lr.age_min[layer]}, ${lr.age_max[layer]}]`;
            }
          },
          grid: baseGrid({ top: 36, bottom: 38 }),
          xAxis: splitAxis({ name: "Age", nameLocation: "middle", nameGap: 24, scale: true }),
          yAxis: splitAxis({ name: "Layer", minInterval: 1 }),
          series: [
            {
              name: "Age range",
              type: "custom",
              encode: { x: [1, 2], y: 0 },
              renderItem(_params, api) {
                const layer = api.value(0), lo = api.value(1), hi = api.value(2);
                const pLo = api.coord([lo, layer]), pHi = api.coord([hi, layer]);
                const cap = 6;
                return {
                  type: "group",
                  children: [
                    { type: "line", shape: { x1: pLo[0], y1: pLo[1], x2: pHi[0], y2: pHi[1] }, style: { stroke: primaryColor(), lineWidth: 3 } },
                    { type: "line", shape: { x1: pLo[0], y1: pLo[1] - cap, x2: pLo[0], y2: pLo[1] + cap }, style: { stroke: primaryColor(), lineWidth: 2 } },
                    { type: "line", shape: { x1: pHi[0], y1: pHi[1] - cap, x2: pHi[0], y2: pHi[1] + cap }, style: { stroke: primaryColor(), lineWidth: 2 } }
                  ]
                };
              },
              data: errData,
              markLine: markLines.length ? { symbol: "none", lineStyle: { color: "#ffd43b", type: "dashed" }, data: markLines } : undefined
            },
            {
              name: "Mean age",
              type: "scatter",
              symbolSize: 8,
              itemStyle: { color: "#ff9f43" },
              data: meanPts
            }
          ]
        };
      }
    );
  }

  start();
})();
)JS";

}  // namespace

bool write_monitor_html(const std::filesystem::path &output_path,
                        const monitor_state &state,
                        const monitor::options &options,
                        bool live)
{
  const auto payload(build_monitor_json(state, options, live));
  return write_echarts_html(output_path, "WOPR Monitor", payload, monitor_body,
                            monitor_js, live);
}

}  // namespace ultra::wopr
