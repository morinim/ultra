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
/// Iterator for a population.
///
/// `layered_population<I>::base_iterator` / `layered_population<I>::begin()` /
/// `layered_population<I>::end()` are general and clear, so they should be the
/// preferred way to scan / perform an action over every individual of a
/// layered population.
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

  // A requirement for `std::input_iterator` is that it must be
  // default-initializable.
  base_iterator() noexcept = default;

  /// \param[in] p     a population
  /// \param[in] begin `false` for the `end()` iterator
  base_iterator(pop &p, bool begin) noexcept
    : pop_(&p), layer_(begin ? 0 : p.layers())
  {
  }

  /// Prefix increment operator.
  /// \return iterator to the next individual
  /// \warning
  /// Advancing past the `end()` iterator results in undefined behaviour.
  base_iterator &operator++() noexcept
  {
    if (++index_ >= pop_->layer(layer_).size())
    {
      index_ = 0;

      do  // skipping empty layers
        ++layer_;
      while (layer_ < pop_->layers() && pop_->layer(layer_).empty());
    }

    assert((layer_ < pop_->layers()
            && index_ < pop_->layer(layer_).size())
           || (layer_ == pop_->layers() && index_ == 0));

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

  /// \param[in] rhs second term of comparison
  /// \return        `true` if iterators point to correspondant individuals
  [[nodiscard]] bool operator==(const base_iterator &rhs) const noexcept
    = default;

  [[nodiscard]] std::size_t layer() const noexcept
  {
    return layer_;
  }

  [[nodiscard]] typename layered_population<I>::coord coord() const noexcept
  {
    return {layer_, index_};
  }


  /// \return reference to the current individual
  [[nodiscard]] ref operator*() const noexcept
  {
    return pop_->operator[]({layer_, index_});
  }

  /// \return pointer to the current individual
  [[nodiscard]] ptr operator->() const noexcept
  {
    return &operator*();
  }

  friend std::ostream &operator<<(std::ostream &out, const base_iterator &i)
  {
    out << '[' << i.layer_ << ',' << i.index_ << ']';
    return out;
  }

private:
  std::conditional_t<is_const,
                     const layered_population *,
                     layered_population *> pop_ {nullptr};

  std::size_t layer_ {0};
  std::size_t index_ {0};
};

#endif  // include guard
