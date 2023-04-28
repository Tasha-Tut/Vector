#include <cassert>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <memory>
#include <iostream>

#ifndef VECTOR_H
#define VECTOR_H

#ifndef VECTOR_MEMORY_IMPLEMENTED
#define VECTOR_MEMORY_IMPLEMENTED

namespace detail {
template <typename T>
void Construct(T *p, const T &other) {
  new (p) T(other);
}
template <typename T>
void Construct(T *p, T &&other) {
  new (p) T(std::move(other));
}
template <class T>
void Destroy(T *p) {
  p->~T();
}
template <typename FwdIter>
void Destroy(FwdIter first, FwdIter last) {
  while (first != last) {
    Destroy(&*first);
    first++;
  }
}
}  // namespace detail

template <typename T>
class VectorBuf {
  using ValueType = T;
  using Pointer = T *;
  using SizeType = std::size_t;

 protected:
  SizeType size_, used_ = 0;
  Pointer data_ = nullptr;
  explicit VectorBuf(SizeType size = 0)
      : size_(size)
      , data_(((size_ == 0) ? nullptr : static_cast<Pointer>(::operator new(sizeof(ValueType) * size)))) {  // NOLINT
  }

 public:
  VectorBuf(const VectorBuf &) = delete;
  VectorBuf &operator=(const VectorBuf &) = delete;

 protected:
  VectorBuf(VectorBuf &&other) noexcept : size_(other.size_), used_(other.used_), data_(other.data_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.used_ = 0;
  }
  VectorBuf &operator=(VectorBuf &&other) noexcept {
    if (&other == this) {
      return *this;
    }
    detail::Destroy(data_, data_ + used_);
    ::operator delete(data_);
    data_ = other.data_;
    used_ = other.used_;
    size_ = other.size_;
    other.data_ = nullptr;
    other.size_ = 0;
    other.used_ = 0;
    return *this;
  }
  ~VectorBuf() {
    detail::Destroy(data_, data_ + used_);
    ::operator delete(data_);
  }
};

template <typename T>
class Vector final : private VectorBuf<T> {
  // usings
 public:
  using ValueType = T;
  using Pointer = T *;
  using ConstPointer = const T *;
  using Reference = T &;
  using ConstReference = const T &;
  using SizeType = std::size_t;
  using Iterator = T *;
  using ConstIterator = const T *;
  using Base = VectorBuf<T>;
  using ReverseIterator = std::reverse_iterator<Iterator>;
  using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

 private:
  using Base::data_;
  using Base::size_;
  using Base::used_;

 public:
  // constructors
  explicit Vector(SizeType size = 0) : Base(size) {
    while (used_ < size_) {
      detail::Construct(data_ + used_, T{});
      used_++;
    }
  }
  Vector(SizeType size, ConstReference value) : Base(size) {
    std::uninitialized_fill(data_, data_ + size_, value);
    used_ = size_;
  }

  template <class Iterator, class = std::enable_if_t<std::is_base_of_v<
                                std::forward_iterator_tag, typename std::iterator_traits<Iterator>::iterator_category>>>
  Vector(Iterator first, Iterator last) : Base(std::distance(first, last)) {
    std::uninitialized_copy(first, last, data_);
    used_ = size_;
  }
  Vector(std::initializer_list<T> initlist) : Vector(initlist.begin(), initlist.end()) {
  }

 public:
  // rule 5
  Vector(Vector &&other) = default;
  Vector &operator=(Vector &&other) = default;
  ~Vector() = default;
  Vector(const Vector &other) : Base(other.used_) {
    while (used_ < other.used_) {
      detail::Construct(data_ + used_, other.data_[used_]);
      used_++;
    }
  }
  Vector &operator=(const Vector &other) {
    Vector tmp(other);
    std::swap(*this, tmp);
    return *this;
  }
  SizeType Size() const {
    return used_;
  }
  SizeType Capacity() const {
    return size_;
  }
  bool Empty() const {
    return (used_ == 0);
  }

  constexpr Reference operator[](SizeType index) noexcept {
    return data_[index];
  }
  constexpr ConstReference operator[](SizeType index) const noexcept {
    return data_[index];
  }
  constexpr Reference At(SizeType index) {
    if (index >= used_) {
      throw std::out_of_range{"Vector Out Of Range"};
    }
    return data_[index];
  }
  constexpr ConstReference At(SizeType index) const {
    if (index >= used_) {
      throw std::out_of_range{"Vector Out Of Range"};
    }
    return data_[index];
  }
  constexpr Reference Front() {
    return data_[0];
  }
  constexpr ConstReference Front() const {
    return data_[0];
  }
  constexpr Reference Back() {
    return data_[used_ - 1];
  }
  constexpr ConstReference Back() const {
    return data_[used_ - 1];
  }
  Pointer Data() {
    return data_;
  }

  ConstPointer Data() const {
    return data_;
  }
  void Swap(Vector &other) noexcept {
    std::swap(size_, other.size_);
    std::swap(used_, other.used_);
    std::swap(data_, other.data_);
  }

