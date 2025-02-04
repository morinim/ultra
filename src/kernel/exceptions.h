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

#if !defined(ULTRA_EXCEPTIONS_H)
#define      ULTRA_EXCEPTIONS_H

#include <stdexcept>

///
/// Groups the custom exceptions used in Ultra.
///
/// There is a separate header because one can ignore custom exception types
/// (since they are descendants of `std::exception`) in general case, but can
/// also explicitly include the header and deal with custom exceptions if
/// necessary.
///
namespace ultra::exception
{

class data_format : public std::runtime_error
{ using std::runtime_error::runtime_error; };
class insufficient_data : public std::logic_error
{ using std::logic_error::logic_error; };

}  // namespace ultra::exception

#endif  // include guard
