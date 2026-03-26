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

#if !defined(ULTRA_SRC_EVALUATOR_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_SRC_EVALUATOR_TCC)
#define      ULTRA_SRC_EVALUATOR_TCC

///
/// Constructs the evaluator bound to a dataset.
///
/// \param[in] d dataset used for fitness evaluation
///
template<EvaluationDataset D>
evaluator<D>::evaluator(D &d) noexcept : dat_(&d)
{
}

///
/// Returns the dataset currently used for evaluation.
///
/// \return reference to the active dataset
///
/// For `multi_dataset `, this returns the currently selected dataset.
/// Otherwise, it returns the dataset itself.
///
template<EvaluationDataset D>
const auto &evaluator<D>::data() const noexcept
{
  if constexpr (is_multi_dataset_v<D>)
    return dat_->selected();
  else
    return *dat_;
}

///
/// Constructs the evaluator.
///
/// \param[in] d training dataset
///
template<Individual P, class F, class D, aggregation_mode A, evaluation_mode M>
requires ExampleEvaluator<F, D, P>
aggregate_evaluator<P, F, D, A, M>::aggregate_evaluator(D &d) noexcept
  : evaluator<D>(d)
{
}

///
/// Computes the fitness using all training examples.
///
/// \param[in] prg program to evaluate
/// \return        fitness value (greater is better)
///
template<Individual P, class F, class D, aggregation_mode A, evaluation_mode M>
requires ExampleEvaluator<F, D, P>
double aggregate_evaluator<P, F, D, A, M>::operator()(const P &prg) const
{
  return eval_impl(prg, 1);
}

///
/// Computes a (possibly approximate) fitness value using subsampling.
///
/// \param[in] prg program to evaluate
/// \return        fitness value (exact or approximate)
///
/// For sufficiently large datasets, the program is evaluated on a subset
/// of the examples (one every `step` elements) to reduce computation time.
///
/// For smaller datasets, the full evaluation is performed, making this
/// function equivalent to operator()().
///
/// For average-based evaluators, subsampling yields an estimate of the
/// true average. For sum-based evaluators, the result is rescaled to remain
/// comparable with operator()().
///
template<Individual P, class F, class D, aggregation_mode A, evaluation_mode M>
requires ExampleEvaluator<F, D, P>
double aggregate_evaluator<P, F, D, A, M>::fast(const P &prg) const
{
  const auto n(std::ranges::distance(this->data()));
  return eval_impl(prg, n >= fast_min_examples ? fast_step : 1);
}

///
/// Builds a regression oracle associated with a program.
///
/// \param[in] prg program to transform into an oracle
/// \return        oracle object associated with `prg`
///
/// \remark
/// This member function is conditionally enabled only when the functor `F`
/// used by the evaluator is known to support regression semantics (as
/// indicated by `has_regression_oracle<F, P>`). It is not available for
/// classification or generic score-based evaluators.
///
template<Individual P, class F, class D, aggregation_mode A, evaluation_mode M>
requires ExampleEvaluator<F, D, P>
auto aggregate_evaluator<P, F, D, A, M>::oracle(const P &prg) const
  requires internal::has_regression_oracle_v<F, P>
{
  return reg_oracle<P>(prg);
}

///
/// \param[in] prg  program to evaluate
/// \param[in] step sampling step (one example every `step`)
/// \return         fitness value
///
template<Individual P, class F, class D, aggregation_mode A, evaluation_mode M>
requires ExampleEvaluator<F, D, P>
double aggregate_evaluator<P, F, D, A, M>::eval_impl(const P &prg,
                                                     std::ptrdiff_t step) const
{
  const auto &dat(this->data());
  const auto examples(std::ranges::distance(dat));
  Expects(0 < step && step <= examples);

  const F f(prg);

  auto it(std::begin(dat));
  const auto end(std::end(dat));

  double acc(0.0);
  std::size_t n(0);

  if constexpr (A == aggregation_mode::average)
  {
    acc = f(*it);
    ++n;
    std::ranges::advance(it, step, end);

    while (it != end)
    {
      acc += (f(*it) - acc) / static_cast<double>(++n);
      std::ranges::advance(it, step, end);
    }
  }
  else
  {
    while (it != end)
    {
      acc += f(*it);
      ++n;
      std::ranges::advance(it, step, end);
    }

    // This keeps `fast()` comparable with `operator()` for sum evaluators.
    if (step > 1)
      acc *= static_cast<double>(examples) / static_cast<double>(n);
  }

  Ensures(0 < n && n <= static_cast<std::size_t>(examples));

  if constexpr (M == evaluation_mode::error)
    return -acc;
  else
    return acc;
}

///
/// Sets up the environment for error measurement.
///
/// \param[in] prg the program to be measured
///
template<Individual P>
mae_error_functor<P>::mae_error_functor(const P &prg) : oracle_(prg)
{
}

///
/// \param[in] example current training case
/// \return            a measurement of the error of the model/program on the
///                    given training case (value in the `[0;+inf[` range)
///
template<Individual P>
double mae_error_functor<P>::operator()(const example &example) const
{
  if (const auto foreseen(oracle_(example.input)); has_value(foreseen))
    return std::fabs(std::get<D_DOUBLE>(foreseen)
                     - label_as<D_DOUBLE>(example));

  return std::numeric_limits<double>::max() / 1000.0;
}

///
/// Sets up the environment for error measurement.
///
/// \param[in] prg the program to be measured
///
template<Individual P>
rmae_error_functor<P>::rmae_error_functor(const P &prg) : oracle_(prg)
{
}

