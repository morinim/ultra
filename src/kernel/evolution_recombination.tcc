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
/// \param[in]  eva   a fitness function
/// \param[in]  prob  the current problem
/// \param[out] stats reference to the current set of statistics
///
template<Evaluator E>
strategy<E>::strategy(E &eva, const problem &prob,
                      summary<closure_arg_t<E>, closure_return_t<E>> &stats)
  : eva_(eva), prob_(prob), stats_(stats)
{
}

///
/// This is a quite standard crossover + mutation operator.
///
/// \param[in] parents a vector of ordered parents
/// \return            the offspring (single child)
///
template<Evaluator E>
template<SizedRangeOfIndividuals R>
[[nodiscard]] std::vector<std::ranges::range_value_t<R>>
base<E>::operator()(const R &parents) const
{
  static_assert(std::is_same_v<closure_arg_t<E>,
                               std::ranges::range_value_t<R>>);

  const auto p_cross(this->prob_.env.evolution.p_cross);
  const auto brood_recombination(this->prob_.env.evolution.brood_recombination);

  Expects(0.0 <= p_cross && p_cross <= 1.0);
  Expects(brood_recombination);
  Expects(parents.size() >= 2);

  if (random::boolean(p_cross))
  {
    const auto cross_and_mutate(
      [this](const auto &p1, const auto &p2)
      {
        auto ret(crossover(p1, p2));
        ++this->stats_.crossovers;

        if (this->prob_.env.evolution.p_mutation > 0.0)
        {
          // This could be an original contribution of Vita (now ported to
          // Ultra) but it's hard to be sure.
          // It remembers of the hereditary repulsion constraint (I guess you
          // could call it signature repulsion) and seems to:
          // - maintain diversity during the exploration phase;
          // - optimize the exploitation phase.
          while (p1.signature() == ret.signature()
                 || p2.signature() == ret.signature())
            this->stats_.mutations += ret.mutation(this->prob_);
        }

        return ret;
      });

    auto off(cross_and_mutate(parents[0], parents[1]));

    if (brood_recombination > 1)
    {
      auto fit_off(this->eva_(off));

      for (unsigned i(1); i < brood_recombination; ++i)
      {
        const auto tmp(cross_and_mutate(parents[0], parents[1]));

        if (const auto fit_tmp(this->eva_(tmp)); fit_tmp > fit_off)
        {
          off     =     tmp;
          fit_off = fit_tmp;
        }
      }
    }

    return {off};
  }

  // !crossover
  auto off(parents[random::boolean()]);
  this->stats_.mutations += off.mutation(this->prob_);

  return {off};
}

#endif  // include guard
