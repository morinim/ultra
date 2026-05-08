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

#if !defined(ULTRA_SRC_SEARCH_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_SRC_SEARCH_TCC)
#define      ULTRA_SRC_SEARCH_TCC

/// Sentinel type representing the absence of an oracle.
///
/// This type is produced when the active evaluator does not provide an
/// `oracle()` method.
struct no_oracle
{
};

namespace internal
{

template<class O>
[[nodiscard]] std::unique_ptr<basic_oracle> make_basic_oracle(O &&o)
{
  using oracle_t = std::remove_cvref_t<O>;

  if constexpr (std::same_as<oracle_t, no_oracle>)
    return nullptr;
  else
  {
    static_assert(std::derived_from<oracle_t, basic_oracle>);
    return std::make_unique<oracle_t>(std::forward<O>(o));
  }
}

}  // namespace internal

[[nodiscard]] constexpr std::underlying_type_t<metric_flags> operator&(
  metric_flags f1, metric_flags f2)
{
  return as_integer(f1) & as_integer(f2);
}

///
/// Builds a new search class, user chooses the evaluator.
///
/// \param[in] p the problem we're working on. The lifetime of `p` must exceed
///              the lifetime of `this` class
/// \param[in] m a bit field used to specify matrics we have to calculate while
///              searching
///
template<template<class> class ES, Evaluator E>
basic_search<ES, E>::basic_search(problem &p, metric_flags m)
  : ultra::basic_search<ES, E>(p, E(p.data)), metrics_(m)
{
  Ensures(this->is_valid());
}

///
/// Creates an oracle associated with a given individual.
///
/// \param[in] ind individual used as building block for the oracle
/// \return        the evaluator-specific oracle, or `no_oracle` if the active
///                evaluator does not provide one
///
/// The returned object depends on the evaluator used during training:
/// - if the evaluator exposes an `oracle()` method, its result is forwarded;
/// - otherwise, a `no_oracle` sentinel is returned.
///
/// This function is always well-formed: lack of oracle support is handled at
/// compile time via the `no_oracle` fallback rather than via runtime errors.
///
template<template<class> class ES, Evaluator E>
auto basic_search<ES, E>::oracle(const individual_t &ind) const
{
  const auto &eva_core(this->eva_.core());

  if constexpr (requires { eva_core.oracle(ind); })
    return eva_core.oracle(ind);
  else
    return no_oracle();
}

///
/// \return a reference to the current problem
///
template<template<class> class ES, Evaluator E>
problem &basic_search<ES, E>::prob() const noexcept
{
  return static_cast<problem &>(this->prob_);
}

///
/// Calculates various performance metrics.
///
/// \param[in] prg best individual from the evolution run just finished
/// \return        measurements about the individual
///
/// Fitness and accuracy are calculated by default. Additional must be
/// explicitly requested in the basic_search constructor.
///
/// \warning
/// Can be very time consuming.
///
template<template<class> class ES, Evaluator E>
model_measurements<typename basic_search<ES, E>::fitness_t>
basic_search<ES, E>::calculate_metrics(const individual_t &prg) const
{
  auto ret(ultra::basic_search<ES, E>::calculate_metrics(prg));
  const auto prg_oracle(oracle(prg));

  if constexpr (ClassificationPredictor<decltype(prg_oracle)>)
    if ((metrics_ & metric_flags::accuracy) && prob().classification())
    {
      ret.accuracy = metrics::accuracy(prg_oracle, prob().data.selected());
    }

  return ret;
}

