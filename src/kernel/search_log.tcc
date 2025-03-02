/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_SEARCH_LOG_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_SEARCH_LOG_TCC)
#define      ULTRA_SEARCH_LOG_TCC

namespace internal
{

//
// A convenient arrangement for inserting stream-aware objects into
// `XMLPrinter`.
//
// \tparam T type of the value
//
// \param[out] p output device
// \param[in]  e new xml element
// \param[in]  v new xml element's value
//
template<class T>
void set_text(tinyxml2::XMLPrinter &p, const std::string &e, const T &v)
{
  std::string str_v;

  if constexpr (std::is_same_v<T, std::string>)
    str_v = v;
  else
  {
    std::ostringstream ss;
    ss << v;
    str_v = ss.str();
  }

  p.OpenElement(e.c_str());
  p.PushText(str_v.c_str());
  p.CloseElement(e.c_str());
}

}  // namespace internal

template<Individual I, Fitness F>
void search_log::save_dynamic(const summary<I, F> &sum,
                              const distribution<F> &fit_dist)
{
  if (!dynamic_file.is_open())
    return;

  if (sum.generation == 0)
    dynamic_file << "\n\n";

  dynamic_file << sum.generation;

  const auto best(sum.best());
  if (best.ind.empty())
    dynamic_file << " ?";
  else
    dynamic_file << ' ' << best.fit;

  const auto length_dist(sum.az.length_dist());
  dynamic_file << ' ' << fit_dist.mean()
               << ' ' << fit_dist.standard_deviation()
               << ' ' << fit_dist.min()
               << ' ' << static_cast<unsigned>(length_dist.mean())
               << ' ' << length_dist.standard_deviation()
               << ' ' << static_cast<unsigned>(length_dist.max());

  if (best.ind.empty())
    dynamic_file << " ?";
  else
    dynamic_file << " \"" << out::in_line << best.ind << '"';

  dynamic_file << '\n' << std::flush;
}

template<Fitness F>
void search_log::save_population(unsigned generation,
                                 const distribution<F> &fit_dist)
{
  if (!population_file.is_open())
    return;

  if (generation == 0)
    population_file << "\n\n";

  population_file << generation;

  for (const auto &[fit, freq] : fit_dist.seen())
    population_file << ' ' << std::fixed << std::scientific << fit << ' '
                    << freq;

  population_file << '\n' << std::flush;
}

///
/// Saves working / statistical informations about layer status.
///
/// \param[in] pop complete evolved population
/// \param[in] sum up to date evolution summary
///
template<Population P, Fitness F>
void search_log::save_layers(
  const P &pop, const summary<std::ranges::range_value_t<P>, F> &sum)
{
  if (!layers_file.is_open())
    return;

  const auto &params(pop.problem().params);

  if (sum.generation == 0)
    layers_file << "\n\n";

  layers_file << sum.generation;

  const auto layers(pop.layers());
  for (std::size_t l(0); l < layers; ++l)
  {
    layers_file << ' ';

    if (const auto ma(params.alps.max_age(l, layers));
        ma == std::numeric_limits<decltype(ma)>::max())
      layers_file << '0';
    else
      layers_file << ma + 1;

    const auto &current_layer(pop.layer(l));

    const auto &age_dist(sum.az.age_dist(current_layer));
    const auto &fit_dist(sum.az.fit_dist(current_layer));

    layers_file << ' ' << age_dist.mean()
                << ' ' << age_dist.standard_deviation()
                << ' ' << static_cast<unsigned>(age_dist.min())
                << ' ' << static_cast<unsigned>(age_dist.max())
                << ' ' << fit_dist.mean()
                << ' ' << fit_dist.standard_deviation()
                << ' ' << fit_dist.min()
                << ' ' << fit_dist.max()
                << ' ' << current_layer.size();
  }

  layers_file << '\n' << std::flush;
}

///
/// Saves working / statistical informations in a log file.
///
/// Data are written in a CSV-like fashion and are partitioned in blocks
/// separated by two blank lines:
///
///     [BLOCK_1]\n\n
///     [BLOCK_2]\n\n
///     ...
///     [BLOCK_x]
///
/// where each block is a set of lines such as this:
///
///     data_1 [space] data_2 [space] ... [space] data_n
///
/// We use this format, instead of XML, because statistics are produced
/// incrementally, making it easy and fast to append new data to a CSV-like
/// file. Additionally, extracting and plotting data with GNU Plot is simple.
///
template<Population P, Fitness F>
void search_log::save_snapshot(
  const P &pop, const summary<std::ranges::range_value_t<P>, F> &sum)
{
  if ((!dynamic_file_path.empty() && !dynamic_file.is_open())
      || (!layers_file_path.empty() && !layers_file.is_open())
      || (!population_file_path.empty() && !population_file.is_open()))
    open();

  const auto fit_dist(sum.az.fit_dist());

  save_dynamic(sum, fit_dist);
  save_population(sum.generation, fit_dist);
  save_layers(pop, sum);
}

template<Individual I, Fitness F> void search_log::save_summary(
  const search_stats<I, F> &stats) const
{
  if (!is_valid())
    return;

  const auto solutions(stats.good_runs.size());
  const auto success_rate(
    stats.runs ? static_cast<double>(solutions)
                 / static_cast<double>(stats.runs)
               : 0);

  tinyxml2::XMLPrinter doc;

  doc.OpenElement("ultra");
  doc.OpenElement("summary");

  using internal::set_text;
  set_text(doc, "runs", stats.runs);
  set_text(doc, "elapsed_time", stats.elapsed.count());
  set_text(doc, "success_rate", success_rate);

  doc.OpenElement("distributions");

  doc.OpenElement("fitness");
  set_text(doc, "mean", stats.fitness_distribution.mean());
  set_text(doc, "standard_deviation",
           stats.fitness_distribution.standard_deviation());
  doc.CloseElement();  // fitness

  doc.CloseElement();  // distributions

  doc.OpenElement("best");
  if (stats.best_measurements.fitness)
    set_text(doc, "fitness", *stats.best_measurements.fitness);
  set_text(doc, "run", stats.best_run);

  std::ostringstream ss;
  ss << out::in_line << stats.best_individual;
  set_text(doc, "code", ss.str());
  doc.CloseElement();  // best

  doc.OpenElement("solutions");
  for (const auto &gr : stats.good_runs)
    set_text(doc, "run", gr);
  doc.CloseElement();  // solutions

  doc.CloseElement();  // summary

  set_text(doc, "checksum", "00000000");
  doc.CloseElement();  // ultra

  const std::string base_xml(doc.CStr());
  const std::string signed_xml(ultra::crc32::embed_xml_signature(base_xml));

  const auto path(build_path(summary_file_path));
  if (std::ofstream out(path); out)
    out << signed_xml << std::flush;
}

#endif  // include guard
