/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_DATAFRAME_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_DATAFRAME_TCC)
#define      ULTRA_DATAFRAME_TCC

template<class RR> concept RangeOfSizedRanges =
  std::ranges::range<RR>
  && std::ranges::sized_range<std::ranges::range_value_t<RR>>;

namespace internals
{

[[nodiscard]] inline value_t lexical_cast(value_t v, domain_t d)
{
  switch (d)
  {
  case d_int:    return lexical_cast<D_INT>(v);
  case d_double: return lexical_cast<D_DOUBLE>(v);
  case d_string: return lexical_cast<D_STRING>(v);
  default:  throw exception::data_format("Invalid domain for lexical_cast");
  }
}

template<class T>
void inplace_trim(T &v)
{
  if constexpr (std::same_as<T, std::string>)
    v = trim(v);

  if constexpr (std::same_as<T, value_t>)
    if (auto *p = std::get_if<D_STRING>(v))
      *p = trim(*p);
}

}  // namespace internals

///
/// \param[in] r            a range containing the example
/// \param[in] add_instance should we automatically add instances for text
///                         features?
/// \return                 `r` converted to `example` type
///
/// \remark
/// When `add_instance` is `true` the function can have side-effects (changing
/// the set of admissible instances associated with a text-feature).
///
template<std::ranges::sized_range R>
example dataframe::to_example(const R &r, bool add_instance)
{
  Expects(v.size());
  Expects(v.size() == columns.size());

  example ret;

  for (std::size_t i(0); auto feature : r)
  {
    if (const auto domain(columns[i].domain()); domain != d_void)
    {
      assert(basic_data_type(domain));
      internals::inplace_trim(feature);

      if (i == 0)
      {
        if (!is_number(lexical_cast<D_STRING>(feature)))  // classification task
          ret.output = static_cast<D_INT>(encode(feature));
        else  // symbolic regression
          ret.output = internals::lexical_cast(feature, domain);
      }
      else  // input value
        ret.input.push_back(internals::lexical_cast(feature, domain));

      if (add_instance && domain == d_string)
        columns[i].add_state(lexical_cast<D_STRING>(feature));
    }

    ++i;
  }

  return ret;
}
/*
///
/// Loads a matrix into the dataframe.
///
/// \param[in] container input data
/// \param[in] p         additional, optional, parameters (see `params`
///                      structure). Only `p.data_typing` and `p.output_index`
///                      are used
/// \return              number of elements parsed (0 in case of error)
///
template<RangeOfSizedRanges R>
std::size_t dataframe::read_table(const R &container, params &p)
{
  columns.data_typing(p.data_typing);

  clear();

  std::size_t count(0);
  for (auto record : container)
  {
    if (p.output_index)
    {
      if (p.output_index == params::index::back)
        p.output_index = record.size() - 1;

      assert(p.output_index < record.size());

      if (p.output_index > 0)
        std::rotate(record.begin(),
                    std::next(record.begin(), *p.output_index),
                    std::next(record.begin(), *p.output_index + 1));
    }
    else
      // When the output index is unspecified, all the columns are treated as
      // input columns (this is obtained adding a surrogate, empty output
      // column).
      record.insert(record.begin(), "");

    read_record(record, true);

    ++count;
  }

  if (!is_valid() || !size())
    throw exception::insufficient_data("Empty / undersized CSV data file");

  return size();
}
*/
#endif  // include guard
