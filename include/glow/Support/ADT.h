#ifndef GLOW_SUPPORT_ADT_H
#define GLOW_SUPPORT_ADT_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <string>
#include <vector>

namespace glow {
/// \brief A range adaptor for a pair of iterators.
///
/// This just wraps two iterators into a range-compatible interface. Nothing
/// fancy at all.
template <typename IteratorT> class iterator_range {
  IteratorT begin_iterator, end_iterator;

public:
  template <typename Container>
  iterator_range(Container &&c)
      : begin_iterator(c.begin()), end_iterator(c.end()) {}
  iterator_range(IteratorT begin_iterator, IteratorT end_iterator)
      : begin_iterator(std::move(begin_iterator)),
        end_iterator(std::move(end_iterator)) {}

  IteratorT begin() const { return begin_iterator; }
  IteratorT end() const { return end_iterator; }
};

/// \brief Convenience function for iterating over sub-ranges.
template <class T> iterator_range<T> make_range(T x, T y) {
  return iterator_range<T>(std::move(x), std::move(y));
}

template <typename T> iterator_range<T> make_range(std::pair<T, T> p) {
  return iterator_range<T>(std::move(p.first), std::move(p.second));
}

template <typename T>
iterator_range<decltype(begin(std::declval<T>()))> drop_begin(T &&t, int n) {
  return make_range(std::next(begin(t), n), end(t));
}

/// ArrayRef - represent a constant reference to an array.
/// Derived from LLVM's ArrayRef.
template <typename T> class ArrayRef {
public:
  using iterator = const T *;
  using const_iterator = const T *;
  using size_type = size_t;
  using reverse_iterator = std::reverse_iterator<iterator>;

private:
  const T *data_ = nullptr;
  size_type length_ = 0;

public:
  /// Construct an empty ArrayRef.
  ArrayRef() = default;

  /// Construct an ArrayRef from a single element.
  ArrayRef(const T &one) : data_(&one), length_(1) {}

  /// Construct an ArrayRef from a pointer and length.
  ArrayRef(const T *data, size_t length) : data_(data), length_(length) {}

  /// Construct an ArrayRef from a range.
  ArrayRef(const T *begin, const T *end) : data_(begin), length_(end - begin) {}

  /// Construct an ArrayRef from a std::initializer_list.
  ArrayRef(const std::initializer_list<T> &vec)
      : data_(vec.begin() == vec.end() ? (T *)nullptr : vec.begin()),
        length_(vec.size()) {}

  /// Construct an ArrayRef from a std::vector.
  template <typename A>
  ArrayRef(const std::vector<T, A> &vec)
      : data_(vec.data()), length_(vec.size()) {}

  iterator begin() const { return data_; }
  iterator end() const { return data_ + length_; }

  reverse_iterator rbegin() const { return reverse_iterator(end()); }
  reverse_iterator rend() const { return reverse_iterator(begin()); }

  bool empty() const { return length_ == 0; }

  const T *data() const { return data_; }

  size_t size() const { return length_; }

  /// Drop the first element of the array.
  ArrayRef<T> drop_front() const {
    assert(size() >= 1 && "Array is empty");
    return ArrayRef(data() + 1, size() - 1);
  }

  /// Drop the last element of the array.
  ArrayRef<T> drop_back(size_t N = 1) const {
    assert(size() >= 1 && "Array is empty");
    return ArrayRef(data(), size() - 1);
  }

  const T &operator[](size_t Index) const {
    assert(Index < length_ && "Invalid index!");
    return data_[Index];
  }

  /// equals - Check for element-wise equality.
  bool equals(ArrayRef RHS) const {
    if (length_ != RHS.length_) {
      return false;
    }
    return std::equal(begin(), end(), RHS.begin());
  }

  std::vector<T> vec() const { return std::vector<T>(begin(), end()); }
};

/// MutableArrayRef - Represent a mutable reference to an array (0 or more
/// elements consecutively in memory), i.e. a start pointer and a length.  It
/// allows various APIs to take and modify consecutive elements easily and
/// conveniently.
///
/// This class does not own the underlying data, it is expected to be used in
/// situations where the data resides in some other buffer, whose lifetime
/// extends past that of the MutableArrayRef. For this reason, it is not in
/// general safe to store a MutableArrayRef.
///
/// This is intended to be trivially copyable, so it should be passed by
/// value.
/// Derived from LLVM's MutableArrayRef.
template <typename T> class MutableArrayRef : public ArrayRef<T> {
public:
  using iterator = T *;
  using reverse_iterator = std::reverse_iterator<iterator>;

  MutableArrayRef() = default;

  /// Construct an MutableArrayRef from a single element.
  MutableArrayRef(T &OneElt) : ArrayRef<T>(OneElt) {}

  /// Construct an MutableArrayRef from a pointer and length.
  MutableArrayRef(T *data, size_t length) : ArrayRef<T>(data, length) {}

  /// Construct an MutableArrayRef from a range.
  MutableArrayRef(T *begin, T *end) : ArrayRef<T>(begin, end) {}

  MutableArrayRef(const std::initializer_list<T> &vec) : ArrayRef<T>(vec) {}

  T *data() const { return const_cast<T *>(ArrayRef<T>::data()); }

  iterator begin() const { return data(); }
  iterator end() const { return data() + this->size(); }

  reverse_iterator rbegin() const { return reverse_iterator(end()); }
  reverse_iterator rend() const { return reverse_iterator(begin()); }

  /// front - Get the first element.
  T &front() const {
    assert(!this->empty());
    return data()[0];
  }

  /// back - Get the last element.
  T &back() const {
    assert(!this->empty());
    return data()[this->size() - 1];
  }

  /// slice(n, m) - Chop off the first N elements of the array, and keep M
  /// elements in the array.
  MutableArrayRef<T> slice(size_t N, size_t M) const {
    assert(N + M <= this->size() && "Invalid specifier");
    return MutableArrayRef<T>(this->data() + N, M);
  }

  /// slice(n) - Chop off the first N elements of the array.
  MutableArrayRef<T> slice(size_t N) const {
    return slice(N, this->size() - N);
  }

  /// \brief Drop the first \p N elements of the array.
  MutableArrayRef<T> drop_front(size_t N = 1) const {
    assert(this->size() >= N && "Dropping more elements than exist");
    return slice(N, this->size() - N);
  }

  MutableArrayRef<T> drop_back(size_t N = 1) const {
    assert(this->size() >= N && "Dropping more elements than exist");
    return slice(0, this->size() - N);
  }

  T &operator[](size_t Index) const {
    assert(Index < this->size() && "Invalid index!");
    return data()[Index];
  }
};

template <typename T> inline bool operator==(ArrayRef<T> LHS, ArrayRef<T> RHS) {
  return LHS.equals(RHS);
}

template <typename T> inline bool operator!=(ArrayRef<T> LHS, ArrayRef<T> RHS) {
  return !(LHS == RHS);
}

template <typename T>
inline bool operator==(MutableArrayRef<T> LHS, MutableArrayRef<T> RHS) {
  return LHS.equals(RHS);
}

template <typename T>
inline bool operator!=(MutableArrayRef<T> LHS, MutableArrayRef<T> RHS) {
  return !(LHS == RHS);
}

/// StringRef - Represent a constant reference to a string, i.e. a character
/// array and a length, which need not be null terminated.
///
/// This class does not own the string data, it is expected to be used in
/// situations where the character data resides in some other buffer, whose
/// lifetime extends past that of the StringRef. For this reason, it is not in
/// general safe to store a StringRef.
class StringRef {
public:
  static const size_t npos = ~size_t(0);

