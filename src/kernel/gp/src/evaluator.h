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

/// An error function/functor returns a measurement of the error that a program
/// commits in a given training case.
template<class ERRF, class DAT> concept ErrorFunction
  = DataSet<DAT>
    && std::invocable<ERRF, decltype(*std::declval<DAT>().begin())>;

///
/// An evaluator specialized for symbolic regression / classification problems.
///
/// This specialization of the evaluator class is "dataset-aware". It's useful
/// to group common factors of more specialized symbolic regression or
/// classification classes.
///
template<DataSet DAT>
class evaluator
{
protected:
  explicit evaluator(multi_dataset<DAT> &) noexcept;

  multi_dataset<DAT> *dat_ {nullptr};
};

///
/// An evaluator to minimize the sum of some sort of error.
///
/// This class drive the evolution towards the minimum sum of some sort of
/// error.
///
/// \see
/// mse_evaluator, mae_evaluator, rmae_evaluator, count_evaluator
///
template<Individual P, template<class> class ERRF, class DAT = dataframe>
requires ErrorFunction<ERRF<P>, DAT>
class sum_of_errors_evaluator : public evaluator<DAT>
{
public:
  explicit sum_of_errors_evaluator(multi_dataset<DAT> &);

  [[nodiscard]] auto operator()(const P &) const;
  [[nodiscard]] auto fast(const P &) const;

  [[nodiscard]] std::unique_ptr<basic_oracle> oracle(const P &) const;

private:
  [[nodiscard]] auto sum_of_errors_impl(const P &,
                                        typename DAT::difference_type) const;
};

///
/// Mean Absolute Error.
///
/// This functor will drive the evolution towards the minimum sum of
/// absolute errors (\f$\frac{1}{n} \sum_{i=1}^n |target_i - actual_i|\f$).
///
/// There is also a penalty for illegal values (it's a function of the number
/// of illegal values).
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
class mae_evaluator : public sum_of_errors_evaluator<P, mae_error_functor>
{
public:
  using mae_evaluator::sum_of_errors_evaluator::sum_of_errors_evaluator;
};

///
/// Mean of Relative Differences.
///
/// This functor will drive the evolution towards the minimum sum of
/// relative differences between target values and actual ones:
///
/// \f[\frac{1}{n} \sum_{i=1}^n \frac{|target_i - actual_i|}{\frac{|target_i| + |actual_i|}{2}}\f]
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
class rmae_evaluator : public sum_of_errors_evaluator<P, rmae_error_functor>
{
public:
  using rmae_evaluator::sum_of_errors_evaluator::sum_of_errors_evaluator;
};

///
/// Mean Squared Error.
///
/// This fumctpr will drive the evolution towards the minimum sum of
/// squared errors (\f$\frac{1}{n} \sum_{i=1}^n (target_i - actual_i)^2\f$).
///
/// There is also a penalty for illegal values (it's a function of the number
/// of illegal values).
///
/// \note
/// Real data always have noise (sampling/measurement errors), and this noise
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
class mse_evaluator : public sum_of_errors_evaluator<P, mse_error_functor>
{
public:
  using mse_evaluator::sum_of_errors_evaluator::sum_of_errors_evaluator;
};

///
/// Number of matches functor.
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
class count_evaluator : public sum_of_errors_evaluator<P, count_error_functor>
{
public:
  using count_evaluator::sum_of_errors_evaluator::sum_of_errors_evaluator;
};

///
/// Gaussian distribution for multiclass object classification.
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
class gaussian_evaluator : public evaluator<dataframe>
{
public:
  explicit gaussian_evaluator(multi_dataset<dataframe> &);

  [[nodiscard]] double operator()(const P &) const;
  [[nodiscard]] std::unique_ptr<basic_oracle> oracle(const P &) const;
};

///
/// Single class evaluator for classification problems.
///
template<Individual P>
class binary_evaluator : public evaluator<dataframe>
{
public:
  explicit binary_evaluator(multi_dataset<dataframe> &);

  [[nodiscard]] double operator()(const P &) const;
  [[nodiscard]] std::unique_ptr<basic_oracle> oracle(const P &) const;
};

#include "kernel/gp/src/evaluator.tcc"

}  // namespace ultra::src

#endif  // include guard
