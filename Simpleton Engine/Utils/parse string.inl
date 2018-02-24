//
//  parse string.inl
//  Simpleton Engine
//
//  Created by Indi Kernick on 30/9/17.
//  Copyright © 2017 Indi Kernick. All rights reserved.
//

#include <cerrno>
#include "endian.hpp"
#include <functional>

namespace Utils {
  inline auto charEqualTo(const char ch) {
    return [ch] (const char c) -> bool {
      return c == ch;
    };
  }
}

inline Utils::ParseStringExpectError::ParseStringExpectError(
  const char c,
  const unsigned line,
  const unsigned col)
  : mLine(line),
    mCol(col),
    mExpected(c) {}

inline char Utils::ParseStringExpectError::expectedChar() const {
  return mExpected;
}

inline unsigned Utils::ParseStringExpectError::line() const {
  return mLine;
}

inline unsigned Utils::ParseStringExpectError::column() const {
  return mCol;
}

inline const char *Utils::ParseStringExpectError::what() const noexcept {
  //@TODO use static memory when std::to_chars arrives
  return (std::string("Expected character '")
         + mExpected
         + "' at "
         + std::to_string(mLine)
         + ':'
         + std::to_string(mCol)).c_str();
}

inline Utils::ParseStringNumberError::ParseStringNumberError(const std::string &error)
  : std::runtime_error("Error while parsing number: " + error) {}

inline Utils::ParseString::ParseString(const std::string &string)
  : mData(string.data()), mSize(string.size()) {}

inline Utils::ParseString::ParseString(const std::string_view view)
  : mData(view.data()), mSize(view.size()) {
  throwIfNull(view.data());
}

inline Utils::ParseString::ParseString(const char *data, const size_t size)
  : mData(data), mSize(size) {
  throwIfNull(data);
}

inline const char *Utils::ParseString::data() const {
  return mData;
}

inline size_t Utils::ParseString::size() const {
  return mSize;
}

inline Utils::ParseString::LineCol Utils::ParseString::lineCol() const {
  return mLineCol;
}

inline std::string_view Utils::ParseString::view() const {
  return {mData, mSize};
}

inline std::string_view Utils::ParseString::view(const size_t numChars) const {
  return {mData, mSize < numChars ? mSize : numChars};
}

inline bool Utils::ParseString::empty() const {
  return mSize == 0;
}

inline char Utils::ParseString::operator[](const size_t i) const {
  return mData[i];
}

inline char Utils::ParseString::at(const size_t i) const {
  if (i >= mSize) {
    throw std::out_of_range("Index on parse string out of range");
  }
  return mData[i];
}

inline char Utils::ParseString::front() const {
  return *mData;
}

inline void Utils::ParseString::advance(const size_t numChars) {
  if (numChars > mSize) {
    throw std::out_of_range("Advanced parse string too many characters");
  }
  advanceNoCheck(numChars);
}

inline void Utils::ParseString::advance() {
  if (mSize == 0) {
    throw std::out_of_range("Advanced parse string too many characters");
  }
  advanceNoCheck();
}

inline void Utils::ParseString::skip(const char c) {
  skip(charEqualTo(c));
}

template <typename Pred>
void Utils::ParseString::skip(Pred &&pred) {
  size_t numChars = 0;
  while (numChars < mSize && pred(mData[numChars])) {
    ++numChars;
  }
  advanceNoCheck(numChars);
}

inline void Utils::ParseString::skipWhitespace() {
  skip(isspace);
}

inline void Utils::ParseString::skipUntil(const char c) {
  skipUntil(charEqualTo(c));
}

template <typename Pred>
void Utils::ParseString::skipUntil(Pred &&pred) {
  skip(std::not_fn(pred));
}

inline void Utils::ParseString::skipUntilWhitespace() {
  skipUntil(isspace);
}

inline void Utils::ParseString::expect(const char c) {
  if (mSize == 0 || *mData != c) {
    throw ParseStringExpectError(c, mLineCol.line(), mLineCol.col());
  }
  advanceNoCheck();
}

inline void Utils::ParseString::expect(const char *data, const size_t size) {
  if (mSize < size || std::memcmp(mData, data, size) != 0) {
    throw ParseStringExpectError(*data, mLineCol.line(), mLineCol.col());
  }
  advanceNoCheck(size);
}

inline void Utils::ParseString::expect(const std::string_view view) {
  expect(view.data(), view.size());
}

template <typename Pred>
void Utils::ParseString::expectAfter(Pred &&pred, const char c) {
  skip(pred);
  expect(c);
}

inline void Utils::ParseString::expectAfterWhitespace(const char c) {
  expectAfter(isspace, c);
}

inline bool Utils::ParseString::check(const char c) {
  if (mSize == 0 || *mData != c) {
    return false;
  } else {
    advanceNoCheck();
    return true;
  }
}

inline bool Utils::ParseString::check(const char *data, const size_t size) {
  if (mSize < size) {
    return false;
  }
  if (size == 0) {
    return true;
  }
  if (std::memcmp(mData, data, size) == 0) {
    advanceNoCheck(size);
    return true;
  } else {
    return false;
  }
}

inline bool Utils::ParseString::check(const std::string_view view) {
  return check(view.data(), view.size());
}

template <typename Pred>
bool Utils::ParseString::check(Pred &&pred) {
  if (mSize == 0 || !pred(*mData)) {
    return false;
  } else {
    advanceNoCheck();
    return true;
  }
}

