/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_INDIVIDUAL_FORMAT_H)
#define      ULTRA_INDIVIDUAL_FORMAT_H

#include "kernel/individual.h"

#include <algorithm>
#include <format>
#include <iosfwd>

namespace ultra
{

std::ostream &operator<<(std::ostream &, const individual &);

namespace out
{

print_format_t print_format_flag(std::ostream &);

class print_format
{
public:
  explicit print_format(print_format_t t) : t_(t) {}

  friend std::ostream &operator<<(std::ostream &, print_format);

private:
  print_format_t t_;
};

std::ostream &c_language(std::ostream &);
std::ostream &cpp_language(std::ostream &);
std::ostream &dump(std::ostream &);
std::ostream &graphviz(std::ostream &);
std::ostream &in_line(std::ostream &);
std::ostream &list(std::ostream &);
std::ostream &python_language(std::ostream &);
std::ostream &tree(std::ostream &);

}  // namespace out
}  // namespace ultra

namespace std
{

template<> struct formatter<ultra::individual, char>
{
  ultra::out::print_format_t fmt_ = ultra::out::list_f;

  [[nodiscard]] constexpr auto parse(std::format_parse_context &ctx)
  {
    auto it(ctx.begin());
    const auto end(ctx.end());

    if (it == end || *it == '}')
      return it;

    const auto consume([&]<std::size_t N>(const char (&s)[N],
                                          ultra::out::print_format_t f)
    {
      auto probe(it);

      for (std::size_t i(0); i < N - 1; ++i)
      {
        if (probe == end || *probe != s[i])
          return false;
        ++probe;
      }

      fmt_ = f;
      it = probe;
      return true;
    });

    // Test longer prefixes first (remember "cpp" starts with "c", so `{:cpp}`
    // will match "c" first, then fail on trailing characters "pp").
    if (consume("graphviz", ultra::out::graphviz_f)
        || consume("python", ultra::out::python_language_f)
        || consume("inline", ultra::out::in_line_f)
        || consume("dump", ultra::out::dump_f)
        || consume("list", ultra::out::list_f)
        || consume("tree", ultra::out::tree_f)
        || consume("cpp", ultra::out::cpp_language_f)
        || consume("c", ultra::out::c_language_f))
    {
      if (it != end && *it != '}')
        throw std::format_error("invalid trailing format specifier for "
                                "ultra::individual");

      return it;
    }

    throw std::format_error("invalid format specifier for ultra::individual");
  }

  template<class FormatContext>
  [[nodiscard]] auto format(const ultra::individual &ind,
                            FormatContext &ctx) const
  {
    const auto s(ind.format(fmt_));
    return std::ranges::copy(s, ctx.out()).out;
  }
};

}  // namespace std

namespace ultra::internal
{

template<class Individual> struct derived_individual_formatter
{
  std::formatter<ultra::individual, char> formatter_;

  [[nodiscard]] constexpr auto parse(std::format_parse_context &ctx)
  {
    return formatter_.parse(ctx);
  }

  template<class FormatContext>
  [[nodiscard]] auto format(const Individual &ind, FormatContext &ctx) const
  {
    return formatter_.format(ind, ctx);
  }
};

}  // namespace ultra::internal

#endif  // include guard
