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

#if !defined(ULTRA_ORACLE_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_ORACLE_TCC)
#define      ULTRA_ORACLE_TCC

// ***********************************************************************
// *  basic_reg_oracle                                                   *
// ***********************************************************************

template<Individual P, bool S>
const std::string basic_reg_oracle<P, S>::SERIALIZE_ID(
  gp::Team<P> ? "TEAM_REG_ORACLE" : "REG_ORACLE");

///
/// \param[in] prg the program (individual/team) to be lambdified
///
template<Individual P, bool S>
basic_reg_oracle<P, S>::basic_reg_oracle(const P &prg)
  : internal::reg_oracle_storage<P, S>(prg)
{
  Expects(!prg.empty());

  Ensures(is_valid());
}

///
/// \param[in] in input stream
/// \param[in] ss active symbol set
///
template<Individual P, bool S>
basic_reg_oracle<P, S>::basic_reg_oracle(std::istream &in,
                                         const symbol_set &ss)
  : internal::reg_oracle_storage<P, S>(in, ss)
{
  static_assert(S, "reg_oracle requires storage for de-serialization");

  Ensures(is_valid());
}

///
/// \param[in] e input example for the oracle
/// \return      the output value associated with `e`
///
template<Individual P, bool S>
value_t basic_reg_oracle<P, S>::operator()(const std::vector<value_t> &e) const
{
  return this->run(e);
}

///
/// \return the function doesn't return
///
/// \warning
/// This function is useful only for classification tasks.
///
template<Individual P, bool S>
classification_result basic_reg_oracle<P, S>::tag(
  const std::vector<value_t> &) const
{
  throw std::logic_error("tag() called on regression oracle");
}

///
/// \param[in] a value produced by basic_oracle::operator()
/// \return      the string version of `a`
///
template<Individual P, bool S>
std::string basic_reg_oracle<P, S>::name(const value_t &a) const
{
  return lexical_cast<std::string>(a);
}

///
/// \return `true` if the object passes the internal consistency check
///
template<Individual P, bool S>
bool basic_reg_oracle<P, S>::is_valid() const
{
  return internal::reg_oracle_storage<P, S>::is_valid();
}

///
/// Saves the object on persistent storage.
///
/// \param[out] out output stream
/// \return         `true` if the oracle was saved correctly
///
template<Individual P, bool S>
bool basic_reg_oracle<P, S>::save(std::ostream &out) const
{
  return internal::reg_oracle_storage<P, S>::save(out);
}

// ***********************************************************************
// *  reg_oracle_adaptor                                                 *
// ***********************************************************************

///
/// \param[in] oracle predictor instance to adapt
///
template<RegressionPredictor O>
reg_oracle_adaptor<O>::reg_oracle_adaptor(O oracle)
  : oracle_(std::move(oracle))
{
}

///
/// Returns the predicted value for the input example.
///
/// \param[in] x input example
/// \return      predicted value
///
/// This function forwards the call to the underlying regression predictor.
///
template<RegressionPredictor O>
value_t reg_oracle_adaptor<O>::operator()(const std::vector<value_t> &x) const
{
  return oracle_(x);
}

///
/// Returns a string representation of the predicted value.
///
/// \param[in] v numeric prediction
/// \return      string representation of `v`
///
template<RegressionPredictor O>
std::string reg_oracle_adaptor<O>::name(const value_t &v) const
{
  return lexical_cast<std::string>(v);
}

///
/// Classification is not supported for regression predictors.
///
/// \throws std::logic_error always
///
template<RegressionPredictor O>
classification_result reg_oracle_adaptor<O>::tag(
  const std::vector<value_t> &) const
{
  throw std::logic_error("tag() called on regression oracle");
}

///
/// \return identifier used for serialization
///
/// \note
/// The adaptor is not serializable. This identifier is provided only to
/// satisfy the `basic_oracle` interface.
///
template<RegressionPredictor O>
std::string reg_oracle_adaptor<O>::serialize_id() const
{
  return "REG_ORACLE_ADAPTOR";
}

