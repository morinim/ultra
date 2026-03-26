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
#define      ULTRA_SRC_EVALUATOR_H

#include "kernel/evaluator.h"
#include "kernel/gp/src/dataframe.h"
#include "kernel/gp/src/multi_dataset.h"
#include "kernel/gp/src/oracle.h"

namespace ultra::src
{

#include "kernel/gp/src/evaluator_internal.tcc"

///
/// Concept modelling a dataset that provides examples for evaluation.
///
/// Supported forms:
/// - `multi_dataset<T>`: examples are taken from the currently selected
///   dataset via `selected()`
/// - plain datasets (`DataSet<T>`): examples are taken directly from the
///   dataset
///
/// This concept ensures that evaluators can uniformly access examples
/// regardless of whether a single dataset or a multi-dataset container is
/// used.
///
template<class D> concept EvaluationDataset =
  (is_multi_dataset_v<D> && DataSet<decltype(std::declval<D>().selected())>)
  || DataSet<D>;

///
/// Concept modelling a functor that evaluates a single dataset example.
///
/// An `ExampleEvaluator` is a callable object that takes one example from a
/// dataset and returns a value representing its contribution to the overall
/// evaluation.
///
/// The returned value can represent:
/// - an error (lower is better);
/// - a score or reward (higher is better).
///
/// This concept is intentionally agnostic with respect to the interpretation
/// of the returned value. The aggregation semantics are defined by the
/// evaluator using the functor (e.g. sum vs average, error vs score).
///
/// \note
/// The functor is typically constructed from an individual/program and may
/// internally use an oracle or interpreter to compute predictions.
///
template<class F, class D, class P> concept ExampleEvaluator =
  EvaluationDataset<D> && Individual<P>
  && std::invocable<F, internal::dataset_example_t<D>>;

///
/// Base class for dataset-aware evaluators.
///
/// \tparam D dataset type
///
/// This class stores a reference to a dataset and provides a uniform access
/// interface for both plain datasets and `multi_dataset` specialisations.
/// It is intended to be used as a protected base class for concrete
/// evaluators.
///
template<EvaluationDataset D>
class evaluator
{
protected:
  explicit evaluator(D &) noexcept;

  [[nodiscard]] const auto &data() const noexcept;

private:
  D *dat_ {nullptr};
};

enum class aggregation_mode { sum, average };
enum class evaluation_mode  { error, score };

template<Individual P, class F, class D, aggregation_mode A, evaluation_mode M>
requires ExampleEvaluator<F, D, P>
class aggregate_evaluator : public evaluator<D>
{
public:
  explicit aggregate_evaluator(D &) noexcept;

  [[nodiscard]] double operator()(const P &) const;
  [[nodiscard]] double fast(const P &) const;

  [[nodiscard]] auto oracle(const P &) const
    requires internal::has_regression_oracle_v<F, P>;

private:
  static constexpr std::ptrdiff_t fast_min_examples = 100;
  static constexpr std::ptrdiff_t fast_step = 5;

  [[nodiscard]] double eval_impl(const P &, std::ptrdiff_t) const;
};

///
/// Evaluator minimising the error over a dataset.
///
/// \tparam P individual type
/// \tparam F error functor type
/// \tparam D dataset type (defaults to `multi_dataset<dataframe>`)
///
/// This class drives the evolution towards the minimum sum of some sort of
/// error.
///
/// Fitness values are normalised so that:
/// - higher is better;
/// - values are comparable between full and fast evaluation modes.
///
/// \see
/// mse_evaluator, mae_evaluator, rmae_evaluator, count_error_evaluator
///
template<Individual P, class F, class D = multi_dataset<dataframe>>
using avg_error_evaluator = aggregate_evaluator<P, F, D,
                                                aggregation_mode::average,
                                                evaluation_mode::error>;

template<Individual P, class F, class D = multi_dataset<dataframe>>
using sum_error_evaluator = aggregate_evaluator<P, F, D,
                                                aggregation_mode::sum,
                                                evaluation_mode::error>;

template<Individual P, class F, class D = multi_dataset<dataframe>>
using avg_score_evaluator = aggregate_evaluator<P, F, D,
                                                aggregation_mode::average,
                                                evaluation_mode::score>;

template<Individual P, class F, class D = multi_dataset<dataframe>>
using sum_score_evaluator = aggregate_evaluator<P, F, D,
                                                aggregation_mode::sum,
                                                evaluation_mode::score>;

///
/// Mean absolute error functor for evaluating a program on a single example.
///
/// \tparam P individual type
///
/// The functor owns an oracle initialised from the program and computes a
/// scalar error value for each training example.
///
/// Computes (\f$\frac{1}{n} \sum_{i=1}^n |target_i - actual_i|\f$).
///
/// Illegal values are assigned a large penalty.
///
/// \see mae_evaluator
///
template<Individual P>
class mae_error_functor
{
public:
  mae_error_functor(const P &);

