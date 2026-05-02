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

namespace internal
{

enum class phase {evolution, refinement};

struct print_status
{
  phase run_phase {phase::evolution};

  std::atomic<unsigned> planned_evolve_calls {0};
  std::atomic<unsigned> completed_evolve_calls {0};

  std::atomic<unsigned> to_be_refined {0};
  std::atomic<unsigned> refined {0};

  timer from_start {};
  timer from_last_msg {};

  void new_generation()
  {
    run_phase = phase::evolution;

    planned_evolve_calls.store(0, std::memory_order_relaxed);
    completed_evolve_calls.store(0, std::memory_order_relaxed);
    to_be_refined.store(0, std::memory_order_relaxed);
    refined.store(0, std::memory_order_relaxed);
  }
};

}  // namespace internal

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
  if (sum_.generation >= planned_generations)
    return true;

  if (console.user_stop())
    return true;

  return external_stop_source_.stop_requested();
}

template<Evaluator E>
void evolution<E>::print(message m, internal::print_status &ps) const
{
  if (!emit_messages_ || log::reporting_level > log::lPAROUT)
    return;

  const std::string tags(tag_.empty() ? tag_ : "[" + tag_ + "] ");

  const std::string_view str_phase(ps.run_phase == internal::phase::evolution
                                   ? "evolution" : "refinement");

  const auto elapsed(ps.from_start.elapsed());

  if (m == message::summary)
  {
    ultraPAROUT << tags << std::setw(8) << lexical_cast<std::string>(elapsed)
                << std::setw(8) << sum_.generation
                << std::setw(12) << str_phase << ':'
                << std::setw(13) << sum_.best().fit;
  }
  else  // message::status
  {
    std::string perc;

    if (ps.run_phase == internal::phase::evolution)
    {
      const auto planned(
        ps.planned_evolve_calls.load(std::memory_order_relaxed));

      if (planned)
      {
        const auto completed(
          ps.completed_evolve_calls.load(std::memory_order_relaxed));

        perc = std::to_string(100u * completed / planned) + "%";
      }
    }
    else
    {
      const auto to_be_refined(
        ps.to_be_refined.load(std::memory_order_relaxed));

      if (to_be_refined)
      {
        const auto refined(
          ps.refined.load(std::memory_order_relaxed));

        perc = std::to_string(100u * refined / to_be_refined) + "%";
      }
    }

    if (log::reporting_level == log::lPAROUT)
    {
      static const std::string_view chrs("|/-\\");

      std::cout << tags << chrs[elapsed.count() % chrs.size()] << ' '
                << str_phase << ' ' << perc;
    }
    else if (log::reporting_level <= log::lSTDOUT)
    {
      const auto seconds(
        std::max(std::chrono::duration_cast<std::chrono::seconds>(elapsed),
                 1s).count());

      std::cout << lexical_cast<std::string>(elapsed) << "  gen "
                << sum_.generation << ' ' << str_phase << ' ' << perc;

      if (sum_.generation)
      {
        double gph(3600.0 * sum_.generation / seconds);
        if (gph > 2.0)
          gph = std::floor(gph);

        std::cout << "  [" << pop_.layers() << "x " << gph << "gph]";
      }
    }

    std::cout << "                            \r" << std::flush;
  }

  if (ps.run_phase == internal::phase::evolution)
    ps.from_last_msg.restart();
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

template<Evaluator E>
evolution<E> &evolution<E>::refinement(refinement_callback_t f)
{
  refinement_callback_ = std::move(f);
  return *this;
}

template<Evaluator E>
evolution<E> &evolution<E>::messages(bool m) noexcept
{
  emit_messages_ = m;
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
evolution<E> &evolution<E>::shake_function(shake_function_callback_t f)
{
  shake_ = std::move(f);
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

template<Evaluator E>
evolution<E> &evolution<E>::on_new_best(on_new_best_callback_t f)
{
  sum_.on_new_best(std::move(f));
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
/// Performs refinement on a subset of the population.
///
/// \param[in] ps console print-related data
///
/// A fraction of individuals (controlled by `refinement_fraction`) is selected
/// and refined via local optimisation.
///
/// Individuals are sampled randomly (with possible repetitions) from the
/// population.
///
template<Evaluator E>
void evolution<E>::perform_refinement(internal::print_status &ps)
{
  if (!pop_.size())
    return;

  const auto refinement_fraction(pop_.problem().params.refinement.fraction);
  Expects(in_0_1(refinement_fraction));

  if (issmall(refinement_fraction))
    return;

  const auto base_n(
    static_cast<std::size_t>(refinement_fraction * pop_.size()));
  const std::size_t n(std::max(base_n, 1uz));

  std::vector<typename decltype(pop_)::coord> coords(n);

  std::ranges::generate(coords, [this] { return random::coord(pop_); });

  const refiner optimiser(pop_.problem(), false);

  ps.run_phase = internal::phase::refinement;
  ps.to_be_refined.store(n, std::memory_order_relaxed);
  ps.refined.store(0, std::memory_order_relaxed);

  for (auto c : coords)
  {
    auto &ind(pop_[c]);
    print(message::status, ps);

    if (const auto fit(optimiser.optimise(ind, eva_, refinement_callback_));
        fit)
    {
      const scored_individual si(ind, *fit);
      if (sum_.update_if_better(si))
        print(message::summary, ps);
    }

    ps.refined.fetch_add(1, std::memory_order_relaxed);
  }
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

  if (pop_.problem().params.needs_init())
    throw std::logic_error("Parameters still contain auto-tune values. "
                           "Call parameters::init() (or use src::search) "
                           "before evolution::run().");

  using namespace std::chrono_literals;

  ES<E> strategy(pop_.problem(), eva_);

  scored_individual previous_best {sum_.best()};

  internal::print_status ps;

  std::stop_source source;

  // Asynchronous population update: each newly generated offspring can replace
  // an individual of the current population (aka steady state population).
  // Asynchronous update permits new individual to contribute to the evolution
  // immediately and can speed up the convergence.
  const auto evolve_subpop(
    [&, stop_token = source.get_token()](auto subpop_it)
    {
      auto evolve(strategy.operations(pop_, subpop_it,
                                      sum_.starting_status()));

      // We must use `safe_size()` because other threads might migrate
      // individuals in this subpopulation.
      auto cycles(subpop_it->safe_size());

      ps.planned_evolve_calls.fetch_add(cycles, std::memory_order_relaxed);

      while (cycles--)
        if (!stop_token.stop_requested())
        {
          evolve();
          ps.completed_evolve_calls.fetch_add(1, std::memory_order_relaxed);
        }
    });

  const auto print_and_update_if_better(
    [&](auto candidate)
    {
      if (previous_best < candidate)
      {
        previous_best = candidate;
        print(message::summary, ps);
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

    ps.new_generation();

    const auto subpops(pop_.range_of_layers());
    for (auto l(subpops.begin()); l != subpops.end(); ++l)
      pool.execute(evolve_subpop, l);

    ultraDEBUG << "Tasks running";

    // Poll the pool state while performing progress reporting and
    // stop-condition handling. All tasks have already been submitted;
    // no new tasks will be enqueued during this loop.
    while (pool.has_pending_tasks())
    {
      if (ps.from_last_msg.elapsed() > 2s)
      {
        use_sleep = true;

        if (!print_and_update_if_better(sum_.best()))
          print(message::status, ps);
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

    if (refinement_callback_)
      perform_refinement(ps);

    sum_.az = analyzer(pop_, eva_);
    if (search_log_)
      search_log_->save_snapshot(pop_, sum_);

    strategy.after_generation(pop_, sum_);  // strategy-specific bookkeeping
    if (after_generation_callback_)
      after_generation_callback_(pop_, sum_);
  }

  sum_.elapsed = ps.from_start.elapsed();

  if (emit_messages_)
  {
    ultraINFO << "Evolution completed at generation: " << sum_.generation
              << ". Elapsed time: "
              << lexical_cast<std::string>(ps.from_start.elapsed());
  }

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
