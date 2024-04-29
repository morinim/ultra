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

#if !defined(ULTRA_ORACLE_INTERNAL_TCC)
#define      ULTRA_ORACLE_INTERNAL_TCC

namespace internal
{

// ***********************************************************************
// *  reg_oracle_storage                                                 *
// ***********************************************************************

// The general template.
template<class P, bool S> class reg_oracle_storage;

// ********* First specialization (individual stored inside) *********
template<>
class reg_oracle_storage<gp::individual, true>
{
public:
  explicit reg_oracle_storage(const gp::individual &ind) : ind_(ind),
                                                           int_(ind_)
  { Ensures(is_valid()); }

  explicit reg_oracle_storage(const reg_oracle_storage &ros) : ind_(ros.ind_),
                                                               int_(ind_)
  {
    Ensures(is_valid());
  }

  reg_oracle_storage(std::istream &in, const symbol_set &ss) : int_(ind_)
  {
    if (!ind_.load(in, ss))
      throw exception::data_format("Cannot load individual");

    int_ = src::interpreter(ind_);

    Ensures(is_valid());
  }

  reg_oracle_storage &operator=(const reg_oracle_storage &rhs)
  {
    if (this != &rhs)
    {
      ind_ = rhs.ind_;
      int_ = src::interpreter(ind_);
    }

    Ensures(is_valid());
    return *this;
  }

  template<class ...Args> [[nodiscard]] value_t run(Args && ...args) const
  {
    // There are situations in which `&int_.program() != &ind_` and this
    // function will blow up.
    // It shouldn't happen: `is_valid` checks for this problematic condition,
    // `operator=` resets the pointer to the reference individual... but it's
    // not enough.
    // Consider a scenario in which a vector of `reg_oracle_storage` objects
    // needs reallocation after a `push_back`. All the `int_` objects are
    // invalidated...
    //
    // `src::interpreter` is a lightweight object so we could recreate it here
    // every time or we could add a check:
    // `if (&int_.program() != &ind_)  int_ = src::interpreter<T>(&ind_);`
    // Both solutions should be checked for possible performance hits.
    return int_.run(std::forward<Args>(args)...);
  }

  [[nodiscard]] bool is_valid() const
  {
    return &int_.program() == &ind_;
  }

  // Serialization.
  bool save(std::ostream &out) const { return ind_.save(out); }

private:
  gp::individual ind_;
  mutable src::interpreter int_;
};

// ********* Second specialization (individual not stored) *********
template<>
class reg_oracle_storage<gp::individual, false>
{
public:
  explicit reg_oracle_storage(const gp::individual &ind) : int_(ind)
  { Ensures(is_valid()); }

  template<class ...Args> [[nodiscard]] value_t run(Args && ...args) const
  {
    return int_.run(std::forward<Args>(args)...);
  }

  [[nodiscard]] bool is_valid() const { return true; }

  // Serialization
  bool save(std::ostream &out) const
  {
    return int_.program().save(out);
  }

private:
  mutable src::interpreter int_;
};

// ********* Third specialization (teams) *********
template<Team T, bool S>
class reg_oracle_storage<T, S>
{
public:
  explicit reg_oracle_storage(const T &t)
  {
    team_.reserve(t.size());
    for (const auto &ind : t)
      team_.emplace_back(ind);

    Ensures(is_valid());
  }

  reg_oracle_storage(std::istream &in, const symbol_set &ss)
  {
    std::size_t n;
    if (!(in >> n) || !n)
      throw exception::data_format("Unknown/wrong number of programs");

    team_.reserve(n);
    for (std::size_t j(0); j < n; ++j)
      team_.emplace_back(in, ss);

    Ensures(is_valid());
  }

  [[nodiscard]] bool is_valid() const
  {
    return true;
  }

  // Serialization.
  bool save(std::ostream &o) const
  {
    return (o << team_.size() << '\n')
           && std::ranges::all_of(
                team_,
                [&o](const auto &ind) { return ind.save(o); });
  }

public:
  std::vector<reg_oracle_storage<typename T::value_type, S>> team_ {};
};

// ***********************************************************************
// *  class_names                                                        *
// ***********************************************************************

///
/// A class which (optionally) stores a vector of names.
///
/// \tparam N if `true` stores the names (otherwise saves memory)
///
/// This class is used for optimize the storage of basic_class_oracle. The
/// strategy used is the so called 'Empty Base Class Optimization': the
/// compiler is allowed to flatten the inheritance hierarchy in a way that
/// the empty base class does not consume space (this is not true for empty
/// class data members because C++ requires data member to have non-zero size
/// to ensure object identity).
///
/// \see
/// https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Empty_Base_Optimization
///
template<bool N>
class class_names
{
public:
  // *** Serialization ***
  bool load(std::istream &) { return true; }
  bool save(std::ostream &) const { return true; }

protected:
  class_names() = default;

  /// Without names... there is nothing to do.
  explicit class_names(const dataframe &) {}

  /// \param[in] a id of a class
  /// \return      the name of class `a`
  [[nodiscard]] std::string string(const value_t &a) const
  {
    return std::to_string(std::get<D_INT>(a));
  }
};

template<>
class class_names<true>
{
public:
  // *** Serialization ***

  /// Loads the names from storage.
  ///
  /// \param[in] in input stream
  /// \return       `true` on success
  bool load(std::istream &in)
  {
    std::size_t n;
    if (!(in >> n) || !n)
      return false;

    decltype(names_) v(n);

    // When used immediately after whitespace-delimited input (e.g. after
    // `int n; std::cin >> n;`) `getline` consumes the endline character left
    // on the input stream by `operator>>` and returns immediately. A common
    // solution, before switching to line-oriented input, is to ignore all
    // leftover characters on the line of input with:
    std::ws(in);

    for (auto &line : v)
      if (!getline(in, line))
        return false;

    names_ = v;

    return true;
  }

  /// Saves the names.
  ///
  /// \param[out] o output stream
  /// \return       `true` on success
  ///
  /// One name per line, end of line character is `\n`. First line contains
  /// the number of names.
  bool save(std::ostream &o) const
  {
    if (!(o << names_.size() << '\n'))
      return false;

    for (const auto &n : names_)
      if (!(o << n << '\n'))
        return false;

    return o.good();
  }

protected:
  class_names() = default;

  /// \param[in] d the training set
  explicit class_names(const dataframe &d) : names_(d.classes())
  {
    Expects(d.classes() > 1);
    const auto classes(d.classes());

    for (std::size_t i(0); i < classes; ++i)
      names_[i] = d.class_name(i);
  }

  /// \param[in] a id of a class
  /// \return      the name of class `a`
  [[nodiscard]] std::string string(const value_t &a) const
  {
    return names_[std::get<D_INT>(a)];
  }

private:
  std::vector<std::string> names_ {};
};

}  // namespace internal

#endif  // include guard