  using iterator = const char *;
  using const_iterator = const char *;
  using size_type = size_t;

private:
  /// The start of the string, in an external buffer.
  const char *Data = nullptr;

  /// The length of the string.
  size_t Length = 0;

  // Workaround memcmp issue with null pointers (undefined behavior)
  // by providing a specialized version
  static int compareMemory(const char *Lhs, const char *Rhs, size_t Length) {
    if (Length == 0) {
      return 0;
    }
    return ::memcmp(Lhs, Rhs, Length);
  }

public:
  /// Construct an empty string ref.
  StringRef() = default;

  /// Disable conversion from nullptr.  This prevents things like
  /// if (S == nullptr)
  StringRef(std::nullptr_t) = delete;

  /// Construct a string ref from a cstring.
  StringRef(const char *Str) : Data(Str), Length(Str ? ::strlen(Str) : 0) {}

  /// Construct a string ref from a pointer and length.
  constexpr StringRef(const char *data, size_t length)
      : Data(data), Length(length) {}

  /// Construct a string ref from an std::string.
  StringRef(const std::string &Str) : Data(Str.data()), Length(Str.length()) {}

  static StringRef withNullAsEmpty(const char *data) {
    return StringRef(data ? data : "");
  }

  iterator begin() const { return Data; }
  iterator end() const { return Data + Length; }

