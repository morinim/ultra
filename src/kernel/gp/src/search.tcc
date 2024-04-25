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
/// \param[in] p   the problem we're working on. The lifetime of `p` must
///                exceed the lifetime of `this` class
/// \param[in] eva evaluator used during evolution. Must be copyable
/// \param[in] m   a bit field used to specify matrics we have to calculate
///                while searching
///
template<template<class> class ES, Evaluator E>
basic_search<ES, E>::basic_search(problem &p, E eva, metric_flags m)
  : ultra::search<ES, E>(p, eva), metrics_(m)
{
  Ensures(this->is_valid());
}

///
/// Creates an oracle associated with a given individual.
///
/// \param[in] ind individual used as building block for the oracle
/// \return        a pointer to the oracle (`nullptr` in case of errors)
///
/// The lambda function depends on the active training evaluator.
///
template<template<class> class ES, Evaluator E>
std::unique_ptr<basic_oracle> basic_search<ES, E>::oracle(
  const individual_t &ind) const
{
  if constexpr (requires { this->es_.evaluator().core().oracle(ind); })
    return this->es_.evaluator().core().oracle(ind);

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
/// \param[out] s update summary of the evolution run just finished
///               (metrics regarding `s.best.solution`)
///
/// Accuracy calculation is performed if AT LEAST ONE of the following
/// conditions is satisfied:
///
/// * the accuracy threshold is defined (`env.threshold.accuracy > 0.0`);
/// * we explicitly asked for accuracy calculation in the constructor.
///
/// Otherwise the function skips accuracy calculation.
///
/// \warning Can be very time consuming.
///
/*template<class T, template<class> class ES>
void basic_search<ES, E>::calculate_metrics(summary<T> *s) const
{
  if ((metrics & metric_flags::accuracy)
      || prob().env.threshold.accuracy > 0.0)
  {
    const auto model(lambdify(s->best.solution));
    const auto &d(can_validate() ? validation_data() : training_data());
    s->best.score.accuracy = model->measure(accuracy_metric(), d);
  }

  search<T, ES>::calculate_metrics(s);
  }*/

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
/// * "Parameter Setting in Evolutionary Algorithms" (F.G. Lobo, C.F. Lima,
///   Z. Michalewicz) - Springer;
/// - https://github.com/morinim/ultra/wiki/bibliography#9
///
template<template<class> class ES, Evaluator E>
void basic_search<ES, E>::tune_parameters()
{
  // The `shape` function modifies the default parameters with
  // strategy-specific values.
  const auto dflt(this->es_.shape(parameters().init()));

  auto &params(prob().params);

  // Contains user-specified parameters that will be partly changed by this
  // function.
  const auto constrained(params);

  ultra::search<ES, E>::tune_parameters();

  const auto d_size(prob().data().size());
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
  // size (e.g. https://github.com/morinim/ultra/wiki/bibliography#9 suggests
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

  //if (!constrained.validation_percentage.has_value()
  //  && typeid(this->vs_.get()) == typeid(holdout_validation))
  //env.validation_percentage = dflt.validation_percentage;

  Ensures(params.is_valid(true));
}

///
/// Writes end-of-run logs (run summary, results for test...).
///
/// \param[in]  s summary information regarding the search
/// \param[out] d output xml document
///
/*template<class T, template<class> class ES>
void basic_search<ES, E>::log_stats(const search_stats<T> &s,
                                  tinyxml2::XMLDocument *d) const
{
  Expects(d);

  const auto &stat(prob().env.stat);

  search<T, ES>::log_stats(s, d);

  if (!stat.summary_file.empty())
  {
    assert(d->FirstChild());
    assert(d->FirstChild()->FirstChildElement("summary"));

    auto *e_best(d->FirstChild()->FirstChildElement("summary")
                 ->FirstChildElement("best"));
    assert(e_best);
    set_text(e_best, "accuracy", s.overall.best.score.accuracy);
  }

  // Test set results logging.
  if (!stat.test_file.empty() && test_data().size())
  {
    const auto lambda(lambdify(s.overall.best.solution));

    std::ofstream tf(stat.dir / stat.test_file);
    for (const auto &example : test_data())
      tf << lambda->name((*lambda)(example)) << '\n';
  }
  }*/

///
/// Sets the active validation strategy.
///
/// \param[in] id  numerical id of the validator to be activated
/// \return        a reference to the search class (used for method chaining)
///
/// \exception std::invalid_argument unknown validation strategy
///
/*template<class T, template<class> class ES>
src_search<T, ES> &src_search<T, ES>::validation_strategy(validator_id id)
{
  switch (id)
  {
  case validator_id::as_is:
    search<T, ES>::template validation_strategy<as_is_validation>();
    break;

  case validator_id::dss:
    assert(this->eva1_);
    assert(this->eva2_);
    search<T, ES>::template validation_strategy<dss>(prob(),
                                               *this->eva1_, *this->eva2_);
    break;

  case validator_id::holdout:
    search<T, ES>::template validation_strategy<holdout_validation>(prob());
    break;

  default:
    throw std::invalid_argument("Unknown validation strategy");
  }

  return *this;
  }*/

/*
template<class T, template<class> class ES>
template<class E, class... Args>
void src_search<T, ES>::set_evaluator(Args && ...args)
{
  search<T, ES>::template training_evaluator<E>(
    training_data(), std::forward<Args>(args)...);

  search<T, ES>::template validation_evaluator<E>(
    validation_data(), std::forward<Args>(args)...);
}

///
/// \param[in] id  numerical id of the evaluator to be activated
/// \param[in] msg input parameters for the evaluator constructor
/// \return        a reference to the search class (used for method chaining)
///
/// \exception std::invalid_argument unknown evaluator
///
/// \note
/// If the evaluator `id` is not compatible with the problem type the
/// function returns `false` and the active evaluator stays the same.
///
template<class T, template<class> class ES>
src_search<T, ES> &src_search<T, ES>::evaluator(evaluator_id id,
                                                const std::string &msg)
{
  if (training_data().classes() > 1)
  {
    switch (id)
    {
    case evaluator_id::bin:
      set_evaluator<binary_evaluator<T>>();
      break;

    case evaluator_id::dyn_slot:
      {
        auto x_slot(static_cast<unsigned>(msg.empty() ? 10ul
                                                      : std::stoul(msg)));
        set_evaluator<dyn_slot_evaluator<T>>(x_slot);
      }
      break;

    case evaluator_id::gaussian:
      set_evaluator<gaussian_evaluator<T>>();
      break;

    default:
      throw std::invalid_argument("Unknown evaluator");
    }
  }
  else  // symbolic regression
  {
    switch (id)
    {
    case evaluator_id::count:
      set_evaluator<count_evaluator<T>>();
      break;

    case evaluator_id::mae:
      set_evaluator<mae_evaluator<T>>();
      break;

    case evaluator_id::rmae:
      set_evaluator<rmae_evaluator<T>>();
      break;

    case evaluator_id::mse:
      set_evaluator<mse_evaluator<T>>();
      break;

    default:
      throw std::invalid_argument("Unknown evaluator");
    }
  }

  return *this;
}
*/

///
/// \return `true` if the object passes the internal consistency check
///
template<template<class> class ES, Evaluator E>
bool basic_search<ES, E>::is_valid() const
{
  return ultra::search<ES, E>::is_valid();
}

#endif  // include guard