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
/// Construct an evolution object.
///
/// \param[in] prob problem definition and evolutionary parameters
/// \param[in] eva  evaluator used to compute fitness values
///
/// Initialises the population according to the supplied problem description
/// and stores a reference to the evaluator.
///
template<Evaluator E>
evolution<E>::evolution(const problem &prob, E &eva) : pop_(prob), eva_(eva)
{
  Ensures(is_valid());

  ultraDEBUG << "Creating a new instance of evolution class";
}

///
/// Check whether the evolution should stop.
///
/// \return `true` if the evolution must terminate
///
/// Evaluates all termination conditions, including:
/// - planned generation limit;
/// - user interrupt;
/// - external stop requests.
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
/// Attaches a search logger.
///
/// \param[in] sl logger instance
/// \return       reference to *this* object (fluent interface)
///
/// When set, the logger records snapshots of the population and summary data
/// at the end of each generation.
///
/// \remark Logging is disabled by default.
///
template<Evaluator E>
evolution<E> &evolution<E>::logger(search_log &sl)
{
  search_log_ = &sl;
  return *this;
}

///
/// Assigns an identification tag.
///
/// \param[in] t tag string
/// \return      reference to *this* object (fluent interface)
///
/// The tag is used in progress reporting to distinguish multiple concurrent
/// evolution instances.
///
template<Evaluator E>
evolution<E> &evolution<E>::tag(const std::string &t)
{
  tag_ = t;
  return *this;
}

///
/// Sets a per-generation shake function.
///
/// \param[in] f shake function
/// \return      reference to *this* object (fluent interface)
///
/// The shake function is invoked at the beginning of each generation and can
/// be used to dynamically alter the problem environment (e.g. data resampling,
/// noise injection...).
///
template<Evaluator E>
evolution<E> &evolution<E>::shake_function(
  const std::function<bool(unsigned)> &f)
{
  shake_ = f;
  return *this;
}

///
/// Registers a callback executed after each generation.
///
/// \param[in] f callback function
/// \return      a reference to *this* object (fluent interface)
///
template<Evaluator E>
evolution<E> &evolution<E>::after_generation(after_generation_callback_t f)
{
  after_generation_callback_ = std::move(f);
  return *this;
}

///
/// Sets an external stop source.
///
/// \param[in] ss stop source
/// \return       reference to *this* object (fluent interface)
///
/// Enables cooperative cancellation of the evolutionary process from another
/// execution context.
///
template<Evaluator E>
evolution<E> &evolution<E>::stop_source(std::stop_source ss)
{
  external_stop_source_ = ss;
  return *this;
}

///
/// The evolutionary core loop.
///
/// \tparam ES evolution strategy template
///
/// \return a summary describing the outcome of the evolutionary search
///
/// Runs the main evolutionary loop using the specified evolution strategy.
/// The loop proceeds generation by generation until a stop condition is met.
///
/// The evolution strategy controls how offspring are generated and inserted
/// into the population, while this class manages scheduling, monitoring,
/// logging, and coordination.
///
/// The returned summary contains partial statistics: fitness-related measures
/// are always computed, while more expensive metrics may be omitted unless
/// explicitly requested elsewhere.
///
/// \see evolution_strategy
/// \see summary
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
  strategy.init(pop_);  // strategy-specific customisation point

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

    // Poll the pool state while performing progress reporting and
    // stop-condition handling. All tasks have already been submitted;
    // no new tasks will be enqueued during this loop.
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
/// Verify internal consistency.
///
/// \return `true` if the object passes the internal consistency check
///
template<Evaluator E>
bool evolution<E>::is_valid() const
{
  return pop_.is_valid();
}

#endif  // include guard