///
/// Serialization is not supported.
///
/// \throws std::logic_error always
///
template<RegressionPredictor O>
bool reg_oracle_adaptor<O>::save(std::ostream &) const
{
  throw std::logic_error("reg_oracle_adaptor is not serializable");
}

// ***********************************************************************
// *  basic_gaussian_oracle                                              *
// ***********************************************************************

template<Individual I, bool S>
const std::string basic_gaussian_oracle<I, S>::SERIALIZE_ID("GAUSSIAN_ORACLE");

///
/// \param[in] ind   program "to be transformed" into an oracle
/// \param[in] d     the non-empty training set
/// \param[in] names optional class names
///
template<Individual I, bool S>
basic_gaussian_oracle<I, S>::basic_gaussian_oracle(
  const I &ind, const dataframe &d, std::vector<std::string> names)
  : basic_gaussian_oracle(ind, d, nullptr, std::move(names))
{
}

///
/// \param[in]  ind         program "to be transformed" into an oracle
/// \param[in]  d           the non-empty training set
/// \param[out] predictions container populated with predictions on `d`
/// \param[in]  names       optional class names
///
template<Individual I, bool S>
basic_gaussian_oracle<I, S>::basic_gaussian_oracle(
  const I &ind, const dataframe &d,
  std::vector<classification_result> &predictions,
  std::vector<std::string> names)
  : basic_gaussian_oracle(ind, d, &predictions, std::move(names))
{
}

///
/// \param[in]  ind         program "to be transformed" into an oracle
/// \param[in]  d           the non-empty training set
/// \param[out] predictions pointer to a container with predictions on `d`
/// \param[in]  names       class names
///
template<Individual I, bool S>
basic_gaussian_oracle<I, S>::basic_gaussian_oracle(
  const I &ind, const dataframe &d,
  std::vector<classification_result> *predictions,
  std::vector<std::string> names)
  : basic_class_oracle(std::move(names)), oracle_(ind), gauss_dist_(d.classes())
{
  Expects(!ind.empty());
  Expects(!d.empty());
  Expects(d.classes() > 1);
  Expects(names_size() == 0 || names_size() == d.classes());

  fill_vector(d, predictions);

  Ensures(is_valid());
}

///
/// Constructs the object reading data from an input stream.
///
/// \param[in] in input stream
/// \param[in] ss active symbol set
///
template<Individual I, bool S>
basic_gaussian_oracle<I, S>::basic_gaussian_oracle(std::istream &in,
                                                   const symbol_set &ss)
  : basic_class_oracle(), oracle_(in, ss)
{
  static_assert(
    S, "gaussian_oracle requires storage space for de-serialization");

  std::size_t n;
  if (!(in >> n))
    throw exception::data_format("Cannot read gaussian_oracle size component");

  for (std::size_t i(0); i < n; ++i)
  {
    distribution<double> d;
    if (!d.load(in))
      throw exception::data_format(
        "Cannot read gaussian_oracle distribution component");

    gauss_dist_.push_back(d);
  }

  if (!load_names(in)
      || (names_size() != 0 && names_size() != gauss_dist_.size()))
    throw exception::data_format(
      "Cannot read gaussian_oracle class_names component");

  missing_fallback_ = calculate_missing_fallback();

  Ensures(is_valid());
}

