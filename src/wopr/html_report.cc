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

#include "html_report.h"

#include "html_shell.h"

#include <string>
#include <string_view>
#include <utility>

namespace ultra::wopr
{

namespace
{

[[nodiscard]] const char *task_state_label(rs::task_state state) noexcept
{
  switch (state)
  {
  case rs::task_state::queued:    return "Queued";
  case rs::task_state::running:   return "Running";
  case rs::task_state::finished:  return "Finished";
  case rs::task_state::failed:    return "Failed";
  case rs::task_state::cancelled: return "Cancelled";
  }
  std::unreachable();
}

}  // namespace

static void append_json(std::string &out, const side_snapshot::elite_entry &e)
{
  out.push_back('{');
  append_json_field(out, "id", e.id, false);
  append_json_field(out, "fit", e.fit);
  append_json_field(out, "acc", e.acc);
  out.push_back('}');
}

static void append_json(std::string &out, const side_snapshot &side)
{
  out.push_back('{');
  append_json_field(out, "runs", side.runs, false);
  append_json_field(out, "success_rate", side.success_rate);
  append_json_field(out, "time_per_run", side.time_per_run);
  append_json_field(out, "fit_best", side.fit_best);
  append_json_field(out, "fit_mean", side.fit_mean);
  append_json_field(out, "fit_std_dev", side.fit_std_dev);
  append_json_field(out, "elite", side.elite);
  append_json_field(out, "best_prg", side.best_prg);
  out.push_back('}');
}

static void append_json(std::string &out, const dataset_snapshot &ds)
{
  out.push_back('{');
  append_json_field(out, "name", ds.name, false);
  append_json_field(out, "capacity_runs", ds.capacity_runs);
  append_json_field(out, "has_threshold", ds.has_threshold);
  append_json_field(out, "remaining_label", ds.remaining_label);
  if (ds.status)
    append_json_field(out, "status", task_state_label(ds.status->state));
  else
    append_json_field(out, "status", nullptr);
  append_json_field(out, "error",
                    ds.status ? ds.status->error : std::string{});
  append_json_field(out, "current", ds.current);
  append_json_field(out, "reference", ds.reference);
  out.push_back('}');
}

namespace
{

[[nodiscard]] std::string build_json_payload(const results_snapshot &snapshot,
                                             bool live,
                                             unsigned refresh_seconds)
{
  std::string out;
  out.reserve(4096);

  out.push_back('{');
  append_json_field(out, "live", live, false);
  append_json_field(out, "refresh_seconds", refresh_seconds);
  append_json_field(out, "references_available",
                    snapshot.references_available);
  append_json_field(out, "show_success_rate", snapshot.show_success_rate);
  append_json_field(out, "datasets", snapshot.datasets);
  out.push_back('}');

  return out;
}

constexpr std::string_view results_body = R"HTML(
<header>
  <div class="brand">
    <h1><button id="theme-toggle" type="button" aria-label="WOPR Terminal theme" aria-pressed="false">WOPR</button></h1>
    <span id="subtitle"></span>
  </div>
  <div class="controls" id="controls"></div>
</header>
<main>
  <section class="dashboard" id="dashboard"></section>
  <section class="panel" id="details-panel" style="min-height: 0;">
    <div class="panel-header">
      <div class="panel-title">DATASETS &amp; BEST PROGRAMS</div>
    </div>
    <div class="panel-body table-wrap" id="details-body"></div>
  </section>
</main>
)HTML";

constexpr std::string_view results_js = R"JS(
(() => {
  const {
    data, state, initChart, h, makeCheckbox, makeCodeBlock,
    baseGrid, splitAxis, primaryColor, barColor, renderAll, start
  } = createDashboard({
    storagePrefix: "wopr-results:",
    initState: (d, s) => ({
      showRef: s.showRef !== undefined ? s.showRef : d.references_available
    }),
    defaultVisible: () => ({
      runs: true,
      metric: true,
      fad: true,
      elite: true,
      details: true
    }),
    serializeState: (s) => ({ showRef: s.showRef }),
    getControls: () => [
      ["runs", "runs"],
      ["metric", data.show_success_rate ? "success rate" : "time per run"],
      ["fad", "fitness across datasets"],
      ["elite", "elite"],
      ["details", "best programs"]
    ],
    appendControls: (controls) => {
      if (!data.references_available) return;
      controls.appendChild(makeCheckbox("reference values", state.showRef, (checked) => {
        state.showRef = checked;
        renderAll();
      }));
    },
    renderHeader: () => {
      const subtitle = document.getElementById("subtitle");
      const list = data.datasets || [];
      let running = 0;
      let queued = 0;
      let failed = 0;
      let regressions = 0;
      let doneRuns = 0;
      let totalRuns = 0;

      for (const d of list) {
        if (d.status === "Running") ++running;
        else if (d.status === "Queued") ++queued;
        else if (d.status === "Failed") ++failed;

        if (d.capacity_runs > 0) {
          totalRuns += d.capacity_runs;
          doneRuns += Math.min(d.current.runs || 0, d.capacity_runs);
        }

        if (state.showRef && d.current.fit_best !== null && d.reference.fit_best !== null &&
            d.current.fit_best < d.reference.fit_best) {
          ++regressions;
        }
      }

      const parts = [`${list.length} datasets`];
      if (totalRuns > 0 && (running > 0 || queued > 0)) parts.push(`${doneRuns}/${totalRuns} runs`);
      if (running > 0) parts.push(`${running} running`);
      if (queued > 0) parts.push(`${queued} queued`);

      subtitle.innerHTML = "";
      subtitle.appendChild(document.createTextNode(parts.join(" · ")));
      if (failed > 0) {
        subtitle.appendChild(h("span", { className: "hdr-alert hdr-alert-failed" }, `${failed} FAILED`));
      }
      if (regressions > 0) {
        subtitle.appendChild(h("span", { className: "hdr-alert hdr-alert-warn" },
          `${regressions} REGRESSION${regressions > 1 ? "S" : ""}`));
      }
    },
    getCipherState: () => {
      const list = data.datasets || [];
      const active = Boolean(data.live) &&
        list.some((d) => d.status === "Running");
      if (!active) return { active: false, progress: 0 };

      let total = 0;
      let done = 0;
      for (const d of list) {
        const cap = Math.max(d.capacity_runs || 1, 1);
        total += cap;
        const terminal = d.status === "Finished" || d.status === "Failed" || d.status === "Cancelled";
        done += terminal ? cap : Math.min(d.current && d.current.runs ? d.current.runs : 0, cap);
      }
      return { active: true, progress: total > 0 ? done / total : 0 };
    },
    getPanels: () => [
      { key: "runs", title: "RUNS", render: renderRuns },
      {
        key: "metric",
        title: data.show_success_rate ? "SUCCESS RATE" : "TIME PER RUN",
        render: data.show_success_rate ? renderSuccessRate : renderTimePerRun
      },
      { key: "fad", title: "FITNESS ACROSS DATASETS", render: renderFitnessAcrossDatasets },
      { key: "elite", title: "ELITE RUNS", render: renderElite }
    ],
    afterRender: () => renderDetails()
  });

  const dsList = data.datasets || [];
  const labels = dsList.map((d) => d.name);

  function escapeHtml(s) {
    return String(s)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;")
      .replace(/'/g, "&#39;");
  }

  function fmtNum(v, digits = 3) {
    if (v === null || !Number.isFinite(v)) return "N/A";
    if (Math.abs(v) >= 1000 || (Math.abs(v) > 0 && Math.abs(v) < 0.001)) {
      return v.toExponential(2);
    }
    return Number(v.toFixed(digits)).toString();
  }

  function makeBarSeries(currentData, refData, extra = {}) {
    const series = [Object.assign({
      name: "Current",
      type: "bar",
      itemStyle: { color: barColor() },
      barMaxWidth: 28,
      data: currentData
    }, extra)];

    if (state.showRef) {
      series.push(Object.assign({
        name: "Reference",
        type: "bar",
        itemStyle: { color: "#ff9f43" },
        barMaxWidth: 28,
        data: refData
      }, extra));
    }

    return series;
  }

  function makeDatasetBarOption({ yAxis, tooltipHtml, series }) {
    const n = dsList.length;
    return {
      backgroundColor: "transparent",
      tooltip: {
        trigger: "axis",
        axisPointer: { type: "shadow" },
        formatter(params) { return tooltipHtml(params[0].dataIndex); }
      },
      legend: { show: state.showRef, top: 4, textStyle: { color: "#cfd8e3" } },
      grid: baseGrid({ bottom: n > 6 ? 64 : 36 }),
      xAxis: {
        type: "category",
        data: labels,
        axisLabel: { color: "#cfd8e3", rotate: n > 6 ? 30 : 0 }
      },
      yAxis: splitAxis(yAxis),
      series
    };
  }

  function renderRuns(container) {
    const box = h("div", { className: "chart-box" });
    container.appendChild(box);
    const n = dsList.length;
    if (!n) return;

    const curRuns = dsList.map((d) => d.current.runs);
    const refRuns = dsList.map((d) => d.reference.runs);
    const maxY = Math.max(
      0,
      ...dsList.map((d) => Math.max(d.capacity_runs, d.current.runs, state.showRef ? d.reference.runs : 0))
    ) + 1;
    const series = makeBarSeries(curRuns, refRuns);

    const targets = [];
    for (let i = 0; i < n; ++i) {
      if (dsList[i].capacity_runs > 1) targets.push([i, dsList[i].capacity_runs]);
    }
    if (targets.length) {
      series.push({
        name: "Target runs",
        type: "custom",
        itemStyle: { color: "#ff6666" },
        renderItem(_params, api) {
          const idx = api.value(0);
          const cap = api.value(1);
          const pt = api.coord([idx, cap]);
          const span = api.size([1, 0])[0] * 0.6;
          return {
            type: "line",
            shape: {
              x1: pt[0] - span / 2,
              y1: pt[1],
              x2: state.showRef ? pt[0] : pt[0] + span / 2,
              y2: pt[1]
            },
            style: { stroke: "#ff6666", lineWidth: 2.5 }
          };
        },
        data: targets,
        z: 10
      });
    }

    initChart(box).setOption(makeDatasetBarOption({
      yAxis: { name: "Runs", min: 0, max: maxY },
      tooltipHtml(i) {
        const d = dsList[i];
        const lines = [
          escapeHtml(d.name),
          `Current: ${d.current.runs} / ${d.capacity_runs} runs`
        ];
        if (state.showRef) lines.push(`Reference: ${d.reference.runs} runs`);
        return lines.join("<br>");
      },
      series
    }));
  }

  function successRateTooltipHtml(i) {
    const d = dsList[i];
    const cur = d.current;
    const ref = d.reference;
    const lines = [escapeHtml(d.name)];
    if (!d.has_threshold) {
      lines.push("No success threshold defined");
      return lines.join("<br>");
    }
    lines.push(cur.runs
      ? `Current: ${cur.success_rate.toFixed(1)}% (${cur.runs} completed runs)`
      : "Current: awaiting results");
    if (state.showRef) {
      lines.push(ref.runs
        ? `Reference: ${ref.success_rate.toFixed(1)}% (${ref.runs} completed runs)`
        : "Reference: no reference");
      if (cur.runs && ref.runs) {
        const diff = cur.success_rate - ref.success_rate;
        lines.push(`Difference: ${diff >= 0 ? "+" : ""}${diff.toFixed(1)} percentage points`);
      }
    }
    return lines.join("<br>");
  }

  function renderSuccessRate(container) {
    const box = h("div", { className: "chart-box" });
    container.appendChild(box);
    if (!dsList.length) return;

    const currentData = dsList.map((d) => d.has_threshold && d.current.runs ? d.current.success_rate : null);
    const refData = dsList.map((d) => d.has_threshold && d.reference.runs ? d.reference.success_rate : null);

    initChart(box).setOption(makeDatasetBarOption({
      yAxis: { name: "Success rate", min: 0, max: 100, axisLabel: { formatter: "{value}%" } },
      tooltipHtml: successRateTooltipHtml,
      series: makeBarSeries(currentData, refData)
    }));
  }

  function timePerRunTooltipHtml(i) {
    const d = dsList[i];
    const cur = d.current;
    const ref = d.reference;
    const lines = [escapeHtml(d.name)];
    lines.push(cur.runs && cur.time_per_run !== null
      ? `Current: ${cur.time_per_run.toFixed(3)} s/run (${cur.runs} completed runs)`
      : "Current: awaiting results");
    const remaining = Math.max(d.capacity_runs, cur.runs) - cur.runs;
    lines.push(`Runs left: ${remaining}`);
    lines.push(`Estimated time remaining: ${escapeHtml(d.remaining_label)}`);
    if (state.showRef) {
      lines.push(ref.runs && ref.time_per_run !== null
        ? `Reference: ${ref.time_per_run.toFixed(3)} s/run (${ref.runs} completed runs)`
        : "Reference: no reference");
    }
    return lines.join("<br>");
  }

  function renderTimePerRun(container) {
    container.appendChild(h("div", { className: "note" }, "Remaining times update after each completed run."));
    const box = h("div", { className: "chart-box" });
    container.appendChild(box);
    if (!dsList.length) return;

    const currentData = [];
    const refData = [];
    let hasTiming = false;
    for (const d of dsList) {
      const cv = d.current.runs && d.current.time_per_run !== null ? d.current.time_per_run : null;
      const rv = state.showRef && d.reference.runs && d.reference.time_per_run !== null ? d.reference.time_per_run : null;
      if (cv !== null || rv !== null) hasTiming = true;
      currentData.push(cv);
      refData.push(rv);
    }

    initChart(box).setOption({
      backgroundColor: "transparent",
      tooltip: {
        trigger: "axis",
        axisPointer: { type: "shadow" },
        formatter(params) { return timePerRunTooltipHtml(params[0].dataIndex); }
      },
      legend: { show: state.showRef, top: 4, textStyle: { color: "#cfd8e3" } },
      grid: baseGrid({ left: 100, right: 105, bottom: 32 }),
      xAxis: splitAxis({ name: "Time per run (s)", nameLocation: "middle", nameGap: 24, min: 0, max: hasTiming ? null : 1 }),
      yAxis: [
        {
          type: "category",
          inverse: true,
          data: labels,
          axisLabel: { color: "#cfd8e3" }
        },
        {
          type: "category",
          inverse: true,
          position: "right",
          data: dsList.map((d) => d.remaining_label),
          axisLine: { show: false },
          axisTick: { show: false },
          axisLabel: { color: "#8b98a7" }
        }
      ],
      series: makeBarSeries(currentData, refData, { yAxisIndex: 0, barMaxWidth: 20 })
    });
  }

  function renderErrorBarSeries(name, xVal, mean, sd, best, color) {
    if (mean === null && best === null) return [];
    const out = [];
    if (mean !== null) {
      const s = sd || 0;
      out.push({
        name,
        type: "custom",
        encode: { x: 0, y: [1, 2] },
        renderItem(_params, api) {
          const x = api.value(0), lo = api.value(1), hi = api.value(2);
          const pLo = api.coord([x, lo]), pHi = api.coord([x, hi]);
          const cap = 5;
          return {
            type: "group",
            children: [
              { type: "line", shape: { x1: pLo[0], y1: pLo[1], x2: pHi[0], y2: pHi[1] }, style: { stroke: color, lineWidth: 2 } },
              { type: "line", shape: { x1: pLo[0] - cap, y1: pLo[1], x2: pLo[0] + cap, y2: pLo[1] }, style: { stroke: color, lineWidth: 2 } },
              { type: "line", shape: { x1: pHi[0] - cap, y1: pHi[1], x2: pHi[0] + cap, y2: pHi[1] }, style: { stroke: color, lineWidth: 2 } }
            ]
          };
        },
        data: [[xVal, mean - s, mean + s]],
        z: 5
      });
      out.push({
        name: name + " mean",
        type: "scatter",
        symbol: "circle",
        symbolSize: 8,
        itemStyle: { color },
        data: [[xVal, mean]],
        z: 6
      });
    }
    if (best !== null) {
      out.push({
        name: name + " best",
        type: "scatter",
        symbol: "triangle",
        symbolSize: 9,
        itemStyle: { color },
        data: [[xVal, best]],
        z: 7
      });
    }
    return out;
  }

  function renderFitnessAcrossDatasets(container) {
    const grid = h("div", { className: "subplots" });
    container.appendChild(grid);
    const pending = [];

    for (const d of dsList) {
      const cur = d.current;
      const ref = d.reference;
      let warnClass = "";
      if (state.showRef && cur.fit_best !== null && ref.fit_best !== null && cur.fit_best < ref.fit_best) {
        warnClass = (cur.fit_mean !== null && ref.fit_mean !== null && cur.fit_mean < ref.fit_mean)
          ? " warn-strong" : " warn-mild";
      }
      const card = h("div", { className: "subplot" + warnClass });
      card.appendChild(h("div", { className: "subplot-title" }, d.name));
      const box = h("div", { className: "subplot-chart" });
      card.appendChild(box);
      grid.appendChild(card);

      const refX = ref.runs + (Math.abs(cur.runs - ref.runs) < 1e-9 ? 0.1 : 0);
      const minX = state.showRef && ref.runs > 0
        ? 0.95 * Math.min(cur.runs, refX) : 0;
      const maxX = state.showRef && ref.runs > 0
        ? 1.05 * Math.max(cur.runs, refX)
        : 2 * Math.max(cur.runs, 1);

      const series = renderErrorBarSeries("Current", cur.runs, cur.fit_mean, cur.fit_std_dev, cur.fit_best, primaryColor());
      if (state.showRef) {
        series.push(...renderErrorBarSeries("Reference", refX, ref.fit_mean, ref.fit_std_dev, ref.fit_best, "#ff9f43"));
      }

      pending.push([box, {
        backgroundColor: "transparent",
        tooltip: {
          trigger: "item",
          formatter() {
            const lines = [
              escapeHtml(d.name),
              `Current (${cur.runs} runs): best=${fmtNum(cur.fit_best)}, mean=${fmtNum(cur.fit_mean)} ± ${fmtNum(cur.fit_std_dev)}`
            ];
            if (state.showRef) {
              lines.push(`Reference (${ref.runs} runs): best=${fmtNum(ref.fit_best)}, mean=${fmtNum(ref.fit_mean)} ± ${fmtNum(ref.fit_std_dev)}`);
            }
            return lines.join("<br>");
          }
        },
        grid: baseGrid({ left: 48, right: 16, top: 18, bottom: 28 }),
        xAxis: splitAxis({ min: minX, max: maxX }),
        yAxis: splitAxis({ scale: true }),
        series
      }]);
    }

    for (const [box, opt] of pending) initChart(box).setOption(opt);
  }

  function renderElite(container) {
    const grid = h("div", { className: "subplots" });
    container.appendChild(grid);
    const pending = [];

    for (const d of dsList) {
      const cur = (d.current.elite || []).slice(0, 10);
      const ref = (d.reference.elite || []).slice(0, 10);
      const rankCount = Math.max(cur.length, ref.length);

      const card = h("div", { className: "subplot" });
      const titleRow = h("div", { className: "subplot-title" });
      titleRow.appendChild(h("span", {}, d.name));
      if (d.status) {
        const badge = h("span", { className: "badge badge-" + d.status }, `[${d.status}]`);
        if (d.error) badge.title = d.error;
        titleRow.appendChild(badge);
      } else if (d.has_threshold && d.current.runs > 0 && d.current.success_rate === 0) {
        titleRow.appendChild(h("span", { className: "badge badge-NoHit" }, "[NO HIT]"));
      }
      card.appendChild(titleRow);
      if (d.error) {
        card.appendChild(h("div", { className: "note", style: "color: var(--failed);" }, d.error));
      }

      const box = h("div", { className: "subplot-chart" });
      card.appendChild(box);
      grid.appendChild(card);

      if (!cur.length && !ref.length) {
        box.appendChild(h("div", { className: "note" },
          d.status === "Failed" ? "No elite runs recorded." : "Awaiting elite runs."));
        continue;
      }

      const ranks = [];
      const topIds = [];
      for (let k = 0; k < rankCount; ++k) {
        ranks.push(String(k + 1));
        topIds.push(k < cur.length ? String(cur[k].id) : "");
      }

      const series = [
        {
          name: "Current",
          type: "bar",
          xAxisIndex: 0,
          itemStyle: { color: barColor() },
          label: {
            show: true,
            position: "inside",
            rotate: 90,
            color: "#e6edf3",
            textBorderColor: "#0f141a",
            textBorderWidth: 2,
            fontSize: 9,
            formatter(p) {
              const e = cur[p.dataIndex];
              return e && e.acc !== null && e.acc !== undefined ? (e.acc * 100).toFixed(2) + "%" : "";
            }
          },
          data: cur.map((e) => e.fit)
        }
      ];
      if (state.showRef) {
        series.push({
          name: "Reference",
          type: "bar",
          xAxisIndex: 0,
          itemStyle: { color: "#ff9f43", opacity: 0.7 },
          data: ref.map((e) => e.fit)
        });
      }

      pending.push([box, {
        backgroundColor: "transparent",
        tooltip: {
          trigger: "axis",
          axisPointer: { type: "shadow" },
          formatter(params) {
            const k = params[0].dataIndex;
            const lines = [`${escapeHtml(d.name)} - Rank ${k + 1}`];
            if (k < cur.length && cur[k].fit !== null) {
              const acc = cur[k].acc !== null ? ` (${(cur[k].acc * 100).toFixed(2)}%)` : "";
              lines.push(`Current (Run ${cur[k].id}): ${fmtNum(cur[k].fit)}${acc}`);
            }
            if (state.showRef && k < ref.length && ref[k].fit !== null) {
              const acc = ref[k].acc !== null ? ` (${(ref[k].acc * 100).toFixed(2)}%)` : "";
              lines.push(`Reference (Run ${ref[k].id}): ${fmtNum(ref[k].fit)}${acc}`);
            }
            return lines.join("<br>");
          }
        },
        grid: baseGrid({ left: 46, right: 12, top: 26, bottom: 26 }),
        xAxis: [
          { type: "category", data: ranks, axisLabel: { color: "#cfd8e3" } },
          { type: "category", position: "top", data: topIds, axisLabel: { color: "#8b98a7", fontSize: 10 }, axisTick: { show: false } }
        ],
        yAxis: splitAxis({ scale: true, boundaryGap: ["10%", "10%"] }),
        series
      }]);
    }

    for (const [box, opt] of pending) initChart(box).setOption(opt);
  }

  function renderDetails() {
    const panel = document.getElementById("details-panel");
    panel.hidden = !state.visible.details;
    if (!state.visible.details) return;

    const body = document.getElementById("details-body");
    body.innerHTML = "";

    const tbl = h("table");
    const thead = h("thead");
    const hr = h("tr");
    const headers = [
      "Dataset",
      "Status",
      "Runs",
      data.show_success_rate ? "Success rate" : "Time / run",
      "Fitness (mean ± sd)",
      "Best fitness",
      "Best program"
    ];
    for (const label of headers) hr.appendChild(h("th", {}, label));
    thead.appendChild(hr);
    tbl.appendChild(thead);

    const tbody = h("tbody");
    for (const d of dsList) {
      const cur = d.current;
      const ref = d.reference;
      const tr = h("tr");
      tr.appendChild(h("td", {}, d.name));
      const statusText = d.status ? d.status : (cur.runs ? "Loaded" : "Pending");
      tr.appendChild(h("td", {}, statusText + (d.error ? `: ${d.error}` : "")));
      tr.appendChild(h("td", { className: "num" }, `${cur.runs} / ${d.capacity_runs}` +
        (state.showRef ? ` (ref: ${ref.runs})` : "")));

      if (data.show_success_rate) {
        const sr = d.has_threshold && cur.runs && cur.success_rate !== null
          ? cur.success_rate.toFixed(1) + "%" : "N/A";
        tr.appendChild(h("td", { className: "num" }, sr));
      } else {
        const tpr = cur.runs && cur.time_per_run !== null
          ? cur.time_per_run.toFixed(3) + " s" : d.remaining_label;
        tr.appendChild(h("td", { className: "num" }, tpr));
      }

      tr.appendChild(h("td", { className: "num" }, cur.fit_mean !== null
        ? `${fmtNum(cur.fit_mean)} ± ${fmtNum(cur.fit_std_dev)}` : "N/A"));
      tr.appendChild(h("td", { className: "num" }, fmtNum(cur.fit_best)));

      const prgTd = h("td");
      if (cur.best_prg) {
        prgTd.appendChild(makeCodeBlock(cur.best_prg));
      } else {
        prgTd.appendChild(h("span", { className: "note" }, "N/A"));
      }
      if (state.showRef && ref.best_prg) {
        prgTd.appendChild(h("div", { className: "note", style: "margin-top:6px;" }, "Reference:"));
        prgTd.appendChild(makeCodeBlock(ref.best_prg));
      }
      tr.appendChild(prgTd);
      tbody.appendChild(tr);
    }
    tbl.appendChild(tbody);
    body.appendChild(tbl);
  }

  start();
})();
)JS";

}  // namespace

std::string render_html_report(const results_snapshot &snapshot,
                               bool live, unsigned refresh_seconds)
{
  const auto payload(build_json_payload(snapshot, live, refresh_seconds));
  return build_echarts_html("WOPR Report", payload, results_body, results_js);
}

bool write_html_report(const std::filesystem::path &output_path,
                       const results_snapshot &snapshot,
                       bool live, unsigned refresh_seconds)
{
  const auto payload(build_json_payload(snapshot, live, refresh_seconds));
  return write_echarts_html(output_path, "WOPR Report", payload, results_body,
                            results_js, live);
}

}  // namespace ultra::wopr