template <typename Number>
void Utils::ParseString::parseNumber(Number &number) {
  if constexpr (std::is_integral<Number>::value) {
    if constexpr (std::is_unsigned<Number>::value) {
      char *end;
      const unsigned long long num = std::strtoull(mData, &end, 0);
      if (errno == ERANGE || num > std::numeric_limits<Number>::max()) {
        throw ParseStringNumberError("Number out of range");
      }
      if (num == 0 && end == mData) {
        throw ParseStringNumberError("Invalid number");
      }
      advanceNoCheck(end - mData);
      number = static_cast<Number>(num);
    } else if constexpr (std::is_signed<Number>::value) {
      char *end;
      const long long num = std::strtoll(mData, &end, 0);
      if (errno == ERANGE || num < std::numeric_limits<Number>::lowest() || num > std::numeric_limits<Number>::max()) {
        throw ParseStringNumberError("Number out of range");
      }
      if (num == 0 && end == mData) {
        throw ParseStringNumberError("Invalid number");
      }
      advanceNoCheck(end - mData);
      number = static_cast<Number>(num);
    }
  } else if constexpr (std::is_floating_point<Number>::value) {
    char *end;
    const long double num = std::strtold(mData, &end);
    if (errno == ERANGE || num < std::numeric_limits<Number>::lowest() || num > std::numeric_limits<Number>::max()) {
      throw ParseStringNumberError("Number out of range");
    }
    if (num == 0 && end == mData) {
      throw ParseStringNumberError("Invalid number");
    }
    advanceNoCheck(end - mData);
    number = static_cast<Number>(num);
  }

  /*
  @TODO
  
  const auto [end, error] = std::from_chars(mData, mData + mSize, number);
  if (error) {
    throw ParseStringNumberError(error.message());
  }
  advanceNoCheck(end - mData);
  */
}

template <typename Number>
Number Utils::ParseString::parseNumber() {
  Number number;
  parseNumber(number);
  return number;
}

template <typename Number>
void Utils::ParseString::readNumberLil(Number &n) {
  if (mSize < sizeof(Number)) {
    throw ParseStringNumberError("String is insufficient size to read number");
  }
  copyFromLilEndian(&n, mData, 1);
  advanceBin(sizeof(Number));
}

template <typename Number>
void Utils::ParseString::readNumberBig(Number &n) {
  if (mSize < sizeof(Number)) {
    throw ParseStringNumberError("String is insufficient size to read number");
  }
  copyFromBigEndian(&n, mData, 1);
  advanceBin(sizeof(Number));
}

template <typename Number>
void Utils::ParseString::readNumbersLil(Number *n, const size_t size) {
  if (mSize < sizeof(Number) * size) {
    throw ParseStringNumberError("String is insufficient size to read number");
  }
  copyFromLilEndian(n, mData, size);
  advanceBin(sizeof(Number) * size);
}

template <typename Number>
void Utils::ParseString::readNumbersBig(Number *n, const size_t size) {
  if (mSize < sizeof(Number) * size) {
    throw ParseStringNumberError("String is insufficient size to read number");
  }
  copyFromBigEndian(n, mData, size);
  advanceBin(sizeof(Number) * size);
}

inline size_t Utils::ParseString::parseEnum(const std::string_view *const names, const size_t numNames) {
  throwIfNull(names);
  const std::string_view *const end = names + numNames;
  for (const std::string_view *n = names; n != end; ++n) {
    if (check(*n)) {
      return n - names;
    }
  }
  return numNames;
}

inline size_t Utils::ParseString::copy(char *const dst, const size_t dstSize) {
  throwIfNull(dst);
  const size_t numChars = mSize < dstSize ? mSize : dstSize;
  std::memcpy(dst, mData, numChars);
  advanceNoCheck(numChars);
  return numChars;
}

inline void Utils::ParseString::copy(std::string &dst, const size_t copySize) {
  const size_t numChars = mSize < copySize ? mSize : copySize;
  dst.append(mData, numChars);
  advanceNoCheck(numChars);
}

template <typename Pred>
size_t Utils::ParseString::copyWhile(char *dst, const size_t dstSize, Pred &&pred) {
  throwIfNull(dst);
  size_t numChars = 0;
  const size_t maxChars = mSize < dstSize ? mSize : dstSize;
  while (numChars < maxChars && pred(*mData)) {
    *dst = *mData;
    ++dst;
    advanceNoCheck(); // increments mData
    ++numChars;
  }
  return numChars;
}

template <typename Pred>
void Utils::ParseString::copyWhile(std::string &dst, Pred &&pred) {
  while (mSize && pred(*mData)) {
    dst.push_back(*mData);
    advanceNoCheck();
  }
}

inline size_t Utils::ParseString::copyUntil(char *const dst, const size_t dstSize, const char c) {
  return copyUntil(dst, dstSize, charEqualTo(c));
}

inline void Utils::ParseString::copyUntil(std::string &dst, const char c) {
  return copyUntil(dst, charEqualTo(c));
}

template <typename Pred>
size_t Utils::ParseString::copyUntil(char *const dst, const size_t dstSize, Pred &&pred) {
  return copyWhile(dst, dstSize, std::not_fn(pred));
}

template <typename Pred>
void Utils::ParseString::copyUntil(std::string &dst, Pred &&pred) {
  copyWhile(dst, std::not_fn(pred));
}

inline size_t Utils::ParseString::copyUntilWhitespace(char *const dst, const size_t dstSize) {
  return copyUntil(dst, dstSize, isspace);
}

inline void Utils::ParseString::copyUntilWhitespace(std::string &dst) {
  copyUntil(dst, isspace);
}

inline void Utils::ParseString::advanceNoCheck(const size_t numChars) {
  mLineCol.putString(mData, numChars);
  mData += numChars;
  mSize -= numChars;
}

inline void Utils::ParseString::advanceNoCheck() {
  mLineCol.putChar(*mData);
  ++mData;
  --mSize;
}

inline void Utils::ParseString::advanceBin(const size_t numChars) {
  mData += numChars;
  mSize -= numChars;
}