///
/// Sets up the per-class Gaussian distributions used for classification.
///
/// \param[in]  d           the training set
/// \param[out] predictions optional container populated with predictions on `d`
///
/// Each class is modelled by a Gaussian distribution whose parameters are
/// estimated from the outputs produced by the individual on the training
/// examples of that class.
///
/// ### Handling of missing outputs
///
/// The individual may fail to produce a valid numerical output for some
/// training examples (e.g. due to domain errors or undefined operations).
/// Such cases are treated explicitly as *missing information* rather than
/// as numerical values.
///
/// Construction proceeds in two phases:
///
/// 1. **Observation phase**
///    - Valid outputs are collected normally;
///    - missing outputs are counted per class but do not immediately affect
///      the distribution.
///
/// 2. **Uncertainty injection phase**
///    - Each missing output contributes one unit of uncertainty;
///    - uncertainty is represented by injecting a symmetric pair of synthetic
///      samples at $\pm 2\sigma$ around the class mean, preserving the mean
///      exactly while inflating the variance;
///    - two missing outputs correspond to one symmetric pair, ensuring that
///      the effective sample count remains consistent with the number of
///      training examples.
///
/// Non-finite outputs are treated as missing. Finite outputs are clamped to
/// the same bounds during fitting and prediction to keep statistics bounded.
/// Classes with no valid outputs remain empty and receive zero likelihood.
///
/// This approach avoids arbitrary value substitution, prevents spurious
/// confidence and yields stable confidence estimates during classification.
///
template<Individual I, bool S>
void basic_gaussian_oracle<I, S>::fill_vector(
  const dataframe &d, std::vector<classification_result> *predictions)
{
  Expects(d.classes() > 1);

  std::vector<std::optional<double>> outputs;
  if (predictions)
  {
    outputs.reserve(d.size());
    predictions->clear();
    predictions->reserve(d.size());
  }

  std::vector<unsigned> unknown_in_class(d.classes(), 0);

  for (const auto &ex : d)
  {
    const auto cl(label(ex));
    const auto out(normalise_output(oracle_(ex.input)));

    if (predictions)
      outputs.push_back(out);

    if (out)
      gauss_dist_[cl].add(*out);
    else
      ++unknown_in_class[cl];
  }

  constexpr double min_variance(1e-6);
  constexpr unsigned missing_per_inflation(2);

  for (std::size_t cl(0); cl < d.classes(); ++cl)
  {
    auto &g(gauss_dist_[cl]);

    if (const unsigned k(unknown_in_class[cl] / missing_per_inflation);
        g.size() > 0)
    {
      const double mean(g.mean());
      const double var(std::max(g.variance(), min_variance));
      const double delta(2.0 * std::sqrt(var));

      for (std::size_t i(0); i < k; ++i)
      {
        g.add(mean + delta);
        g.add(mean - delta);
      }
    }

    Ensures(g.empty() || std::isfinite(g.mean()));
    Ensures(g.empty() || (g.variance() >= 0.0 && std::isfinite(g.variance())));
  }

  missing_fallback_ = calculate_missing_fallback();

  if (predictions)
    for (const auto &out : outputs)
      predictions->push_back(predict(out));
}

///
/// Classifies an example using Gaussian class models and returns a
/// confidence-weighted prediction.
///
/// \param[in] ex input example to classify
/// \return       a `classification_result` containing the predicted class
///               label and an associated confidence value (the value is in the
///               `[0,1]` interval and the sum of all the confidence levels of
///               each class equals `1`)
///
/// This method assigns a class label to an input example by evaluating the
/// program oracle and comparing the result against a set of per-class Gaussian
/// distributions previously constructed.
///
/// ### Inference modes
///
/// The behaviour depends on the availability of a valid oracle output:
///
/// - **Normal inference (oracle output available)**
///   The inferred value is compared to each class distribution using a
///   Gaussian-like likelihood score:
///
///   $$
///   p_i = \exp\left(-\frac{(x - \mu_i)^2}{\sigma_i^2}\right)
///   $$
///
///   where $\mu_i$ and $\sigma_i^2$ are the mean and variance of the
///   distribution associated with class $i$.
///   Degenerate or nearly-degenerate distributions (very small variance) are
///   treated as point masses centred at the mean.
///
/// - **Missing inference (oracle output unavailable)**
///   If the oracle cannot produce a value for the input, the classifier falls
///   back to **class priors**, selecting the most frequent class based on the
///   effective sample counts stored in the Gaussian models.
///
///   These counts include both real observations and synthetic samples added
///   during variance inflation and therefore represent *effective* class
///   support rather than raw dataset frequencies.
///
/// ### Confidence semantics
///
/// The returned confidence is a normalised measure in the range $[0, 1]$:
///
/// - in normal inference, it is defined as:
///
///   $$
///   \text{confidence} = \frac{p_{\text{best}}}{\sum_i p_i}
///   $$
///
///   and represents how dominant the selected class likelihood is relative to
///   the others;
///
/// - in missing inference, it is defined as:
///
///   $$
///   \text{confidence} = \frac{n_{\text{best}}}{\sum_i n_i}
///   $$
///
///   where $n_i$ is the effective sample count of class $i$.
///
/// A confidence close to `1` indicates a clear decision, while lower values
/// signal ambiguity or weak evidence.
///
/// ### Special cases
///
/// If only one class is present, that class is returned with confidence `1`.
///
template<Individual I, bool S>
classification_result basic_gaussian_oracle<I, S>::tag(
  const std::vector<value_t> &ex) const
{
  return predict(normalise_output(oracle_(ex)));
}

