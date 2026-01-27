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
  if constexpr (!gp::Team<P>)
    return this->run(e);
  else
  {
    D_DOUBLE sum(0), count(0);

    for (const auto &core : this->team_)
    {
      const auto res(core.run(e));

      if (has_value(res))
      {
        sum += std::get<D_DOUBLE>(res);
        ++count;
      }
    }

    if (count > 0.0)
      return sum / count;

    return {};
  }
}

///
/// \return the functions doesn't return
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
/// Calls (dynamic dispatch) polymhorphic model_metric `m` on `this`.
///
/// \param[in] m a metric we are evaluating
/// \param[in] d a dataset
/// \return      the value of `this` according to metric `m`
///
template<Individual P, bool S>
double basic_reg_oracle<P, S>::measure(const model_metric &m,
                                       const dataframe &d) const
{
  return m(this, d);
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
// *  basic_class_oracle                                                 *
// ***********************************************************************

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

// ***********************************************************************
// *  basic_gaussian_oracle                                              *
// ***********************************************************************

template<class I, bool S, bool N>
const std::string basic_gaussian_oracle<I, S, N>::SERIALIZE_ID(
  "GAUSSIAN_ORACLE");

///
/// \param[in] ind program "to be transformed" into an oracle
/// \param[in] d   the training set
///
template<class I, bool S, bool N>
basic_gaussian_oracle<I, S, N>::basic_gaussian_oracle(const I &ind,
                                                      const dataframe &d)
  : basic_class_oracle<N>(d), oracle_(ind), gauss_dist_(d.classes())
{
  static_assert(Individual<I>);
  Expects(!ind.empty());
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
template<class I, bool S, bool N>
basic_gaussian_oracle<I, S, N>::basic_gaussian_oracle(std::istream &in,
                                                      const symbol_set &ss)
  : basic_class_oracle<N>(), oracle_(in, ss)
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
template<class I, bool S, bool N>
void basic_gaussian_oracle<I, S, N>::fill_vector(const dataframe &d)
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
template<class I, bool S, bool N>
classification_result basic_gaussian_oracle<I, S, N>::tag(
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
template<class I, bool S, bool N>
bool basic_gaussian_oracle<I, S, N>::save(std::ostream &out) const
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
template<class I, bool S, bool N>
bool basic_gaussian_oracle<I, S, N>::is_valid() const
{
  return oracle_.is_valid();
}

// ***********************************************************************
// *  basic_binary_oracle                                                *
// ***********************************************************************

template<class I, bool S, bool N>
const std::string basic_binary_oracle<I, S, N>::SERIALIZE_ID(
  "BINARY_ORACLE");

///
/// \param[in] ind individual "to be transformed" into an oracle
/// \param[in] d   the training set
///
template<class I, bool S, bool N>
basic_binary_oracle<I, S, N>::basic_binary_oracle(const I &ind,
                                                  const dataframe &d)
  : basic_class_oracle<N>(d), oracle_(ind)
{
  static_assert(Individual<I>);
  Expects(!ind.empty());
  Expects(d.classes() == 2);

  Ensures(is_valid());
}

///
/// Constructs the object reading data from an input stream.
///
/// \param[in] in input stream
/// \param[in] ss active symbol set
///
template<class I, bool S, bool N>
basic_binary_oracle<I, S, N>::basic_binary_oracle(std::istream &in,
                                                  const symbol_set &ss)
  : basic_class_oracle<N>(), oracle_(in, ss)
{
  static_assert(
    S, "binary_oracle requires storage space for de-serialization");

  if (!internal::class_names<N>::load(in))
      throw exception::data_format(
        "Cannot read binary_oracle class_names component");

  Ensures(is_valid());
}

///
/// \param[in] ex input example for the lambda function
/// \return       the class of `e` (numerical id) and the confidence level (in
///               the `[0,1]` interval)
///
template<class I, bool S, bool N>
classification_result basic_binary_oracle<I, S, N>::tag(
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
template<class I, bool S, bool N>
bool basic_binary_oracle<I, S, N>::is_valid() const
{
  return oracle_.is_valid();
}

///
/// Saves the oracle on persistent storage.
///
/// \param[out] out output stream
/// \return         `true` on success
///
template<class I, bool S, bool N>
bool basic_binary_oracle<I, S, N>::save(std::ostream &out) const
{
  if (!oracle_.save(out))
    return false;

  return internal::class_names<N>::save(out);
}

// ***********************************************************************
// *  team_class_oracle                                                  *
// ***********************************************************************

template<Individual I, bool S, bool N,
         template<class, bool, bool> class L,
         team_composition C>
const std::string team_class_oracle<I, S, N, L, C>::SERIALIZE_ID(
  "TEAM_" + L<I, S, N>::SERIALIZE_ID);

///
/// \param[in] t    team "to be transformed" into an oracle
/// \param[in] d    the training set
/// \param[in] args auxiliary parameters for the specific oracle
///
template<Individual I, bool S, bool N, template<class, bool, bool> class L,
         team_composition C>
template<class... Args>
team_class_oracle<I, S, N, L, C>::team_class_oracle(const gp::team<I> &t,
                                                    const dataframe &d,
                                                    Args&&... args)
  : basic_class_oracle<N>(d), classes_(d.classes())
{
  team_.reserve(t.size());
  for (const auto &ind : t)
    team_.emplace_back(ind, d, std::forward<Args>(args)...);
}

///
/// Constructs the object reading data from an input stream.
///
/// \param[in] in input stream
/// \param[in] ss active symbol set
///
template<Individual I, bool S, bool N, template<class, bool, bool> class L,
         team_composition C>
team_class_oracle<I, S, N, L, C>::team_class_oracle(std::istream &in,
                                                    const symbol_set &ss)
  : basic_class_oracle<N>()
{
  static_assert(
    S, "team_class_oracle requires storage space for de-serialization");

  if (!(in >> classes_))
    throw exception::data_format("Cannot read number of classes");

  std::size_t s;
  if (!(in >> s))
    throw exception::data_format("Cannot read team size");

  team_.reserve(s);
  for (std::size_t i(0); i < s; ++i)
    team_.emplace_back(in, ss);

  if (!internal::class_names<N>::load(in))
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
template<Individual I, bool S, bool N, template<class, bool, bool> class L,
         team_composition C>
classification_result team_class_oracle<I, S, N, L, C>::tag(
  const std::vector<value_t> &instance) const
{
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

    src::class_t max(0);
    for (auto i(max + 1); i < classes_; ++i)
      if (votes[i] > votes[max])
        max = i;

    return {max, static_cast<double>(votes[max])
                 / static_cast<double>(team_.size())};
  }
}

///
/// Saves the oracle team on persistent storage.
///
/// \param[out] out output stream
/// \return         `true` on success
///
template<Individual I, bool S, bool N, template<class, bool, bool> class L,
         team_composition C>
bool team_class_oracle<I, S, N, L, C>::save(std::ostream &out) const
{
  if (!(out << classes_ << '\n'))
    return false;

  if (!(out << team_.size() << '\n'))
    return false;

  for (const auto &i : team_)
    if (!i.save(out))
      return false;

  return internal::class_names<N>::save(out);
}

///
/// \return class ID used for serialization
///
template<Individual I, bool S, bool N, template<class, bool, bool> class L,
         team_composition C>
std::string team_class_oracle<I, S, N, L, C>::serialize_id() const
{
  Expects(team_.size());
  return "TEAM_" + L<I, S, N>::SERIALIZE_ID;
}

///
/// \return `true` if the object passes the internal consistency check
///
template<Individual I, bool S, bool N, template<class, bool, bool> class L,
         team_composition C>
bool team_class_oracle<I, S, N, L, C>::is_valid() const
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
