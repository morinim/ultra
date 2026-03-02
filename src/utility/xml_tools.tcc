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

#if !defined(ULTRA_XML_TOOLS_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_XML_TOOLS_TCC)
#define      ULTRA_XML_TOOLS_TCC

///
/// A convenient arrangement for inserting stream-aware objects into
/// `XMLPrinter`.
///
/// \tparam T type of the value
///
/// \param[out] p output device
/// \param[in]  e XML element name
/// \param[in]  v XML element value
///
template<class T>
void set_text(tinyxml2::XMLPrinter &p, const char *e, const T &v)
{
  Expects(e);
  xml_closer element(p, e);

  std::string s;

  if constexpr (requires { std::string_view{v}; })
    s = std::string_view(v);  // safe for views/substrings
  else
  {
    std::ostringstream ss;
    ss << v;
    s = ss.str();
  }

  p.PushText(s.c_str());
}

///
/// Sets an XML attribute from an integral value in a portable way.
///
/// \param[out] p    output device
/// \param[in]  name attribute name (must not be null and must be a valid C
///                  string)
/// \param[in]  v    attribute value
///
/// tinyxml2 does not provide `PushAttribute` overloads for `std::size_t` (and
/// other platform-dependent integral types). Depending on the platform and
/// compiler, passing such types directly may lead to ambiguous overload
/// resolution.
///
/// This helper normalises all integral values to a well-defined 64-bit signed
/// or unsigned type before calling tinyxml2, ensuring consistent behaviour
/// across toolchains (e.g. Apple Clang vs GCC/Clang on Linux).
///
template<std::integral T>
inline void set_attr(tinyxml2::XMLPrinter &p, const char *name, T v)
{
  Expects(name);

  if constexpr (std::is_signed_v<T>)
    p.PushAttribute(name, static_cast<std::int64_t>(v));
  else
    p.PushAttribute(name, static_cast<std::uint64_t>(v));
}

///
/// Sets an XML attribute from a floating point value in a portable way.
///
/// \param[out] p    output device
/// \param[in]  name attribute name (must not be null and must be a valid C
///                  string)
/// \param[in]  v    attribute value
///
/// This helper provides a simple and uniform way to assign floating point
/// attributes to a tinyxml2 printer. It complements the integral overload of
/// `set_attr()` and encourages consistent attribute handling throughout the
/// codebase.
///
template<std::floating_point T>
inline void set_attr(tinyxml2::XMLPrinter &p, const char *name, T v)
{
  Expects(name);

  p.PushAttribute(name, static_cast<double>(v));
}

///
/// Sets an XML attribute from a string value.
///
/// \param[out] p    output device
/// \param[in]  name attribute name (must not be null and must be a valid C
///                  string)
/// \param[in]  v    attribute value
///
/// This helper provides a simple and uniform way to assign string attributes
/// to a tinyxml2 printer. It complements the integral overload of `set_attr()`
/// and encourages consistent attribute handling throughout the codebase.
///
inline void set_attr(tinyxml2::XMLPrinter &p, const char *name,
                     const std::string &v)
{
  Expects(name);
  p.PushAttribute(name, v.c_str());
}

#endif  // include guard