///
/// Tries to tune search parameters for the current problem.
///
/// Parameter tuning is a typical approach to algorithm design. Such tuning
/// is done by experimenting with different values and selecting the ones
/// that give the best results on the test problems at hand.
///
/// However, the number of possible parameters and their different values
/// means that this is a very complex and time-consuming task; it is
/// something we do not want users to worry about (power users can force many
/// parameters, but our idea is "simple by default").
///
/// So if user sets an environment parameter he will force the search class
/// to use it as is. Otherwise this function will try to guess a good
/// starting point and changes its hint after every run. The code is a mix of
/// black magic, experience, common logic and randomness but it seems
/// reasonable.
///
/// \note
/// It has been formally proven, in the No-Free-Lunch theorem, that it is
/// impossible to tune a search algorithm such that it will have optimal
/// settings for all possible problems, but parameters can be properly
/// set for a given problem.
///
/// \see
/// - https://github.com/morinim/ultra/wiki/bibliography#11
/// - https://github.com/morinim/ultra/wiki/bibliography#12
///
template<template<class> class ES, Evaluator E>
void basic_search<ES, E>::tune_parameters()
{
  // The `shape` function modifies the default parameters with
  // strategy-specific values.
  const auto dflt(ES<E>::shape(parameters().init()));

  auto &params(prob().params);

  // Contains user-specified parameters that will be partly changed by this
  // function.
  const auto constrained(params);

  ultra::basic_search<ES, E>::tune_parameters();

  const auto d_size(prob().data.selected().size());
  Expects(d_size);

  if (!constrained.population.init_subgroups)
  {
    if (dflt.population.init_subgroups > 1 && d_size > 8)
      params.population.init_subgroups =
        static_cast<std::size_t>(std::log(d_size));
    else
      params.population.init_subgroups = dflt.population.init_subgroups;

    ultraINFO << "Number of layers set to "
              << params.population.init_subgroups;
  }

  // A larger number of training cases requires an increase in the population
  // size (e.g. https://github.com/morinim/ultra/wiki/bibliography#11 suggests
  // 10 - 1000 individuals for smaller problems; between 1000 and 10000
  // individuals for complex problem (more than 200 fitness cases).
  //
  // We chose a strictly increasing function to link training set size and
  // population size.
  if (!constrained.population.individuals)
  {
    if (d_size > 8)
    {
      params.population.individuals =
        2 * static_cast<std::size_t>(std::pow(std::log2(d_size), 3))
        / params.population.init_subgroups;
    }
    else
      params.population.individuals = dflt.population.individuals;

    if (params.population.individuals < 4)
      params.population.individuals = 4;

    ultraINFO << "Population size set to " << params.population.individuals;
  }

  Ensures(params.is_valid(true));
}

///
/// \return `true` if the object passes the internal consistency check
///
template<template<class> class ES, Evaluator E>
bool basic_search<ES, E>::is_valid() const
{
  return ultra::basic_search<ES, E>::is_valid();
}

template<Individual P>
search<P>::search(problem &p, metric_flags m)
  : engine_(make_engine(p, m)), prob_(p), classification_(p.classification())
{
}

template<Individual P>
typename search<P>::engine_t search<P>::make_engine(problem &p, metric_flags m)
{
  if (p.classification())
    return engine_t(std::in_place_type<class_search_t>, p, m);

  return engine_t(std::in_place_type<reg_search_t>, p, m);
}

template<Individual P>
bool search<P>::problem_type_unchanged() const noexcept
{
  return classification_ == prob_.classification();
}

