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

template<IndividualOrTeam P, bool S>
const std::string basic_reg_oracle<P, S>::SERIALIZE_ID(
  Team<P> ? "TEAM_REG_ORACLE" : "REG_ORACLE");

template<IndividualOrTeam P, bool S, bool N>
const std::string basic_gaussian_oracle<P, S, N>::SERIALIZE_ID(
  "GAUSSIAN_ORACLE");

///
/// \param[in] prg the program (individual/team) to be lambdified
///
template<IndividualOrTeam P, bool S>
basic_reg_oracle<P, S>::basic_reg_oracle(const P &prg)
  : internal::reg_oracle_storage<P, S>(prg)
{
  Ensures(is_valid());
}

///
/// \param[in] in input stream
/// \param[in] ss active symbol set
///
template<IndividualOrTeam P, bool S>
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
template<IndividualOrTeam P, bool S>
value_t basic_reg_oracle<P, S>::operator()(const std::vector<value_t> &e) const
{
  if constexpr (!Team<P>)
    return this->run(e);
  else
  {
    D_DOUBLE avg(0), count(0);

    // Calculate the running average.
    for (const auto &core : this->team_)
    {
      const auto res(core.run(e));

      if (has_value(res))
        avg += (std::get<D_DOUBLE>(res) - avg) / ++count;
    }

    if (count > 0.0)
      return avg;

    return {};
  }
}

///
/// \return a *failed* status
///
/// \warning This function is useful only for classification tasks.
///
template<IndividualOrTeam P, bool S>
classification_result basic_reg_oracle<P, S>::tag(
  const std::vector<value_t> &) const
{
  return {0, 0};
}

///
/// \param[in] a value produced by basic_oracle::operator()
/// \return      the string version of `a`
///
template<IndividualOrTeam P, bool S>
std::string basic_reg_oracle<P, S>::name(const value_t &a) const
{
  return lexical_cast<std::string>(a);
}

///
/// Calls (dynamic dispatch) polymhorphic model_metric `m` on `this`.
///
/// \param[in] m a metric we are evaluating
/// \param[in] d a dataset
/// \return      the value of `this` according to metric `m`
///
template<IndividualOrTeam P, bool S>
double basic_reg_oracle<P, S>::measure(const model_metric &m,
                                       const dataframe &d) const
{
  return m(this, d);
}

///
/// \return `true` if the object passes the internal consistency check
///
template<IndividualOrTeam P, bool S>
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
template<IndividualOrTeam P, bool S>
bool basic_reg_oracle<P, S>::save(std::ostream &out) const
{
  return internal::reg_oracle_storage<P, S>::save(out);
}

///
/// \param[in] d the training set
///
template<bool N>
basic_class_oracle<N>::basic_class_oracle(const dataframe &d)
  : internal::class_names<N>(d)
{
}

///
/// \param[in] e example to be classified
/// \return      the label of the class that includes `e`
///
template<bool N>
value_t basic_class_oracle<N>::operator()(const std::vector<value_t> &e) const
{
  return static_cast<D_INT>(this->tag(e).label);
}

///
/// Calls (dynamic dispatch) polymhorphic model_metric `m` on `this`.
///
/// \param[in] m a metric we are evaluating
/// \param[in] d a dataset
/// \return      the value of `this` according to metric `m`
///
template<bool N>
double basic_class_oracle< N>::measure(const model_metric &m,
                                       const dataframe &d) const
{
  return m(this, d);
}

///
/// \param[in] a id of a class
/// \return      the name of class `a`
///
template<bool N>
std::string basic_class_oracle<N>::name(const value_t &a) const
{
  return internal::class_names<N>::string(a);
}

///
/// \param[in] prg program "to be transformed" into an oracle
/// \param[in] d   the training set
///
template<IndividualOrTeam P, bool S, bool N>
basic_gaussian_oracle<P, S, N>::basic_gaussian_oracle(const P &prg,
                                                      dataframe &d)
  : basic_class_oracle<N>(d), oracle_(prg), gauss_dist_(d.classes())
{
  Expects(d.classes() > 1);

  fill_vector(d);

  Ensures(is_valid());
}