///
/// Bounds finite outputs and treats non-finite outputs as missing.
///
template<Individual I, bool S>
std::optional<double> basic_gaussian_oracle<I, S>::normalise_output(
  const value_t &res)
{
  if (!has_value(res))
    return std::nullopt;

  const double out(std::get<D_DOUBLE>(res));
  if (!std::isfinite(out))
    return std::nullopt;

  constexpr double cut(100000000.0);
  return std::clamp(out, -cut, cut);
}

///
/// Precomputes the fallback result for missing outputs based on class priors.
///
/// \return predicted class label and confidence derived from effective support
///
template<Individual I, bool S>
classification_result basic_gaussian_oracle<I, S>::calculate_missing_fallback() const
{
  const std::size_t classes(gauss_dist_.size());

  if (classes == 1)
    return {0, 1.0};

  std::size_t best(0);
  double best_count(0.0), total(0.0);

  for (std::size_t i(0); i < classes; ++i)
  {
    // Effective sample count, including variance inflation.
    const auto n(static_cast<double>(gauss_dist_[i].size()));
    total += n;

    if (n > best_count)
    {
      best_count = n;
      best = i;
    }
  }

  const double confidence(total > 0.0 ? best_count / total : 0.0);
  return {best, confidence};
}

///
/// Classifies a single scalar output value using Gaussian class models.
///
/// \param[in] x scalar output value to classify
/// \return      predicted class label and confidence level
///
template<Individual I, bool S>
classification_result basic_gaussian_oracle<I, S>::predict(double x) const
{
  const std::size_t classes(gauss_dist_.size());

  if (classes == 1)
    return {0, 1.0};

  double best_p(0.0), sum_p(0.0);
  src::class_t probable_class(0);

  for (std::size_t i(0); i < classes; ++i)
    if (!gauss_dist_[i].empty())  // unobserved classes receive p = 0.0
    {
      const auto mean(gauss_dist_[i].mean());
      const auto variance(gauss_dist_[i].variance());
      const auto distance(x - mean);

      double p(0.0);
      if (issmall(variance))                // degenerate or nearly-degenerate
        p = issmall(distance) ? 1.0 : 0.0;  // treat as a point mass at the mean
      else
        p = std::exp(-distance * distance / variance);

      if (p > best_p)
      {
        best_p = p;
        probable_class = i;
      }

      sum_p += p;
    }

  // Do not change `sum_p <= 0.0` with
  // - `issmall(sum_p)` => when `sum_p` is small, `best_p` is smaller and the
  //   division works well.
  // - `sum_p == 0.0` => it's the same thing but will produce a warning with
  //   `-Wfloat-equal`
  if (sum_p <= 0.0)
    return {missing_fallback_.label, 0.0};

  return {probable_class, best_p / sum_p};
}

///
/// Saves the oracle on persistent storage.
///
/// \param[out] out output stream
/// \return         `true` on success
///
template<Individual I, bool S>
bool basic_gaussian_oracle<I, S>::save(std::ostream &out) const
{
  if (!oracle_.save(out))
    return false;

  if (!(out << gauss_dist_.size() << '\n'))
    return false;

  for (const auto &g : gauss_dist_)
    if (!g.save(out))
      return false;

  return save_names(out);
}

