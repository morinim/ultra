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

#if !defined(ULTRA_DISTRIBUTION_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_DISTRIBUTION_TCC)
#define      ULTRA_DISTRIBUTION_TCC

///
/// Resets gathered statics.
///
template<ArithmeticFloatingType T>
void distribution<T>::clear()
{
  *this = distribution();
}

///
/// \return number of elements of the distribution
///
template<ArithmeticFloatingType T>
std::size_t distribution<T>::size() const
{
  return size_;
}

///
/// \return the maximum value of the distribution
///
template<ArithmeticFloatingType T>
T distribution<T>::max() const
{
  Expects(size());
  return max_;
}

///
/// \return the minimum value of the distribution
///
template<ArithmeticFloatingType T>
T distribution<T>::min() const
{
  Expects(size());
  return min_;
}

///
/// \return the mean value of the distribution
///
template<ArithmeticFloatingType T>
T distribution<T>::mean() const
{
  Expects(size());
  return mean_;
}

///
/// \return the variance of the distribution
///
template<ArithmeticFloatingType T>
T distribution<T>::variance() const
{
  Expects(size());
  return m2_ / static_cast<double>(size());
}

///
/// Add a new value to the distribution.
///
/// \param[in] val new value upon which statistics are recalculated
///
/// \remark
/// Function ignores NAN values.
///
template<ArithmeticFloatingType T>
void distribution<T>::add(T val)
{
  using std::isnan;
  if (isnan(val))
    return;

  if (!size())
    min_ = max_ = mean_ = val;
  else if (val < min())
    min_ = val;
  else if (val > max())
    max_ = val;

  ++size_;

  ++seen_[round_to(val)];
  update_variance(val);
}

template<ArithmeticFloatingType T>
const std::map<T, std::uintmax_t> &distribution<T>::seen() const
{
  return seen_;
}

///
/// \return the entropy of the distribution
///
/// \f$H(X)=-\sum_{i=1}^n p(x_i) \dot log_b(p(x_i))\f$
///
/// We use an offline algorithm
/// (http://en.wikipedia.org/wiki/Online_algorithm).
///
template<ArithmeticFloatingType T>
double distribution<T>::entropy() const
{
  const double c(1.0 / std::log(2.0));

  double h(0.0);
  for (const auto &f : seen())  // f.first: fitness, f.second: sightings
  {
    const auto p(static_cast<double>(f.second) / static_cast<double>(size()));

    h -= p * std::log(p) * c;
  }

  return h;
}

///
/// \param[in] val new value upon which statistics are recalculated
///
/// Calculate running variance and cumulative average of a set. The
/// algorithm used is due to Knuth (Donald E. Knuth - The Art of Computer
/// Programming, volume 2: Seminumerical Algorithms, 3rd edn., p. 232.
/// Addison-Wesley).
///
/// \see
/// - https://en.wikipedia.org/wiki/Online_algorithm
/// - https://en.wikipedia.org/wiki/Moving_average#Cumulative_average
///
template<ArithmeticFloatingType T>
void distribution<T>::update_variance(T val)
{
  Expects(size());

  const auto c1(static_cast<double>(size()));

  const T delta(val - mean());
  mean_ += delta / c1;

  // This expression uses the new value of mean.
  if (size() > 1)
    m2_ += delta * (val - mean());
  else
    m2_ =  delta * (val - mean());
}

///
/// \return the standard deviation of the distribution
///
template<ArithmeticFloatingType T>
T distribution<T>::standard_deviation() const
{
  // This way, for "regular" types we'll use std::sqrt ("taken in" by the
  // using statement), while for our types the overload will prevail due to
  // Koenig lookup (http://www.gotw.ca/gotw/030.htm).
  using std::sqrt;

  return sqrt(variance());
}

///
/// Saves the distribution on persistent storage.
///
/// \param[out] out output stream
/// \return         true on success
///
template<ArithmeticFloatingType T>
bool distribution<T>::save(std::ostream &out) const
{
  SAVE_FLAGS(out);

  out << size() << '\n'
      << std::fixed << std::scientific
      << std::setprecision(std::numeric_limits<T>::digits10 + 1)
      << mean() << '\n'
      << min() << '\n'
      << max() << '\n'
      << m2_ << '\n';

  out << seen().size() << '\n';
  for (const auto &elem : seen())
    out << elem.first << ' ' << elem.second << '\n';

  return out.good();
}

///
/// Loads the distribution from persistent storage.
///
/// \param[in] in input stream
/// \return       true on success
///
/// \note
/// If the load operation isn't successful the current object isn't modified.
///
template<ArithmeticFloatingType T>
bool distribution<T>::load(std::istream &in)
{
  SAVE_FLAGS(in);

  decltype(size_) c;
  if (!(in >> c))
    return false;

  in >> std::fixed >> std::scientific
     >> std::setprecision(std::numeric_limits<T>::digits10 + 1);

  decltype(mean_) m;
  if (!(in >> m))
    return false;

  decltype(min_) mn;
  if (!(in >> mn))
    return false;

  decltype(max_) mx;
  if (!(in >> mx))
    return false;

  decltype(m2_) m2__;
  if (!(in >> m2__))
    return false;

  typename decltype(seen_)::size_type n;
  if (!(in >> n))
    return false;

  decltype(seen_) s;
  for (decltype(n) i(0); i < n; ++i)
  {
    typename decltype(seen_)::key_type key;
    typename decltype(seen_)::mapped_type val;
    if (!(in >> key >> val))
      return false;

    s[key] = val;
  }

  size_ = c;
  mean_ = m;
  min_ = mn;
  max_ = mx;
  m2_ = m2__;
  seen_ = s;

  return true;
}

///
/// \return `true` if the object passes the internal consistency check
///
template<ArithmeticFloatingType T>
bool distribution<T>::is_valid() const
{
  // This way, for "regular" types we'll use std::infinite / std::isnan
  // ("taken in" by the using statement), while for our types the overload
  // will prevail due to Koenig lookup (http://www.gotw.ca/gotw/030.htm).
  using std::isfinite;
  using std::isnan;

  if (size() && isfinite(min()) && isfinite(mean()) && min() > mean())
  {
    ultraERROR << "Distribution: min=" << min() << " > mean=" << mean();
    return false;
  }

  if (size() && isfinite(max()) && isfinite(mean()) && max() < mean())
  {
    ultraERROR << "Distribution: max=" << max() << " < mean=" << mean();
    return false;
  }

  if (size() && (isnan(variance()) || !isnonnegative(variance())))
  {
    ultraERROR << "Distribution: negative variance";
    return false;
  }

  return true;
}

#endif  // include guard
