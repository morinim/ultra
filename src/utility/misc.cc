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

#include <atomic>
#include <thread>

#include "utility/misc.h"
#include "kernel/nullary.h"

namespace ultra
{
///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparison
/// \return        `true` if all elements in both strings are same (case
///                insensitively)
///
bool iequals(const std::string &lhs, const std::string &rhs)
{
  return std::ranges::equal(lhs, rhs,
                            [](auto c1, auto c2)
                            { return std::tolower(c1) == std::tolower(c2); });
}

///
/// \param[in] s the string to be tested
/// \return      `true` if `s` contains a number
///
bool is_number(std::string s)
{
  s = trim(s);

  char *end;
  const double val(std::strtod(s.c_str(), &end));  // if no conversion can be
                                                   // performed, `end` is set
                                                   // to `s.c_str()`
  return end != s.c_str() && *end == '\0' && std::isfinite(val);
}

///
/// \param[in] s the input string
/// \return      a copy of `s` with spaces removed on both sides of the string
///
/// \see http://stackoverflow.com/a/24425221/3235496
///
std::string trim(const std::string &s)
{
  auto front = std::ranges::find_if_not(
    s, [](auto c) { return std::isspace(c); });

  // The search is limited in the reverse direction to the last non-space value
  // found in the search in the forward direction.
  auto back = std::find_if_not(s.rbegin(), std::make_reverse_iterator(front),
                               [](auto c) { return std::isspace(c); }).base();

  return {front, back};
}

///
/// Replaces the first occurrence of a string with another string.
///
/// \param[in] s    input string
/// \param[in] from substring to be searched for
/// \param[in] to   substitute string
/// \return         the modified input
///
std::string replace(std::string s,
                    const std::string &from, const std::string &to)
{
  const auto start_pos(s.find(from));
  if (start_pos != std::string::npos)
    s.replace(start_pos, from.length(), to);

  return s;
}

///
/// Replaces all occurrences of a string with another string.
///
/// \param[in] s    input string
/// \param[in] from substring to be searched for
/// \param[in] to   substitute string
/// \return         the modified input
///
std::string replace_all(std::string s,
                        const std::string &from, const std::string &to)
{
  if (!from.empty())
  {
    std::size_t start(0);
    while ((start = s.find(from, start)) != std::string::npos)
    {
      s.replace(start, from.length(), to);
      start += to.length();  // in case `to` contains `from`, like replacing
                             // "x" with "yx"
    }
  }

  return s;

  // With std::regex it'd be something like:
  //     s = std::regex_replace(s, std::regex(from), to);
  // (possibly escaping special characters in the `from` string)
}

///
/// Converts a `value_t` to `double`.
///
/// \param[in] v value that should be converted to `double`
/// \return      the result of the conversion of `v`. If the conversion cannot
///              be performed returns `0.0`
///
/// This function is useful for:
/// * debugging purpose;
/// * symbolic regression and classification task (the value returned by
///   the interpreter will be used in a "numeric way").
///
/// \note
/// This is not the same of `std::get<T>(v)`.
///
template<>
double lexical_cast<double>(const ultra::value_t &v)
{
  using namespace ultra;

  switch (v.index())
  {
  case d_double:  return std::get<D_DOUBLE>(v);
  case d_int:     return std::get<D_INT>(v);
  case d_string:  return lexical_cast<double>(std::get<D_STRING>(v));
  default:        return 0.0;
  }
}

template<>
int lexical_cast<int>(const ultra::value_t &v)
{
  using namespace ultra;

  switch (v.index())
  {
  case d_double:  return static_cast<int>(std::get<D_DOUBLE>(v));
  case d_int:     return std::get<D_INT>(v);
  case d_string:  return lexical_cast<int>(std::get<D_STRING>(v));
  default:        return 0;
  }
}

///
/// Converts a `value_t` to `std::string`.
///
/// \param[in] v value that should be converted to `std::string`
/// \return      the result of the conversion of `v`. If the conversion cannot
///              be performed returns an empty string
///
/// This function is useful for debugging purpose.
///
template<>
std::string lexical_cast<std::string>(const ultra::value_t &v)
{
  using namespace ultra;

  switch (v.index())
  {
  case d_double:  return std::to_string(std::get<D_DOUBLE>(v));
  case d_int:     return std::to_string(   std::get<D_INT>(v));
  case d_string:  return std::get<D_STRING>(v);
  case d_nullary: return get_if_nullary(v)->name();
  default:        return {};
  }
}

template<>
std::string lexical_cast<std::string>(std::chrono::milliseconds d)
{
  using namespace std;
  using namespace std::chrono_literals;

  const auto ds(chrono::duration_cast<chrono::days>(d));
  const auto hrs(chrono::duration_cast<chrono::hours>(d - ds));
  const auto mins(chrono::duration_cast<chrono::minutes>(d - ds - hrs));
  const auto secs(chrono::duration_cast<chrono::seconds>(d - ds - hrs - mins));

  std::stringstream ss;

  if (ds.count())
    ss << ds.count() << ':';
  if (ds.count() || hrs.count())
    ss << std::setw(2) << std::setfill('0') << hrs.count() << ':';
  if (ds.count() || hrs.count() || mins.count())
    ss << std::setw(2) << std::setfill('0') << mins.count() << ':';
  if (ds.count() || hrs.count() || mins.count())
    ss << std::setw(2);

  ss << std::setfill('0') << secs.count();

  if (ds.count() == 0 && hrs.count() == 0 && mins.count() == 0)
  {
    const auto ms(chrono::duration_cast<chrono::milliseconds>(d - ds - hrs
                                                              - mins - secs));
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
  }

  return ss.str();
}

app_level_uid::operator unsigned() const noexcept
{
  return val_;
}

unsigned app_level_uid::next_id() noexcept
{
  // The initialisation is thread-safe because it's static and C++11 guarantees
  // that static initialisation will be thread-safe. Subsequent access will be
  // thread-safe because it's an atomic.
  static std::atomic<unsigned> count(0);

  return count++;
}

namespace lock_file
{
/*
namespace internal
{

bool remove_file(const std::filesystem::path &f)
{
  namespace fs = std::filesystem;

  std::error_code err_cod;
  if (fs::remove(f, err_cod))
    return true;

  if (err_cod)
  {
    fs::remove(f, err_cod);
    return false;
  }
}

}*/

///
/// Checks if a lock file exists and is not stale.
///
/// Stale condition is determined by comparing the last write time of the lock
/// file with to the current time.
///
/// \remark
/// If the lock file is stale, this function deletes it.
///
bool is_valid(const std::filesystem::path &lock_file,
              std::chrono::seconds lock_timeout)
{
  namespace fs = std::filesystem;

  if (!fs::exists(lock_file))
    return false;

  std::error_code err_cod;
  const auto last_write_time(fs::last_write_time(lock_file, err_cod));
  if (err_cod)
    return false;

  const auto current_time(std::chrono::file_clock::now());

  // Lock file is stale. Removing it.
  if (current_time - last_write_time > lock_timeout)
  {
    fs::remove(lock_file);
    return false;
  }

  return true;
}

///
/// Creates a write lock file.
///
/// The writer effectively blocks all new readers as soon as it starts its
/// operation, ensuring it gets precedence without conflicts.
///
void acquire_write(const std::filesystem::path &f)
{
  namespace fs = std::filesystem;

  // Create write lock.
  const auto write_lock_file(fs::path(f).replace_extension(".write.lock"));
  std::fstream(write_lock_file).close();

  // Wait for readers to finish.
  // After creating the write lock, the writer waits for all readers to finish
  // before proceeding. This ensures that existing readers are not interrupted,
  // but no new readers can start.
  const auto read_lock_file(fs::path(f).replace_extension(".read.lock"));
  while (is_valid(read_lock_file))
    std::this_thread::sleep_for(100ms);
}

///
/// Creates a read lock file.
///
/// Readers will gracefully step aside for the writer, allowing it to complete
/// its operation before resuming their own. This approach ensures minimal
/// contention and aligns well with scenarios where writing is more critical
/// than reading.
///
bool acquire_read(const std::filesystem::path &f)
{
  namespace fs = std::filesystem;

  // Check for write lock.
  const auto write_lock_file(fs::path(f).replace_extension(".write.lock"));
  if (is_valid(write_lock_file))
    return false;  // writer is active, aborting

  // Create read lock.
  const auto read_lock_file(fs::path(f).replace_extension(".read.lock"));
  std::fstream(read_lock_file).close();

  // Recheck for write lock.
  // When the writer creates the write lock, readers that check for the write
  // lock will immediately abort, ensuring the writer can proceed as soon as
  // it's ready.
  if (is_valid(write_lock_file))
  {
    fs::remove(read_lock_file);  // cleanup
    return false;
  }

  return true;
}

/// Removes a write lock file.
void release_write(std::filesystem::path f)
{
  namespace fs = std::filesystem;

  if (const auto lock_file(f.replace_extension(".write.lock"));
      fs::exists(lock_file))
    fs::remove(lock_file);
}

/// Removes a read lock file.
void release_read(std::filesystem::path f)
{
  namespace fs = std::filesystem;

  if (const auto lock_file(f.replace_extension(".read.lock"));
      fs::exists(lock_file))
    fs::remove(lock_file);
}

}  // namespace lock file

}  // namespace ultra
