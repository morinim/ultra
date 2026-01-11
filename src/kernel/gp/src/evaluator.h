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

///
/// Concept checking whether a type derives from a given class template.
///
/// This concept evaluates to true if `Derived` is publicly convertible to
/// some instantiation of the class template `Base<Ts...>`.
///
/// It is mainly used to detect whether a dataset type is a specialisation
/// of `multi_dataset`.
///
template<class Derived, template<class...> class Base>
concept derived_from_template = requires(Derived &d)
{
  []<class... Ts>(Base<Ts...> &) {}(d);
};

///
/// Concept modelling an error-measuring functor.
///
/// An error function computes the error committed by a program on a single
/// training example.
///
/// Supported dataset types:
/// - `multi_dataset<T>`: the functor must be invocable with examples from the
///   currently selected dataset;
/// - plain datasets (`DataSet<T>`): the functor must be invocable with
///   examples obtained directly from the dataset.
///
template<class F, class D> concept ErrorFunction =
  (derived_from_template<D, multi_dataset>
   && DataSet<decltype(std::declval<D>().selected())>
   && std::invocable<F, decltype(*std::declval<D>().selected().begin())>)
  || (DataSet<D> && std::invocable<F, decltype(*std::declval<D>().begin())>);

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
template<class D>
class evaluator
{
protected:
  explicit evaluator(D &) noexcept;

  [[nodiscard]] auto *data() const noexcept;

private:
  D *dat_ {nullptr};
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
/// mse_evaluator, mae_evaluator, rmae_evaluator, count_evaluator
///
template<Individual P, class F, class D = multi_dataset<dataframe>>
requires ErrorFunction<F, D>
class sum_of_errors_evaluator : public evaluator<D>
{
public:
  explicit sum_of_errors_evaluator(D &);

  [[nodiscard]] auto operator()(const P &) const;
  [[nodiscard]] auto fast(const P &) const;

  [[nodiscard]] std::unique_ptr<basic_oracle> oracle(const P &) const;

private:
  [[nodiscard]] auto sum_of_errors_impl(const P &, std::ptrdiff_t) const;
};

///
/// Mean absolute error functor for evaluating a program on a single examle.
///
/// \tparam P individual type
///
/// The functor owns an oracle initialised from the program and computes a
/// a scalar error value for each training example.
///
/// Computes (\f$\frac{1}{n} \sum_{i=1}^n |target_i - actual_i|\f$).
///
/// Illegal values are assigned a large penalty.
///
/// \see
/// mae_evaluator
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
/// \see
/// mae_error_functor
///
template<Individual P>
class mae_evaluator : public sum_of_errors_evaluator<P, mae_error_functor<P>>
{
public:
  using mae_evaluator::sum_of_errors_evaluator::sum_of_errors_evaluator;
};

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
class rmae_evaluator : public sum_of_errors_evaluator<P, rmae_error_functor<P>>
{
public:
  using rmae_evaluator::sum_of_errors_evaluator::sum_of_errors_evaluator;
};

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
/// \see
/// mse_error_functor
///
template<Individual P>
class mse_evaluator : public sum_of_errors_evaluator<P, mse_error_functor<P>>
{
public:
  using mse_evaluator::sum_of_errors_evaluator::sum_of_errors_evaluator;
};

///
/// Classification error functor based on exact matches.
///
/// \tparam P individual type
///
/// The functor owns an oracle initialised from the program and computes a
/// scalar error value for each training example.
///
/// This functor will drive the evolution towards the maximum sum of matches
/// (\f$\sum_{i=1}^n target_i == actual_i\f$). Incorrect answers receive the
/// same penalty.
///
/// \see
/// count_evaluator
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
class count_evaluator : public sum_of_errors_evaluator<P,
                                                       count_error_functor<P>>
{
public:
  using count_evaluator::sum_of_errors_evaluator::sum_of_errors_evaluator;
};

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
  [[nodiscard]] std::unique_ptr<basic_oracle> oracle(const P &) const;
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
  [[nodiscard]] std::unique_ptr<basic_oracle> oracle(const P &) const;
};

#include "kernel/gp/src/evaluator.tcc"

}  // namespace ultra::src

#endif  // include guard