///
/// \return `true` if the object passes the internal consistency check
///
template<Individual I, bool S>
bool basic_gaussian_oracle<I, S>::is_valid() const
{
  return oracle_.is_valid() && !gauss_dist_.empty()
         && (names_size() == 0 || names_size() == gauss_dist_.size());
}

// ***********************************************************************
// *  basic_binary_oracle                                                *
// ***********************************************************************

template<Individual I, bool S>
const std::string basic_binary_oracle<I, S>::SERIALIZE_ID("BINARY_ORACLE");

///
/// \param[in] ind   individual "to be transformed" into an oracle
/// \param[in] d     the training set
/// \param[in] names optional class names
///
template<Individual I, bool S>
basic_binary_oracle<I, S>::basic_binary_oracle(
  const I &ind, [[maybe_unused]] const dataframe &d,
  std::vector<std::string> names)
  : basic_class_oracle(std::move(names)), oracle_(ind)
{
  Expects(!ind.empty());
  Expects(d.classes() == 2);
  Expects(names_size() == 0 || names_size() == 2);

  Ensures(is_valid());
}

///
/// Constructs the object reading data from an input stream.
///
/// \param[in] in input stream
/// \param[in] ss active symbol set
///
template<Individual I, bool S>
basic_binary_oracle<I, S>::basic_binary_oracle(std::istream &in,
                                               const symbol_set &ss)
  : basic_class_oracle(), oracle_(in, ss)
{
  static_assert(
    S, "binary_oracle requires storage space for de-serialization");

  if (!load_names(in) || (names_size() != 0 && names_size() != 2))
    throw exception::data_format(
      "Cannot read binary_oracle class_names component");

  Ensures(is_valid());
}

///
/// \param[in] ex input example for the lambda function
/// \return       the class of `e` (numerical id) and the confidence level (in
///               the `[0,1]` interval)
///
template<Individual I, bool S>
classification_result basic_binary_oracle<I, S>::tag(
  const std::vector<value_t> &ex) const
{
  const auto res(oracle_(ex));
  const double val(has_value(res) ? std::get<D_DOUBLE>(res) : 0.0);

  return {val > 0.0 ? 1u : 0u,
          std::fabs(2.0 * std::numbers::inv_pi * std::atan(val))};
}

///
/// \return `true` if the object passes the internal consistency check
///
template<Individual I, bool S>
bool basic_binary_oracle<I, S>::is_valid() const
{
  return oracle_.is_valid() && (names_size() == 0 || names_size() == 2);
}

///
/// Saves the oracle on persistent storage.
///
/// \param[out] out output stream
/// \return         `true` on success
///
template<Individual I, bool S>
bool basic_binary_oracle<I, S>::save(std::ostream &out) const
{
  if (!oracle_.save(out))
    return false;

  return save_names(out);
}

// ***********************************************************************
// *  class_oracle_adaptor                                               *
// ***********************************************************************

///
/// \param[in] oracle predictor instance to adapt
/// \param[in] names  optional class names used for human-readable output
///
template<ClassificationPredictor O>
class_oracle_adaptor<O>::class_oracle_adaptor(O oracle,
                                              std::vector<std::string> names)
  : basic_class_oracle(std::move(names)), oracle_(std::move(oracle))
{
}

///
/// Returns the classification result for the input example.
///
/// \param[in] x input example
/// \return      predicted class label and confidence
///
/// This function forwards the call to the underlying predictor.
///
template<ClassificationPredictor O>
classification_result class_oracle_adaptor<O>::tag(
  const std::vector<value_t> &x) const
{
  return oracle_.tag(x);
}

///
/// \return identifier used for serialization
///
/// \remark
/// The adaptor is not serializable. This identifier is provided only to
/// satisfy the `basic_oracle` interface.
///
template<ClassificationPredictor O>
std::string class_oracle_adaptor<O>::serialize_id() const
{
  return "CLASS_ORACLE_ADAPTOR";
}

