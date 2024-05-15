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

#if !defined(ULTRA_EVOLUTION_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_EVOLUTION_TCC)
#define      ULTRA_EVOLUTION_TCC

///
/// \param[in] strategy evolution strategy to be used for evolution
///
template<Strategy S>
evolution<S>::evolution(S strategy) : pop_(strategy.get_problem()),
                                      es_(std::move(strategy))
{
  Ensures(is_valid());
}

///
/// \return `true` when evolution should be interrupted
///
template<Strategy S>
bool evolution<S>::stop_condition() const
{
  const auto planned_generations(pop_.problem().params.evolution.generations);
  Expects(planned_generations);

  // Check the number of generations.
  if (sum_.generation > planned_generations)
    return true;

  if (term::user_stop())
    return true;

  return false;
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
/// where each block is a set of line like this:
///
///     data_1 [space] data_2 [space] ... [space] data_n
///
/// We use this format, instead of XML, because statistics are produced
/// incrementally and so it's simple and fast to append new data to a
/// CSV-like file. Note also that it's simple to extract and plot data with
/// GNU Plot.
///
template<Strategy S>
void evolution<S>::log_evolution() const
{
  static unsigned run_count(-1);

  if (sum_.generation == 0)
    ++run_count;

  const auto &params(pop_.problem().params);

  const auto stat_stream([&params](const std::filesystem::path &f)
                         {
                           return std::ofstream(params.stat.dir / f,
                                                std::ios_base::app);
                         });

  if (!params.stat.dynamic_file.empty())
  {
    if (auto f_dyn(stat_stream(params.stat.dynamic_file)); f_dyn.good())
    {
      if (sum_.generation == 0)
        f_dyn << "\n\n";

      f_dyn << run_count << ' ' << sum_.generation;

      const auto best(sum_.best());
      if (best.ind.empty())
        f_dyn << " ?";
      else
        f_dyn << ' ' << best.fit;

      f_dyn << ' ' << sum_.az.fit_dist().mean()
            << ' ' << sum_.az.fit_dist().standard_deviation()
            << ' ' << sum_.az.fit_dist().entropy()
            << ' ' << sum_.az.fit_dist().min()
            << ' ' << static_cast<unsigned>(sum_.az.length_dist().mean())
            << ' ' << sum_.az.length_dist().standard_deviation()
            << ' ' << static_cast<unsigned>(sum_.az.length_dist().max());

      if (best.ind.empty())
        f_dyn << " ?";
      else
        f_dyn << " \"" << out::in_line << best.ind << '"';

      f_dyn << '\n';
    }
    else
    {
      ultraWARNING << "Cannot open dynamic file " << params.stat.dynamic_file;
    }
  }

  if (!params.stat.population_file.empty())
  {
    if (auto f_pop(stat_stream(params.stat.population_file)); f_pop.good())
    {
      if (sum_.generation == 0)
        f_pop << "\n\n";

      for (const auto &f : sum_.az.fit_dist().seen())
        // f.first: value, f.second: frequency
        f_pop << run_count << ' ' << sum_.generation << ' '
              << std::fixed << std::scientific
              << f.first << ' ' << f.second << '\n';
    }
    else
    {
      ultraWARNING << "Cannot open population file "
                   << params.stat.population_file;
    }
  }

  es_.log_strategy(pop_, sum_);
}

template<Strategy S>
void evolution<S>::print(bool summary, std::chrono::milliseconds elapsed,
                         timer *from_last_msg) const
{
  if (log::reporting_level <= log::lOUTPUT)
  {
    if (summary)
    {
      std::cout << std::string(50, ' ') << '\r' << std::flush;
      ultraOUTPUT << std::setw(8) << lexical_cast<std::string>(elapsed)
                  << std::setw(8) << sum_.generation
                  << ':' << std::setw(13) << sum_.best().fit;
    }
    else
    {
      const auto seconds(
        std::max(
          std::chrono::duration_cast<std::chrono::seconds>(elapsed),
          std::chrono::seconds(1)).count());

      double gph(3600.0 * sum_.generation / seconds);
      if (gph > 2.0)
        gph = std::floor(gph);

      std::cout << lexical_cast<std::string>(elapsed) << "  gen "
                << sum_.generation << "  [" << pop_.layers();

      if (sum_.generation)
        std::cout << "x " << gph << "gph";

      std::cout << ']' << "                              \r" << std::flush;
    }

    from_last_msg->restart();
  }
}

///
/// Sets the shake function.
///
/// \param[in] f the shaking function
/// \return      a reference to *this* object (method chaining / fluent
///              interface)
///
/// The shake function is called every new generation and is used to alter
/// the environment of the evolution (i.e. it could change the points for a
/// symbolic regression problem, the examples for a classification task...).
///
template<Strategy S>
evolution<S> &evolution<S>::shake_function(
  const std::function<bool(unsigned)> &f)
{
  shake_ = f;
  return *this;
}

///
/// Sets a callback function called at the end of every generation.
///
/// \param[in] f callback function
/// \return      a reference to *this* object (method chaining / fluent
///              interface)
///
template<Strategy S>
evolution<S> &evolution<S>::after_generation(after_generation_callback_t f)
{
  after_generation_callback_ = std::move(f);
  return *this;
}

/// The evolutionary core loop.
///
/// \return a partial summary of the search (see notes)
///
/// \note
/// The return value is a partial summary: the `mode_measurement` section is
/// only partially filled (fitness) since many metrics are expensive to
/// calculate and not always significative  (e.g. f1-score for a symbolic
/// regression problem). The `src_search` class has a simple scheme to
/// request the computation of additional metrics.
///
template<Strategy S>
summary<typename S::individual_t, typename S::fitness_t>
evolution<S>::run()
{
  Expects(sum_.generation == 0);

  using namespace std::chrono_literals;

  timer from_start, from_last_msg;

  std::stop_source source;

  const auto evolve_subpop(
    [&, stop_token = source.get_token()](auto iter)
    {
      auto evolve(es_.operations(pop_, iter, sum_.starting_status()));

      // Asynchronous population update: each newly generated offspring can
      // replace an individual of the current population (aka steady state
      // population).
      // Asynchronous update permits new individual to contribute to the
      // evolution immediately and can speed up the convergence.
      for (auto cycles(iter->size()); cycles; --cycles)
        if (!stop_token.stop_requested())
          evolve();
    });

  const auto task_completed(
    [](const auto &future)
    {
      return future.wait_for(0ms) == std::future_status::ready;
    });

  scored_individual previous_best(sum_.best());

  const auto print_and_update_if_better(
    [&](auto candidate)
    {
      if (previous_best < candidate)
      {
        previous_best = candidate;
        print(true, from_start.elapsed(), &from_last_msg);
        return true;
      }

      return false;
    });

  term::set();
  es_.init(pop_);  // customizatin point for strategy-specific initialization

  for (bool stop(false); !stop; ++sum_.generation)
  {
    if (shake_)
      shake_(sum_.generation);

    ultraDEBUG << "Launching tasks for generation " << sum_.generation;

    const auto range(pop_.range_of_layers());

    std::vector<std::future<void>> tasks;
    for (auto l(range.begin()); l != range.end(); ++l)
      tasks.push_back(std::async(std::launch::async, evolve_subpop, l));

    while (!std::ranges::all_of(tasks, task_completed))
    {
      if (from_last_msg.elapsed() > 2s)
      {
        if (!print_and_update_if_better(sum_.best()))
          print(false, from_start.elapsed(), &from_last_msg);
      }

      if (!stop && (stop = stop_condition()))
      {
        source.request_stop();
        ultraDEBUG << "Sending closing message to tasks";
      }

      std::this_thread::yield();
    }

    print_and_update_if_better(sum_.best());

    sum_.az = analyze(pop_, es_.evaluator());
    log_evolution();

    es_.after_generation(pop_, sum_);  // strategy-specific bookkeeping
    if (after_generation_callback_)
      after_generation_callback_(pop_, sum_);
  }

  sum_.elapsed = from_start.elapsed();

  ultraINFO << "Evolution completed at generation: " << sum_.generation
            << ". Elapsed time: "
            << lexical_cast<std::string>(from_start.elapsed());

  term::reset();
  return sum_;
}

template<Strategy S>
bool evolution<S>::is_valid() const
{
  return pop_.is_valid();
}

#endif  // include guard