///
/// Constructs the object reading data from an input stream.
///
/// \param[in] in input stream
/// \param[in] ss active symbol set
///
template<IndividualOrTeam P, bool S, bool N>
basic_gaussian_oracle<P, S, N>::basic_gaussian_oracle(std::istream &in,
                                                      const symbol_set &ss)
  : basic_class_oracle<N>(), oracle_(in, ss)
{
  static_assert(
    S, "gaussian_lambda_f requires storage space for de-serialization");

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

  if (!internal::class_names<N>::load(in))
      throw exception::data_format(
        "Cannot read gaussian_oracle class_names component");

  Ensures(is_valid());
}

///
/// Sets up the data structures needed by the gaussian algorithm.
///
/// \param[in] d the training set
///
template<IndividualOrTeam P, bool S, bool N>
void basic_gaussian_oracle<P, S, N>::fill_vector(dataframe &d)
{
  Expects(d.classes() > 1);

  // For a set of training data, we assume that the behaviour of a program
  // classifier is modelled using multiple Gaussian distributions, each of
  // which corresponds to a particular class. The distribution of a class is
  // determined by evaluating the program on the examples of the class in
  // the training set. This is done by taking the mean and standard deviation
  // of the program outputs for those training examples for that class.
  for (const auto &ex : d)
  {
    const auto res(oracle_(ex.input));

    double val(has_value(res) ? std::get<D_DOUBLE>(res) : 0.0);

    constexpr double cut(10000000.0);
    val = std::clamp(val, -cut, cut);

    gauss_dist_[label(ex)].add(val);
  }
}

///
/// \param[in] ex input value whose class we are interested in
/// \return       the class of `ex` (numerical id) and the confidence level
///               (how sure you can be that `ex` is properly classified. The
///               value is in the `[0,1]` interval and the sum of all the
///               confidence levels of each class equals `1`)
///
template<IndividualOrTeam P, bool S, bool N>
classification_result basic_gaussian_oracle<P, S, N>::tag(
  const std::vector<value_t> &ex) const
{
  const auto res(oracle_(ex));
  const double x(has_value(res) ? std::get<D_DOUBLE>(res) : 0.0);

  double val_(0.0), sum_(0.0);
  src::class_t probable_class(0);

  const std::size_t classes(gauss_dist_.size());
  for (std::size_t i(0); i < classes; ++i)
  {
    const double distance(std::fabs(x - gauss_dist_[i].mean()));
    const double variance(gauss_dist_[i].variance());

    double p(0.0);
    if (issmall(variance))     // these are borderline cases
      if (issmall(distance))   // these are borderline cases
        p = 1.0;
      else
        p = 0.0;
    else                       // this is the standard case
      p = std::exp(-distance * distance / variance);

    if (p > val_)
    {
      val_ = p;
      probable_class = i;
    }

    sum_ += p;
  }

  // Normalized confidence value.
  // Do not change `sum_ > 0.0` with
  // - `issmall(sum_)` => when `sum_` is small, `val_` is smaller and the
  //   division works well.
  // - `sum_` => it's the same thing but will produce a warning with
  //             `-Wfloat-equal`
  const double confidence(sum_ > 0.0 ? val_ / sum_ : 0.0);

  return {probable_class, confidence};
}

///
/// Saves the oracle on persistent storage.
///
/// \param[out] out output stream
/// \return         `true` on success
///
template<IndividualOrTeam P, bool S, bool N>
bool basic_gaussian_oracle<P, S, N>::save(std::ostream &out) const
{
  if (!oracle_.save(out))
    return false;

  if (!(out << gauss_dist_.size() << '\n'))
    return false;

  for (const auto &g : gauss_dist_)
    if (!g.save(out))
      return false;

  return internal::class_names<N>::save(out);
}

///
/// \return `true` if the object passes the internal consistency check
///
template<IndividualOrTeam P, bool S, bool N>
bool basic_gaussian_oracle<P, S, N>::is_valid() const
{
  return true;
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

template<IndividualOrTeam T>
std::unique_ptr<basic_oracle> load(std::istream &in, const symbol_set &ss)
{
  if (internal::factory_.find(reg_oracle<T>::SERIALIZE_ID)
      == internal::factory_.end())
  {
    insert<reg_oracle<T>>(reg_oracle<T>::SERIALIZE_ID);
    //insert<dyn_slot_lambda_f<T>>(dyn_slot_lambda_f<T>::SERIALIZE_ID);
    //insert<gaussian_lambda_f<T>>(gaussian_lambda_f<T>::SERIALIZE_ID);
    //insert<binary_lambda_f<T>>(binary_lambda_f<T>::SERIALIZE_ID);
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
