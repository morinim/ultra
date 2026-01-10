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

///
/// Constructs an evaluator proxy.
///
/// \param[in] eva the underlying ("real") evaluator
/// \param[in] ts  cache size expressed as a bit width; the cache contains
///                `2^ts` elements
///
template<Evaluator E>
evaluator_proxy<E>::evaluator_proxy(E eva, bitwidth ts) : eva_(std::move(eva)),
                                                          cache_(ts)
{
  static_assert(!std::derived_from<E, evaluator_proxy>);
  Expects(ts);
}

///
/// Evaluates the fitness of an individual.
///
/// \param[in] prg the individual to evaluate
/// \return        the fitness value
///
/// If caching is enabled and a cached value exists for the individual's
/// signature, the cached fitness is returned. Otherwise, the underlying
/// evaluator is invoked and the result is stored in the cache.
///
template<Evaluator E>
evaluator_fitness_t<E> evaluator_proxy<E>::operator()(
  const evaluator_individual_t<E> &prg) const
{
  if (!cache_.bits())
    return eva_(prg);

  if (const auto cached_fit(cache_.find(prg.signature())); cached_fit)
  {
    // Hash collision checking code can severely slow down the program.
#if !defined(NDEBUG)
    const auto effective_fit(eva_(prg));

    if (!almost_equal(*cached_fit, effective_fit))
    {
      ultraERROR << "COLLISION [" << *cached_fit << " != " << effective_fit
                 << ']';
    }

    // The above comparison may produce false positives.
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

    return *cached_fit;
  }

  // Not found in cache.
  const auto effective_fit(eva_(prg));
  cache_.insert(prg.signature(), effective_fit);

  return effective_fit;
}

///
/// Computes a fast (approximate) fitness value for an individual.
///
/// \param[in] prg the individual to evaluate
/// \return        a (possibly approximate) fitness value
///
/// If the underlying evaluator provides a `fast` member function, that
/// function is invoked. Otherwise, this function falls back to the standard
/// evaluator call.
///
/// The fast evaluation:
/// - is not cached;
/// - may return an approximation of the true fitness;
/// - is intended for heuristics, pre-filtering or speculative evaluation.
///
/// \remark
/// The approximated ("fast") fitness isn't stored in the cache.
///
template<Evaluator E>
evaluator_fitness_t<E> evaluator_proxy<E>::fast(
  const evaluator_individual_t<E> &prg) const
{
  if constexpr (requires { eva_.fast(prg); })
    return eva_.fast(prg);

  return eva_(prg);
}

///
/// Loads the contents of the evaluation cache.
///
/// \param[in,out] in input stream
/// \return           `true` if the cache was loaded successfully
///
/// \warning
/// If the load operation fails, the cache may be left in a partially modified
/// state.
///
template<Evaluator E>
bool evaluator_proxy<E>::load_cache(std::istream &in) const
{
  return cache_.load(in);
}

///
/// Loads the persistent state of the proxy.
///
/// \param[in,out] in input stream
/// \return           `true` if the proxy was loaded successfully
///
/// \warning
/// If the load operation fails, the object may be left in a partially modified
/// state.
/// The temporary object needed to holds values from the stream conceivably is
/// too big to justify the "no change" warranty.
///
template<Evaluator E>
bool evaluator_proxy<E>::load(std::istream &in)
{
  return load_eva(in, &eva_) && load_cache(in);
}

///
/// Saves the contents of the evaluation cache.
///
// \param[out] out output stream
/// \return        `true` if the cache was saved successfully
///
template<Evaluator E>
bool evaluator_proxy<E>::save_cache(std::ostream &out) const
{
  return cache_.save(out);
}

///
/// Saves the persistent state of the proxy.
///
/// This function saves:
/// - the state of the underlying evaluator (if it supports persistence);
/// - the contents of the evaluation cache.
///
/// \param[out] out output stream
/// \return         `true` if the proxy was saved successfully
///
template<Evaluator E>
bool evaluator_proxy<E>::save(std::ostream &out) const
{
  return save_eva(out, eva_) && save_cache(out);
}

///
/// Clears the entire evaluation cache.
///
/// Subsequent evaluations will recompute fitness values as if the cache were
/// empty.
///
template<Evaluator E>
void evaluator_proxy<E>::clear() const
{
  cache_.clear();
}

///
/// Clears a specific cache entry.
///
/// \param[in] h signature of the individual whose cached fitness should be
///              removed
///
template<Evaluator E>
void evaluator_proxy<E>::clear(const hash_t &h) const
{
  cache_.clear(h);
}

///
/// Provides read-only access to the underlying evaluator.
///
/// \return a const reference to the wrapped evaluator
///
template<Evaluator E>
const E &evaluator_proxy<E>::core() const noexcept
{
  return eva_;
}

#endif  // include guard
