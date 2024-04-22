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
value_t basic_reg_oracle<P, S>::operator()(const example &e) const
{
  if constexpr (!Team<P>)
    return this->run(e.input);
  else
  {
    D_DOUBLE avg(0), count(0);

    // Calculate the running average.
    for (const auto &core : this->team_)
    {
      const auto res(core.run(e.input));

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
classification_result basic_reg_oracle<P, S>::tag(const example &) const
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
//template<IndividualOrTeam P, bool S>
//double basic_reg_omega<P, S>::measure(const model_metric &m,
//                                      const dataframe &d) const
//{
//  return m(this, d);
//}

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


namespace serialize
{

namespace lambda
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
}

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

}  // namespace oracle

}  // namespace serialize

#endif  // include guard
