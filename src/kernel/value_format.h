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

#if !defined(ULTRA_VALUE_FORMAT_H)
#define      ULTRA_VALUE_FORMAT_H

#include "kernel/value.h"
#include "kernel/nullary.h"
#include "kernel/gp/src/variable.h"
#include "utility/misc.h"   // for as_integer(E)

#include <format>
#include <string_view>
#include <type_traits>

namespace ultra::detail
{

// Minimal `std::quoted`-like escaping for " and \.
inline auto format_quoted(std::format_context &ctx, std::string_view s)
  -> decltype(ctx.out())
{
  auto out(ctx.out());
  *out++ = '"';

  for (auto ch : s)
    if (ch == '"' || ch == '\\')
    {
      *out++ = '\\';
      *out++ = ch;
    }
    else
      *out++ = ch;

  *out++ = '"';
  return out;
}

}  // namespace ultra::detail


template<>
struct std::formatter<ultra::value_t>
{
  constexpr auto parse(std::format_parse_context &ctx)
  {
    // Only allow "{}" (no custom specifiers).
    auto it(ctx.begin());
    if (it != ctx.end() && *it != '}')
      throw std::format_error("Invalid format specifier for ultra::value_t");
    return it;
  }

  auto format(const ultra::value_t &v, std::format_context &ctx) const
  {
    return std::visit(
      [&ctx](const auto &x) -> decltype(ctx.out())
      {
        using T = std::remove_cvref_t<decltype(x)>;

        if constexpr (std::is_same_v<T, ultra::D_VOID>)
          return std::format_to(ctx.out(), "{{}}");
        else if constexpr (std::is_same_v<T, ultra::D_INT>
                           || std::is_same_v<T, ultra::D_DOUBLE>)
          return std::format_to(ctx.out(), "{}", x);
        else if constexpr (std::is_same_v<T, ultra::D_STRING>)
          return ultra::detail::format_quoted(ctx, x);
        else if constexpr (std::is_same_v<T, const ultra::D_NULLARY*>)
          return x ? std::format_to(ctx.out(), "{}", x->to_string())
                   : std::format_to(ctx.out(), "<nullary:null>");
        else if constexpr (std::is_same_v<T, ultra::D_ADDRESS>)
          return std::format_to(ctx.out(), "[{}]", ultra::as_integer(x));
        else if constexpr (std::is_same_v<T, const ultra::D_VARIABLE *>)
          return x ? std::format_to(ctx.out(), "{}", x->to_string())
                   : std::format_to(ctx.out(), "<var:null>");
        else if constexpr (std::is_same_v<T, ultra::D_IVECTOR>)
        {
          auto out(ctx.out());
          *out++ = '{';

          if (!x.empty())
          {
            out = std::format_to(out, "{}", x[0]);
            for (std::size_t i(1); i < x.size(); ++i)
              out = std::format_to(out, " {}", x[i]);
          }

          *out++ = '}';
          return out;
        }
        else
          static_assert(!sizeof(T), "Unhandled alternative in ultra::value_t");
      },
      v);
  }
};

#endif  // include guard
