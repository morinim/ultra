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

//
// A convenient arrangement for inserting stream-aware objects into
// `XMLPrinter`.
//
// \tparam T type of the value
//
// \param[out] p output device
// \param[in]  e new xml element
// \param[in]  v new xml element's value
//
template<class T>
void set_text(tinyxml2::XMLPrinter &p, const std::string &e, const T &v)
{
  std::string str_v;

  if constexpr (std::is_same_v<T, std::string>)
    str_v = v;
  else
  {
    std::ostringstream ss;
    ss << v;
    str_v = ss.str();
  }

  xml_closer element(p, e.c_str());
  p.PushText(str_v.c_str());
}

#endif  // include guard
