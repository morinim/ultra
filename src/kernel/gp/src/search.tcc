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
/// \return        a pointer to the oracle (`nullptr` in case of errors)
///
/// The oracle depends on the active training evaluator.
///
template<template<class> class ES, Evaluator E>
std::unique_ptr<basic_oracle> basic_search<ES, E>::oracle(
  const individual_t &ind) const
{
  if constexpr (requires { this->eva_.core().oracle(ind); })
    return this->eva_.core().oracle(ind);

  return nullptr;
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

  if ((metrics_ & metric_flags::accuracy))
  {
    const auto prg_oracle(oracle(prg));
    ret.accuracy = prg_oracle->measure(accuracy_metric(),
                                       prob().data.selected());
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
search<P>::search(problem &p, metric_flags m) : prob_(p), metrics_(m)
{
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
  stop_source_ = ss;
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
/// The tag is used to identify this object when multiple searches are
/// performed in parallel.
///
template<Individual P>
search<P> &search<P>::tag(const std::string &t)
{
  tag_ = t;
  return *this;
}

template<Individual P>
search_stats<P, typename search<P>::fitness_t> search<P>::run(
  unsigned n, const model_measurements<fitness_t> &threshold)
{
  const auto search_scheme([&]<Evaluator E>()
  {
    basic_search<alps_es, E> alps(prob_, metrics_);

    if (vs_)
      alps.validation_strategy(*vs_);
    if (search_log_)
      alps.logger(*search_log_);
    alps.after_generation(after_generation_callback_)
        .stop_source(stop_source_).tag(tag_);

    return alps.run(n, threshold);
  });

  if (prob_.classification())
    return search_scheme.template operator()<class_evaluator_t>();
  else
    return search_scheme.template operator()<reg_evaluator_t>();
}

template<Individual P>
std::unique_ptr<basic_oracle> search<P>::oracle(const P &prg) const
{
  if (prob_.classification())
    return class_evaluator_t(prob_.data).oracle(prg);
  else
    return reg_evaluator_t(prob_.data).oracle(prg);
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
  after_generation_callback_ = std::move(f);
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
  vs_ = std::make_unique<V>(std::forward<Args>(args)...);
  return *this;
}

#endif  // include guard
