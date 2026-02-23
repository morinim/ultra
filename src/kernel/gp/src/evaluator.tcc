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
template<ErrorDataset D>
evaluator<D>::evaluator(D &d) noexcept : dat_(&d)
{
}

///
/// Returns the dataset currently used for evaluation.
///
/// \return pointer to the active dataset
///
/// For `multi_dataset `, this returns the currently selected dataset.
/// Otherwise, it returns the dataset itself.
///
template<ErrorDataset D>
auto *evaluator<D>::data() const noexcept
{
  if constexpr (derived_from_template<D, multi_dataset>)
    return &dat_->selected();
  else
    return dat_;
}


///
/// Constructs the evaluator.
///
/// \param[in] d training dataset
///
template<Individual P, class F, class D>
requires ErrorFunction<F, D>
sum_of_errors_evaluator<P, F, D>::sum_of_errors_evaluator(D &d)
  : evaluator<D>(d)
{
}

///
/// Computes the average error using a configurable sampling step.
///
/// \param[in] prg  program to evaluate
/// \param[in] step sampling steo (one example every `step`)
/// \return         fitness value
///
template<Individual P, class F, class D>
requires ErrorFunction<F, D>
auto sum_of_errors_evaluator<P, F, D>::sum_of_errors_impl(
  const P &prg, std::ptrdiff_t step) const
{
  const auto &dat(*this->data());

  Expects(std::ranges::distance(dat) >= step);
  Expects(step > 0);

  const F err_fctr(prg);

  auto it(std::begin(dat));
  auto average_error(err_fctr(*it));
  std::advance(it, step);

  double n(1.0);

  const auto end(std::end(dat));
  while (it != end)
  {
    average_error += (err_fctr(*it) - average_error) / ++n;

    std::ranges::advance(it, step, end);
  }

  // Note that we take the average error: this way fast() and operator()
  // outputs can be compared.
  return -average_error;
}

///
/// Computes the fitness using all training examples.
///
/// \param[in] prg program to evaluate
/// \return        fitness value (greater is better, max is `0`)
///
template<Individual P, class F, class D>
requires ErrorFunction<F, D>
auto sum_of_errors_evaluator<P, F, D>::operator()(const P &prg) const
{
  return sum_of_errors_impl(prg, 1);
}

///
/// Computes a faster approximation of the fitness.
///
/// \param[in] prg program to evaluate
/// \return        approximate fitness value
///
/// \pre The dataset must contain at least 100 examples.
///
/// This function is similar to operator()() but will skip `4` out of `5`
/// training instances, so it's faster.
///
template<Individual P, class F, class D>
requires ErrorFunction<F, D>
auto sum_of_errors_evaluator<P, F, D>::fast(const P &prg) const
{
  Expects(std::ranges::distance(*this->data()) >= 100);
  return sum_of_errors_impl(prg, 5);
}

///
/// Builds an oracle associated with a program.
///
/// \param[in] prg Program to transform into an oracle
/// \return        oracle instance (`nullptr` in case of errors)
///
template<Individual P, class F, class D>
requires ErrorFunction<F, D>
std::unique_ptr<basic_oracle>
sum_of_errors_evaluator<P, F, D>::oracle(const P &prg) const
{
  return std::make_unique<reg_oracle<P>>(prg);
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
///                    `[0;+inf[` range
///
template<Individual P>
double count_error_functor<P>::operator()(const example &example) const
{
  const auto foreseen(oracle_(example.input));

  const bool err(!has_value(foreseen)
                 || !issmall(std::get<D_DOUBLE>(foreseen)
                             - label_as<D_DOUBLE>(example)));

  return err ? 1.0 : 0.0;
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
  Expects(this->data()->classes() >= 2);
  const auto &dat(*this->data());
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
/// \return        oracle instance (`nullptr` in case of errors).
///
template<Individual P>
std::unique_ptr<basic_oracle> gaussian_evaluator<P>::oracle(const P &prg) const
{
  return std::make_unique<gaussian_oracle<P>>(prg, *this->data());
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
  Expects(this->data()->classes() == 2);
  const auto &dat(*this->data());
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
/// \return        oracle instance (`nullptr` in case of errors)
///
template<Individual P>
std::unique_ptr<basic_oracle> binary_evaluator<P>::oracle(const P &prg) const
{
  return std::make_unique<binary_oracle<P>>(prg, *this->data());
}

#endif  // include guard