///
/// Set a stop source for performing cooperative task shutdown.
///
/// \param[in] ss stop source to issue a stop request
/// \return      a reference to *this* object (method chaining / fluent
///              interface)
///
template<Individual P>
search<P> &search<P>::stop_source(std::stop_source ss)
{
  std::visit([&](auto &s) { s.stop_source(ss); }, engine_);
  return *this;
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
/// disabled.
///
template<Individual P>
search<P> &search<P>::logger(search_log &sl)
{
  Expects(sl.is_valid());

  std::visit([&](auto &s) { s.logger(sl); }, engine_);
  return *this;
}

template<Individual P>
search<P> &search<P>::messages(bool m) noexcept
{
  std::visit([&](auto &s) { s.messages(m); }, engine_);
  return *this;
}

///
/// Sets the identification tag for this object.
///
/// \param[in] t identification tag
/// \return      a reference to *this* object (method chaining / fluent
///              interface)
///
/// The tag is used to identify this object when multiple searches are
/// performed in parallel.
///
template<Individual P>
search<P> &search<P>::tag(const std::string &t)
{
  std::visit([&](auto &s) { s.tag(t); }, engine_);
  return *this;
}

template<Individual P>
search_stats<P, typename search<P>::fitness_t> search<P>::run(
  unsigned n, const model_measurements<fitness_t> &threshold)
{
  Expects(problem_type_unchanged());

  return std::visit([&](auto &s) { return s.run(n, threshold); }, engine_);
}

///
/// Sets a refinement callback used to locally improve individuals.
///
/// \param[in] f callable object implementing the refinement logic
/// \return      a reference to `*this` object (method chaining / fluent
///              interface)
///
/// The refinement callback is invoked during the evolutionary process to
/// perform local optimisation (e.g. numerical tuning of parameters) on
/// candidate solutions.
///
/// The callable must have a signature compatible with at least one of the
/// following forms:
///
/// ```
/// std::optional<fitness_t>(
///   P &, const evaluator_proxy<class_evaluator_t> &,
///   const parameters::refinement_parameters &)
///
/// std::optional<fitness_t>(
///   P &, const evaluator_proxy<reg_evaluator_t> &,
///   const parameters::refinement_parameters &)
/// ```
///
/// A generic callable can be used to support both cases:
///
/// ```
/// [](P &ind, const auto &eva,
///    const parameters::refinement_parameters &params)
/// {
///   // eva is the evaluator proxy used by the search engine
///   // (includes caching and other internal mechanisms)
///   return std::optional<fitness_t>();
/// }
/// ```
///
/// The evaluator passed to the callback is the same object used internally
/// by the evolutionary engine (i.e. an `evaluator_proxy`), ensuring that
/// refinement benefits from cached evaluations and consistent semantics.
///
/// \note
/// The concrete evaluator type depends on the problem (classification or
/// regression) and is selected automatically at runtime.
///
template<Individual P>
template<class F>
search<P> &search<P>::refinement(F &&f)
{
  using callback_t = std::remove_cvref_t<F>;

  constexpr bool class_ok = std::is_invocable_r_v<
    std::optional<fitness_t>, callback_t &, P &,
    const evaluator_proxy<class_evaluator_t> &,
    const parameters::refinement_parameters &>;

  constexpr bool reg_ok = std::is_invocable_r_v<
    std::optional<fitness_t>, callback_t &, P &,
    const evaluator_proxy<reg_evaluator_t> &,
    const parameters::refinement_parameters &>;

  static_assert(class_ok || reg_ok,
                "Refinement callback is not compatible with available "
                "evaluator signatures");

  auto callback(std::forward<F>(f));

  std::visit(
    [&](auto &s)
    {
      using search_t = std::remove_cvref_t<decltype(s)>;

      if constexpr (std::same_as<search_t, class_search_t>)
      {
        if constexpr (class_ok)
          s.refinement(callback);
        else
          s.refinement({});
      }
      else
      {
        static_assert(std::same_as<search_t, reg_search_t>);

        if constexpr (reg_ok)
          s.refinement(callback);
        else
          s.refinement({});
      }
    },
    engine_);

  return *this;
}

///
/// Creates a runtime-polymorphic oracle associated with a given individual.
///
/// \param[in] prg individual used as building block for the oracle
/// \return        a dynamically allocated oracle or `nullptr` if the active
///                evaluator does not provide one
///
/// This function provides a uniform interface over different evaluator types.
/// If the selected evaluator exposes an `oracle()` method, its result is
/// wrapped into a `std::unique_ptr<basic_oracle>`. Otherwise `nullptr` is
/// returned.
///
/// \note
/// Oracle availability is detected at compile time and mapped to a runtime
/// polymorphic interface.
///
/// \see basic_search::oracle
///
template<Individual P>
std::unique_ptr<basic_oracle> search<P>::oracle(const P &prg) const
{
  Expects(problem_type_unchanged());

  return std::visit(
    [&](const auto &s) { return internal::make_basic_oracle(s.oracle(prg)); },
    engine_);
}

///
/// Sets a callback function executed at the end of every generation.
///
/// \param[in] f callback function
/// \return      a reference to *this* object (method chaining / fluent
///              interface)
///
template<Individual P>
search<P> &search<P>::after_generation(after_generation_callback_t f)
{
  std::visit([&](auto &s) { s.after_generation(std::move(f)); }, engine_);
  return *this;
}

///
/// Hook invoked whenever the evolution discovers a new training-set best.
///
/// \param[in] f callback function
/// \return      a reference to *this* object (method chaining / fluent
///              interface)
///
template<Individual P>
search<P> &search<P>::on_training_new_best(on_training_new_best_callback_t f)
{
  std::visit([&](auto &s) { s.on_training_new_best(std::move(f)); }, engine_);
  return *this;
}

///
/// Builds and sets the active validation strategy.
///
/// \param[in] args parameters for the validation strategy
/// \return         reference to the search class (used for method chaining)
///
template<Individual P>
template<ValidationStrategy V, class... Args>
search<P> &search<P>::validation_strategy(Args && ...args)
{
  std::visit(
    [&](auto &s)
    { s.template validation_strategy<V>(std::forward<Args>(args)...); },
    engine_);
  return *this;
}

#endif  // include guard