  const unsigned char *bytes_begin() const {
    return reinterpret_cast<const unsigned char *>(begin());
  }
  const unsigned char *bytes_end() const {
    return reinterpret_cast<const unsigned char *>(end());
  }
  iterator_range<const unsigned char *> bytes() const {
    return make_range(bytes_begin(), bytes_end());
  }

  /// data - Get a pointer to the start of the string (which may not be null
  /// terminated).
  const char *data() const { return Data; }

  /// empty - Check if the string is empty.
  bool empty() const { return Length == 0; }

  /// size - Get the string size.
  size_t size() const { return Length; }

  /// front - Get the first character in the string.
  char front() const {
    assert(!empty());
    return Data[0];
  }

  /// back - Get the last character in the string.
  char back() const {
    assert(!empty());
    return Data[Length - 1];
  }

  // copy - Allocate copy in Allocator and return StringRef to it.
  template <typename Allocator> StringRef copy(Allocator &A) const {
    // Don't request a length 0 copy from the allocator.
    if (empty())
      return StringRef();
    char *S = A.template Allocate<char>(Length);
    std::copy(begin(), end(), S);
    return StringRef(S, Length);
  }

  /// equals - Check for string equality, this is more efficient than
  /// compare() when the relative ordering of inequal strings isn't needed.
  bool equals(StringRef RHS) const {
    return (Length == RHS.Length &&
            compareMemory(Data, RHS.Data, RHS.Length) == 0);
  }

  /// compare - Compare two strings; the result is -1, 0, or 1 if this string
  /// is lexicographically less than, equal to, or greater than the \p RHS.
  int compare(StringRef RHS) const {
    // Check the prefix for a mismatch.
    if (int Res = compareMemory(Data, RHS.Data, std::min(Length, RHS.Length)))
      return Res < 0 ? -1 : 1;

    // Otherwise the prefixes match, so we only need to check the lengths.
    if (Length == RHS.Length)
      return 0;
    return Length < RHS.Length ? -1 : 1;
  }

  /// str - Get the contents as an std::string.
  std::string str() const {
    if (!Data)
      return std::string();
    return std::string(Data, Length);
  }

  char operator[](size_t Index) const {
    assert(Index < Length && "Invalid index!");
    return Data[Index];
  }

  /// Disallow accidental assignment from a temporary std::string.
  ///
  /// The declaration here is extra complicated so that `stringRef = {}`
  /// and `stringRef = "abc"` continue to select the move assignment operator.
  template <typename T>
  typename std::enable_if<std::is_same<T, std::string>::value, StringRef>::type
      &
      operator=(T &&Str) = delete;

  operator std::string() const { return str(); }

  /// Check if this string starts with the given \p Prefix.
  bool startswith(StringRef Prefix) const {
    return Length >= Prefix.Length &&
           compareMemory(Data, Prefix.Data, Prefix.Length) == 0;
  }

  /// Check if this string ends with the given \p Suffix.
  bool endswith(StringRef Suffix) const {
    return Length >= Suffix.Length &&
           compareMemory(end() - Suffix.Length, Suffix.Data, Suffix.Length) ==
               0;
  }

  /// Search for the first character \p C in the string.
  ///
  /// \returns The index of the first occurrence of \p C, or npos if not
  /// found.
  size_t find(char C, size_t From = 0) const {
    size_t FindBegin = std::min(From, Length);
    if (FindBegin < Length) { // Avoid calling memchr with nullptr.
      // Just forward to memchr, which is faster than a hand-rolled loop.
      if (const void *P = ::memchr(Data + FindBegin, C, Length - FindBegin))
        return static_cast<const char *>(P) - Data;
    }
    return npos;
  }

  /// Search for the last character \p C in the string.
  ///
  /// \returns The index of the last occurrence of \p C, or npos if not
  /// found.
  size_t rfind(char C, size_t From = npos) const {
    From = std::min(From, Length);
    size_t i = From;
    while (i != 0) {
      --i;
      if (Data[i] == C)
        return i;
    }
    return npos;
  }

  /// Find the first character in the string that is \p C, or npos if not
  /// found. Same as find.
  size_t find_first_of(char C, size_t From = 0) const { return find(C, From); }

  /// Find the last character in the string that is \p C, or npos if not
  /// found.
  size_t find_last_of(char C, size_t From = npos) const {
    return rfind(C, From);
  }

