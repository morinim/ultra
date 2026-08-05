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

#if !defined(ULTRA_EVOLUTION_RECOMBINATION_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_EVOLUTION_RECOMBINATION_TCC)
#define      ULTRA_EVOLUTION_RECOMBINATION_TCC

///
/// \param[in]  eva    a fitness function
/// \param[in]  prob   the current problem
///
/// \warning
/// The lifetime of `eva` must exceed lifetime of `this` class.
///
template<Evaluator E>
strategy<E>::strategy(E &eva, const problem &prob) : eva_(eva), prob_(prob)
{
}

///
/// This is a quite standard crossover + mutation operator.
///
/// \param[in] parents a vector of ordered parents
/// \return            the offspring (single child)
///
template<Evaluator E>
template<RandomAccessIndividuals R>
[[nodiscard]] std::ranges::range_value_t<R>
base<E>::operator()(const R &parents) const
{
  static_assert(std::is_same_v<evaluator_individual_t<E>,
                               std::ranges::range_value_t<R>>);

  const auto &params(this->prob_.params.evolution);

  Expects(in_0_1(params.p_cross));
  Expects(params.brood_recombination);
  Expects(parents.size() >= 2);

  const bool use_crossover(random::boolean(params.p_cross));

  const auto generate([&]
  {
    if (!use_crossover)
    {
      auto offspring(parents[random::boolean()]);
      offspring.mutation(this->prob_);
      return offspring;
    }

    auto offspring(crossover(this->prob_, parents[0], parents[1]));

    if (params.p_mutation > 0.0)
    {
      constexpr unsigned max_attempts(32);

      // This could be an original contribution of Vita (now ported to Ultra) but
      // it's hard to be sure.
      // It remembers of the hereditary repulsion constraint (I guess you could
      // call it signature repulsion) and seems to:
      // - maintain diversity during the exploration phase;
      // - optimize the exploitation phase.
      for (unsigned i(0);
           i < max_attempts
             && (offspring.signature() == parents[0].signature()
                 || offspring.signature() == parents[1].signature());
           ++i)
        offspring.mutation(this->prob_, static_cast<double>(i + 1));
    }

    return offspring;
  });

  const unsigned brood_size(use_crossover || params.p_mutation > 0.0
                            ? params.brood_recombination : 1);

  if (brood_size == 1)
    return generate();

  const auto brood_trial([&]
  {
    auto offspring(generate());
    auto fitness(this->eva_(offspring));
    return scored_individual(std::move(offspring), std::move(fitness));
  });

  auto best(brood_trial());

  for (unsigned i(1); i < brood_size; ++i)
    if (auto candidate(brood_trial()); candidate > best)
      best = std::move(candidate);

  return std::move(best.ind);
}

///
/// This is strictly based on the DE crossover operator.
///
/// \param[in] engaged a vector of ordered individuals (parent1, parent2/base,
///                    random1, random2)
/// \return            the offspring / trial vector
///
/// Used parameters: `evolution.p_cross`, `de.weight`.
///
template<Individual I>
I de::operator()(const ultra::selection::de::selected_refs<I> &engaged) const
{
  const auto &params(prob_.params);
  Expects(in_0_1(params.evolution.p_cross));

  return engaged.target.crossover(params.evolution.p_cross, params.de.weight,
                                  engaged.base, engaged.a, engaged.b);
}

#endif  // include guard
