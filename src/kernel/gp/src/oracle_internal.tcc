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
///
/// Specialised oracle storage for GP individuals with lazy interpretation.
///
/// This specialisation stores a `gp::individual` and provides execution
/// support via a lazily constructed interpreter. The interpreter is treated as
/// an internal cache and is not part of the observable state of the object.
///
/// The interpreter is created on first use and rebound to the stored
/// individual on every invocation of `run()`. This guarantees correctness even
/// after copy or move operations, without requiring special handling in
/// constructors or assignment operators.
///
/// \note
/// The interpreter is only ever accessed inside `run()`. All other member
/// functions must not rely on, or modify, the interpreter state.
///
template<>
class reg_oracle_storage<gp::individual, true>
{
public:
  /// Constructs the storage from an existing individual.
  ///
  /// \param[in] ind a valid GP individual
  ///
  /// The interpreter is not created during construction and will be
  /// instantiated lazily on the first call to `run()`.
  explicit reg_oracle_storage(const gp::individual &ind) : ind_(ind)
  {
    Ensures(is_valid());
  }

  /// Deserialises the storage from a stream.
  ///
  /// \param[in] in input stream
  /// \param[in] ss symbol set used to resolve symbols during loading
  ///
  /// The individual is loaded from the input stream using the provided symbol
  /// set. Any existing interpreter state is discarded.
  ///
  /// \throws `exception::data_format` if the individual cannot be loaded
  reg_oracle_storage(std::istream &in, const symbol_set &ss)
  {
    if (!ind_.load(in, ss))
      throw exception::data_format("Cannot load individual");

    int_.reset();

    Ensures(is_valid());
  }

  /// Executes the stored individual.
  ///
  /// \tparam Args argument types forwarded to the interpreter
  ///
  /// \param[in] args arguments passed to the interpreter
  /// \return         the computed value
  ///
  /// The interpreter is lazily constructed on first use. On every invocation,
  /// the interpreter is rebound to the stored individual to restore the
  /// required execution invariant.
  ///
  /// This design ensures that the class remains safely copyable and movable,
  /// while keeping interpreter management strictly local to this method.
  template<class ...Args> [[nodiscard]] value_t run(Args && ...args) const
  {
    if (int_) [[likely]]
      int_->rebind(ind_);
    else
      int_.emplace(ind_);

    return int_->run(std::forward<Args>(args)...);
  }

  /// Checks whether the stored individual is valid.
  ///
  /// \return `true` if the individual is valid, `false` otherwise
  [[nodiscard]] bool is_valid() const
  {
    return ind_.is_valid();
  }

  /// Serialises the stored individual.
  ///
  /// \param[out] out output stream
  /// \return         `true` on success, false otherwise
  ///
  /// The interpreter state is not serialised.
  bool save(std::ostream &out) const { return ind_.save(out); }

private:
  gp::individual ind_;

  // Lazily initialised interpreter cache.
  //
  // This member is mutable to allow lazy construction and rebinding in
  // `run()`, even though execution does not conceptually modify the logical
  // state of the object.
  //
  // The interpreter is never accessed outside `run()`.
  mutable std::optional<src::interpreter> int_ {};
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
template<gp::Team T, bool S>
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