  /// Return the number of occurrences of \p C in the string.
  size_t count(char C) const {
    size_t Count = 0;
    for (size_t i = 0, e = Length; i != e; ++i)
      if (Data[i] == C)
        ++Count;
    return Count;
  }

  /// Return a reference to the substring from [Start, Start + N).
  ///
  /// \param Start The index of the starting character in the substring; if
  /// the index is npos or greater than the length of the string then the
  /// empty substring will be returned.
  ///
  /// \param N The number of characters to included in the substring. If N
  /// exceeds the number of characters remaining in the string, the string
  /// suffix (starting with \p Start) will be returned.
  StringRef substr(size_t Start, size_t N = npos) const {
    Start = std::min(Start, Length);
    return StringRef(Data + Start, std::min(N, Length - Start));
  }

  /// Return a StringRef equal to 'this' but with only the first \p N
  /// elements remaining.  If \p N is greater than the length of the
  /// string, the entire string is returned.
  StringRef take_front(size_t N = 1) const {
    if (N >= size())
      return *this;
    return drop_back(size() - N);
  }

  /// Return a StringRef equal to 'this' but with only the last \p N
  /// elements remaining.  If \p N is greater than the length of the
  /// string, the entire string is returned.
  StringRef take_back(size_t N = 1) const {
    if (N >= size())
      return *this;
    return drop_front(size() - N);
  }

  /// Return a StringRef equal to 'this' but with the first \p N elements
  /// dropped.
  StringRef drop_front(size_t N = 1) const {
    assert(size() >= N && "Dropping more elements than exist");
    return substr(N);
  }

  /// Return a StringRef equal to 'this' but with the last \p N elements
  /// dropped.
  StringRef drop_back(size_t N = 1) const {
    assert(size() >= N && "Dropping more elements than exist");
    return substr(0, size() - N);
  }

  /// Returns true if this StringRef has the given prefix and removes that
  /// prefix.
  bool consume_front(StringRef Prefix) {
    if (!startswith(Prefix))
      return false;

    *this = drop_front(Prefix.size());
    return true;
  }

  /// Returns true if this StringRef has the given suffix and removes that
  /// suffix.
  bool consume_back(StringRef Suffix) {
    if (!endswith(Suffix))
      return false;

    *this = drop_back(Suffix.size());
    return true;
  }

  /// Return a reference to the substring from [Start, End).
  ///
  /// \param Start The index of the starting character in the substring; if
  /// the index is npos or greater than the length of the string then the
  /// empty substring will be returned.
  ///
  /// \param End The index following the last character to include in the
  /// substring. If this is npos or exceeds the number of characters
  /// remaining in the string, the string suffix (starting with \p Start)
  /// will be returned. If this is less than \p Start, an empty string will
  /// be returned.
  StringRef slice(size_t Start, size_t End) const {
    Start = std::min(Start, Length);
    End = std::min(std::max(Start, End), Length);
    return StringRef(Data + Start, End - Start);
  }

  /// Split into two substrings around the first occurrence of a separator
  /// character.
  ///
  /// If \p Separator is in the string, then the result is a pair (LHS, RHS)
  /// such that (*this == LHS + Separator + RHS) is true and RHS is
  /// maximal. If \p Separator is not in the string, then the result is a
  /// pair (LHS, RHS) where (*this == LHS) and (RHS == "").
  ///
  /// \param Separator The character to split on.
  /// \returns The split substrings.
  std::pair<StringRef, StringRef> split(char Separator) const {
    size_t Idx = find(Separator);
    if (Idx == npos)
      return std::make_pair(*this, StringRef());
    return std::make_pair(slice(0, Idx), slice(Idx + 1, npos));
  }
};

inline bool operator==(StringRef LHS, StringRef RHS) { return LHS.equals(RHS); }

inline bool operator!=(StringRef LHS, StringRef RHS) { return !(LHS == RHS); }

inline bool operator<(StringRef LHS, StringRef RHS) {
  return LHS.compare(RHS) == -1;
}

inline bool operator<=(StringRef LHS, StringRef RHS) {
  return LHS.compare(RHS) != 1;
}

inline bool operator>(StringRef LHS, StringRef RHS) {
  return LHS.compare(RHS) == 1;
}

inline bool operator>=(StringRef LHS, StringRef RHS) {
  return LHS.compare(RHS) != -1;
}

inline std::string &operator+=(std::string &buffer, StringRef string) {
  return buffer.append(string.data(), string.size());
}

} // namespace glow

#endif // GLOW_SUPPORT_ADT_H