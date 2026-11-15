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

#include "kernel/gp/src/oracle.h"

#include <limits>

namespace ultra::src
{

/// Constructs a classifier with optional class names.
///
/// \param[in] names class names, or an empty vector for numeric names
basic_class_oracle::basic_class_oracle(std::vector<std::string> names)
  : names_(std::move(names))
{
}

/// Returns the predicted class label.
value_t basic_class_oracle::operator()(const std::vector<value_t> &e) const
{
  return static_cast<D_INT>(tag(e).label);
}

/// Returns the class name, falling back to a numeric name for unknown IDs.
std::string basic_class_oracle::name(const value_t &a) const
{
  if (const auto id(lexical_cast<D_INT>(a));
      0 <= id && static_cast<std::size_t>(id) < names_.size())
    return names_[id];

  return lexical_cast<std::string>(a);
}

/// Collects the class names from the training set.
std::vector<std::string> basic_class_oracle::class_names(const dataframe &d)
{
  Expects(d.classes() > 1);

  std::vector<std::string> names(d.classes());
  for (std::size_t i(0); i < names.size(); ++i)
    names[i] = d.class_name(i);

  return names;
}

/// Loads the class names, accepting a zero count for omitted names.
bool basic_class_oracle::load_names(std::istream &in)
{
  std::size_t n;
  if (!(in >> n))
    return false;

  in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  std::vector<std::string> names(n);
  for (auto &name : names)
    if (!std::getline(in, name))
      return false;

  names_ = std::move(names);
  return true;
}

/// Saves a count followed by one class name per line.
bool basic_class_oracle::save_names(std::ostream &out) const
{
  if (!(out << names_.size() << '\n'))
    return false;

  for (const auto &name : names_)
    if (!(out << name << '\n'))
      return false;

  return out.good();
}

}  // namespace ultra::src

namespace ultra::src::serialize
{

///
/// Saves an oracle on persistent storage.
///
/// \param[in] out output stream
/// \param[in] ora oracle
/// \return        `true` on success
///
bool save(std::ostream &out, const basic_oracle &ora)
{
  out << ora.serialize_id() << '\n';;
  return ora.save(out);
}

bool save(std::ostream &out, const basic_oracle *ora)
{
  Expects(ora);
  return save(out, *ora);
}

bool save(std::ostream &out, const std::unique_ptr<basic_oracle> &ora)
{
  return save(out, ora.get());
}

namespace oracle::internal
{

std::map<std::string, build_func> factory_;

}  // namespace oracle::internal

}  // namespace ultra::src::serialize
