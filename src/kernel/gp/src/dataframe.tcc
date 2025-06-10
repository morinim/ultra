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

namespace internal
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

}  // namespace internal

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
template<std::ranges::range R>
example dataframe::to_example(const R &r, bool add_instance)
{
  Expects(r.size());
  Expects(static_cast<std::size_t>(std::ranges::distance(r)) == columns.size());

  example ret;

  for (std::size_t i(0); auto feature : r)
  {
    if (const auto domain(columns[i].domain()); domain != d_void)
    {
      assert(basic_data_type(domain));
      internal::inplace_trim(feature);

      if (i == 0)
      {
        if (!is_number(lexical_cast<D_STRING>(feature)))  // classification task
          ret.output = static_cast<D_INT>(encode(feature));
        else  // symbolic regression
          ret.output = internal::lexical_cast(feature, domain);
      }
      else  // input value
        ret.input.push_back(internal::lexical_cast(feature, domain));

      if (add_instance && domain == d_string)
        columns[i].add_state(lexical_cast<D_STRING>(feature));
    }

    ++i;
  }

  return ret;
}

///
/// \param[in] r            an example in raw format
/// \param[in] output_index index of the output column
/// \param[in] add_instance should we automatically add instances for text
///                         features?
/// \return                 `true` for a correctly converted/imported record
///
template<std::ranges::range R>
bool dataframe::read_record(R r, std::optional<std::size_t> output_index,
                            bool add_instance)
{
  Expects(!r.empty());
  Expects(!output_index || *output_index < std::ranges::distance(r));

  r = internal::output_column_first(r, output_index);

  // Skip lines with wrong number of columns.
  if (static_cast<std::size_t>(std::ranges::distance(r)) != columns.size())
  {
    ultraWARNING << "Malformed exampled " << size() <<  " skipped";
    return false;
  }

  push_back(to_example(r, add_instance));
  return true;
}


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
std::size_t dataframe::read_table(const R &container, const params &p)
{
  columns.data_typing(p.data_typing);

  clear();

  if (container.size() > 1)
  {
    if (p.output_index == params::index::back)
      p.output_index = container.front().size() - 1;

    columns.build(container, p.output_index);
  }
  else
    return 0;

  for (auto it(std::next(container.cbegin())); it != container.end(); ++it)
    read_record(*it, p.output_index, true);

  if (!is_valid())
    throw exception::insufficient_data("Empty / undersized CSV data file");

  return size();
}

#endif  // include guard
