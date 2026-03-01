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

  dynamic_file << " {";
  for (bool first(true); const auto &[k, v] : sum.az.crossover_types())
  {
    if (!first)
      dynamic_file << ',';

    first = false;
    dynamic_file << k << ':' << v;
  }
  dynamic_file << '}';

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
  const search_stats<I, F> &stats,
  const model_measurements<F> &threshold) const
{
  if (!is_valid())
    return;

  tinyxml2::XMLPrinter doc;

  {
    xml_closer ultra_e(doc, "ultra");

    {
      xml_closer summary_e(doc, "summary");

      set_text(doc, "runs", stats.runs());
      set_text(doc, "elapsed_time", stats.elapsed.count());
      set_text(doc, "success_rate", stats.success_rate(threshold));

      {
        xml_closer distributions_e(doc, "distributions");

        {
          xml_closer fitness_e(doc, "fitness");
          set_text(doc, "mean", stats.fitness_distribution.mean());
          set_text(doc, "standard_deviation",
                   stats.fitness_distribution.standard_deviation());
        }  // fitness
      }  // distributions

      {
        xml_closer best_e(doc, "best");
        if (stats.best_measurements().fitness)
          set_text(doc, "fitness", *stats.best_measurements().fitness);
        if (stats.best_measurements().accuracy)
          set_text(doc, "accuracy", *stats.best_measurements().accuracy);
        set_text(doc, "run", stats.best_run());

        std::ostringstream ss;
        ss << out::in_line << stats.best_individual();
        set_text(doc, "code", ss.str());
      }  // best

      {
        xml_closer solutions_e(doc, "solutions");
        for (const auto good_runs(stats.good_runs(threshold));
             const auto &gr : good_runs)
          set_text(doc, "run", gr);
      }  // solutions

      {
        constexpr double perc(0.05);

        xml_closer elite_e(doc, "elite");
        doc.PushAttribute("percentile", perc * 100.0);
        for (const auto &e : stats.elite_runs(perc))
        {
          xml_closer run_summary_e(doc, "run");
          doc.PushAttribute("id", static_cast<uint64_t>(e.run));

          if (e.best_measurements.fitness)
            set_text(doc, "fitness", *e.best_measurements.fitness);
          if (e.best_measurements.accuracy)
            set_text(doc, "accuracy", *e.best_measurements.accuracy);
        }
      }  // elite
    }  // summary

    set_text(doc, "checksum", "00000000");
  }  // ultra

  const std::string base_xml(doc.CStr());
  const std::string signed_xml(ultra::crc32::embed_xml_signature(base_xml));

  if (std::ofstream out(build_path(summary_file_path), std::ios::binary); out)
    out.write(signed_xml.data(),
              static_cast<std::streamsize>(signed_xml.size()));
}

#endif  // include guard