  [[nodiscard]] double operator()(const example &) const;

private:
  basic_reg_oracle<P, false> oracle_;
};

///
/// Evaluator based on the mean absolute error.
///
/// \see mae_error_functor
///
template<Individual P>
using mae_evaluator = avg_error_evaluator<P, mae_error_functor<P>>;

///
/// Relative mean absolute error functor for evaluating a program on a single
/// example.
///
/// \tparam P individual type
///
/// The functor owns an oracle initialised from the program and computes
/// a scalar error value for each training example.
///
/// Computes a scaled relative difference between target and predicted values:
///
/// \f[
/// \frac{1}{n} \sum_{i=1}^n \frac{|target_i - actual_i|}{\frac{|target_i| + |actual_i|}{2}}
/// \f]
///
/// This is similar to mae_error_functor but here we sum the *relative* errors.
/// The idea is that the absolute difference of `1` between `6` and `5` is more
/// significant than the same absolute difference between `1000001` and
/// `1000000`.
/// The mathematically precise way to express this notion is to calculate the
/// relative difference.
///
/// \see
/// - rmae_evaluator
/// - https://github.com/morinim/documents/blob/master/math_notes/relative_difference.md
///
template<Individual P>
class rmae_error_functor
{
public:
  explicit rmae_error_functor(const P &);

  [[nodiscard]] double operator()(const example &) const;

private:
  basic_reg_oracle<P, false> oracle_;
};

///
/// Evaluator based on the mean of relative differences.
///
/// \see rmae_error_functor
///
template<Individual P>
using rmae_evaluator = avg_error_evaluator<P, rmae_error_functor<P>>;

///
/// Mean squared error functor for evaluating a program on a single example.
///
/// \tparam P individual type
///
/// The functor owns an oracle initialised from the program and computes a
/// scalar error value for each training example.
///
/// Computes: (\f$\frac{1}{n} \sum_{i=1}^n (target_i - actual_i)^2\f$).
///
/// There is also a penalty for illegal values (it's a function of the number
/// of illegal values).
///
/// \note
/// Real data always have noise (sampling/measurement errors) and this noise
/// tends to follow a Gaussian distribution. It can be shown that when we have
/// a set of data with errors drawn from such a distribution, we're most likely
/// to find the 'correct' underlying model by minimizing the sum of squared
/// errors.
///
/// \remark
/// When the dataset contains outliers, the mse_error_functor will heavily
/// weight each of them (this is the result of squaring the outliers).
/// mae_error_functor is less sensitive to the presence of outliers (a
/// desirable property in many applications).
///
/// \see mse_evaluator
///
template<Individual P>
class mse_error_functor
{
public:
  explicit mse_error_functor(const P &);

  [[nodiscard]] double operator()(const example &) const;

private:
  basic_reg_oracle<P, false> oracle_;
};

///
/// Evaluator based on the mean squared error.
///
/// \see mse_error_functor
///
template<Individual P>
using mse_evaluator = avg_error_evaluator<P, mse_error_functor<P>>;

///
/// Classification error functor based on exact matches.
///
/// \tparam P individual type
///
/// The functor owns an oracle initialised from the program and computes a
/// scalar error value for each training example.
///
/// This functor returns `0` for a correct prediction and `1` otherwise. When
/// used with `sum_error_evaluator`, fitness corresponds to the negative number
/// of classification errors.
///
/// \see
/// count_error_evaluator
///
template<Individual P>
class count_error_functor
{
public:
  explicit count_error_functor(const P &);

  [[nodiscard]] double operator()(const example &) const;

private:
  basic_reg_oracle<P, false> oracle_;
};

///
/// Evaluator based on the number of matches.
///
/// \see
/// count_error_functor
///
template<Individual P>
using count_error_evaluator = sum_error_evaluator<P, count_error_functor<P>>;

///
/// Evaluator for multi-class classification using Gaussian models.
///
/// \tparam P individual type
///
/// Instead of using predefined multiple thresholds to form different regions
/// in the program output space for different classes, this approach uses
/// probabilities of different classes, derived from Gaussian distributions,
/// to construct the fitness function for classification.
///
/// \see
/// https://github.com/morinim/ultra/wiki/bibliography#13
///
template<Individual P>
class gaussian_evaluator : public evaluator<multi_dataset<dataframe>>
{
public:
  explicit gaussian_evaluator(multi_dataset<dataframe> &);

  [[nodiscard]] double operator()(const P &) const;
  [[nodiscard]] auto oracle(const P &) const;
};

///
/// Evaluator for binary classification problems.
///
/// \tparam P individual type
///
/// Incorrect predictions are penalised proportionally to the model's
/// confidence.
///
template<Individual P>
class binary_evaluator : public evaluator<multi_dataset<dataframe>>
{
public:
  explicit binary_evaluator(multi_dataset<dataframe> &);

  [[nodiscard]] double operator()(const P &) const;
  [[nodiscard]] auto oracle(const P &) const;
};

namespace internal
{
template<Individual P>
struct has_regression_oracle<mae_error_functor<P>, P> : std::true_type {};

template<Individual P>
struct has_regression_oracle<rmae_error_functor<P>, P> : std::true_type {};

template<Individual P>
struct has_regression_oracle<mse_error_functor<P>, P> : std::true_type {};
}  // namespace internal

#include "kernel/gp/src/evaluator.tcc"

}  // namespace ultra::src

#endif  // include guard
