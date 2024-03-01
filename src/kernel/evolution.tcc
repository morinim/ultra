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
/// \param[in] strategy evolution strategy to be used
///
template<Strategy S>
evolution<S>::evolution(const S &strategy) : sum_(), pop_(strategy.problem()),
                                             es_(strategy)
{
  Ensures(is_valid());
}

///
/// \return `true` when evolution should be interrupted
///
template<Strategy S>
bool evolution<S>::stop_condition() const
{
  const auto planned_generations(pop_.problem().env.evolution.generations);
  Expects(planned_generations);

  // Check the number of generations.
  if (sum_.generation > 100*planned_generations)
    return true;

  if (term::user_stop())
    return true;

  // Check strategy specific stop conditions.
  //return es_.stop_condition();

  return false;
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
      ultraOUTPUT << std::setw(8) << sum_.generation << ": " << sum_.best().fit;
    }
    else
    {
      std::cout << std::setw(8) << sum_.generation << ": " << sum_.best().fit
                << " (" << lexical_cast<std::string>(elapsed) << ")\r"
                << std::flush;
    }

    from_last_msg->restart();
  }
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

  term::set();
  es_.init(pop_);

  for (bool stop(false); !stop; ++sum_.generation)
  {
    ultraDEBUG << "Launching tasks for generation " << sum_.generation;

    const auto range(pop_.range_of_layers());

    std::vector<std::future<void>> tasks;
    for (auto l(range.begin()); l != range.end(); ++l)
      tasks.push_back(std::async(std::launch::async, evolve_subpop, l));

    while (!std::ranges::all_of(tasks, task_completed))
    {
      if (from_last_msg.elapsed() > 2s)
      {
        if (previous_best < sum_.best())
        {
          previous_best = sum_.best();
          print(true, from_start.elapsed(), &from_last_msg);
        }
        else
          print(false, from_start.elapsed(), &from_last_msg);

        if (!stop)
        {
          if ((stop = stop_condition()))
          {
            source.request_stop();
            ultraDEBUG << "Sending closing message to tasks";
          }
        }
      }

      std::this_thread::yield();
    }

    sum_.az = analyze(pop_, es_.evaluator());
    es_.after_generation(sum_.generation, pop_, sum_.az);
  }

  sum_.elapsed = from_start.elapsed();

  ultraINFO << "Elapsed time: "
            << lexical_cast<std::string>(from_start.elapsed());

  term::reset();
  return sum_;
}

template<Strategy S>
bool evolution<S>::is_valid() const
{
  return true;
}

#endif  // include guard
