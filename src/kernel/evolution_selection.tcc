/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_EVOLUTION_SELECTION_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_EVOLUTION_SELECTION_TCC)
#define      ULTRA_EVOLUTION_SELECTION_TCC

///
/// \param[in] env environment (for selection specific parameters)
/// \param[in] eva current evaluator
///
template<Evaluator E>
strategy<E>::strategy(E &eva, const environment &env) : eva_(eva), env_(env)
{
}

///
/// \param[in] pop a population
/// \return        a collection of individuals ordered in descending fitness
///
/// Tournament selection works by selecting a number of individuals from the
/// population at random (a tournament) and then choosing only the best of
/// those individuals.
/// Recall that better individuals have higher fitness.
///
/// Parameters from the environment: `mate_zone`, `tournament_size`.
///
/// \remark
/// Different compilers may optimize the code producing slightly different
/// sortings (due to floating point approximations). This is a known *issue*.
/// Anyway we keep using the `<` operator because:
/// - it's faster than the `std::fabs(delta)` approach;
/// - the additional *noise* is marginal (for the GA/GP standard);
/// - for debugging purposes *compiler-stability* is enough (and we have faith
///   in the test suite).
///
template<Evaluator E>
template<Population P>
std::vector<typename P::value_type>
tournament<E>::operator()(const P &pop) const
{
  const auto rounds(this->env_.evolution.tournament_size);
  const auto mate_zone(this->env_.evolution.mate_zone);
  assert(rounds);

  const auto target(random::coord(pop));
  std::vector<typename P::coord> ret(rounds);

  // This is the inner loop of an insertion sort algorithm. It's simple, fast
  // (if `rounds` is small) and doesn't perform too much comparisons.
  // DO NOT USE `std::sort` it's way slower.
  for (unsigned i(0); i < rounds; ++i)
  {
    const auto new_coord(random::coord(pop, target, mate_zone));
    const auto new_fitness(this->eva_(pop[new_coord]));

    auto j(i);

    for (; j && new_fitness > this->eva_(pop[ret[j - 1]]); --j)
      ret[j] = ret[j - 1];

    ret[j] = new_coord;
  }

  std::vector<typename P::value_type> ri;
  ri.reserve(ret.size());
  std::ranges::transform(ret, std::back_inserter(ri),
                         [&pop](const auto &c) { return pop[c]; });

  Ensures(ri.size() == rounds);
  Ensures(std::ranges::is_sorted(ri,
                                 [this](const auto &i1, const auto &i2)
                                 {
                                   return this->eva_(i1) > this->eva_(i2);
                                 }));

  return ri;
}

///
/// \param[in] pop a population
/// \return        a collection of chosen individuals
///
/// Parameters from the environment:
/// - `tournament_size` to control number of selected individuals.
///
template<Evaluator E>
template<Population P>
std::vector<typename P::value_type>
alps<E>::operator()(const P &pop) const
{
  Expects(this->env_.evolution.tournament_size);

  const auto young([&pop](const auto &prg)
                   { return prg.age() <= pop.max_age(); });

  // Extends the basic fitness with the age and takes advantage of the
  // lexicographic comparison capabilities of `std::pair`.
  const auto alps_fit([&](const auto &prg)
                      { return std::pair(young(prg), this->eva_(prg)); });

  auto c0(random::coord(pop));
  auto c1(random::coord(pop));

  auto fit0{alps_fit(pop[c0])};
  auto fit1{alps_fit(pop[c1])};

  if (fit0 < fit1)
  {
    std::swap(c0, c1);
    std::swap(fit0, fit1);
  }

  assert(fit0 >= fit1);

  for (auto rounds(this->env_.evolution.tournament_size - 1); rounds; --rounds)
  {
    const auto tmp(random::coord(pop));
    const auto tmp_fit{alps_fit(pop[tmp])};

    if (fit0 < tmp_fit)
    {
      c1 = c0;
      fit1 = fit0;

      c0 = tmp;
      fit0 = tmp_fit;
    }
    else if (fit1 < tmp_fit)
    {
      c1 = tmp;
      fit1 = tmp_fit;
    }

    assert(fit0.first == young(pop[c0]));
    assert(fit1.first == young(pop[c1]));
    assert(almost_equal(fit0.second, this->eva_(pop[c0])));
    assert(almost_equal(fit1.second, this->eva_(pop[c1])));
    assert(fit0 >= fit1);
    assert(young(pop[c0]) || !young(pop[c1]));
  }

  return {pop[c0], pop[c1]};
}

///
/// \param[in] pop a population
/// \return        a collection of four individuals suited for DE recombination
///
/// Parameters from the environment: `mate_zone`.
///
template<Evaluator E>
template<Population P>
std::vector<typename P::value_type> de<E>::operator()(const P &pop) const
{
  const auto mate_zone(this->env_.evolution.mate_zone);

  const auto p1(random::coord(pop));
  const auto p2(random::coord(pop));

  auto a(random::coord(pop, p1, mate_zone));

  decltype(a) b;
  do b = random::coord(pop, p1, mate_zone); while (a == b);

  return {pop[p1], pop[p2], pop[a], pop[b]};
}

#endif  // include guard
