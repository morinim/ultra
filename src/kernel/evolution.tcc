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
/// \param[in] prob starting problem for the evolution
/// \param[in] eva  evaluator to be used
///
template<Evaluator E>
evolution<E>::evolution(const problem &prob, E &eva) : pop_(prob), eva_(eva)
{
  Ensures(is_valid());

  ultraDEBUG << "Creating a new instance of evolution class";
}

///
/// \return `true` if the evolution should be interrupted
///
template<Evaluator E>
bool evolution<E>::stop_condition() const
{
  const auto planned_generations(pop_.problem().params.evolution.generations);
  Expects(planned_generations);

  // Check the number of generations.
  if (sum_.generation > planned_generations)
    return true;

  if (console.user_stop())
    return true;

  return external_stop_source_.stop_requested();
}

template<Evaluator E>
void evolution<E>::print(bool summary, std::chrono::milliseconds elapsed,
                         timer *from_last_msg) const
{
  if (log::reporting_level > log::lPAROUT)
    return;

  const std::string tags(tag_.empty() ? tag_ : "[" + tag_ + "] ");

  if (summary)
  {
    ultraPAROUT << tags << std::setw(8) << lexical_cast<std::string>(elapsed)
                << std::setw(8) << sum_.generation
                << ':' << std::setw(13) << sum_.best().fit;
  }
  else
  {
    static const std::string clear_line(std::string(30, ' ')
                                        + std::string(1, '\r'));

    if (log::reporting_level == log::lPAROUT)
    {
      static const std::string chrs("|/-\\");

      std::cout << tags << chrs[elapsed.count() % chrs.size()] << clear_line
                << std::flush;
    }
    else if (log::reporting_level <= log::lSTDOUT)
    {
      const auto seconds(
        std::max(
          std::chrono::duration_cast<std::chrono::seconds>(elapsed),
          1s).count());

      double gph(3600.0 * sum_.generation / seconds);
      if (gph > 2.0)
        gph = std::floor(gph);

      std::cout << lexical_cast<std::string>(elapsed) << "  gen "
                << sum_.generation << "  [" << pop_.layers();

      if (sum_.generation)
        std::cout << "x " << gph << "gph";

      std::cout << ']' << clear_line << std::flush;
    }
  }

  from_last_msg->restart();
}

///
/// Sets the search/evolution logger.
///
/// \param[in] sl logger
/// \return      a reference to *this* object (method chaining / fluent
///              interface)
///
/// \remark
/// Logger must be set before calling `run`. By default, data logging is
/// excluded.
///
template<Evaluator E>
evolution<E> &evolution<E>::logger(search_log &sl)
{
  search_log_ = &sl;
  return *this;
}

///
/// Sets the identification tag for this object.
///
/// \param[in] t identification tag
/// \return      a reference to *this* object (method chaining / fluent
///              interface)
///
/// The tag is used to identify this object when multiple evolutions are
/// performed in parallel.
///
template<Evaluator E>
evolution<E> &evolution<E>::tag(const std::string &t)
{
  tag_ = t;
  return *this;
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
template<Evaluator E>
evolution<E> &evolution<E>::shake_function(
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
template<Evaluator E>
evolution<E> &evolution<E>::after_generation(after_generation_callback_t f)
{
  after_generation_callback_ = std::move(f);
  return *this;
}

///
/// Set an external stop source for performing cooperative task shutdown.
///
/// \param[in] ss stop source to issue a stop request
/// \return      a reference to *this* object (method chaining / fluent
///              interface)
///
template<Evaluator E>
evolution<E> &evolution<E>::stop_source(std::stop_source ss)
{
  external_stop_source_ = ss;
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
template<Evaluator E>
template<template<class> class ES>
summary<typename evolution<E>::individual_t,
        typename evolution<E>::fitness_t> evolution<E>::run()
{
  Expects(sum_.generation == 0);

  using namespace std::chrono_literals;

  timer from_start, from_last_msg;

  ES<E> strategy(pop_.problem(), eva_);

  std::stop_source source;

  // Asynchronous population update: each newly generated offspring can replace   // an individual of the current population (aka steady state population).
  // Asynchronous update permits new individual to contribute to the evolution
  // immediately and can speed up the convergence.
  const auto evolve_subpop(
    [&, stop_token = source.get_token()](auto subpop_it)
    {
      auto evolve(strategy.operations(pop_, subpop_it,
                                      sum_.starting_status()));

      // We must use `safe_size()` because other threads might migrate
      // individuals in this subpopulation.
      for (auto cycles(subpop_it->safe_size()); cycles; --cycles)
        if (!stop_token.stop_requested())
          evolve();
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

  ultraDEBUG << "Calling evolution_strategy init method";
  strategy.init(pop_);  // strategy-specific customizatin point

  bool use_sleep(false);

  ultra::thread_pool pool;

  for (bool stop(false); !stop; ++sum_.generation)
  {
    if (shake_)
      shake_(sum_.generation);

    ultraDEBUG << "Launching tasks for generation " << sum_.generation;

    const auto subpops(pop_.range_of_layers());

    for (auto l(subpops.begin()); l != subpops.end(); ++l)
      pool.execute(evolve_subpop, l);

    ultraDEBUG << "Tasks running";

    while (pool.has_pending_tasks())
    {
      if (from_last_msg.elapsed() > 2s)
      {
        use_sleep = true;

        if (!print_and_update_if_better(sum_.best()))
          print(false, from_start.elapsed(), &from_last_msg);
      }

      if (!stop && (stop = stop_condition()))
      {
        source.request_stop();
        ultraDEBUG << "Sending closing message to tasks";
      }

      if (use_sleep)
        std::this_thread::sleep_for(5ms);
      else
        std::this_thread::yield();
    }

    print_and_update_if_better(sum_.best());

    sum_.az = analyzer(pop_, eva_);
    if (search_log_)
      search_log_->save_snapshot(pop_, sum_);

    strategy.after_generation(pop_, sum_);  // strategy-specific bookkeeping
    if (after_generation_callback_)
      after_generation_callback_(pop_, sum_);
  }

  sum_.elapsed = from_start.elapsed();

  ultraINFO << "Evolution completed at generation: " << sum_.generation
            << ". Elapsed time: "
            << lexical_cast<std::string>(from_start.elapsed());

  return sum_;
}

///
/// \return `true` if the object passes the internal consistency check
///
template<Evaluator E>
bool evolution<E>::is_valid() const
{
  return pop_.is_valid();
}

#endif  // include guard