///
/// Serialization is not supported.
///
/// \throws std::logic_error always
///
template<ClassificationPredictor O>
bool class_oracle_adaptor<O>::save(std::ostream &) const
{
  throw std::logic_error("class_oracle_adaptor is not serializable");
}

// ***********************************************************************
// *  team_class_oracle                                                  *
// ***********************************************************************

template<ClassificationPredictor O, team_composition C>
const std::string team_class_oracle<O, C>::SERIALIZE_ID(
  "TEAM_" + O::SERIALIZE_ID);

///
/// \param[in] t     team "to be transformed" into an oracle
/// \param[in] d     the non-empty training set
/// \param[in] names optional class names
///
template<ClassificationPredictor O, team_composition C>
template<Individual I>
team_class_oracle<O, C>::team_class_oracle(
  const gp::team<I> &t, const dataframe &d, std::vector<std::string> names)
  : basic_class_oracle(std::move(names)), classes_(d.classes())
{
  Expects(!t.empty());
  Expects(!d.empty());
  Expects(d.classes() > 1);
  Expects(names_size() == 0 || names_size() == d.classes());

  team_.reserve(t.size());
  for (const auto &ind : t)
    team_.emplace_back(ind, d);

  Ensures(is_valid());
}

///
/// \param[in]  t           team "to be transformed" into an oracle
/// \param[in]  d           the non-empty training set
/// \param[out] predictions container populated with predictions on `d`
/// \param[in]  names       optional class names
///
template<ClassificationPredictor O, team_composition C>
template<Individual I>
team_class_oracle<O, C>::team_class_oracle(
  const gp::team<I> &t, const dataframe &d,
  std::vector<classification_result> &predictions,
  std::vector<std::string> names)
  : basic_class_oracle(std::move(names)), classes_(d.classes())
{
  Expects(!t.empty());
  Expects(!d.empty());
  Expects(d.classes() > 1);
  Expects(names_size() == 0 || names_size() == d.classes());

  team_.reserve(t.size());

  if constexpr (C == team_composition::wta)
  {
    team_.emplace_back(t[0], d, predictions);

    std::vector<classification_result> member_pred;
    for (std::size_t m(1); m < t.size(); ++m)
    {
      team_.emplace_back(t[m], d, member_pred);
      for (std::size_t i(0); i < d.size(); ++i)
        if (member_pred[i].sureness > predictions[i].sureness)
          predictions[i] = member_pred[i];
    }
  }
  else if constexpr (C == team_composition::mv)
  {
    matrix<unsigned> votes(d.size(), classes_, 0);

    std::vector<classification_result> member_pred;
    for (const auto &ind : t)
    {
      team_.emplace_back(ind, d, member_pred);
      for (std::size_t i(0); i < d.size(); ++i)
        ++votes(i, member_pred[i].label);
    }

    predictions.clear();
    predictions.reserve(d.size());

    for (std::size_t i(0); i < votes.rows(); ++i)
      predictions.push_back(majority_vote(votes.row(i)));
  }

  Ensures(is_valid());
}

///
/// Constructs the object reading data from an input stream.
///
/// \param[in] in input stream
/// \param[in] ss active symbol set
///
template<ClassificationPredictor O, team_composition C>
team_class_oracle<O, C>::team_class_oracle(std::istream &in,
                                         const symbol_set &ss)
  : basic_class_oracle()
{
  if (!(in >> classes_))
    throw exception::data_format("Cannot read number of classes");

  std::size_t s;
  if (!(in >> s))
    throw exception::data_format("Cannot read team size");

  team_.reserve(s);
  for (std::size_t i(0); i < s; ++i)
    team_.emplace_back(in, ss);

  if (!load_names(in))
    throw exception::data_format("Cannot read class_names");
}

