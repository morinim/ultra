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

#if !defined(ULTRA_EVOLUTION_STRATEGY_H)
#define      ULTRA_EVOLUTION_STRATEGY_H

#include "kernel/alps.h"
#include "kernel/analyzer.h"
#include "kernel/evolution_recombination.h"
#include "kernel/evolution_replacement.h"
#include "kernel/evolution_selection.h"
#include "kernel/evolution_summary.h"
#include "kernel/population.h"

#include <thread>

namespace ultra
{

///
/// Base class for compile-time evolutionary strategies.
///
/// An `evolution_strategy` encapsulates all strategy-dependent aspects of the
/// evolutionary process, while leaving the overall control flow to the
/// `evolution` driver.
///
/// This design uses static polymorphism: concrete strategies derive from
/// `evolution_strategy<E>` and provide the required operations at compile
/// time. No runtime polymorphism is involved.
///
/// The `evolution` driver owns the main generational loop, termination
/// conditions, concurrency, and logging, while concrete strategies define
/// how one evolutionary step is assembled and how generation-level
/// maintenance is performed.
///
/// Strategies are expected to be:
/// - stateless or minimally stateful;
/// - reusable across runs;
/// - independent from the evaluation logic.
///
/// The base class stores shared references to the optimisation problem and
/// evaluator, and provides default no-op hooks where appropriate.
///
/// \remark
/// Implementations should avoid performing expensive operations in
/// constructors; initialisation should be deferred to `init()`.
///
template<Evaluator E>
class evolution_strategy
{
public:
  using evaluator_type = E;
  using fitness_t = evaluator_fitness_t<E>;
  using individual_t = evaluator_individual_t<E>;

  /// Sets strategy-specific parameters.
  /// The default implementation doesn't change the user-specified parameters.
  /// Some evolution strategies force parameters to specific values.
  static parameters shape(const parameters &params) { return params; }

  /// Initialises the strategy before the first generation.
  ///
  /// This method is called once, before the evolutionary loop begins.
  /// It allows the strategy to:
  /// - initialise internal state;
  /// - prepare population structures (e.g. layers, age counters);
  /// - validate configuration parameters.
  ///
  template<Population P> void init(P &) const {}

  /// Perform post-generation maintenance.
  ///
  /// This hook is called once per generation, after all evolutionary steps
  /// have been executed.
  ///
  /// Typical responsibilities include:
  /// - updating individual metadata (e.g. age);
  /// - restructuring the population (e.g. layer promotion or merging);
  /// - detecting stagnation or convergence.
  ///
  template<Population P> void after_generation(
    P &, const summary<individual_t, fitness_t> &);

  [[nodiscard]] const problem &get_problem() const noexcept { return prob_; }

protected:
  evolution_strategy(const problem &, E &);

  template<Population P>
  [[nodiscard]] static bool valid_layer(P &, typename P::layer_iter);

  E &eva_;
  const problem &prob_;
};

///
/// Basic ALPS strategy.
///
/// With ALPS, several instances of a search algorithm are run in parallel,
/// each in its own age-layer, and the age of solutions is kept track of. The
/// key properties of ALPS are:
/// * each age-layer has its own sub-population of one or more candidate
///   solutions (individuals);
/// * each age-layer has a maximum age and it may not contain individuals
///   older than that maximum age;
/// * the age of individuals is based on when the original genetic material
///   was created from random;
/// * the search algorithm in a given age-layer can look at individuals in
///   its own sub-population and at the sub-populations in younger age layers
///   but it can only replace individuals in its own population;
/// * at regular intervals, the search algorithm in the first age-layer is
///   restarted.
///
/// Age is a measure of how long an individual's family of
/// genotypic material has been in the population. Randomly generated
/// individuals, such as those that are created when the search algorithm are
/// started, start with an age of `0`. Each generation that an individual stays
/// in the population its age is increased by one. Individuals that are
/// created through mutation or recombination take the age of their oldest
/// parent and add one to it. This differs from conventional measures of age,
/// in which individuals created through applying some type of variation to
/// an existing individual (e.g. mutation or recombination) start with an age
/// of `0`.
///
/// The search algorithm in a given layer acts somewhat independently of the
/// others, with an exception being that it can use individuals from both its
/// layer and the layer below to generated new candidate solutions. Also,
/// each age layer has an upper limit on the age of solutions it can contain.
/// When an individual is too old for its current layer, it cannot be used to
/// generate new individuals for that layer and eventually is removed from
/// that layer. Optionally, an attempt can be made to move this individual up
/// to the next layer -- in which case it replaces some individual there that
/// it's better than. Finally, at regular intervals the bottom layer is
/// replaced with a new sub-population of randomly generated individuals,
/// each with an age of `0`.
///
/// \see <https://github.com/ghornby/alps>
///
template<Evaluator E>
class alps_es : public evolution_strategy<E>
{
public:
  using typename alps_es::evolution_strategy::fitness_t;
  using typename alps_es::evolution_strategy::individual_t;

  alps_es(const problem &, E &);

  template<Population P> [[nodiscard]] auto operations(
    P &, typename P::layer_iter,
    const evolution_status<individual_t, fitness_t> &) const;

  template<Population P> void init(P &) const;
  template<Population P> void after_generation(
    P &, const summary<individual_t, fitness_t> &);

  static parameters shape(parameters);

private:
  const selection::alps<E>     select_;
  const recombination::base<E> recombine_;
  replacement::alps<E>         replace_;
};  // class alps_es

///
/// Standard evolution strategy.
///
template<Evaluator E>
class std_es : public evolution_strategy<E>
{
public:
  using typename std_es::evolution_strategy::fitness_t;
  using typename std_es::evolution_strategy::individual_t;

  std_es(const problem &, E &);

  template<Population P> [[nodiscard]] auto operations(
    P &, typename P::layer_iter,
    const evolution_status<individual_t, fitness_t> &) const;

private:
  const selection::tournament<E>   select_;
  const recombination::base<E>     recombine_;
  const replacement::tournament<E> replace_;
};  // class std_es

///
/// Differential evolution strategy.
///
/// Implemented as described in
/// https://github.com/morinim/ultra/wiki/bibliography#5.
///
template<Evaluator E>
class de_es : public evolution_strategy<E>
{
public:
  using typename de_es::evolution_strategy::fitness_t;
  using typename de_es::evolution_strategy::individual_t;

  de_es(const problem &, E &);

  template<Population P> [[nodiscard]] auto operations(
    P &, typename P::layer_iter,
    const evolution_status<individual_t, fitness_t> &) const;

private:
  const selection::de      select_ {};
  const recombination::de  recombine_;
  const replacement::de<E> replace_;
};  // class de_es


///
/// Concept identifying valid compile-time evolutionary strategies.
///
/// A type satisfies `Strategy` if, after removing cv/ref qualifiers:
/// - it exposes an `evaluator_type`;
/// - that type satisfies `Evaluator`;
/// - the type derives from `evolution_strategy<evaluator_type>`.
///
/// This allows the concept to recognise both `evolution_strategy<E>` itself
/// and concrete strategy classes derived from it, such as `alps_es<E>`,
/// `std_es<E>`, and `de_es<E>`.
///
template<class T>
concept Strategy =
  requires
  {
    typename std::remove_cvref_t<T>::evaluator_type;
  }
  && Evaluator<typename std::remove_cvref_t<T>::evaluator_type>
  && std::derived_from<
       std::remove_cvref_t<T>,
       evolution_strategy<typename std::remove_cvref_t<T>::evaluator_type>>;

#include "kernel/evolution_strategy.tcc"
}  // namespace ultra

#endif  // include guard