  void Resize(SizeType new_size) {
    if (new_size <= used_) {
      while (used_ != new_size) {
        PopBack();
      }
    } else if (new_size <= size_) {
      auto i = used_;
      try {
        while (i < new_size) {
          detail::Construct(data_ + i, ValueType{});
          i++;
        }
      } catch (...) {
        detail::Destroy(data_ + used_, data_ + i);
        throw;
      }
      used_ = new_size;
    } else {
      auto new_data = static_cast<Pointer>(::operator new(sizeof(ValueType) * new_size));
      SizeType i = 0;
      for (; i < used_; i++) {
        detail::Construct(new_data + i, std::move(data_[i]));
      }
      try {
        for (; i < new_size; i++) {
          detail::Construct(new_data + i, ValueType{});
        }
      } catch (...) {
        for (SizeType j = 0; j < used_; j++) {
          detail::Construct(data_ + j, std::move(new_data[j]));
        }
        detail::Destroy(new_data, new_data + i);
        ::operator delete(new_data);
        throw;
      }

      detail::Destroy(data_, data_ + used_);
      ::operator delete(data_);
      data_ = new_data;
      used_ = new_size;
      size_ = new_size;
    }
  }
  void Resize(SizeType count, ConstReference value) {
    if (count < used_) {
      while (used_ != count) {
        PopBack();
      }
    } else {
      while (used_ != count) {
        PushBack(value);
      }
    }
  }
  void Reserve(SizeType new_cap) {
    if (new_cap <= size_) {
      return;
    }

    auto new_data = static_cast<Pointer>(::operator new(sizeof(ValueType) * new_cap));
    for (SizeType i = 0; i < used_; i++) {
      detail::Construct(new_data + i, std::move(data_[i]));
    }

    detail::Destroy(data_, data_ + used_);
    ::operator delete(data_);
    size_ = new_cap;
    data_ = new_data;
  }
  void ShrinkToFit() {
    if (used_ == size_) {
      return;
    }
    Vector tmp{};
    tmp.Reserve(used_);
    while (tmp.used_ < used_) {
      tmp.PushBack(std::move(data_[tmp.used_]));
    }
    std::swap(*this, tmp);
  }
  void Clear() {
    detail::Destroy(data_, data_ + used_);
    used_ = 0;
  }

 private:
  // Help methoods
  bool NeedResizeUp() const {
    return (used_ == size_);
  }
  void ResizeUp() {
    Vector tmp{};
    tmp.Reserve(2 * size_ + 1);
    while (tmp.used_ < used_) {
      tmp.PushBack(std::move(data_[tmp.used_]));
    }
    std::swap(*this, tmp);
  }

 public:
  void PushBack(ConstReference value) {
    auto tmp{value};
    PushBack(std::move(tmp));
  }

  void PushBack(ValueType &&value) {
    if (NeedResizeUp()) {
      ResizeUp();
    }
    detail::Construct(data_ + used_, std::move(value));
    used_++;
  }
  void PopBack() {
    if (Empty()) {
      throw std::underflow_error{"Vector Is Empty"};
    }
    used_--;
    detail::Destroy(data_ + used_);
  }
  template <typename... Args>
  void EmplaceBack(Args &&... args) {
    if (NeedResizeUp()) {
      ResizeUp();
    }
    ::new (data_ + used_) ValueType(std::forward<Args>(args)...);
    used_++;
  }

  // operators
  bool operator>(const Vector<T> &other) const {
    size_t now_size = std::min(used_, other.used_);
    for (size_t i = 0; i < now_size; ++i) {
      if (data_[i] != other.data_[i]) {
        return (data_[i] > other.data_[i]);
      }
    }
    return (used_ > other.used_);
  }
  bool operator<(const Vector<T> &other) const {
    size_t now_size = std::min(used_, other.used_);
    for (size_t i = 0; i < now_size; ++i) {
      if (data_[i] != other.data_[i]) {
        return (data_[i] < other.data_[i]);
      }
    }
    return (used_ < other.used_);
  }
  bool operator==(const Vector<T> &other) const {
    return (!(*this > other) && !(*this < other));
  }
  bool operator!=(const Vector<T> &other) const {
    return (!(*this == other));
  }
  bool operator>=(const Vector<T> &other) const {
    return (!(*this < other));
  }
  bool operator<=(const Vector<T> &other) const {
    return (!(*this > other));
  }

  // Itereators
  Iterator begin() {  // NOLINT
    return Iterator{data_};
  }
  Iterator end() {  // NOLINT
    return Iterator{data_ + used_};
  }
  ConstIterator begin() const {  // NOLINT
    return ConstIterator{data_};
  }
  ConstIterator end() const {  // NOLINT
    return ConstIterator{data_ + used_};
  }

  ConstIterator cbegin() const {  // NOLINT
    return ConstIterator{data_};
  }
  ConstIterator cend() const {  // NOLINT
    return ConstIterator{data_ + used_};
  }

  ReverseIterator rbegin() {  // NOLINT
    return ReverseIterator{data_ + used_};
  }
  ReverseIterator rend() {  // NOLINT
    return ReverseIterator{data_};
  }
  ConstReverseIterator rbegin() const {  // NOLINT
    return ConstReverseIterator{data_ + used_};
  }
  ConstReverseIterator rend() const {  // NOLINT
    return ConstReverseIterator{data_};
  }
  ConstReverseIterator crbegin() const {  // NOLINT
    return ConstReverseIterator{data_ + used_};
  }
  ConstReverseIterator crend() const {  // NOLINT
    return ConstReverseIterator{data_};
  }
};

#endif
#endif
