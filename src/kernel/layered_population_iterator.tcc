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
  {
    const auto layers(p.range_of_layers());

    end_ = layers.end();
    layer_ = begin ? layers.begin() : end_;
    pos_.layer_index = begin ? 0 : p.layers();

    if (begin)
      skip_empty_layers();
  }

  /// Prefix increment operator.
  /// \return iterator to the next individual
  /// \warning
  /// Advancing past the `end()` iterator results in undefined behaviour.
  base_iterator &operator++() noexcept
  {
    Expects(layer_ != end_);

    if (++pos_.individual_coord >= layer_->size())
    {
      ++layer_;
      ++pos_.layer_index;
      pos_.individual_coord = 0;

      skip_empty_layers();
    }

    assert((layer_ != end_ && pos_.individual_coord < layer_->size())
           || (layer_ == end_ && pos_.individual_coord == 0));

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

  /// \return `true` if iterators point to corresponding individuals
  ///
  /// Equality is intentionally defined only in terms of the current layer
  /// iterator and offset within that layer. We do not default this comparison
  /// because cached implementation details that do not identify the position
  /// could make two independently-created iterators to the same individual
  /// compare unequal, violating the multi-pass guarantee required by forward
  /// iterators.
  [[nodiscard]] bool operator==(const base_iterator &other) const noexcept
  {
    return layer_ == other.layer_
           && pos_.individual_coord == other.pos_.individual_coord;
  }

  [[nodiscard]] std::size_t uid() const noexcept
  {
    Expects(layer_ != end_);
    return layer_->uid();
  }

  [[nodiscard]] typename layered_population<I>::coord coord() const noexcept
  {
    Expects(layer_ != end_);
    Expects(pos_.individual_coord < layer_->size());
    return pos_;
  }

  /// \return reference to the current individual
  [[nodiscard]] ref operator*() const noexcept
  {
    Expects(layer_ != end_);
    Expects(pos_.individual_coord < layer_->size());
    return (*layer_)[pos_.individual_coord];
  }

  /// \return pointer to the current individual
  [[nodiscard]] ptr operator->() const noexcept
  {
    return &operator*();
  }

  friend std::ostream &operator<<(std::ostream &out, const base_iterator &i)
  {
    out << '[' << i.pos_.layer_index << ',' << i.pos_.individual_coord << ']';
    return out;
  }

private:
  void skip_empty_layers() noexcept
  {
    while (layer_ != end_ && layer_->empty())
    {
      ++layer_;
      ++pos_.layer_index;
    }
  }

  itr end_ {};
  itr layer_ {};
  layered_population<I>::coord pos_ {0, 0};
};

#endif  // include guard