///
/// \param[in] example current training case
/// \return            measurement of the error of the model/program on the
///                    current training case. The value returned is in the
///                    `[0;200]` range
///
template<Individual P>
double rmae_error_functor<P>::operator()(const example &example) const
{
  constexpr double ERR_SUP(200.0);
  double err(ERR_SUP);

  if (const auto foreseen(oracle_(example.input)); has_value(foreseen))
  {
    const auto approx(lexical_cast<D_DOUBLE>(foreseen));
    const auto target(label_as<D_DOUBLE>(example));

    const auto delta(std::fabs(target - approx));

    // Check if the numbers are really close. Needed when comparing numbers
    // near zero.
    if (delta <= 10.0 * std::numeric_limits<D_DOUBLE>::min())
      err = 0.0;
    else
      err = ERR_SUP * delta / (std::fabs(approx) + std::fabs(target));
    // Some alternatives for the error:
    // * delta / std::max(approx, target)
    // * delta / std::fabs(target)
    //
    // The chosen formula seems numerically more stable and gives a result
    // in a limited range of values.
  }

  return err;
}

///
/// Sets up the environment for error measurement.
///
/// \param[in] prg the program to be measured
///
template<Individual P>
mse_error_functor<P>::mse_error_functor(const P &prg) : oracle_(prg)
{
}

///
/// \param[in] example current training case
/// \return            a measurement of the error of the model/program on the
///                    the current training case. The value returned is in the
///                    `[0;+inf[` range
///
template<Individual P>
double mse_error_functor<P>::operator()(const example &example) const
{
  if (const auto foreseen(oracle_(example.input)); has_value(foreseen))
  {
    const double err(std::get<D_DOUBLE>(foreseen)
                     - label_as<D_DOUBLE>(example));
    return err * err;
  }

  return std::numeric_limits<double>::max() / 1000.0;
}

///
/// Sets up the environment for error measurement.
///
/// \param[in] prg the program to be measured
///
template<Individual P>
count_error_functor<P>::count_error_functor(const P &prg) : oracle_(prg)
{
}

///
/// \param[in] example current training case
/// \return            a measurement of the error of the model/program on the
///                    current training case. The value returned is in the
///                    `[0;1]` range
///
template<Individual P>
double count_error_functor<P>::operator()(const example &example) const
{
  const auto foreseen(oracle_(example.input));

  if (!has_value(foreseen))
    return 1.0;

  if (!issmall(std::get<D_DOUBLE>(foreseen) - label_as<D_DOUBLE>(example)))
    return 1.0;

  return 0.0;
}

///
/// Constructs the evaluator.
///
/// \param[in] d training dataset
///
template<Individual P>
gaussian_evaluator<P>::gaussian_evaluator(multi_dataset<dataframe> &d)
  : evaluator(d)
{
}

///
/// Computes the classification fitness.
///
/// \param[in] prg program to evaluate
/// \return        fitness value (greater is better, max is `0`)
///
template<Individual P>
double gaussian_evaluator<P>::operator()(const P &prg) const
{
  const auto &dat(this->data());
  Expects(dat.classes() >= 2);
  basic_gaussian_oracle<P, false, false> oracle(prg, dat);

  double d(0.0);
  for (auto &example : dat)
    if (const auto res(oracle.tag(example.input)); res.label == label(example))
    {
      const auto scale(static_cast<double>(dat.classes() - 1));
      // Note:
      // * `(1.0 - res.sureness)` is the sum of the errors;
      // * `(res.sureness - 1.0)` is the opposite (standardized fitness);
      // * `(res.sureness - 1.0) / scale` is the opposite of the average error.
      d += (res.sureness - 1.0) / scale;
    }
    else
    {
      // Note:
      // * the maximum single class error is `1.0`;
      // * the maximum average class error is `1.0 / dat.classes()`;
      // So `-1.0` is like to say that we have a complete failure.
      d -= 1.0;
    }

  return d;
}

///
/// Builds a Gaussian oracle for the given program.
///
/// \param[in] prg program to transform into an oracle
/// \return        oracle object associated with `prg`
///
template<Individual P>
auto gaussian_evaluator<P>::oracle(const P &prg) const
{
  return gaussian_oracle<P>(prg, this->data());
}

///
/// Constructs the evaluator.
///
/// \param[in] d training dataset
///
template<Individual P>
binary_evaluator<P>::binary_evaluator(multi_dataset<dataframe> &d)
  : evaluator(d)
{
}

///
/// Computes the binary classification fitness.
///
/// \param[in] prg program to evaluate
/// \return        fitness value (greater is better, max is `0`)
///
/// \pre the dataset must contain exactly two classes
///
template<Individual P>
double binary_evaluator<P>::operator()(const P &prg) const
{
  const auto &dat(this->data());
  Expects(dat.classes() == 2);
  basic_binary_oracle<P, false, false> oracle(prg, dat);

  double err(0.0);

  for (auto &example : dat)
    if (const auto res(oracle.tag(example.input)); res.label != label(example))
      err += 1.0 + res.sureness;

  return -err;
}

///
/// Builds a binary classification oracle.
///
/// \param[in] prg program to transform into an oracle
/// \return        oracle object associated with `prg`
///
template<Individual P>
auto binary_evaluator<P>::oracle(const P &prg) const
{
  return binary_oracle<P>(prg, this->data());
}

#endif  // include guard