///
/// Specialized method for teams.
///
/// \param[in] instance data to be classified
/// \return             the class of `instance` (numerical id) and the
///                     confidence level (in the `[0,1]` interval)
///
/// * `team_composition::mv` the class which most of the individuals predict
///   for a given example is selected as team output.
/// * `team_composition::wta` the winner is the individual with the highest
///   confidence in its decision. Specialization may emerge if different
///   members of the team win this contest for different fitness cases (of
///   course, it isn't a feasible alternative to select the member with the
///   best fitness. Then a decision on unknown data is only possible if the
///   right outputs are known in advance and is not made by the team itself).
///
template<ClassificationPredictor O, team_composition C>
classification_result team_class_oracle<O, C>::tag(
  const std::vector<value_t> &instance) const
{
  Expects(team_.size());

  if constexpr (C == team_composition::wta)
  {
    const auto size(team_.size());
    auto best(team_[0].tag(instance));

    for (std::size_t i(1); i < size; ++i)
    {
      const auto res(team_[i].tag(instance));

      if (res.sureness > best.sureness)
        best = res;
    }

    return best;
  }
  else if constexpr (C == team_composition::mv)
  {
    std::vector<unsigned> votes(classes_);

    for (const auto &oracle : team_)
      ++votes[oracle.tag(instance).label];

    return majority_vote(votes);
  }
}

///
/// Selects the class with the most votes, favouring the first class on ties.
///
template<ClassificationPredictor O, team_composition C>
classification_result team_class_oracle<O, C>::majority_vote(
  std::span<const unsigned> votes) const
{
  src::class_t max(0);
  for (src::class_t i(1); i < classes_; ++i)
    if (votes[i] > votes[max])
      max = i;

  return {max, static_cast<double>(votes[max])
               / static_cast<double>(team_.size())};
}

///
/// Saves the oracle team on persistent storage.
///
/// \param[out] out output stream
/// \return         `true` on success
///
template<ClassificationPredictor O, team_composition C>
bool team_class_oracle<O, C>::save(std::ostream &out) const
{
  if (!(out << classes_ << '\n'))
    return false;

  if (!(out << team_.size() << '\n'))
    return false;

  for (const auto &i : team_)
    if (!i.save(out))
      return false;

  return save_names(out);
}

///
/// \return class ID used for serialization
///
template<ClassificationPredictor O, team_composition C>
std::string team_class_oracle<O, C>::serialize_id() const noexcept
{
  return SERIALIZE_ID;
}

///
/// \return `true` if the object passes the internal consistency check
///
template<ClassificationPredictor O, team_composition C>
bool team_class_oracle<O, C>::is_valid() const
{
  return classes_ > 1;
}

namespace serialize::oracle
{

namespace internal
{
using build_func = std::unique_ptr<basic_oracle> (*)(std::istream &,
                                                     const symbol_set &);

template<class U>
std::unique_ptr<basic_oracle> build(std::istream &in, const symbol_set &ss)
{
  return std::make_unique<U>(in, ss);
}

extern std::map<std::string, build_func> factory_;
}  // namespace internal

///
/// Allows insertion of user defined classificators.
///
template<class U>
bool insert(const std::string &id)
{
  Expects(!id.empty());
  return internal::factory_.insert({id, internal::build<U>}).second;
}

template<Individual T>
std::unique_ptr<basic_oracle> load(std::istream &in, const symbol_set &ss)
{
  if (internal::factory_.find(reg_oracle<T>::SERIALIZE_ID)
      == internal::factory_.end())
  {
    insert<reg_oracle<T>>(reg_oracle<T>::SERIALIZE_ID);
    //insert<dyn_slot_lambda_f<T>>(dyn_slot_lambda_f<T>::SERIALIZE_ID);
    insert<gaussian_oracle<T>>(gaussian_oracle<T>::SERIALIZE_ID);
    insert<binary_oracle<T>>(binary_oracle<T>::SERIALIZE_ID);
  }

  std::string id;
  if (!(in >> id))
    return nullptr;

  const auto iter(internal::factory_.find(id));
  if (iter != internal::factory_.end())
    return iter->second(in, ss);

  return nullptr;
}

}  // namespace serialize::oracle

#endif  // include guard
