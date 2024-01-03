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
/// \param[in] pop a collection of populations. ALPS uses the subpopulations
///                (the first element is the primary population)
/// \return        a collection of chosen individuals
///
/// Parameters from the environment:
/// - `tournament_size` to control number of selected individuals.
/// - `p_main_layer`
///
template<Evaluator E>
template<PopulationWithMutex P>
std::vector<typename P::value_type>
alps<E>::operator()(std::vector<std::reference_wrapper<const P>> pop) const
{
  Expects(this->env_.evolution.tournament_size);
  Expects(pop.size() && pop.size() <= 2);

  const auto young([](const auto &sub_pop, const auto &prg)
                   { return prg.age() <= sub_pop.max_age(); });

  // Extends the basic fitness with the age and takes advantage of the
  // lexicographic comparison capabilities of `std::pair`.
  const auto alps_fit([&](const auto &sp, const auto &prg)
                      { return std::pair(young(sp, prg), this->eva_(prg)); });

  auto p0(random::individual(pop.front().get()));
  auto fit0{alps_fit(pop.front().get(), p0)};

  auto p1(random::individual(pop.front().get()));
  auto fit1{alps_fit(pop.front().get(), p1)};

  if (fit0 < fit1)
  {
    std::swap(p0, p1);
    std::swap(fit0, fit1);
  }

  assert(fit0 >= fit1);

  const auto p(1.0 - this->env_.alps.p_main_layer);

  for (auto rounds(this->env_.evolution.tournament_size - 1); rounds; --rounds)
  {
    const auto &sub_pop(pop[pop.size() > 1 ? random::boolean(p) : 0].get());
    const auto tmp(random::individual(sub_pop));
    const auto tmp_fit{alps_fit(sub_pop, tmp)};

    if (fit0 < tmp_fit)
    {
      p1 = p0;
      fit1 = fit0;

      p0 = tmp;
      fit0 = tmp_fit;
    }
    else if (fit1 < tmp_fit)
    {
      p1 = tmp;
      fit1 = tmp_fit;
    }

    assert(almost_equal(fit0.second, this->eva_(p0)));
    assert(almost_equal(fit1.second, this->eva_(p1)));
    assert(fit0 >= fit1);
    assert(fit0.first || !fit1.first);
  }

  return {p0, p1};
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

  const auto c1(random::coord(pop));
  const auto c2(random::coord(pop));

  auto a(random::coord(pop, c1, mate_zone));

  decltype(a) b;
  do b = random::coord(pop, c1, mate_zone); while (a == b);

  return {pop[c1], pop[c2], pop[a], pop[b]};
}

#endif  // include guard
