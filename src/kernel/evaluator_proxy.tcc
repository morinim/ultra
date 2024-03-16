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

#if !defined(ULTRA_EVALUATOR_PROXY_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_EVALUATOR_PROXY_TCC)
#define      ULTRA_EVALUATOR_PROXY_TCC

template<Evaluator E>
cache<evaluator_fitness_t<E>> evaluator_proxy<E>::cache_ {};

///
/// \param[in] eva the "real" evaluator
/// \param[in] ts  `2^ts` is the number of elements of the cache
///
template<Evaluator E>
evaluator_proxy<E>::evaluator_proxy(E eva, bitwidth ts) : eva_(std::move(eva))
{
  Expects(ts);
  cache_.resize(ts);
}

///
/// \param[in] eva the "real" evaluator
///
template<Evaluator E>
evaluator_proxy<E>::evaluator_proxy(E eva) : eva_(std::move(eva))
{
  Expects(cache_.bits());
}

///
/// \param[in] prg the program (individual/team) whose fitness we want to know
/// \return        the fitness of `prg`
///
template<Evaluator E>
evaluator_fitness_t<E> evaluator_proxy<E>::operator()(
  const evaluator_individual_t<E> &prg) const
{
  Expects(cache_.bits());

  auto *cached_fit(cache_.find(prg.signature()));

  if (cached_fit)
  {
    // Hash collision checking code can severely slow down the program.
#if !defined(NDEBUG)
    const auto effective_fit(eva_(prg));

    if (!almost_equal(*cached_fit, effective_fit))
    {
      ultraERROR << "COLLISION [" << *cached_fit << " != " << effective_fit
                 << ']';
    }

    // The above comparision may produce false positives.
    // E.g. it fails if a component of the fitness is function of the program's
    // length.
    // For example if the fitness is a 2D vector (where the first component
    // is the "score" on the training set and the second one is the effective
    // length of the program), then the following two programs:
    //
    // PROGRAM A                 PROGRAM B
    // ------------------        ------------------
    // [000] FADD 3 3            [000] FADD 3 3
    // [001] FADD 3 3            [001] FADD [000] [000]
    // [002] FADD [000] [001]
    //
    // have the same signature, the same stored "score" but distinct
    // effective size and so distinct fitnesses.
#endif
  }
  else  // not found in cache
  {
    const auto effective_fit(eva_(prg));

    cache_.insert(prg.signature(), effective_fit);

#if !defined(NDEBUG)
    cached_fit = cache_.find(prg.signature());
    assert(cached_fit);
    assert(almost_equal(*cached_fit, effective_fit));
#endif
  }

  return *cached_fit;
}

///
/// \param[in] prg the program (individual/team) whose fitness we want to know
/// \return        an approximation of the fitness of `prg`
///
/// \remark
/// The approximated ("fast") fitness isn't stored in the cache.
///
template<Evaluator E>
evaluator_fitness_t<E> evaluator_proxy<E>::fast(
  const evaluator_individual_t<E> &prg)
{
  if constexpr (requires { eva_(prg, evaluation_type::fast); })
    return eva_.fast(prg);

  return eva_(prg);
}

///
/// \param[in] in input stream
/// \return       `true` if the object loaded correctly
///
/// \warning
/// If the load operation isn't successful the current object COULD BE changed.
/// The temporary object needed to holds values from the stream conceivably is
/// too big to justify the "no change" warranty.
///
template<Evaluator E>
bool evaluator_proxy<E>::load(std::istream &in)
{
  return load(in, eva_) && cache_.load(in);
}

///
/// \param[out] out output stream
/// \return         `true` if the object was saved correctly
///
template<Evaluator E>
bool evaluator_proxy<E>::save(std::ostream &out) const
{
  return save(out, eva_) && cache_.save(out);
}

///
/// Resets the evaluation cache.
///
template<Evaluator E>
void evaluator_proxy<E>::clear()
{
  cache_.clear();
}

///
/// \param[in] prg a program (individual/team)
/// \return        a pointer to the executable version of `prg`
///
//template<Evaluator E>
//std::unique_ptr<basic_lambda_f> evaluator_proxy<E>::lambdify(
//  const T &prg) const
//{
//  return lambdify(prg, eva_);
//}

#endif  // include guard
