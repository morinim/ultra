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

#if !defined(ULTRA_LAYERED_POPULATION_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_LAYERED_POPULATION_ITERATOR_TCC)
#define      ULTRA_LAYERED_POPULATION_ITERATOR_TCC

///
/// Iterator for scanning individuals of a layered population.
///
template<Individual I>
template<bool is_const>
class layered_population<I>::base_iterator
{
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = I;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;
  using difference_type = std::ptrdiff_t;

  using ptr = std::conditional_t<is_const, const_pointer, pointer>;
  using ref = std::conditional_t<is_const, const_reference, reference>;
  using pop = std::conditional_t<is_const,
                                 const layered_population, layered_population>;
  using itr = std::conditional_t<is_const,
                                 typename pop::layer_const_iter,
                                 typename pop::layer_iter>;

  // A requirement for `std::input_iterator` is that it must be
  // default-initializable.
  base_iterator() noexcept = default;

  /// \param[in] p     a population
  /// \param[in] begin `false` for the `end()` iterator
  base_iterator(pop &p, bool begin) noexcept
    : begin_(p.range_of_layers().begin()),
      end_(p.range_of_layers().end()),
    layer_(begin ? begin_ : end_)
  {
  }

  /// Prefix increment operator.
  /// \return iterator to the next individual
  /// \warning
  /// Advancing past the `end()` iterator results in undefined behaviour.
  base_iterator &operator++() noexcept
  {
    if (++index_ >= layer_->size())
    {
      index_ = 0;

      do  // skipping empty layers
        ++layer_;
      while (layer_ != end_ && layer_->empty());
    }

    assert((layer_ != end_ && index_ < layer_->size())
           || (layer_ == end_ && index_ == 0));

    return *this;
  }

  /// Postfix increment operator.
  /// \return iterator to the current individual
  base_iterator operator++(int) noexcept
  {
    base_iterator tmp(*this);
    operator++();
    return tmp;
  }

  /// \return `true` if iterators point to correspondant individuals
  [[nodiscard]] bool operator==(const base_iterator &) const noexcept = default;

  [[nodiscard]] std::size_t uid() const noexcept
  {
    return layer_->uid();
  }

  /// \return reference to the current individual
  [[nodiscard]] ref operator*() const noexcept
  {
    return (*layer_)[index_];
  }

  /// \return pointer to the current individual
  [[nodiscard]] ptr operator->() const noexcept
  {
    return &operator*();
  }

  friend std::ostream &operator<<(std::ostream &out, const base_iterator &i)
  {
    out << '[' << i.uid() << ',' << i.index_ << ']';
    return out;
  }

private:
  itr begin_ {}, end_ {};
  itr layer_ {};
  std::size_t index_ {0};
};

#endif  // include guard
