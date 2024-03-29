// Copyright 2007, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: wan@google.com (Zhanyong Wan)

// Google Mock - a framework for writing C++ mock classes.
//
// This file tests some commonly used argument matchers.

#include <gmock/gmock-matchers.h>

#include <string.h>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

namespace testing {

namespace internal {
string FormatMatcherDescriptionSyntaxError(const char* description,
                                           const char* error_pos);
int GetParamIndex(const char* param_names[], const string& param_name);
string JoinAsTuple(const Strings& fields);
bool SkipPrefix(const char* prefix, const char** pstr);
}  // namespace internal

namespace gmock_matchers_test {

using std::stringstream;
using testing::A;
using testing::AllOf;
using testing::An;
using testing::AnyOf;
using testing::ByRef;
using testing::DoubleEq;
using testing::EndsWith;
using testing::Eq;
using testing::Field;
using testing::FloatEq;
using testing::Ge;
using testing::Gt;
using testing::HasSubstr;
using testing::Le;
using testing::Lt;
using testing::MakeMatcher;
using testing::MakePolymorphicMatcher;
using testing::Matcher;
using testing::MatcherCast;
using testing::MatcherInterface;
using testing::Matches;
using testing::NanSensitiveDoubleEq;
using testing::NanSensitiveFloatEq;
using testing::Ne;
using testing::Not;
using testing::NotNull;
using testing::Pointee;
using testing::PolymorphicMatcher;
using testing::Property;
using testing::Ref;
using testing::ResultOf;
using testing::StartsWith;
using testing::StrCaseEq;
using testing::StrCaseNe;
using testing::StrEq;
using testing::StrNe;
using testing::Truly;
using testing::TypedEq;
using testing::_;
using testing::internal::FloatingEqMatcher;
using testing::internal::FormatMatcherDescriptionSyntaxError;
using testing::internal::GetParamIndex;
using testing::internal::Interpolation;
using testing::internal::Interpolations;
using testing::internal::JoinAsTuple;
using testing::internal::SkipPrefix;
using testing::internal::String;
using testing::internal::Strings;
using testing::internal::ValidateMatcherDescription;
using testing::internal::kInvalidInterpolation;
using testing::internal::kPercentInterpolation;
using testing::internal::kTupleInterpolation;
using testing::internal::string;

#ifdef GMOCK_HAS_REGEX
using testing::ContainsRegex;
using testing::MatchesRegex;
using testing::internal::RE;
#endif  // GMOCK_HAS_REGEX

// Returns the description of the given matcher.
template <typename T>
string Describe(const Matcher<T>& m) {
  stringstream ss;
  m.DescribeTo(&ss);
  return ss.str();
}

// Returns the description of the negation of the given matcher.
template <typename T>
string DescribeNegation(const Matcher<T>& m) {
  stringstream ss;
  m.DescribeNegationTo(&ss);
  return ss.str();
}

// Returns the reason why x matches, or doesn't match, m.
template <typename MatcherType, typename Value>
string Explain(const MatcherType& m, const Value& x) {
  stringstream ss;
  m.ExplainMatchResultTo(x, &ss);
  return ss.str();
}

// Makes sure that the MatcherInterface<T> interface doesn't
// change.
class EvenMatcherImpl : public MatcherInterface<int> {
 public:
  virtual bool Matches(int x) const { return x % 2 == 0; }

  virtual void DescribeTo(::std::ostream* os) const {
    *os << "is an even number";
  }

  // We deliberately don't define DescribeNegationTo() and
  // ExplainMatchResultTo() here, to make sure the definition of these
  // two methods is optional.
};

TEST(MatcherInterfaceTest, CanBeImplemented) {
  EvenMatcherImpl m;
}

// Tests default-constructing a matcher.
TEST(MatcherTest, CanBeDefaultConstructed) {
  Matcher<double> m;
}

// Tests that Matcher<T> can be constructed from a MatcherInterface<T>*.
TEST(MatcherTest, CanBeConstructedFromMatcherInterface) {
  const MatcherInterface<int>* impl = new EvenMatcherImpl;
  Matcher<int> m(impl);
  EXPECT_TRUE(m.Matches(4));
  EXPECT_FALSE(m.Matches(5));
}

// Tests that value can be used in place of Eq(value).
TEST(MatcherTest, CanBeImplicitlyConstructedFromValue) {
  Matcher<int> m1 = 5;
  EXPECT_TRUE(m1.Matches(5));
  EXPECT_FALSE(m1.Matches(6));
}

// Tests that NULL can be used in place of Eq(NULL).
TEST(MatcherTest, CanBeImplicitlyConstructedFromNULL) {
  Matcher<int*> m1 = NULL;
  EXPECT_TRUE(m1.Matches(NULL));
  int n = 0;
  EXPECT_FALSE(m1.Matches(&n));
}

// Tests that matchers are copyable.
TEST(MatcherTest, IsCopyable) {
  // Tests the copy constructor.
  Matcher<bool> m1 = Eq(false);
  EXPECT_TRUE(m1.Matches(false));
  EXPECT_FALSE(m1.Matches(true));

  // Tests the assignment operator.
  m1 = Eq(true);
  EXPECT_TRUE(m1.Matches(true));
  EXPECT_FALSE(m1.Matches(false));
}

// Tests that Matcher<T>::DescribeTo() calls
// MatcherInterface<T>::DescribeTo().
TEST(MatcherTest, CanDescribeItself) {
  EXPECT_EQ("is an even number",
            Describe(Matcher<int>(new EvenMatcherImpl)));
}

// Tests that a C-string literal can be implicitly converted to a
// Matcher<string> or Matcher<const string&>.
TEST(StringMatcherTest, CanBeImplicitlyConstructedFromCStringLiteral) {
  Matcher<string> m1 = "hi";
  EXPECT_TRUE(m1.Matches("hi"));
  EXPECT_FALSE(m1.Matches("hello"));

  Matcher<const string&> m2 = "hi";
  EXPECT_TRUE(m2.Matches("hi"));
  EXPECT_FALSE(m2.Matches("hello"));
}

// Tests that a string object can be implicitly converted to a
// Matcher<string> or Matcher<const string&>.
TEST(StringMatcherTest, CanBeImplicitlyConstructedFromString) {
  Matcher<string> m1 = string("hi");
  EXPECT_TRUE(m1.Matches("hi"));
  EXPECT_FALSE(m1.Matches("hello"));

  Matcher<const string&> m2 = string("hi");
  EXPECT_TRUE(m2.Matches("hi"));
  EXPECT_FALSE(m2.Matches("hello"));
}

// Tests that MakeMatcher() constructs a Matcher<T> from a
// MatcherInterface* without requiring the user to explicitly
// write the type.
TEST(MakeMatcherTest, ConstructsMatcherFromMatcherInterface) {
  const MatcherInterface<int>* dummy_impl = NULL;
  Matcher<int> m = MakeMatcher(dummy_impl);
}

// Tests that MakePolymorphicMatcher() constructs a polymorphic
// matcher from its implementation.
const int bar = 1;
class ReferencesBarOrIsZeroImpl {
 public:
  template <typename T>
  bool Matches(const T& x) const {
    const void* p = &x;
    return p == &bar || x == 0;
  }

  void DescribeTo(::std::ostream* os) const { *os << "bar or zero"; }

  void DescribeNegationTo(::std::ostream* os) const {
    *os << "doesn't reference bar and is not zero";
  }
};

// This function verifies that MakePolymorphicMatcher() returns a
// PolymorphicMatcher<T> where T is the argument's type.
PolymorphicMatcher<ReferencesBarOrIsZeroImpl> ReferencesBarOrIsZero() {
  return MakePolymorphicMatcher(ReferencesBarOrIsZeroImpl());
}

TEST(MakePolymorphicMatcherTest, ConstructsMatcherFromImpl) {
  // Using a polymorphic matcher to match a reference type.
  Matcher<const int&> m1 = ReferencesBarOrIsZero();
  EXPECT_TRUE(m1.Matches(0));
  // Verifies that the identity of a by-reference argument is preserved.
  EXPECT_TRUE(m1.Matches(bar));
  EXPECT_FALSE(m1.Matches(1));
  EXPECT_EQ("bar or zero", Describe(m1));

  // Using a polymorphic matcher to match a value type.
  Matcher<double> m2 = ReferencesBarOrIsZero();
  EXPECT_TRUE(m2.Matches(0.0));
  EXPECT_FALSE(m2.Matches(0.1));
  EXPECT_EQ("bar or zero", Describe(m2));
}

// Tests that MatcherCast<T>(m) works when m is a polymorphic matcher.
TEST(MatcherCastTest, FromPolymorphicMatcher) {
  Matcher<int> m = MatcherCast<int>(Eq(5));
  EXPECT_TRUE(m.Matches(5));
  EXPECT_FALSE(m.Matches(6));
}

// For testing casting matchers between compatible types.
class IntValue {
 public:
  // An int can be statically (although not implicitly) cast to a
  // IntValue.
  explicit IntValue(int value) : value_(value) {}

  int value() const { return value_; }
 private:
  int value_;
};

// For testing casting matchers between compatible types.
bool IsPositiveIntValue(const IntValue& foo) {
  return foo.value() > 0;
}

// Tests that MatcherCast<T>(m) works when m is a Matcher<U> where T
// can be statically converted to U.
TEST(MatcherCastTest, FromCompatibleType) {
  Matcher<double> m1 = Eq(2.0);
  Matcher<int> m2 = MatcherCast<int>(m1);
  EXPECT_TRUE(m2.Matches(2));
  EXPECT_FALSE(m2.Matches(3));

  Matcher<IntValue> m3 = Truly(IsPositiveIntValue);
  Matcher<int> m4 = MatcherCast<int>(m3);
  // In the following, the arguments 1 and 0 are statically converted
  // to IntValue objects, and then tested by the IsPositiveIntValue()
  // predicate.
  EXPECT_TRUE(m4.Matches(1));
  EXPECT_FALSE(m4.Matches(0));
}

// Tests that MatcherCast<T>(m) works when m is a Matcher<const T&>.
TEST(MatcherCastTest, FromConstReferenceToNonReference) {
  Matcher<const int&> m1 = Eq(0);
  Matcher<int> m2 = MatcherCast<int>(m1);
  EXPECT_TRUE(m2.Matches(0));
  EXPECT_FALSE(m2.Matches(1));
}

// Tests that MatcherCast<T>(m) works when m is a Matcher<T&>.
TEST(MatcherCastTest, FromReferenceToNonReference) {
  Matcher<int&> m1 = Eq(0);
  Matcher<int> m2 = MatcherCast<int>(m1);
  EXPECT_TRUE(m2.Matches(0));
  EXPECT_FALSE(m2.Matches(1));
}

// Tests that MatcherCast<const T&>(m) works when m is a Matcher<T>.
TEST(MatcherCastTest, FromNonReferenceToConstReference) {
  Matcher<int> m1 = Eq(0);
  Matcher<const int&> m2 = MatcherCast<const int&>(m1);
  EXPECT_TRUE(m2.Matches(0));
  EXPECT_FALSE(m2.Matches(1));
}

// Tests that MatcherCast<T&>(m) works when m is a Matcher<T>.
TEST(MatcherCastTest, FromNonReferenceToReference) {
  Matcher<int> m1 = Eq(0);
  Matcher<int&> m2 = MatcherCast<int&>(m1);
  int n = 0;
  EXPECT_TRUE(m2.Matches(n));
  n = 1;
  EXPECT_FALSE(m2.Matches(n));
}

// Tests that MatcherCast<T>(m) works when m is a Matcher<T>.
TEST(MatcherCastTest, FromSameType) {
  Matcher<int> m1 = Eq(0);
  Matcher<int> m2 = MatcherCast<int>(m1);
  EXPECT_TRUE(m2.Matches(0));
  EXPECT_FALSE(m2.Matches(1));
}

// Tests that A<T>() matches any value of type T.
TEST(ATest, MatchesAnyValue) {
  // Tests a matcher for a value type.
  Matcher<double> m1 = A<double>();
  EXPECT_TRUE(m1.Matches(91.43));
  EXPECT_TRUE(m1.Matches(-15.32));

  // Tests a matcher for a reference type.
  int a = 2;
  int b = -6;
  Matcher<int&> m2 = A<int&>();
  EXPECT_TRUE(m2.Matches(a));
  EXPECT_TRUE(m2.Matches(b));
}

// Tests that A<T>() describes itself properly.
TEST(ATest, CanDescribeSelf) {
  EXPECT_EQ("is anything", Describe(A<bool>()));
}

// Tests that An<T>() matches any value of type T.
TEST(AnTest, MatchesAnyValue) {
  // Tests a matcher for a value type.
  Matcher<int> m1 = An<int>();
  EXPECT_TRUE(m1.Matches(9143));
  EXPECT_TRUE(m1.Matches(-1532));

  // Tests a matcher for a reference type.
  int a = 2;
  int b = -6;
  Matcher<int&> m2 = An<int&>();
  EXPECT_TRUE(m2.Matches(a));
  EXPECT_TRUE(m2.Matches(b));
}

// Tests that An<T>() describes itself properly.
TEST(AnTest, CanDescribeSelf) {
  EXPECT_EQ("is anything", Describe(An<int>()));
}

// Tests that _ can be used as a matcher for any type and matches any
// value of that type.
TEST(UnderscoreTest, MatchesAnyValue) {
  // Uses _ as a matcher for a value type.
  Matcher<int> m1 = _;
  EXPECT_TRUE(m1.Matches(123));
  EXPECT_TRUE(m1.Matches(-242));

  // Uses _ as a matcher for a reference type.
  bool a = false;
  const bool b = true;
  Matcher<const bool&> m2 = _;
  EXPECT_TRUE(m2.Matches(a));
  EXPECT_TRUE(m2.Matches(b));
}

// Tests that _ describes itself properly.
TEST(UnderscoreTest, CanDescribeSelf) {
  Matcher<int> m = _;
  EXPECT_EQ("is anything", Describe(m));
}

// Tests that Eq(x) matches any value equal to x.
TEST(EqTest, MatchesEqualValue) {
  // 2 C-strings with same content but different addresses.
  const char a1[] = "hi";
  const char a2[] = "hi";

  Matcher<const char*> m1 = Eq(a1);
  EXPECT_TRUE(m1.Matches(a1));
  EXPECT_FALSE(m1.Matches(a2));
}

// Tests that Eq(v) describes itself properly.

class Unprintable {
 public:
  Unprintable() : c_('a') {}

  bool operator==(const Unprintable& rhs) { return true; }
 private:
  char c_;
};

TEST(EqTest, CanDescribeSelf) {
  Matcher<Unprintable> m = Eq(Unprintable());
  EXPECT_EQ("is equal to 1-byte object <61>", Describe(m));
}

// Tests that Eq(v) can be used to match any type that supports
// comparing with type T, where T is v's type.
TEST(EqTest, IsPolymorphic) {
  Matcher<int> m1 = Eq(1);
  EXPECT_TRUE(m1.Matches(1));
  EXPECT_FALSE(m1.Matches(2));

  Matcher<char> m2 = Eq(1);
  EXPECT_TRUE(m2.Matches('\1'));
  EXPECT_FALSE(m2.Matches('a'));
}

// Tests that TypedEq<T>(v) matches values of type T that's equal to v.
TEST(TypedEqTest, ChecksEqualityForGivenType) {
  Matcher<char> m1 = TypedEq<char>('a');
  EXPECT_TRUE(m1.Matches('a'));
  EXPECT_FALSE(m1.Matches('b'));

  Matcher<int> m2 = TypedEq<int>(6);
  EXPECT_TRUE(m2.Matches(6));
  EXPECT_FALSE(m2.Matches(7));
}

// Tests that TypedEq(v) describes itself properly.
TEST(TypedEqTest, CanDescribeSelf) {
  EXPECT_EQ("is equal to 2", Describe(TypedEq<int>(2)));
}

// Tests that TypedEq<T>(v) has type Matcher<T>.

// Type<T>::IsTypeOf(v) compiles iff the type of value v is T, where T
// is a "bare" type (i.e. not in the form of const U or U&).  If v's
// type is not T, the compiler will generate a message about
// "undefined referece".
template <typename T>
struct Type {
  static bool IsTypeOf(const T& v) { return true; }

  template <typename T2>
  static void IsTypeOf(T2 v);
};

TEST(TypedEqTest, HasSpecifiedType) {
  // Verfies that the type of TypedEq<T>(v) is Matcher<T>.
  Type<Matcher<int> >::IsTypeOf(TypedEq<int>(5));
  Type<Matcher<double> >::IsTypeOf(TypedEq<double>(5));
}

// Tests that Ge(v) matches anything >= v.
TEST(GeTest, ImplementsGreaterThanOrEqual) {
  Matcher<int> m1 = Ge(0);
  EXPECT_TRUE(m1.Matches(1));
  EXPECT_TRUE(m1.Matches(0));
  EXPECT_FALSE(m1.Matches(-1));
}

// Tests that Ge(v) describes itself properly.
TEST(GeTest, CanDescribeSelf) {
  Matcher<int> m = Ge(5);
  EXPECT_EQ("is greater than or equal to 5", Describe(m));
}

// Tests that Gt(v) matches anything > v.
TEST(GtTest, ImplementsGreaterThan) {
  Matcher<double> m1 = Gt(0);
  EXPECT_TRUE(m1.Matches(1.0));
  EXPECT_FALSE(m1.Matches(0.0));
  EXPECT_FALSE(m1.Matches(-1.0));
}

// Tests that Gt(v) describes itself properly.
TEST(GtTest, CanDescribeSelf) {
  Matcher<int> m = Gt(5);
  EXPECT_EQ("is greater than 5", Describe(m));
}

// Tests that Le(v) matches anything <= v.
TEST(LeTest, ImplementsLessThanOrEqual) {
  Matcher<char> m1 = Le('b');
  EXPECT_TRUE(m1.Matches('a'));
  EXPECT_TRUE(m1.Matches('b'));
  EXPECT_FALSE(m1.Matches('c'));
}

// Tests that Le(v) describes itself properly.
TEST(LeTest, CanDescribeSelf) {
  Matcher<int> m = Le(5);
  EXPECT_EQ("is less than or equal to 5", Describe(m));
}

// Tests that Lt(v) matches anything < v.
TEST(LtTest, ImplementsLessThan) {
  Matcher<const string&> m1 = Lt("Hello");
  EXPECT_TRUE(m1.Matches("Abc"));
  EXPECT_FALSE(m1.Matches("Hello"));
  EXPECT_FALSE(m1.Matches("Hello, world!"));
}

// Tests that Lt(v) describes itself properly.
TEST(LtTest, CanDescribeSelf) {
  Matcher<int> m = Lt(5);
  EXPECT_EQ("is less than 5", Describe(m));
}

// Tests that Ne(v) matches anything != v.
TEST(NeTest, ImplementsNotEqual) {
  Matcher<int> m1 = Ne(0);
  EXPECT_TRUE(m1.Matches(1));
  EXPECT_TRUE(m1.Matches(-1));
  EXPECT_FALSE(m1.Matches(0));
}

// Tests that Ne(v) describes itself properly.
TEST(NeTest, CanDescribeSelf) {
  Matcher<int> m = Ne(5);
  EXPECT_EQ("is not equal to 5", Describe(m));
}

// Tests that NotNull() matches any non-NULL pointer of any type.
TEST(NotNullTest, MatchesNonNullPointer) {
  Matcher<int*> m1 = NotNull();
  int* p1 = NULL;
  int n = 0;
  EXPECT_FALSE(m1.Matches(p1));
  EXPECT_TRUE(m1.Matches(&n));

  Matcher<const char*> m2 = NotNull();
  const char* p2 = NULL;
  EXPECT_FALSE(m2.Matches(p2));
  EXPECT_TRUE(m2.Matches("hi"));
}

// Tests that NotNull() describes itself properly.
TEST(NotNullTest, CanDescribeSelf) {
  Matcher<int*> m = NotNull();
  EXPECT_EQ("is not NULL", Describe(m));
}

// Tests that Ref(variable) matches an argument that references
// 'variable'.
TEST(RefTest, MatchesSameVariable) {
  int a = 0;
  int b = 0;
  Matcher<int&> m = Ref(a);
  EXPECT_TRUE(m.Matches(a));
  EXPECT_FALSE(m.Matches(b));
}

// Tests that Ref(variable) describes itself properly.
TEST(RefTest, CanDescribeSelf) {
  int n = 5;
  Matcher<int&> m = Ref(n);
  stringstream ss;
  ss << "references the variable @" << &n << " 5";
  EXPECT_EQ(string(ss.str()), Describe(m));
}

// Test that Ref(non_const_varialbe) can be used as a matcher for a
// const reference.
TEST(RefTest, CanBeUsedAsMatcherForConstReference) {
  int a = 0;
  int b = 0;
  Matcher<const int&> m = Ref(a);
  EXPECT_TRUE(m.Matches(a));
  EXPECT_FALSE(m.Matches(b));
}

// Tests that Ref(variable) is covariant, i.e. Ref(derived) can be
// used wherever Ref(base) can be used (Ref(derived) is a sub-type
// of Ref(base), but not vice versa.

class Base {};
class Derived : public Base {};

TEST(RefTest, IsCovariant) {
  Base base, base2;
  Derived derived;
  Matcher<const Base&> m1 = Ref(base);
  EXPECT_TRUE(m1.Matches(base));
  EXPECT_FALSE(m1.Matches(base2));
  EXPECT_FALSE(m1.Matches(derived));

  m1 = Ref(derived);
  EXPECT_TRUE(m1.Matches(derived));
  EXPECT_FALSE(m1.Matches(base));
  EXPECT_FALSE(m1.Matches(base2));
}

// Tests string comparison matchers.

TEST(StrEqTest, MatchesEqualString) {
  Matcher<const char*> m = StrEq(string("Hello"));
  EXPECT_TRUE(m.Matches("Hello"));
  EXPECT_FALSE(m.Matches("hello"));
  EXPECT_FALSE(m.Matches(NULL));

  Matcher<const string&> m2 = StrEq("Hello");
  EXPECT_TRUE(m2.Matches("Hello"));
  EXPECT_FALSE(m2.Matches("Hi"));
}

TEST(StrEqTest, CanDescribeSelf) {
  Matcher<string> m = StrEq("Hi-\'\"\?\\\a\b\f\n\r\t\v\xD3");
  EXPECT_EQ("is equal to \"Hi-\'\\\"\\?\\\\\\a\\b\\f\\n\\r\\t\\v\\xD3\"",
      Describe(m));

  string str("01204500800");
  str[3] = '\0';
  Matcher<string> m2 = StrEq(str);
  EXPECT_EQ("is equal to \"012\\04500800\"", Describe(m2));
  str[0] = str[6] = str[7] = str[9] = str[10] = '\0';
  Matcher<string> m3 = StrEq(str);
  EXPECT_EQ("is equal to \"\\012\\045\\0\\08\\0\\0\"", Describe(m3));
}

TEST(StrNeTest, MatchesUnequalString) {
  Matcher<const char*> m = StrNe("Hello");
  EXPECT_TRUE(m.Matches(""));
  EXPECT_TRUE(m.Matches(NULL));
  EXPECT_FALSE(m.Matches("Hello"));

  Matcher<string> m2 = StrNe(string("Hello"));
  EXPECT_TRUE(m2.Matches("hello"));
  EXPECT_FALSE(m2.Matches("Hello"));
}

TEST(StrNeTest, CanDescribeSelf) {
  Matcher<const char*> m = StrNe("Hi");
  EXPECT_EQ("is not equal to \"Hi\"", Describe(m));
}

TEST(StrCaseEqTest, MatchesEqualStringIgnoringCase) {
  Matcher<const char*> m = StrCaseEq(string("Hello"));
  EXPECT_TRUE(m.Matches("Hello"));
  EXPECT_TRUE(m.Matches("hello"));
  EXPECT_FALSE(m.Matches("Hi"));
  EXPECT_FALSE(m.Matches(NULL));

  Matcher<const string&> m2 = StrCaseEq("Hello");
  EXPECT_TRUE(m2.Matches("hello"));
  EXPECT_FALSE(m2.Matches("Hi"));
}

TEST(StrCaseEqTest, MatchesEqualStringWith0IgnoringCase) {
  string str1("oabocdooeoo");
  string str2("OABOCDOOEOO");
  Matcher<const string&> m0 = StrCaseEq(str1);
  EXPECT_FALSE(m0.Matches(str2 + string(1, '\0')));

  str1[3] = str2[3] = '\0';
  Matcher<const string&> m1 = StrCaseEq(str1);
  EXPECT_TRUE(m1.Matches(str2));

  str1[0] = str1[6] = str1[7] = str1[10] = '\0';
  str2[0] = str2[6] = str2[7] = str2[10] = '\0';
  Matcher<const string&> m2 = StrCaseEq(str1);
  str1[9] = str2[9] = '\0';
  EXPECT_FALSE(m2.Matches(str2));

  Matcher<const string&> m3 = StrCaseEq(str1);
  EXPECT_TRUE(m3.Matches(str2));

  EXPECT_FALSE(m3.Matches(str2 + "x"));
  str2.append(1, '\0');
  EXPECT_FALSE(m3.Matches(str2));
  EXPECT_FALSE(m3.Matches(string(str2, 0, 9)));
}

TEST(StrCaseEqTest, CanDescribeSelf) {
  Matcher<string> m = StrCaseEq("Hi");
  EXPECT_EQ("is equal to (ignoring case) \"Hi\"", Describe(m));
}

TEST(StrCaseNeTest, MatchesUnequalStringIgnoringCase) {
  Matcher<const char*> m = StrCaseNe("Hello");
  EXPECT_TRUE(m.Matches("Hi"));
  EXPECT_TRUE(m.Matches(NULL));
  EXPECT_FALSE(m.Matches("Hello"));
  EXPECT_FALSE(m.Matches("hello"));

  Matcher<string> m2 = StrCaseNe(string("Hello"));
  EXPECT_TRUE(m2.Matches(""));
  EXPECT_FALSE(m2.Matches("Hello"));
}

TEST(StrCaseNeTest, CanDescribeSelf) {
  Matcher<const char*> m = StrCaseNe("Hi");
  EXPECT_EQ("is not equal to (ignoring case) \"Hi\"", Describe(m));
}

// Tests that HasSubstr() works for matching string-typed values.
TEST(HasSubstrTest, WorksForStringClasses) {
  const Matcher<string> m1 = HasSubstr("foo");
  EXPECT_TRUE(m1.Matches(string("I love food.")));
  EXPECT_FALSE(m1.Matches(string("tofo")));

  const Matcher<const std::string&> m2 = HasSubstr("foo");
  EXPECT_TRUE(m2.Matches(std::string("I love food.")));
  EXPECT_FALSE(m2.Matches(std::string("tofo")));
}

// Tests that HasSubstr() works for matching C-string-typed values.
TEST(HasSubstrTest, WorksForCStrings) {
  const Matcher<char*> m1 = HasSubstr("foo");
  EXPECT_TRUE(m1.Matches(const_cast<char*>("I love food.")));
  EXPECT_FALSE(m1.Matches(const_cast<char*>("tofo")));
  EXPECT_FALSE(m1.Matches(NULL));

  const Matcher<const char*> m2 = HasSubstr("foo");
  EXPECT_TRUE(m2.Matches("I love food."));
  EXPECT_FALSE(m2.Matches("tofo"));
  EXPECT_FALSE(m2.Matches(NULL));
}

// Tests that HasSubstr(s) describes itself properly.
TEST(HasSubstrTest, CanDescribeSelf) {
  Matcher<string> m = HasSubstr("foo\n\"");
  EXPECT_EQ("has substring \"foo\\n\\\"\"", Describe(m));
}

// Tests StartsWith(s).

TEST(StartsWithTest, MatchesStringWithGivenPrefix) {
  const Matcher<const char*> m1 = StartsWith(string(""));
  EXPECT_TRUE(m1.Matches("Hi"));
  EXPECT_TRUE(m1.Matches(""));
  EXPECT_FALSE(m1.Matches(NULL));

  const Matcher<const string&> m2 = StartsWith("Hi");
  EXPECT_TRUE(m2.Matches("Hi"));
  EXPECT_TRUE(m2.Matches("Hi Hi!"));
  EXPECT_TRUE(m2.Matches("High"));
  EXPECT_FALSE(m2.Matches("H"));
  EXPECT_FALSE(m2.Matches(" Hi"));
}

TEST(StartsWithTest, CanDescribeSelf) {
  Matcher<const std::string> m = StartsWith("Hi");
  EXPECT_EQ("starts with \"Hi\"", Describe(m));
}

// Tests EndsWith(s).

TEST(EndsWithTest, MatchesStringWithGivenSuffix) {
  const Matcher<const char*> m1 = EndsWith("");
  EXPECT_TRUE(m1.Matches("Hi"));
  EXPECT_TRUE(m1.Matches(""));
  EXPECT_FALSE(m1.Matches(NULL));

  const Matcher<const string&> m2 = EndsWith(string("Hi"));
  EXPECT_TRUE(m2.Matches("Hi"));
  EXPECT_TRUE(m2.Matches("Wow Hi Hi"));
  EXPECT_TRUE(m2.Matches("Super Hi"));
  EXPECT_FALSE(m2.Matches("i"));
  EXPECT_FALSE(m2.Matches("Hi "));
}

TEST(EndsWithTest, CanDescribeSelf) {
  Matcher<const std::string> m = EndsWith("Hi");
  EXPECT_EQ("ends with \"Hi\"", Describe(m));
}

#ifdef GMOCK_HAS_REGEX

// Tests MatchesRegex().

TEST(MatchesRegexTest, MatchesStringMatchingGivenRegex) {
  const Matcher<const char*> m1 = MatchesRegex("a.*z");
  EXPECT_TRUE(m1.Matches("az"));
  EXPECT_TRUE(m1.Matches("abcz"));
  EXPECT_FALSE(m1.Matches(NULL));

  const Matcher<const string&> m2 = MatchesRegex(new RE("a.*z"));
  EXPECT_TRUE(m2.Matches("azbz"));
  EXPECT_FALSE(m2.Matches("az1"));
  EXPECT_FALSE(m2.Matches("1az"));
}

TEST(MatchesRegexTest, CanDescribeSelf) {
  Matcher<const std::string> m1 = MatchesRegex(string("Hi.*"));
  EXPECT_EQ("matches regular expression \"Hi.*\"", Describe(m1));

  Matcher<const char*> m2 = MatchesRegex(new RE("[a-z].*"));
  EXPECT_EQ("matches regular expression \"[a-z].*\"", Describe(m2));
}

// Tests ContainsRegex().

TEST(ContainsRegexTest, MatchesStringContainingGivenRegex) {
  const Matcher<const char*> m1 = ContainsRegex(string("a.*z"));
  EXPECT_TRUE(m1.Matches("az"));
  EXPECT_TRUE(m1.Matches("0abcz1"));
  EXPECT_FALSE(m1.Matches(NULL));

  const Matcher<const string&> m2 = ContainsRegex(new RE("a.*z"));
  EXPECT_TRUE(m2.Matches("azbz"));
  EXPECT_TRUE(m2.Matches("az1"));
  EXPECT_FALSE(m2.Matches("1a"));
}

TEST(ContainsRegexTest, CanDescribeSelf) {
  Matcher<const std::string> m1 = ContainsRegex("Hi.*");
  EXPECT_EQ("contains regular expression \"Hi.*\"", Describe(m1));

  Matcher<const char*> m2 = ContainsRegex(new RE("[a-z].*"));
  EXPECT_EQ("contains regular expression \"[a-z].*\"", Describe(m2));
}
#endif  // GMOCK_HAS_REGEX

// Tests for wide strings.
#if GTEST_HAS_STD_WSTRING
TEST(StdWideStrEqTest, MatchesEqual) {
  Matcher<const wchar_t*> m = StrEq(::std::wstring(L"Hello"));
  EXPECT_TRUE(m.Matches(L"Hello"));
  EXPECT_FALSE(m.Matches(L"hello"));
  EXPECT_FALSE(m.Matches(NULL));

  Matcher<const ::std::wstring&> m2 = StrEq(L"Hello");
  EXPECT_TRUE(m2.Matches(L"Hello"));
  EXPECT_FALSE(m2.Matches(L"Hi"));

  Matcher<const ::std::wstring&> m3 = StrEq(L"\xD3\x576\x8D3\xC74D");
  EXPECT_TRUE(m3.Matches(L"\xD3\x576\x8D3\xC74D"));
  EXPECT_FALSE(m3.Matches(L"\xD3\x576\x8D3\xC74E"));

  ::std::wstring str(L"01204500800");
  str[3] = L'\0';
  Matcher<const ::std::wstring&> m4 = StrEq(str);
  EXPECT_TRUE(m4.Matches(str));
  str[0] = str[6] = str[7] = str[9] = str[10] = L'\0';
  Matcher<const ::std::wstring&> m5 = StrEq(str);
  EXPECT_TRUE(m5.Matches(str));
}

TEST(StdWideStrEqTest, CanDescribeSelf) {
  Matcher< ::std::wstring> m = StrEq(L"Hi-\'\"\?\\\a\b\f\n\r\t\v");
  EXPECT_EQ("is equal to L\"Hi-\'\\\"\\?\\\\\\a\\b\\f\\n\\r\\t\\v\"",
    Describe(m));

  Matcher< ::std::wstring> m2 = StrEq(L"\xD3\x576\x8D3\xC74D");
  EXPECT_EQ("is equal to L\"\\xD3\\x576\\x8D3\\xC74D\"",
    Describe(m2));

  ::std::wstring str(L"01204500800");
  str[3] = L'\0';
  Matcher<const ::std::wstring&> m4 = StrEq(str);
  EXPECT_EQ("is equal to L\"012\\04500800\"", Describe(m4));
  str[0] = str[6] = str[7] = str[9] = str[10] = L'\0';
  Matcher<const ::std::wstring&> m5 = StrEq(str);
  EXPECT_EQ("is equal to L\"\\012\\045\\0\\08\\0\\0\"", Describe(m5));
}

TEST(StdWideStrNeTest, MatchesUnequalString) {
  Matcher<const wchar_t*> m = StrNe(L"Hello");
  EXPECT_TRUE(m.Matches(L""));
  EXPECT_TRUE(m.Matches(NULL));
  EXPECT_FALSE(m.Matches(L"Hello"));

  Matcher< ::std::wstring> m2 = StrNe(::std::wstring(L"Hello"));
  EXPECT_TRUE(m2.Matches(L"hello"));
  EXPECT_FALSE(m2.Matches(L"Hello"));
}

TEST(StdWideStrNeTest, CanDescribeSelf) {
  Matcher<const wchar_t*> m = StrNe(L"Hi");
  EXPECT_EQ("is not equal to L\"Hi\"", Describe(m));
}

TEST(StdWideStrCaseEqTest, MatchesEqualStringIgnoringCase) {
  Matcher<const wchar_t*> m = StrCaseEq(::std::wstring(L"Hello"));
  EXPECT_TRUE(m.Matches(L"Hello"));
  EXPECT_TRUE(m.Matches(L"hello"));
  EXPECT_FALSE(m.Matches(L"Hi"));
  EXPECT_FALSE(m.Matches(NULL));

  Matcher<const ::std::wstring&> m2 = StrCaseEq(L"Hello");
  EXPECT_TRUE(m2.Matches(L"hello"));
  EXPECT_FALSE(m2.Matches(L"Hi"));
}

TEST(StdWideStrCaseEqTest, MatchesEqualStringWith0IgnoringCase) {
  ::std::wstring str1(L"oabocdooeoo");
  ::std::wstring str2(L"OABOCDOOEOO");
  Matcher<const ::std::wstring&> m0 = StrCaseEq(str1);
  EXPECT_FALSE(m0.Matches(str2 + ::std::wstring(1, L'\0')));

  str1[3] = str2[3] = L'\0';
  Matcher<const ::std::wstring&> m1 = StrCaseEq(str1);
  EXPECT_TRUE(m1.Matches(str2));

  str1[0] = str1[6] = str1[7] = str1[10] = L'\0';
  str2[0] = str2[6] = str2[7] = str2[10] = L'\0';
  Matcher<const ::std::wstring&> m2 = StrCaseEq(str1);
  str1[9] = str2[9] = L'\0';
  EXPECT_FALSE(m2.Matches(str2));

  Matcher<const ::std::wstring&> m3 = StrCaseEq(str1);
  EXPECT_TRUE(m3.Matches(str2));

  EXPECT_FALSE(m3.Matches(str2 + L"x"));
  str2.append(1, L'\0');
  EXPECT_FALSE(m3.Matches(str2));
  EXPECT_FALSE(m3.Matches(::std::wstring(str2, 0, 9)));
}

TEST(StdWideStrCaseEqTest, CanDescribeSelf) {
  Matcher< ::std::wstring> m = StrCaseEq(L"Hi");
  EXPECT_EQ("is equal to (ignoring case) L\"Hi\"", Describe(m));
}

TEST(StdWideStrCaseNeTest, MatchesUnequalStringIgnoringCase) {
  Matcher<const wchar_t*> m = StrCaseNe(L"Hello");
  EXPECT_TRUE(m.Matches(L"Hi"));
  EXPECT_TRUE(m.Matches(NULL));
  EXPECT_FALSE(m.Matches(L"Hello"));
  EXPECT_FALSE(m.Matches(L"hello"));

  Matcher< ::std::wstring> m2 = StrCaseNe(::std::wstring(L"Hello"));
  EXPECT_TRUE(m2.Matches(L""));
  EXPECT_FALSE(m2.Matches(L"Hello"));
}

TEST(StdWideStrCaseNeTest, CanDescribeSelf) {
  Matcher<const wchar_t*> m = StrCaseNe(L"Hi");
  EXPECT_EQ("is not equal to (ignoring case) L\"Hi\"", Describe(m));
}

// Tests that HasSubstr() works for matching wstring-typed values.
TEST(StdWideHasSubstrTest, WorksForStringClasses) {
  const Matcher< ::std::wstring> m1 = HasSubstr(L"foo");
  EXPECT_TRUE(m1.Matches(::std::wstring(L"I love food.")));
  EXPECT_FALSE(m1.Matches(::std::wstring(L"tofo")));

  const Matcher<const ::std::wstring&> m2 = HasSubstr(L"foo");
  EXPECT_TRUE(m2.Matches(::std::wstring(L"I love food.")));
  EXPECT_FALSE(m2.Matches(::std::wstring(L"tofo")));
}

// Tests that HasSubstr() works for matching C-wide-string-typed values.
TEST(StdWideHasSubstrTest, WorksForCStrings) {
  const Matcher<wchar_t*> m1 = HasSubstr(L"foo");
  EXPECT_TRUE(m1.Matches(const_cast<wchar_t*>(L"I love food.")));
  EXPECT_FALSE(m1.Matches(const_cast<wchar_t*>(L"tofo")));
  EXPECT_FALSE(m1.Matches(NULL));

  const Matcher<const wchar_t*> m2 = HasSubstr(L"foo");
  EXPECT_TRUE(m2.Matches(L"I love food."));
  EXPECT_FALSE(m2.Matches(L"tofo"));
  EXPECT_FALSE(m2.Matches(NULL));
}

// Tests that HasSubstr(s) describes itself properly.
TEST(StdWideHasSubstrTest, CanDescribeSelf) {
  Matcher< ::std::wstring> m = HasSubstr(L"foo\n\"");
  EXPECT_EQ("has substring L\"foo\\n\\\"\"", Describe(m));
}

// Tests StartsWith(s).

TEST(StdWideStartsWithTest, MatchesStringWithGivenPrefix) {
  const Matcher<const wchar_t*> m1 = StartsWith(::std::wstring(L""));
  EXPECT_TRUE(m1.Matches(L"Hi"));
  EXPECT_TRUE(m1.Matches(L""));
  EXPECT_FALSE(m1.Matches(NULL));

  const Matcher<const ::std::wstring&> m2 = StartsWith(L"Hi");
  EXPECT_TRUE(m2.Matches(L"Hi"));
  EXPECT_TRUE(m2.Matches(L"Hi Hi!"));
  EXPECT_TRUE(m2.Matches(L"High"));
  EXPECT_FALSE(m2.Matches(L"H"));
  EXPECT_FALSE(m2.Matches(L" Hi"));
}

TEST(StdWideStartsWithTest, CanDescribeSelf) {
  Matcher<const ::std::wstring> m = StartsWith(L"Hi");
  EXPECT_EQ("starts with L\"Hi\"", Describe(m));
}

// Tests EndsWith(s).

TEST(StdWideEndsWithTest, MatchesStringWithGivenSuffix) {
  const Matcher<const wchar_t*> m1 = EndsWith(L"");
  EXPECT_TRUE(m1.Matches(L"Hi"));
  EXPECT_TRUE(m1.Matches(L""));
  EXPECT_FALSE(m1.Matches(NULL));

  const Matcher<const ::std::wstring&> m2 = EndsWith(::std::wstring(L"Hi"));
  EXPECT_TRUE(m2.Matches(L"Hi"));
  EXPECT_TRUE(m2.Matches(L"Wow Hi Hi"));
  EXPECT_TRUE(m2.Matches(L"Super Hi"));
  EXPECT_FALSE(m2.Matches(L"i"));
  EXPECT_FALSE(m2.Matches(L"Hi "));
}

TEST(StdWideEndsWithTest, CanDescribeSelf) {
  Matcher<const ::std::wstring> m = EndsWith(L"Hi");
  EXPECT_EQ("ends with L\"Hi\"", Describe(m));
}

#endif  // GTEST_HAS_STD_WSTRING

#if GTEST_HAS_GLOBAL_WSTRING
TEST(GlobalWideStrEqTest, MatchesEqual) {
  Matcher<const wchar_t*> m = StrEq(::wstring(L"Hello"));
  EXPECT_TRUE(m.Matches(L"Hello"));
  EXPECT_FALSE(m.Matches(L"hello"));
  EXPECT_FALSE(m.Matches(NULL));

  Matcher<const ::wstring&> m2 = StrEq(L"Hello");
  EXPECT_TRUE(m2.Matches(L"Hello"));
  EXPECT_FALSE(m2.Matches(L"Hi"));

  Matcher<const ::wstring&> m3 = StrEq(L"\xD3\x576\x8D3\xC74D");
  EXPECT_TRUE(m3.Matches(L"\xD3\x576\x8D3\xC74D"));
  EXPECT_FALSE(m3.Matches(L"\xD3\x576\x8D3\xC74E"));

  ::wstring str(L"01204500800");
  str[3] = L'\0';
  Matcher<const ::wstring&> m4 = StrEq(str);
  EXPECT_TRUE(m4.Matches(str));
  str[0] = str[6] = str[7] = str[9] = str[10] = L'\0';
  Matcher<const ::wstring&> m5 = StrEq(str);
  EXPECT_TRUE(m5.Matches(str));
}

TEST(GlobalWideStrEqTest, CanDescribeSelf) {
  Matcher< ::wstring> m = StrEq(L"Hi-\'\"\?\\\a\b\f\n\r\t\v");
  EXPECT_EQ("is equal to L\"Hi-\'\\\"\\?\\\\\\a\\b\\f\\n\\r\\t\\v\"",
    Describe(m));

  Matcher< ::wstring> m2 = StrEq(L"\xD3\x576\x8D3\xC74D");
  EXPECT_EQ("is equal to L\"\\xD3\\x576\\x8D3\\xC74D\"",
    Describe(m2));

  ::wstring str(L"01204500800");
  str[3] = L'\0';
  Matcher<const ::wstring&> m4 = StrEq(str);
  EXPECT_EQ("is equal to L\"012\\04500800\"", Describe(m4));
  str[0] = str[6] = str[7] = str[9] = str[10] = L'\0';
  Matcher<const ::wstring&> m5 = StrEq(str);
  EXPECT_EQ("is equal to L\"\\012\\045\\0\\08\\0\\0\"", Describe(m5));
}

TEST(GlobalWideStrNeTest, MatchesUnequalString) {
  Matcher<const wchar_t*> m = StrNe(L"Hello");
  EXPECT_TRUE(m.Matches(L""));
  EXPECT_TRUE(m.Matches(NULL));
  EXPECT_FALSE(m.Matches(L"Hello"));

  Matcher< ::wstring> m2 = StrNe(::wstring(L"Hello"));
  EXPECT_TRUE(m2.Matches(L"hello"));
  EXPECT_FALSE(m2.Matches(L"Hello"));
}

TEST(GlobalWideStrNeTest, CanDescribeSelf) {
  Matcher<const wchar_t*> m = StrNe(L"Hi");
  EXPECT_EQ("is not equal to L\"Hi\"", Describe(m));
}

TEST(GlobalWideStrCaseEqTest, MatchesEqualStringIgnoringCase) {
  Matcher<const wchar_t*> m = StrCaseEq(::wstring(L"Hello"));
  EXPECT_TRUE(m.Matches(L"Hello"));
  EXPECT_TRUE(m.Matches(L"hello"));
  EXPECT_FALSE(m.Matches(L"Hi"));
  EXPECT_FALSE(m.Matches(NULL));

  Matcher<const ::wstring&> m2 = StrCaseEq(L"Hello");
  EXPECT_TRUE(m2.Matches(L"hello"));
  EXPECT_FALSE(m2.Matches(L"Hi"));
}

TEST(GlobalWideStrCaseEqTest, MatchesEqualStringWith0IgnoringCase) {
  ::wstring str1(L"oabocdooeoo");
  ::wstring str2(L"OABOCDOOEOO");
  Matcher<const ::wstring&> m0 = StrCaseEq(str1);
  EXPECT_FALSE(m0.Matches(str2 + ::wstring(1, L'\0')));

  str1[3] = str2[3] = L'\0';
  Matcher<const ::wstring&> m1 = StrCaseEq(str1);
  EXPECT_TRUE(m1.Matches(str2));

  str1[0] = str1[6] = str1[7] = str1[10] = L'\0';
  str2[0] = str2[6] = str2[7] = str2[10] = L'\0';
  Matcher<const ::wstring&> m2 = StrCaseEq(str1);
  str1[9] = str2[9] = L'\0';
  EXPECT_FALSE(m2.Matches(str2));

  Matcher<const ::wstring&> m3 = StrCaseEq(str1);
  EXPECT_TRUE(m3.Matches(str2));

  EXPECT_FALSE(m3.Matches(str2 + L"x"));
  str2.append(1, L'\0');
  EXPECT_FALSE(m3.Matches(str2));
  EXPECT_FALSE(m3.Matches(::wstring(str2, 0, 9)));
}

TEST(GlobalWideStrCaseEqTest, CanDescribeSelf) {
  Matcher< ::wstring> m = StrCaseEq(L"Hi");
  EXPECT_EQ("is equal to (ignoring case) L\"Hi\"", Describe(m));
}

TEST(GlobalWideStrCaseNeTest, MatchesUnequalStringIgnoringCase) {
  Matcher<const wchar_t*> m = StrCaseNe(L"Hello");
  EXPECT_TRUE(m.Matches(L"Hi"));
  EXPECT_TRUE(m.Matches(NULL));
  EXPECT_FALSE(m.Matches(L"Hello"));
  EXPECT_FALSE(m.Matches(L"hello"));

  Matcher< ::wstring> m2 = StrCaseNe(::wstring(L"Hello"));
  EXPECT_TRUE(m2.Matches(L""));
  EXPECT_FALSE(m2.Matches(L"Hello"));
}

TEST(GlobalWideStrCaseNeTest, CanDescribeSelf) {
  Matcher<const wchar_t*> m = StrCaseNe(L"Hi");
  EXPECT_EQ("is not equal to (ignoring case) L\"Hi\"", Describe(m));
}

// Tests that HasSubstr() works for matching wstring-typed values.
TEST(GlobalWideHasSubstrTest, WorksForStringClasses) {
  const Matcher< ::wstring> m1 = HasSubstr(L"foo");
  EXPECT_TRUE(m1.Matches(::wstring(L"I love food.")));
  EXPECT_FALSE(m1.Matches(::wstring(L"tofo")));

  const Matcher<const ::wstring&> m2 = HasSubstr(L"foo");
  EXPECT_TRUE(m2.Matches(::wstring(L"I love food.")));
  EXPECT_FALSE(m2.Matches(::wstring(L"tofo")));
}

// Tests that HasSubstr() works for matching C-wide-string-typed values.
TEST(GlobalWideHasSubstrTest, WorksForCStrings) {
  const Matcher<wchar_t*> m1 = HasSubstr(L"foo");
  EXPECT_TRUE(m1.Matches(const_cast<wchar_t*>(L"I love food.")));
  EXPECT_FALSE(m1.Matches(const_cast<wchar_t*>(L"tofo")));
  EXPECT_FALSE(m1.Matches(NULL));

  const Matcher<const wchar_t*> m2 = HasSubstr(L"foo");
  EXPECT_TRUE(m2.Matches(L"I love food."));
  EXPECT_FALSE(m2.Matches(L"tofo"));
  EXPECT_FALSE(m2.Matches(NULL));
}

// Tests that HasSubstr(s) describes itself properly.
TEST(GlobalWideHasSubstrTest, CanDescribeSelf) {
  Matcher< ::wstring> m = HasSubstr(L"foo\n\"");
  EXPECT_EQ("has substring L\"foo\\n\\\"\"", Describe(m));
}

// Tests StartsWith(s).

TEST(GlobalWideStartsWithTest, MatchesStringWithGivenPrefix) {
  const Matcher<const wchar_t*> m1 = StartsWith(::wstring(L""));
  EXPECT_TRUE(m1.Matches(L"Hi"));
  EXPECT_TRUE(m1.Matches(L""));
  EXPECT_FALSE(m1.Matches(NULL));

  const Matcher<const ::wstring&> m2 = StartsWith(L"Hi");
  EXPECT_TRUE(m2.Matches(L"Hi"));
  EXPECT_TRUE(m2.Matches(L"Hi Hi!"));
  EXPECT_TRUE(m2.Matches(L"High"));
  EXPECT_FALSE(m2.Matches(L"H"));
  EXPECT_FALSE(m2.Matches(L" Hi"));
}

TEST(GlobalWideStartsWithTest, CanDescribeSelf) {
  Matcher<const ::wstring> m = StartsWith(L"Hi");
  EXPECT_EQ("starts with L\"Hi\"", Describe(m));
}

// Tests EndsWith(s).

TEST(GlobalWideEndsWithTest, MatchesStringWithGivenSuffix) {
  const Matcher<const wchar_t*> m1 = EndsWith(L"");
  EXPECT_TRUE(m1.Matches(L"Hi"));
  EXPECT_TRUE(m1.Matches(L""));
  EXPECT_FALSE(m1.Matches(NULL));

  const Matcher<const ::wstring&> m2 = EndsWith(::wstring(L"Hi"));
  EXPECT_TRUE(m2.Matches(L"Hi"));
  EXPECT_TRUE(m2.Matches(L"Wow Hi Hi"));
  EXPECT_TRUE(m2.Matches(L"Super Hi"));
  EXPECT_FALSE(m2.Matches(L"i"));
  EXPECT_FALSE(m2.Matches(L"Hi "));
}

TEST(GlobalWideEndsWithTest, CanDescribeSelf) {
  Matcher<const ::wstring> m = EndsWith(L"Hi");
  EXPECT_EQ("ends with L\"Hi\"", Describe(m));
}

#endif  // GTEST_HAS_GLOBAL_WSTRING


typedef ::std::tr1::tuple<long, int> Tuple2;  // NOLINT

// Tests that Eq() matches a 2-tuple where the first field == the
// second field.
TEST(Eq2Test, MatchesEqualArguments) {
  Matcher<const Tuple2&> m = Eq();
  EXPECT_TRUE(m.Matches(Tuple2(5L, 5)));
  EXPECT_FALSE(m.Matches(Tuple2(5L, 6)));
}

// Tests that Eq() describes itself properly.
TEST(Eq2Test, CanDescribeSelf) {
  Matcher<const Tuple2&> m = Eq();
  EXPECT_EQ("argument #0 is equal to argument #1", Describe(m));
}

// Tests that Ge() matches a 2-tuple where the first field >= the
// second field.
TEST(Ge2Test, MatchesGreaterThanOrEqualArguments) {
  Matcher<const Tuple2&> m = Ge();
  EXPECT_TRUE(m.Matches(Tuple2(5L, 4)));
  EXPECT_TRUE(m.Matches(Tuple2(5L, 5)));
  EXPECT_FALSE(m.Matches(Tuple2(5L, 6)));
}

// Tests that Ge() describes itself properly.
TEST(Ge2Test, CanDescribeSelf) {
  Matcher<const Tuple2&> m = Ge();
  EXPECT_EQ("argument #0 is greater than or equal to argument #1",
            Describe(m));
}

// Tests that Gt() matches a 2-tuple where the first field > the
// second field.
TEST(Gt2Test, MatchesGreaterThanArguments) {
  Matcher<const Tuple2&> m = Gt();
  EXPECT_TRUE(m.Matches(Tuple2(5L, 4)));
  EXPECT_FALSE(m.Matches(Tuple2(5L, 5)));
  EXPECT_FALSE(m.Matches(Tuple2(5L, 6)));
}

// Tests that Gt() describes itself properly.
TEST(Gt2Test, CanDescribeSelf) {
  Matcher<const Tuple2&> m = Gt();
  EXPECT_EQ("argument #0 is greater than argument #1", Describe(m));
}

// Tests that Le() matches a 2-tuple where the first field <= the
// second field.
TEST(Le2Test, MatchesLessThanOrEqualArguments) {
  Matcher<const Tuple2&> m = Le();
  EXPECT_TRUE(m.Matches(Tuple2(5L, 6)));
  EXPECT_TRUE(m.Matches(Tuple2(5L, 5)));
  EXPECT_FALSE(m.Matches(Tuple2(5L, 4)));
}

// Tests that Le() describes itself properly.
TEST(Le2Test, CanDescribeSelf) {
  Matcher<const Tuple2&> m = Le();
  EXPECT_EQ("argument #0 is less than or equal to argument #1",
            Describe(m));
}

// Tests that Lt() matches a 2-tuple where the first field < the
// second field.
TEST(Lt2Test, MatchesLessThanArguments) {
  Matcher<const Tuple2&> m = Lt();
  EXPECT_TRUE(m.Matches(Tuple2(5L, 6)));
  EXPECT_FALSE(m.Matches(Tuple2(5L, 5)));
  EXPECT_FALSE(m.Matches(Tuple2(5L, 4)));
}

// Tests that Lt() describes itself properly.
TEST(Lt2Test, CanDescribeSelf) {
  Matcher<const Tuple2&> m = Lt();
  EXPECT_EQ("argument #0 is less than argument #1", Describe(m));
}

// Tests that Ne() matches a 2-tuple where the first field != the
// second field.
TEST(Ne2Test, MatchesUnequalArguments) {
  Matcher<const Tuple2&> m = Ne();
  EXPECT_TRUE(m.Matches(Tuple2(5L, 6)));
  EXPECT_TRUE(m.Matches(Tuple2(5L, 4)));
  EXPECT_FALSE(m.Matches(Tuple2(5L, 5)));
}

// Tests that Ne() describes itself properly.
TEST(Ne2Test, CanDescribeSelf) {
  Matcher<const Tuple2&> m = Ne();
  EXPECT_EQ("argument #0 is not equal to argument #1", Describe(m));
}

// Tests that Not(m) matches any value that doesn't match m.
TEST(NotTest, NegatesMatcher) {
  Matcher<int> m;
  m = Not(Eq(2));
  EXPECT_TRUE(m.Matches(3));
  EXPECT_FALSE(m.Matches(2));
}

// Tests that Not(m) describes itself properly.
TEST(NotTest, CanDescribeSelf) {
  Matcher<int> m = Not(Eq(5));
  EXPECT_EQ("is not equal to 5", Describe(m));
}

// Tests that AllOf(m1, ..., mn) matches any value that matches all of
// the given matchers.
TEST(AllOfTest, MatchesWhenAllMatch) {
  Matcher<int> m;
  m = AllOf(Le(2), Ge(1));
  EXPECT_TRUE(m.Matches(1));
  EXPECT_TRUE(m.Matches(2));
  EXPECT_FALSE(m.Matches(0));
  EXPECT_FALSE(m.Matches(3));

  m = AllOf(Gt(0), Ne(1), Ne(2));
  EXPECT_TRUE(m.Matches(3));
  EXPECT_FALSE(m.Matches(2));
  EXPECT_FALSE(m.Matches(1));
  EXPECT_FALSE(m.Matches(0));

  m = AllOf(Gt(0), Ne(1), Ne(2), Ne(3));
  EXPECT_TRUE(m.Matches(4));
  EXPECT_FALSE(m.Matches(3));
  EXPECT_FALSE(m.Matches(2));
  EXPECT_FALSE(m.Matches(1));
  EXPECT_FALSE(m.Matches(0));

  m = AllOf(Ge(0), Lt(10), Ne(3), Ne(5), Ne(7));
  EXPECT_TRUE(m.Matches(0));
  EXPECT_TRUE(m.Matches(1));
  EXPECT_FALSE(m.Matches(3));
}

// Tests that AllOf(m1, ..., mn) describes itself properly.
TEST(AllOfTest, CanDescribeSelf) {
  Matcher<int> m;
  m = AllOf(Le(2), Ge(1));
  EXPECT_EQ("(is less than or equal to 2) and "
            "(is greater than or equal to 1)",
            Describe(m));

  m = AllOf(Gt(0), Ne(1), Ne(2));
  EXPECT_EQ("(is greater than 0) and "
            "((is not equal to 1) and "
            "(is not equal to 2))",
            Describe(m));


  m = AllOf(Gt(0), Ne(1), Ne(2), Ne(3));
  EXPECT_EQ("(is greater than 0) and "
            "((is not equal to 1) and "
            "((is not equal to 2) and "
            "(is not equal to 3)))",
            Describe(m));


  m = AllOf(Ge(0), Lt(10), Ne(3), Ne(5), Ne(7));
  EXPECT_EQ("(is greater than or equal to 0) and "
            "((is less than 10) and "
            "((is not equal to 3) and "
            "((is not equal to 5) and "
            "(is not equal to 7))))", Describe(m));
}

// Tests that AnyOf(m1, ..., mn) matches any value that matches at
// least one of the given matchers.
TEST(AnyOfTest, MatchesWhenAnyMatches) {
  Matcher<int> m;
  m = AnyOf(Le(1), Ge(3));
  EXPECT_TRUE(m.Matches(1));
  EXPECT_TRUE(m.Matches(4));
  EXPECT_FALSE(m.Matches(2));

  m = AnyOf(Lt(0), Eq(1), Eq(2));
  EXPECT_TRUE(m.Matches(-1));
  EXPECT_TRUE(m.Matches(1));
  EXPECT_TRUE(m.Matches(2));
  EXPECT_FALSE(m.Matches(0));

  m = AnyOf(Lt(0), Eq(1), Eq(2), Eq(3));
  EXPECT_TRUE(m.Matches(-1));
  EXPECT_TRUE(m.Matches(1));
  EXPECT_TRUE(m.Matches(2));
  EXPECT_TRUE(m.Matches(3));
  EXPECT_FALSE(m.Matches(0));

  m = AnyOf(Le(0), Gt(10), 3, 5, 7);
  EXPECT_TRUE(m.Matches(0));
  EXPECT_TRUE(m.Matches(11));
  EXPECT_TRUE(m.Matches(3));
  EXPECT_FALSE(m.Matches(2));
}

// Tests that AnyOf(m1, ..., mn) describes itself properly.
TEST(AnyOfTest, CanDescribeSelf) {
  Matcher<int> m;
  m = AnyOf(Le(1), Ge(3));
  EXPECT_EQ("(is less than or equal to 1) or "
            "(is greater than or equal to 3)",
            Describe(m));

  m = AnyOf(Lt(0), Eq(1), Eq(2));
  EXPECT_EQ("(is less than 0) or "
            "((is equal to 1) or (is equal to 2))",
            Describe(m));

  m = AnyOf(Lt(0), Eq(1), Eq(2), Eq(3));
  EXPECT_EQ("(is less than 0) or "
            "((is equal to 1) or "
            "((is equal to 2) or "
            "(is equal to 3)))",
            Describe(m));

  m = AnyOf(Le(0), Gt(10), 3, 5, 7);
  EXPECT_EQ("(is less than or equal to 0) or "
            "((is greater than 10) or "
            "((is equal to 3) or "
            "((is equal to 5) or "
            "(is equal to 7))))",
            Describe(m));
}

// The following predicate function and predicate functor are for
// testing the Truly(predicate) matcher.

// Returns non-zero if the input is positive.  Note that the return
// type of this function is not bool.  It's OK as Truly() accepts any
// unary function or functor whose return type can be implicitly
// converted to bool.
int IsPositive(double x) {
  return x > 0 ? 1 : 0;
}

// This functor returns true if the input is greater than the given
// number.
class IsGreaterThan {
 public:
  explicit IsGreaterThan(int threshold) : threshold_(threshold) {}

  bool operator()(int n) const { return n > threshold_; }
 private:
  const int threshold_;
};

// For testing Truly().
const int foo = 0;

// This predicate returns true iff the argument references foo and has
// a zero value.
bool ReferencesFooAndIsZero(const int& n) {
  return (&n == &foo) && (n == 0);
}

// Tests that Truly(predicate) matches what satisfies the given
// predicate.
TEST(TrulyTest, MatchesWhatSatisfiesThePredicate) {
  Matcher<double> m = Truly(IsPositive);
  EXPECT_TRUE(m.Matches(2.0));
  EXPECT_FALSE(m.Matches(-1.5));
}

// Tests that Truly(predicate_functor) works too.
TEST(TrulyTest, CanBeUsedWithFunctor) {
  Matcher<int> m = Truly(IsGreaterThan(5));
  EXPECT_TRUE(m.Matches(6));
  EXPECT_FALSE(m.Matches(4));
}

// Tests that Truly(predicate) can describe itself properly.
TEST(TrulyTest, CanDescribeSelf) {
  Matcher<double> m = Truly(IsPositive);
  EXPECT_EQ("satisfies the given predicate",
            Describe(m));
}

// Tests that Truly(predicate) works when the matcher takes its
// argument by reference.
TEST(TrulyTest, WorksForByRefArguments) {
  Matcher<const int&> m = Truly(ReferencesFooAndIsZero);
  EXPECT_TRUE(m.Matches(foo));
  int n = 0;
  EXPECT_FALSE(m.Matches(n));
}

// Tests that Matches(m) is a predicate satisfied by whatever that
// matches matcher m.
TEST(MatchesTest, IsSatisfiedByWhatMatchesTheMatcher) {
  EXPECT_TRUE(Matches(Ge(0))(1));
  EXPECT_FALSE(Matches(Eq('a'))('b'));
}

// Tests that Matches(m) works when the matcher takes its argument by
// reference.
TEST(MatchesTest, WorksOnByRefArguments) {
  int m = 0, n = 0;
  EXPECT_TRUE(Matches(AllOf(Ref(n), Eq(0)))(n));
  EXPECT_FALSE(Matches(Ref(m))(n));
}

// Tests that a Matcher on non-reference type can be used in
// Matches().
TEST(MatchesTest, WorksWithMatcherOnNonRefType) {
  Matcher<int> eq5 = Eq(5);
  EXPECT_TRUE(Matches(eq5)(5));
  EXPECT_FALSE(Matches(eq5)(2));
}

// Tests that ASSERT_THAT() and EXPECT_THAT() work when the value
// matches the matcher.
TEST(MatcherAssertionTest, WorksWhenMatcherIsSatisfied) {
  ASSERT_THAT(5, Ge(2)) << "This should succeed.";
  ASSERT_THAT("Foo", EndsWith("oo"));
  EXPECT_THAT(2, AllOf(Le(7), Ge(0))) << "This should succeed too.";
  EXPECT_THAT("Hello", StartsWith("Hell"));
}

// Tests that ASSERT_THAT() and EXPECT_THAT() work when the value
// doesn't match the matcher.
TEST(MatcherAssertionTest, WorksWhenMatcherIsNotSatisfied) {
  // 'n' must be static as it is used in an EXPECT_FATAL_FAILURE(),
  // which cannot reference auto variables.
  static int n;
  n = 5;
  EXPECT_FATAL_FAILURE(ASSERT_THAT(n, Gt(10)) << "This should fail.",
                       "Value of: n\n"
                       "Expected: is greater than 10\n"
                       "  Actual: 5\n"
                       "This should fail.");
  n = 0;
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(n, AllOf(Le(7), Ge(5))),
                          "Value of: n\n"
                          "Expected: (is less than or equal to 7) and "
                          "(is greater than or equal to 5)\n"
                          "  Actual: 0");
}

// Tests that ASSERT_THAT() and EXPECT_THAT() work when the argument
// has a reference type.
TEST(MatcherAssertionTest, WorksForByRefArguments) {
  // We use a static variable here as EXPECT_FATAL_FAILURE() cannot
  // reference auto variables.
  static int n;
  n = 0;
  EXPECT_THAT(n, AllOf(Le(7), Ref(n)));
  EXPECT_FATAL_FAILURE(ASSERT_THAT(n, Not(Ref(n))),
                       "Value of: n\n"
                       "Expected: does not reference the variable @");
  // Tests the "Actual" part.
  EXPECT_FATAL_FAILURE(ASSERT_THAT(n, Not(Ref(n))),
                       "Actual: 0 (is located @");
}

// Tests that ASSERT_THAT() and EXPECT_THAT() work when the matcher is
// monomorphic.
TEST(MatcherAssertionTest, WorksForMonomorphicMatcher) {
  Matcher<const char*> starts_with_he = StartsWith("he");
  ASSERT_THAT("hello", starts_with_he);

  Matcher<const string&> ends_with_ok = EndsWith("ok");
  ASSERT_THAT("book", ends_with_ok);

  Matcher<int> is_greater_than_5 = Gt(5);
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(5, is_greater_than_5),
                          "Value of: 5\n"
                          "Expected: is greater than 5\n"
                          "  Actual: 5");
}

// Tests floating-point matchers.
template <typename RawType>
class FloatingPointTest : public testing::Test {
 protected:
  typedef typename testing::internal::FloatingPoint<RawType> Floating;
  typedef typename Floating::Bits Bits;

  virtual void SetUp() {
    const size_t max_ulps = Floating::kMaxUlps;

    // The bits that represent 0.0.
    const Bits zero_bits = Floating(0).bits();

    // Makes some numbers close to 0.0.
    close_to_positive_zero_ = Floating::ReinterpretBits(zero_bits + max_ulps/2);
    close_to_negative_zero_ = -Floating::ReinterpretBits(
        zero_bits + max_ulps - max_ulps/2);
    further_from_negative_zero_ = -Floating::ReinterpretBits(
        zero_bits + max_ulps + 1 - max_ulps/2);

    // The bits that represent 1.0.
    const Bits one_bits = Floating(1).bits();

    // Makes some numbers close to 1.0.
    close_to_one_ = Floating::ReinterpretBits(one_bits + max_ulps);
    further_from_one_ = Floating::ReinterpretBits(one_bits + max_ulps + 1);

    // +infinity.
    infinity_ = Floating::Infinity();

    // The bits that represent +infinity.
    const Bits infinity_bits = Floating(infinity_).bits();

    // Makes some numbers close to infinity.
    close_to_infinity_ = Floating::ReinterpretBits(infinity_bits - max_ulps);
    further_from_infinity_ = Floating::ReinterpretBits(
        infinity_bits - max_ulps - 1);

    // Makes some NAN's.
    nan1_ = Floating::ReinterpretBits(Floating::kExponentBitMask | 1);
    nan2_ = Floating::ReinterpretBits(Floating::kExponentBitMask | 200);
  }

  void TestSize() {
    EXPECT_EQ(sizeof(RawType), sizeof(Bits));
  }

  // A battery of tests for FloatingEqMatcher::Matches.
  // matcher_maker is a pointer to a function which creates a FloatingEqMatcher.
  void TestMatches(
      testing::internal::FloatingEqMatcher<RawType> (*matcher_maker)(RawType)) {
    Matcher<RawType> m1 = matcher_maker(0.0);
    EXPECT_TRUE(m1.Matches(-0.0));
    EXPECT_TRUE(m1.Matches(close_to_positive_zero_));
    EXPECT_TRUE(m1.Matches(close_to_negative_zero_));
    EXPECT_FALSE(m1.Matches(1.0));

    Matcher<RawType> m2 = matcher_maker(close_to_positive_zero_);
    EXPECT_FALSE(m2.Matches(further_from_negative_zero_));

    Matcher<RawType> m3 = matcher_maker(1.0);
    EXPECT_TRUE(m3.Matches(close_to_one_));
    EXPECT_FALSE(m3.Matches(further_from_one_));

    // Test commutativity: matcher_maker(0.0).Matches(1.0) was tested above.
    EXPECT_FALSE(m3.Matches(0.0));

    Matcher<RawType> m4 = matcher_maker(-infinity_);
    EXPECT_TRUE(m4.Matches(-close_to_infinity_));

    Matcher<RawType> m5 = matcher_maker(infinity_);
    EXPECT_TRUE(m5.Matches(close_to_infinity_));

    // This is interesting as the representations of infinity_ and nan1_
    // are only 1 DLP apart.
    EXPECT_FALSE(m5.Matches(nan1_));

    // matcher_maker can produce a Matcher<const RawType&>, which is needed in
    // some cases.
    Matcher<const RawType&> m6 = matcher_maker(0.0);
    EXPECT_TRUE(m6.Matches(-0.0));
    EXPECT_TRUE(m6.Matches(close_to_positive_zero_));
    EXPECT_FALSE(m6.Matches(1.0));

    // matcher_maker can produce a Matcher<RawType&>, which is needed in some
    // cases.
    Matcher<RawType&> m7 = matcher_maker(0.0);
    RawType x = 0.0;
    EXPECT_TRUE(m7.Matches(x));
    x = 0.01f;
    EXPECT_FALSE(m7.Matches(x));
  }

  // Pre-calculated numbers to be used by the tests.

  static RawType close_to_positive_zero_;
  static RawType close_to_negative_zero_;
  static RawType further_from_negative_zero_;

  static RawType close_to_one_;
  static RawType further_from_one_;

  static RawType infinity_;
  static RawType close_to_infinity_;
  static RawType further_from_infinity_;

  static RawType nan1_;
  static RawType nan2_;
};

template <typename RawType>
RawType FloatingPointTest<RawType>::close_to_positive_zero_;

template <typename RawType>
RawType FloatingPointTest<RawType>::close_to_negative_zero_;

template <typename RawType>
RawType FloatingPointTest<RawType>::further_from_negative_zero_;

template <typename RawType>
RawType FloatingPointTest<RawType>::close_to_one_;

template <typename RawType>
RawType FloatingPointTest<RawType>::further_from_one_;

template <typename RawType>
RawType FloatingPointTest<RawType>::infinity_;

template <typename RawType>
RawType FloatingPointTest<RawType>::close_to_infinity_;

template <typename RawType>
RawType FloatingPointTest<RawType>::further_from_infinity_;

template <typename RawType>
RawType FloatingPointTest<RawType>::nan1_;

template <typename RawType>
RawType FloatingPointTest<RawType>::nan2_;

// Instantiate FloatingPointTest for testing floats.
typedef FloatingPointTest<float> FloatTest;

TEST_F(FloatTest, FloatEqApproximatelyMatchesFloats) {
  TestMatches(&FloatEq);
}

TEST_F(FloatTest, NanSensitiveFloatEqApproximatelyMatchesFloats) {
  TestMatches(&NanSensitiveFloatEq);
}

TEST_F(FloatTest, FloatEqCannotMatchNaN) {
  // FloatEq never matches NaN.
  Matcher<float> m = FloatEq(nan1_);
  EXPECT_FALSE(m.Matches(nan1_));
  EXPECT_FALSE(m.Matches(nan2_));
  EXPECT_FALSE(m.Matches(1.0));
}

TEST_F(FloatTest, NanSensitiveFloatEqCanMatchNaN) {
  // NanSensitiveFloatEq will match NaN.
  Matcher<float> m = NanSensitiveFloatEq(nan1_);
  EXPECT_TRUE(m.Matches(nan1_));
  EXPECT_TRUE(m.Matches(nan2_));
  EXPECT_FALSE(m.Matches(1.0));
}

TEST_F(FloatTest, FloatEqCanDescribeSelf) {
  Matcher<float> m1 = FloatEq(2.0f);
  EXPECT_EQ("is approximately 2", Describe(m1));
  EXPECT_EQ("is not approximately 2", DescribeNegation(m1));

  Matcher<float> m2 = FloatEq(0.5f);
  EXPECT_EQ("is approximately 0.5", Describe(m2));
  EXPECT_EQ("is not approximately 0.5", DescribeNegation(m2));

  Matcher<float> m3 = FloatEq(nan1_);
  EXPECT_EQ("never matches", Describe(m3));
  EXPECT_EQ("is anything", DescribeNegation(m3));
}

TEST_F(FloatTest, NanSensitiveFloatEqCanDescribeSelf) {
  Matcher<float> m1 = NanSensitiveFloatEq(2.0f);
  EXPECT_EQ("is approximately 2", Describe(m1));
  EXPECT_EQ("is not approximately 2", DescribeNegation(m1));

  Matcher<float> m2 = NanSensitiveFloatEq(0.5f);
  EXPECT_EQ("is approximately 0.5", Describe(m2));
  EXPECT_EQ("is not approximately 0.5", DescribeNegation(m2));

  Matcher<float> m3 = NanSensitiveFloatEq(nan1_);
  EXPECT_EQ("is NaN", Describe(m3));
  EXPECT_EQ("is not NaN", DescribeNegation(m3));
}

// Instantiate FloatingPointTest for testing doubles.
typedef FloatingPointTest<double> DoubleTest;

TEST_F(DoubleTest, DoubleEqApproximatelyMatchesDoubles) {
  TestMatches(&DoubleEq);
}

TEST_F(DoubleTest, NanSensitiveDoubleEqApproximatelyMatchesDoubles) {
  TestMatches(&NanSensitiveDoubleEq);
}

TEST_F(DoubleTest, DoubleEqCannotMatchNaN) {
  // DoubleEq never matches NaN.
  Matcher<double> m = DoubleEq(nan1_);
  EXPECT_FALSE(m.Matches(nan1_));
  EXPECT_FALSE(m.Matches(nan2_));
  EXPECT_FALSE(m.Matches(1.0));
}

TEST_F(DoubleTest, NanSensitiveDoubleEqCanMatchNaN) {
  // NanSensitiveDoubleEq will match NaN.
  Matcher<double> m = NanSensitiveDoubleEq(nan1_);
  EXPECT_TRUE(m.Matches(nan1_));
  EXPECT_TRUE(m.Matches(nan2_));
  EXPECT_FALSE(m.Matches(1.0));
}

TEST_F(DoubleTest, DoubleEqCanDescribeSelf) {
  Matcher<double> m1 = DoubleEq(2.0);
  EXPECT_EQ("is approximately 2", Describe(m1));
  EXPECT_EQ("is not approximately 2", DescribeNegation(m1));

  Matcher<double> m2 = DoubleEq(0.5);
  EXPECT_EQ("is approximately 0.5", Describe(m2));
  EXPECT_EQ("is not approximately 0.5", DescribeNegation(m2));

  Matcher<double> m3 = DoubleEq(nan1_);
  EXPECT_EQ("never matches", Describe(m3));
  EXPECT_EQ("is anything", DescribeNegation(m3));
}

TEST_F(DoubleTest, NanSensitiveDoubleEqCanDescribeSelf) {
  Matcher<double> m1 = NanSensitiveDoubleEq(2.0);
  EXPECT_EQ("is approximately 2", Describe(m1));
  EXPECT_EQ("is not approximately 2", DescribeNegation(m1));

  Matcher<double> m2 = NanSensitiveDoubleEq(0.5);
  EXPECT_EQ("is approximately 0.5", Describe(m2));
  EXPECT_EQ("is not approximately 0.5", DescribeNegation(m2));

  Matcher<double> m3 = NanSensitiveDoubleEq(nan1_);
  EXPECT_EQ("is NaN", Describe(m3));
  EXPECT_EQ("is not NaN", DescribeNegation(m3));
}

TEST(PointeeTest, RawPointer) {
  const Matcher<int*> m = Pointee(Ge(0));

  int n = 1;
  EXPECT_TRUE(m.Matches(&n));
  n = -1;
  EXPECT_FALSE(m.Matches(&n));
  EXPECT_FALSE(m.Matches(NULL));
}

TEST(PointeeTest, RawPointerToConst) {
  const Matcher<const double*> m = Pointee(Ge(0));

  double x = 1;
  EXPECT_TRUE(m.Matches(&x));
  x = -1;
  EXPECT_FALSE(m.Matches(&x));
  EXPECT_FALSE(m.Matches(NULL));
}

TEST(PointeeTest, ReferenceToConstRawPointer) {
  const Matcher<int* const &> m = Pointee(Ge(0));

  int n = 1;
  EXPECT_TRUE(m.Matches(&n));
  n = -1;
  EXPECT_FALSE(m.Matches(&n));
  EXPECT_FALSE(m.Matches(NULL));
}

TEST(PointeeTest, ReferenceToNonConstRawPointer) {
  const Matcher<double* &> m = Pointee(Ge(0));

  double x = 1.0;
  double* p = &x;
  EXPECT_TRUE(m.Matches(p));
  x = -1;
  EXPECT_FALSE(m.Matches(p));
  p = NULL;
  EXPECT_FALSE(m.Matches(p));
}

TEST(PointeeTest, NeverMatchesNull) {
  const Matcher<const char*> m = Pointee(_);
  EXPECT_FALSE(m.Matches(NULL));
}

// Tests that we can write Pointee(value) instead of Pointee(Eq(value)).
TEST(PointeeTest, MatchesAgainstAValue) {
  const Matcher<int*> m = Pointee(5);

  int n = 5;
  EXPECT_TRUE(m.Matches(&n));
  n = -1;
  EXPECT_FALSE(m.Matches(&n));
  EXPECT_FALSE(m.Matches(NULL));
}

TEST(PointeeTest, CanDescribeSelf) {
  const Matcher<int*> m = Pointee(Gt(3));
  EXPECT_EQ("points to a value that is greater than 3", Describe(m));
  EXPECT_EQ("does not point to a value that is greater than 3",
            DescribeNegation(m));
}

// For testing ExplainMatchResultTo().
class GreaterThanMatcher : public MatcherInterface<int> {
 public:
  explicit GreaterThanMatcher(int rhs) : rhs_(rhs) {}

  virtual bool Matches(int lhs) const { return lhs > rhs_; }

  virtual void DescribeTo(::std::ostream* os) const {
    *os << "is greater than " << rhs_;
  }

  virtual void ExplainMatchResultTo(int lhs, ::std::ostream* os) const {
    const int diff = lhs - rhs_;
    if (diff > 0) {
      *os << "is " << diff << " more than " << rhs_;
    } else if (diff == 0) {
      *os << "is the same as " << rhs_;
    } else {
      *os << "is " << -diff << " less than " << rhs_;
    }
  }
 private:
  const int rhs_;
};

Matcher<int> GreaterThan(int n) {
  return MakeMatcher(new GreaterThanMatcher(n));
}

TEST(PointeeTest, CanExplainMatchResult) {
  const Matcher<const string*> m = Pointee(StartsWith("Hi"));

  EXPECT_EQ("", Explain(m, static_cast<const string*>(NULL)));

  const Matcher<int*> m2 = Pointee(GreaterThan(1));
  int n = 3;
  EXPECT_EQ("points to a value that is 2 more than 1", Explain(m2, &n));
}

// An uncopyable class.
class Uncopyable {
 public:
  explicit Uncopyable(int value) : value_(value) {}

  int value() const { return value_; }
 private:
  const int value_;
  GTEST_DISALLOW_COPY_AND_ASSIGN_(Uncopyable);
};

// Returns true iff x.value() is positive.
bool ValueIsPositive(const Uncopyable& x) { return x.value() > 0; }

// A user-defined struct for testing Field().
struct AStruct {
  AStruct() : x(0), y(1.0), z(5), p(NULL) {}
  AStruct(const AStruct& rhs)
      : x(rhs.x), y(rhs.y), z(rhs.z.value()), p(rhs.p) {}

  int x;           // A non-const field.
  const double y;  // A const field.
  Uncopyable z;    // An uncopyable field.
  const char* p;   // A pointer field.
};

// A derived struct for testing Field().
struct DerivedStruct : public AStruct {
  char ch;
};

// Tests that Field(&Foo::field, ...) works when field is non-const.
TEST(FieldTest, WorksForNonConstField) {
  Matcher<AStruct> m = Field(&AStruct::x, Ge(0));

  AStruct a;
  EXPECT_TRUE(m.Matches(a));
  a.x = -1;
  EXPECT_FALSE(m.Matches(a));
}

// Tests that Field(&Foo::field, ...) works when field is const.
TEST(FieldTest, WorksForConstField) {
  AStruct a;

  Matcher<AStruct> m = Field(&AStruct::y, Ge(0.0));
  EXPECT_TRUE(m.Matches(a));
  m = Field(&AStruct::y, Le(0.0));
  EXPECT_FALSE(m.Matches(a));
}

// Tests that Field(&Foo::field, ...) works when field is not copyable.
TEST(FieldTest, WorksForUncopyableField) {
  AStruct a;

  Matcher<AStruct> m = Field(&AStruct::z, Truly(ValueIsPositive));
  EXPECT_TRUE(m.Matches(a));
  m = Field(&AStruct::z, Not(Truly(ValueIsPositive)));
  EXPECT_FALSE(m.Matches(a));
}

// Tests that Field(&Foo::field, ...) works when field is a pointer.
TEST(FieldTest, WorksForPointerField) {
  // Matching against NULL.
  Matcher<AStruct> m = Field(&AStruct::p, static_cast<const char*>(NULL));
  AStruct a;
  EXPECT_TRUE(m.Matches(a));
  a.p = "hi";
  EXPECT_FALSE(m.Matches(a));

  // Matching a pointer that is not NULL.
  m = Field(&AStruct::p, StartsWith("hi"));
  a.p = "hill";
  EXPECT_TRUE(m.Matches(a));
  a.p = "hole";
  EXPECT_FALSE(m.Matches(a));
}

// Tests that Field() works when the object is passed by reference.
TEST(FieldTest, WorksForByRefArgument) {
  Matcher<const AStruct&> m = Field(&AStruct::x, Ge(0));

  AStruct a;
  EXPECT_TRUE(m.Matches(a));
  a.x = -1;
  EXPECT_FALSE(m.Matches(a));
}

// Tests that Field(&Foo::field, ...) works when the argument's type
// is a sub-type of Foo.
TEST(FieldTest, WorksForArgumentOfSubType) {
  // Note that the matcher expects DerivedStruct but we say AStruct
  // inside Field().
  Matcher<const DerivedStruct&> m = Field(&AStruct::x, Ge(0));

  DerivedStruct d;
  EXPECT_TRUE(m.Matches(d));
  d.x = -1;
  EXPECT_FALSE(m.Matches(d));
}

// Tests that Field(&Foo::field, m) works when field's type and m's
// argument type are compatible but not the same.
TEST(FieldTest, WorksForCompatibleMatcherType) {
  // The field is an int, but the inner matcher expects a signed char.
  Matcher<const AStruct&> m = Field(&AStruct::x,
                                    Matcher<signed char>(Ge(0)));

  AStruct a;
  EXPECT_TRUE(m.Matches(a));
  a.x = -1;
  EXPECT_FALSE(m.Matches(a));
}

// Tests that Field() can describe itself.
TEST(FieldTest, CanDescribeSelf) {
  Matcher<const AStruct&> m = Field(&AStruct::x, Ge(0));

  EXPECT_EQ("the given field is greater than or equal to 0", Describe(m));
  EXPECT_EQ("the given field is not greater than or equal to 0",
            DescribeNegation(m));
}

// Tests that Field() can explain the match result.
TEST(FieldTest, CanExplainMatchResult) {
  Matcher<const AStruct&> m = Field(&AStruct::x, Ge(0));

  AStruct a;
  a.x = 1;
  EXPECT_EQ("", Explain(m, a));

  m = Field(&AStruct::x, GreaterThan(0));
  EXPECT_EQ("the given field is 1 more than 0", Explain(m, a));
}

// Tests that Field() works when the argument is a pointer to const.
TEST(FieldForPointerTest, WorksForPointerToConst) {
  Matcher<const AStruct*> m = Field(&AStruct::x, Ge(0));

  AStruct a;
  EXPECT_TRUE(m.Matches(&a));
  a.x = -1;
  EXPECT_FALSE(m.Matches(&a));
}

// Tests that Field() works when the argument is a pointer to non-const.
TEST(FieldForPointerTest, WorksForPointerToNonConst) {
  Matcher<AStruct*> m = Field(&AStruct::x, Ge(0));

  AStruct a;
  EXPECT_TRUE(m.Matches(&a));
  a.x = -1;
  EXPECT_FALSE(m.Matches(&a));
}

// Tests that Field() does not match the NULL pointer.
TEST(FieldForPointerTest, DoesNotMatchNull) {
  Matcher<const AStruct*> m = Field(&AStruct::x, _);
  EXPECT_FALSE(m.Matches(NULL));
}

// Tests that Field(&Foo::field, ...) works when the argument's type
// is a sub-type of const Foo*.
TEST(FieldForPointerTest, WorksForArgumentOfSubType) {
  // Note that the matcher expects DerivedStruct but we say AStruct
  // inside Field().
  Matcher<DerivedStruct*> m = Field(&AStruct::x, Ge(0));

  DerivedStruct d;
  EXPECT_TRUE(m.Matches(&d));
  d.x = -1;
  EXPECT_FALSE(m.Matches(&d));
}

// Tests that Field() can describe itself when used to match a pointer.
TEST(FieldForPointerTest, CanDescribeSelf) {
  Matcher<const AStruct*> m = Field(&AStruct::x, Ge(0));

  EXPECT_EQ("the given field is greater than or equal to 0", Describe(m));
  EXPECT_EQ("the given field is not greater than or equal to 0",
            DescribeNegation(m));
}

// Tests that Field() can explain the result of matching a pointer.
TEST(FieldForPointerTest, CanExplainMatchResult) {
  Matcher<const AStruct*> m = Field(&AStruct::x, Ge(0));

  AStruct a;
  a.x = 1;
  EXPECT_EQ("", Explain(m, static_cast<const AStruct*>(NULL)));
  EXPECT_EQ("", Explain(m, &a));

  m = Field(&AStruct::x, GreaterThan(0));
  EXPECT_EQ("the given field is 1 more than 0", Explain(m, &a));
}

// A user-defined class for testing Property().
class AClass {
 public:
  AClass() : n_(0) {}

  // A getter that returns a non-reference.
  int n() const { return n_; }

  void set_n(int new_n) { n_ = new_n; }

  // A getter that returns a reference to const.
  const string& s() const { return s_; }

  void set_s(const string& new_s) { s_ = new_s; }

  // A getter that returns a reference to non-const.
  double& x() const { return x_; }
 private:
  int n_;
  string s_;

  static double x_;
};

double AClass::x_ = 0.0;

// A derived class for testing Property().
class DerivedClass : public AClass {
 private:
  int k_;
};

// Tests that Property(&Foo::property, ...) works when property()
// returns a non-reference.
TEST(PropertyTest, WorksForNonReferenceProperty) {
  Matcher<const AClass&> m = Property(&AClass::n, Ge(0));

  AClass a;
  a.set_n(1);
  EXPECT_TRUE(m.Matches(a));

  a.set_n(-1);
  EXPECT_FALSE(m.Matches(a));
}

// Tests that Property(&Foo::property, ...) works when property()
// returns a reference to const.
TEST(PropertyTest, WorksForReferenceToConstProperty) {
  Matcher<const AClass&> m = Property(&AClass::s, StartsWith("hi"));

  AClass a;
  a.set_s("hill");
  EXPECT_TRUE(m.Matches(a));

  a.set_s("hole");
  EXPECT_FALSE(m.Matches(a));
}

// Tests that Property(&Foo::property, ...) works when property()
// returns a reference to non-const.
TEST(PropertyTest, WorksForReferenceToNonConstProperty) {
  double x = 0.0;
  AClass a;

  Matcher<const AClass&> m = Property(&AClass::x, Ref(x));
  EXPECT_FALSE(m.Matches(a));

  m = Property(&AClass::x, Not(Ref(x)));
  EXPECT_TRUE(m.Matches(a));
}

// Tests that Property(&Foo::property, ...) works when the argument is
// passed by value.
TEST(PropertyTest, WorksForByValueArgument) {
  Matcher<AClass> m = Property(&AClass::s, StartsWith("hi"));

  AClass a;
  a.set_s("hill");
  EXPECT_TRUE(m.Matches(a));

  a.set_s("hole");
  EXPECT_FALSE(m.Matches(a));
}

// Tests that Property(&Foo::property, ...) works when the argument's
// type is a sub-type of Foo.
TEST(PropertyTest, WorksForArgumentOfSubType) {
  // The matcher expects a DerivedClass, but inside the Property() we
  // say AClass.
  Matcher<const DerivedClass&> m = Property(&AClass::n, Ge(0));

  DerivedClass d;
  d.set_n(1);
  EXPECT_TRUE(m.Matches(d));

  d.set_n(-1);
  EXPECT_FALSE(m.Matches(d));
}

// Tests that Property(&Foo::property, m) works when property()'s type
// and m's argument type are compatible but different.
TEST(PropertyTest, WorksForCompatibleMatcherType) {
  // n() returns an int but the inner matcher expects a signed char.
  Matcher<const AClass&> m = Property(&AClass::n,
                                      Matcher<signed char>(Ge(0)));

  AClass a;
  EXPECT_TRUE(m.Matches(a));
  a.set_n(-1);
  EXPECT_FALSE(m.Matches(a));
}

// Tests that Property() can describe itself.
TEST(PropertyTest, CanDescribeSelf) {
  Matcher<const AClass&> m = Property(&AClass::n, Ge(0));

  EXPECT_EQ("the given property is greater than or equal to 0", Describe(m));
  EXPECT_EQ("the given property is not greater than or equal to 0",
            DescribeNegation(m));
}

// Tests that Property() can explain the match result.
TEST(PropertyTest, CanExplainMatchResult) {
  Matcher<const AClass&> m = Property(&AClass::n, Ge(0));

  AClass a;
  a.set_n(1);
  EXPECT_EQ("", Explain(m, a));

  m = Property(&AClass::n, GreaterThan(0));
  EXPECT_EQ("the given property is 1 more than 0", Explain(m, a));
}

// Tests that Property() works when the argument is a pointer to const.
TEST(PropertyForPointerTest, WorksForPointerToConst) {
  Matcher<const AClass*> m = Property(&AClass::n, Ge(0));

  AClass a;
  a.set_n(1);
  EXPECT_TRUE(m.Matches(&a));

  a.set_n(-1);
  EXPECT_FALSE(m.Matches(&a));
}

// Tests that Property() works when the argument is a pointer to non-const.
TEST(PropertyForPointerTest, WorksForPointerToNonConst) {
  Matcher<AClass*> m = Property(&AClass::s, StartsWith("hi"));

  AClass a;
  a.set_s("hill");
  EXPECT_TRUE(m.Matches(&a));

  a.set_s("hole");
  EXPECT_FALSE(m.Matches(&a));
}

// Tests that Property() does not match the NULL pointer.
TEST(PropertyForPointerTest, WorksForReferenceToNonConstProperty) {
  Matcher<const AClass*> m = Property(&AClass::x, _);
  EXPECT_FALSE(m.Matches(NULL));
}

// Tests that Property(&Foo::property, ...) works when the argument's
// type is a sub-type of const Foo*.
TEST(PropertyForPointerTest, WorksForArgumentOfSubType) {
  // The matcher expects a DerivedClass, but inside the Property() we
  // say AClass.
  Matcher<const DerivedClass*> m = Property(&AClass::n, Ge(0));

  DerivedClass d;
  d.set_n(1);
  EXPECT_TRUE(m.Matches(&d));

  d.set_n(-1);
  EXPECT_FALSE(m.Matches(&d));
}

// Tests that Property() can describe itself when used to match a pointer.
TEST(PropertyForPointerTest, CanDescribeSelf) {
  Matcher<const AClass*> m = Property(&AClass::n, Ge(0));

  EXPECT_EQ("the given property is greater than or equal to 0", Describe(m));
  EXPECT_EQ("the given property is not greater than or equal to 0",
            DescribeNegation(m));
}

// Tests that Property() can explain the result of matching a pointer.
TEST(PropertyForPointerTest, CanExplainMatchResult) {
  Matcher<const AClass*> m = Property(&AClass::n, Ge(0));

  AClass a;
  a.set_n(1);
  EXPECT_EQ("", Explain(m, static_cast<const AClass*>(NULL)));
  EXPECT_EQ("", Explain(m, &a));

  m = Property(&AClass::n, GreaterThan(0));
  EXPECT_EQ("the given property is 1 more than 0", Explain(m, &a));
}

// Tests ResultOf.

// Tests that ResultOf(f, ...) compiles and works as expected when f is a
// function pointer.
string IntToStringFunction(int input) { return input == 1 ? "foo" : "bar"; }

TEST(ResultOfTest, WorksForFunctionPointers) {
  Matcher<int> matcher = ResultOf(&IntToStringFunction, Eq(string("foo")));

  EXPECT_TRUE(matcher.Matches(1));
  EXPECT_FALSE(matcher.Matches(2));
}

// Tests that ResultOf() can describe itself.
TEST(ResultOfTest, CanDescribeItself) {
  Matcher<int> matcher = ResultOf(&IntToStringFunction, StrEq("foo"));

  EXPECT_EQ("result of the given callable is equal to \"foo\"",
            Describe(matcher));
  EXPECT_EQ("result of the given callable is not equal to \"foo\"",
            DescribeNegation(matcher));
}

// Tests that ResultOf() can explain the match result.
int IntFunction(int input) { return input == 42 ? 80 : 90; }

TEST(ResultOfTest, CanExplainMatchResult) {
  Matcher<int> matcher = ResultOf(&IntFunction, Ge(85));
  EXPECT_EQ("", Explain(matcher, 36));

  matcher = ResultOf(&IntFunction, GreaterThan(85));
  EXPECT_EQ("result of the given callable is 5 more than 85",
            Explain(matcher, 36));
}

// Tests that ResultOf(f, ...) compiles and works as expected when f(x)
// returns a non-reference.
TEST(ResultOfTest, WorksForNonReferenceResults) {
  Matcher<int> matcher = ResultOf(&IntFunction, Eq(80));

  EXPECT_TRUE(matcher.Matches(42));
  EXPECT_FALSE(matcher.Matches(36));
}

// Tests that ResultOf(f, ...) compiles and works as expected when f(x)
// returns a reference to non-const.
double& DoubleFunction(double& input) { return input; }

Uncopyable& RefUncopyableFunction(Uncopyable& obj) {
  return obj;
}

TEST(ResultOfTest, WorksForReferenceToNonConstResults) {
  double x = 3.14;
  double x2 = x;
  Matcher<double&> matcher = ResultOf(&DoubleFunction, Ref(x));

  EXPECT_TRUE(matcher.Matches(x));
  EXPECT_FALSE(matcher.Matches(x2));

  // Test that ResultOf works with uncopyable objects
  Uncopyable obj(0);
  Uncopyable obj2(0);
  Matcher<Uncopyable&> matcher2 =
      ResultOf(&RefUncopyableFunction, Ref(obj));

  EXPECT_TRUE(matcher2.Matches(obj));
  EXPECT_FALSE(matcher2.Matches(obj2));
}

// Tests that ResultOf(f, ...) compiles and works as expected when f(x)
// returns a reference to const.
const string& StringFunction(const string& input) { return input; }

TEST(ResultOfTest, WorksForReferenceToConstResults) {
  string s = "foo";
  string s2 = s;
  Matcher<const string&> matcher = ResultOf(&StringFunction, Ref(s));

  EXPECT_TRUE(matcher.Matches(s));
  EXPECT_FALSE(matcher.Matches(s2));
}

// Tests that ResultOf(f, m) works when f(x) and m's
// argument types are compatible but different.
TEST(ResultOfTest, WorksForCompatibleMatcherTypes) {
  // IntFunction() returns int but the inner matcher expects a signed char.
  Matcher<int> matcher = ResultOf(IntFunction, Matcher<signed char>(Ge(85)));

  EXPECT_TRUE(matcher.Matches(36));
  EXPECT_FALSE(matcher.Matches(42));
}

#if GTEST_HAS_DEATH_TEST
// Tests that the program aborts when ResultOf is passed
// a NULL function pointer.
TEST(ResultOfDeathTest, DiesOnNullFunctionPointers) {
  EXPECT_DEATH(
      ResultOf(static_cast<string(*)(int)>(NULL), Eq(string("foo"))),
               "NULL function pointer is passed into ResultOf\\(\\)\\.");
}
#endif  // GTEST_HAS_DEATH_TEST

// Tests that ResultOf(f, ...) compiles and works as expected when f is a
// function reference.
TEST(ResultOfTest, WorksForFunctionReferences) {
  Matcher<int> matcher = ResultOf(IntToStringFunction, StrEq("foo"));
  EXPECT_TRUE(matcher.Matches(1));
  EXPECT_FALSE(matcher.Matches(2));
}

// Tests that ResultOf(f, ...) compiles and works as expected when f is a
// function object.
struct Functor : public ::std::unary_function<int, string> {
  result_type operator()(argument_type input) const {
    return IntToStringFunction(input);
  }
};

TEST(ResultOfTest, WorksForFunctors) {
  Matcher<int> matcher = ResultOf(Functor(), Eq(string("foo")));

  EXPECT_TRUE(matcher.Matches(1));
  EXPECT_FALSE(matcher.Matches(2));
}

// Tests that ResultOf(f, ...) compiles and works as expected when f is a
// functor with more then one operator() defined. ResultOf() must work
// for each defined operator().
struct PolymorphicFunctor {
  typedef int result_type;
  int operator()(int n) { return n; }
  int operator()(const char* s) { return static_cast<int>(strlen(s)); }
};

TEST(ResultOfTest, WorksForPolymorphicFunctors) {
  Matcher<int> matcher_int = ResultOf(PolymorphicFunctor(), Ge(5));

  EXPECT_TRUE(matcher_int.Matches(10));
  EXPECT_FALSE(matcher_int.Matches(2));

  Matcher<const char*> matcher_string = ResultOf(PolymorphicFunctor(), Ge(5));

  EXPECT_TRUE(matcher_string.Matches("long string"));
  EXPECT_FALSE(matcher_string.Matches("shrt"));
}

const int* ReferencingFunction(const int& n) { return &n; }

struct ReferencingFunctor {
  typedef const int* result_type;
  result_type operator()(const int& n) { return &n; }
};

TEST(ResultOfTest, WorksForReferencingCallables) {
  const int n = 1;
  const int n2 = 1;
  Matcher<const int&> matcher2 = ResultOf(ReferencingFunction, Eq(&n));
  EXPECT_TRUE(matcher2.Matches(n));
  EXPECT_FALSE(matcher2.Matches(n2));

  Matcher<const int&> matcher3 = ResultOf(ReferencingFunctor(), Eq(&n));
  EXPECT_TRUE(matcher3.Matches(n));
  EXPECT_FALSE(matcher3.Matches(n2));
}


class DivisibleByImpl {
 public:
  explicit DivisibleByImpl(int divider) : divider_(divider) {}

  template <typename T>
  bool Matches(const T& n) const {
    return (n % divider_) == 0;
  }

  void DescribeTo(::std::ostream* os) const {
    *os << "is divisible by " << divider_;
  }

  void DescribeNegationTo(::std::ostream* os) const {
    *os << "is not divisible by " << divider_;
  }

  int divider() const { return divider_; }
 private:
  const int divider_;
};

// For testing using ExplainMatchResultTo() with polymorphic matchers.
template <typename T>
void ExplainMatchResultTo(const DivisibleByImpl& impl, const T& n,
                          ::std::ostream* os) {
  *os << "is " << (n % impl.divider()) << " modulo "
      << impl.divider();
}

PolymorphicMatcher<DivisibleByImpl> DivisibleBy(int n) {
  return MakePolymorphicMatcher(DivisibleByImpl(n));
}

// Tests that when AllOf() fails, only the first failing matcher is
// asked to explain why.
TEST(ExplainMatchResultTest, AllOf_False_False) {
  const Matcher<int> m = AllOf(DivisibleBy(4), DivisibleBy(3));
  EXPECT_EQ("is 1 modulo 4", Explain(m, 5));
}

// Tests that when AllOf() fails, only the first failing matcher is
// asked to explain why.
TEST(ExplainMatchResultTest, AllOf_False_True) {
  const Matcher<int> m = AllOf(DivisibleBy(4), DivisibleBy(3));
  EXPECT_EQ("is 2 modulo 4", Explain(m, 6));
}

// Tests that when AllOf() fails, only the first failing matcher is
// asked to explain why.
TEST(ExplainMatchResultTest, AllOf_True_False) {
  const Matcher<int> m = AllOf(Ge(1), DivisibleBy(3));
  EXPECT_EQ("is 2 modulo 3", Explain(m, 5));
}

// Tests that when AllOf() succeeds, all matchers are asked to explain
// why.
TEST(ExplainMatchResultTest, AllOf_True_True) {
  const Matcher<int> m = AllOf(DivisibleBy(2), DivisibleBy(3));
  EXPECT_EQ("is 0 modulo 2; is 0 modulo 3", Explain(m, 6));
}

TEST(ExplainMatchResultTest, AllOf_True_True_2) {
  const Matcher<int> m = AllOf(Ge(2), Le(3));
  EXPECT_EQ("", Explain(m, 2));
}

TEST(ExplainmatcherResultTest, MonomorphicMatcher) {
  const Matcher<int> m = GreaterThan(5);
  EXPECT_EQ("is 1 more than 5", Explain(m, 6));
}

// The following two tests verify that values without a public copy
// ctor can be used as arguments to matchers like Eq(), Ge(), and etc
// with the help of ByRef().

class NotCopyable {
 public:
  explicit NotCopyable(int value) : value_(value) {}

  int value() const { return value_; }

  bool operator==(const NotCopyable& rhs) const {
    return value() == rhs.value();
  }

  bool operator>=(const NotCopyable& rhs) const {
    return value() >= rhs.value();
  }
 private:
  int value_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(NotCopyable);
};

TEST(ByRefTest, AllowsNotCopyableConstValueInMatchers) {
  const NotCopyable const_value1(1);
  const Matcher<const NotCopyable&> m = Eq(ByRef(const_value1));

  const NotCopyable n1(1), n2(2);
  EXPECT_TRUE(m.Matches(n1));
  EXPECT_FALSE(m.Matches(n2));
}

TEST(ByRefTest, AllowsNotCopyableValueInMatchers) {
  NotCopyable value2(2);
  const Matcher<NotCopyable&> m = Ge(ByRef(value2));

  NotCopyable n1(1), n2(2);
  EXPECT_FALSE(m.Matches(n1));
  EXPECT_TRUE(m.Matches(n2));
}

// Tests ContainerEq with different container types, and
// different element types.

template <typename T>
class ContainerEqTest : public testing::Test {
 public:
};

typedef testing::Types<
    std::set<int>,
    std::vector<size_t>,
    std::multiset<size_t>,
    std::list<int> >
    ContainerEqTestTypes;

TYPED_TEST_CASE(ContainerEqTest, ContainerEqTestTypes);

// Tests that the filled container is equal to itself.
TYPED_TEST(ContainerEqTest, EqualsSelf) {
  static const int vals[] = {1, 1, 2, 3, 5, 8};
  TypeParam my_set(vals, vals + 6);
  const Matcher<TypeParam> m = ContainerEq(my_set);
  EXPECT_TRUE(m.Matches(my_set));
  EXPECT_EQ("", Explain(m, my_set));
}

// Tests that missing values are reported.
TYPED_TEST(ContainerEqTest, ValueMissing) {
  static const int vals[] = {1, 1, 2, 3, 5, 8};
  static const int test_vals[] = {2, 1, 8, 5};
  TypeParam my_set(vals, vals + 6);
  TypeParam test_set(test_vals, test_vals + 4);
  const Matcher<TypeParam> m = ContainerEq(my_set);
  EXPECT_FALSE(m.Matches(test_set));
  EXPECT_EQ("Not in actual: 3", Explain(m, test_set));
}

// Tests that added values are reported.
TYPED_TEST(ContainerEqTest, ValueAdded) {
  static const int vals[] = {1, 1, 2, 3, 5, 8};
  static const int test_vals[] = {1, 2, 3, 5, 8, 46};
  TypeParam my_set(vals, vals + 6);
  TypeParam test_set(test_vals, test_vals + 6);
  const Matcher<const TypeParam&> m = ContainerEq(my_set);
  EXPECT_FALSE(m.Matches(test_set));
  EXPECT_EQ("Only in actual: 46", Explain(m, test_set));
}

// Tests that added and missing values are reported together.
TYPED_TEST(ContainerEqTest, ValueAddedAndRemoved) {
  static const int vals[] = {1, 1, 2, 3, 5, 8};
  static const int test_vals[] = {1, 2, 3, 8, 46};
  TypeParam my_set(vals, vals + 6);
  TypeParam test_set(test_vals, test_vals + 5);
  const Matcher<TypeParam> m = ContainerEq(my_set);
  EXPECT_FALSE(m.Matches(test_set));
  EXPECT_EQ("Only in actual: 46; not in actual: 5", Explain(m, test_set));
}

// Tests duplicated value -- expect no explanation.
TYPED_TEST(ContainerEqTest, DuplicateDifference) {
  static const int vals[] = {1, 1, 2, 3, 5, 8};
  static const int test_vals[] = {1, 2, 3, 5, 8};
  TypeParam my_set(vals, vals + 6);
  TypeParam test_set(test_vals, test_vals + 5);
  const Matcher<const TypeParam&> m = ContainerEq(my_set);
  // Depending on the container, match may be true or false
  // But in any case there should be no explanation.
  EXPECT_EQ("", Explain(m, test_set));
}

// Tests that mutliple missing values are reported.
// Using just vector here, so order is predicatble.
TEST(ContainerEqExtraTest, MultipleValuesMissing) {
  static const int vals[] = {1, 1, 2, 3, 5, 8};
  static const int test_vals[] = {2, 1, 5};
  std::vector<int> my_set(vals, vals + 6);
  std::vector<int> test_set(test_vals, test_vals + 3);
  const Matcher<std::vector<int> > m = ContainerEq(my_set);
  EXPECT_FALSE(m.Matches(test_set));
  EXPECT_EQ("Not in actual: 3, 8", Explain(m, test_set));
}

// Tests that added values are reported.
// Using just vector here, so order is predicatble.
TEST(ContainerEqExtraTest, MultipleValuesAdded) {
  static const int vals[] = {1, 1, 2, 3, 5, 8};
  static const int test_vals[] = {1, 2, 92, 3, 5, 8, 46};
  std::list<size_t> my_set(vals, vals + 6);
  std::list<size_t> test_set(test_vals, test_vals + 7);
  const Matcher<const std::list<size_t>&> m = ContainerEq(my_set);
  EXPECT_FALSE(m.Matches(test_set));
  EXPECT_EQ("Only in actual: 92, 46", Explain(m, test_set));
}

// Tests that added and missing values are reported together.
TEST(ContainerEqExtraTest, MultipleValuesAddedAndRemoved) {
  static const int vals[] = {1, 1, 2, 3, 5, 8};
  static const int test_vals[] = {1, 2, 3, 92, 46};
  std::list<size_t> my_set(vals, vals + 6);
  std::list<size_t> test_set(test_vals, test_vals + 5);
  const Matcher<const std::list<size_t> > m = ContainerEq(my_set);
  EXPECT_FALSE(m.Matches(test_set));
  EXPECT_EQ("Only in actual: 92, 46; not in actual: 5, 8",
            Explain(m, test_set));
}

// Tests to see that duplicate elements are detected,
// but (as above) not reported in the explanation.
TEST(ContainerEqExtraTest, MultiSetOfIntDuplicateDifference) {
  static const int vals[] = {1, 1, 2, 3, 5, 8};
  static const int test_vals[] = {1, 2, 3, 5, 8};
  std::vector<int> my_set(vals, vals + 6);
  std::vector<int> test_set(test_vals, test_vals + 5);
  const Matcher<std::vector<int> > m = ContainerEq(my_set);
  EXPECT_TRUE(m.Matches(my_set));
  EXPECT_FALSE(m.Matches(test_set));
  // There is nothing to report when both sets contain all the same values.
  EXPECT_EQ("", Explain(m, test_set));
}

// Tests that ContainerEq works for non-trivial associative containers,
// like maps.
TEST(ContainerEqExtraTest, WorksForMaps) {
  std::map<int, std::string> my_map;
  my_map[0] = "a";
  my_map[1] = "b";

  std::map<int, std::string> test_map;
  test_map[0] = "aa";
  test_map[1] = "b";

  const Matcher<const std::map<int, std::string>&> m = ContainerEq(my_map);
  EXPECT_TRUE(m.Matches(my_map));
  EXPECT_FALSE(m.Matches(test_map));

  EXPECT_EQ("Only in actual: (0, \"aa\"); not in actual: (0, \"a\")",
            Explain(m, test_map));
}

// Tests GetParamIndex().

TEST(GetParamIndexTest, WorksForEmptyParamList) {
  const char* params[] = { NULL };
  EXPECT_EQ(kTupleInterpolation, GetParamIndex(params, "*"));
  EXPECT_EQ(kInvalidInterpolation, GetParamIndex(params, "a"));
}

TEST(GetParamIndexTest, RecognizesStar) {
  const char* params[] = { "a", "b", NULL };
  EXPECT_EQ(kTupleInterpolation, GetParamIndex(params, "*"));
}

TEST(GetParamIndexTest, RecognizesKnownParam) {
  const char* params[] = { "foo", "bar", NULL };
  EXPECT_EQ(0, GetParamIndex(params, "foo"));
  EXPECT_EQ(1, GetParamIndex(params, "bar"));
}

TEST(GetParamIndexTest, RejectsUnknownParam) {
  const char* params[] = { "foo", "bar", NULL };
  EXPECT_EQ(kInvalidInterpolation, GetParamIndex(params, "foobar"));
}

// Tests SkipPrefix().

TEST(SkipPrefixTest, SkipsWhenPrefixMatches) {
  const char* const str = "hello";

  const char* p = str;
  EXPECT_TRUE(SkipPrefix("", &p));
  EXPECT_EQ(str, p);

  p = str;
  EXPECT_TRUE(SkipPrefix("hell", &p));
  EXPECT_EQ(str + 4, p);
}

TEST(SkipPrefixTest, DoesNotSkipWhenPrefixDoesNotMatch) {
  const char* const str = "world";

  const char* p = str;
  EXPECT_FALSE(SkipPrefix("W", &p));
  EXPECT_EQ(str, p);

  p = str;
  EXPECT_FALSE(SkipPrefix("world!", &p));
  EXPECT_EQ(str, p);
}

// Tests FormatMatcherDescriptionSyntaxError().
TEST(FormatMatcherDescriptionSyntaxErrorTest, FormatsCorrectly) {
  const char* const description = "hello%world";
  EXPECT_EQ("Syntax error at index 5 in matcher description \"hello%world\": ",
            FormatMatcherDescriptionSyntaxError(description, description + 5));
}

// Tests ValidateMatcherDescription().

TEST(ValidateMatcherDescriptionTest, AcceptsEmptyDescription) {
  const char* params[] = { "foo", "bar", NULL };
  EXPECT_THAT(ValidateMatcherDescription(params, ""),
              ElementsAre());
}

TEST(ValidateMatcherDescriptionTest,
     AcceptsNonEmptyDescriptionWithNoInterpolation) {
  const char* params[] = { "foo", "bar", NULL };
  EXPECT_THAT(ValidateMatcherDescription(params, "a simple description"),
              ElementsAre());
}

// We use MATCHER_P3() to define a matcher for testing
// ValidateMatcherDescription(); otherwise we'll end up with much
// plumbing code.  This is not circular as
// ValidateMatcherDescription() doesn't affect whether the matcher
// matches a value or not.
MATCHER_P3(EqInterpolation, start, end, index, "equals Interpolation%(*)s") {
  return arg.start_pos == start && arg.end_pos == end &&
      arg.param_index == index;
}

TEST(ValidateMatcherDescriptionTest, AcceptsPercentInterpolation) {
  const char* params[] = { "foo", NULL };
  const char* const desc = "one %%";
  EXPECT_THAT(ValidateMatcherDescription(params, desc),
              ElementsAre(EqInterpolation(desc + 4, desc + 6,
                                          kPercentInterpolation)));
}

TEST(ValidateMatcherDescriptionTest, AcceptsTupleInterpolation) {
  const char* params[] = { "foo", "bar", "baz", NULL };
  const char* const desc = "%(*)s after";
  EXPECT_THAT(ValidateMatcherDescription(params, desc),
              ElementsAre(EqInterpolation(desc, desc + 5,
                                          kTupleInterpolation)));
}

TEST(ValidateMatcherDescriptionTest, AcceptsParamInterpolation) {
  const char* params[] = { "foo", "bar", "baz", NULL };
  const char* const desc = "a %(bar)s.";
  EXPECT_THAT(ValidateMatcherDescription(params, desc),
              ElementsAre(EqInterpolation(desc + 2, desc + 9, 1)));
}

TEST(ValidateMatcherDescriptionTest, AcceptsMultiplenterpolations) {
  const char* params[] = { "foo", "bar", "baz", NULL };
  const char* const desc = "%(baz)s %(foo)s %(bar)s";
  EXPECT_THAT(ValidateMatcherDescription(params, desc),
              ElementsAre(EqInterpolation(desc, desc + 7, 2),
                          EqInterpolation(desc + 8, desc + 15, 0),
                          EqInterpolation(desc + 16, desc + 23, 1)));
}

TEST(ValidateMatcherDescriptionTest, AcceptsRepeatedParams) {
  const char* params[] = { "foo", "bar", NULL };
  const char* const desc = "%(foo)s and %(foo)s";
  EXPECT_THAT(ValidateMatcherDescription(params, desc),
              ElementsAre(EqInterpolation(desc, desc + 7, 0),
                          EqInterpolation(desc + 12, desc + 19, 0)));
}

TEST(ValidateMatcherDescriptionTest, RejectsUnknownParam) {
  const char* params[] = { "a", "bar", NULL };
  EXPECT_NONFATAL_FAILURE({
    EXPECT_THAT(ValidateMatcherDescription(params, "%(foo)s"),
                ElementsAre());
  }, "Syntax error at index 2 in matcher description \"%(foo)s\": "
     "\"foo\" is an invalid parameter name.");
}

TEST(ValidateMatcherDescriptionTest, RejectsUnfinishedParam) {
  const char* params[] = { "a", "bar", NULL };
  EXPECT_NONFATAL_FAILURE({
    EXPECT_THAT(ValidateMatcherDescription(params, "%(foo)"),
                ElementsAre());
  }, "Syntax error at index 0 in matcher description \"%(foo)\": "
     "an interpolation must end with \")s\", but \"%(foo)\" does not.");

  EXPECT_NONFATAL_FAILURE({
    EXPECT_THAT(ValidateMatcherDescription(params, "x%(a"),
                ElementsAre());
  }, "Syntax error at index 1 in matcher description \"x%(a\": "
     "an interpolation must end with \")s\", but \"%(a\" does not.");
}

TEST(ValidateMatcherDescriptionTest, RejectsSinglePercent) {
  const char* params[] = { "a", NULL };
  EXPECT_NONFATAL_FAILURE({
    EXPECT_THAT(ValidateMatcherDescription(params, "a %."),
                ElementsAre());
  }, "Syntax error at index 2 in matcher description \"a %.\": "
     "use \"%%\" instead of \"%\" to print \"%\".");

}

// Tests JoinAsTuple().

TEST(JoinAsTupleTest, JoinsEmptyTuple) {
  EXPECT_EQ("", JoinAsTuple(Strings()));
}

TEST(JoinAsTupleTest, JoinsOneTuple) {
  const char* fields[] = { "1" };
  EXPECT_EQ("1", JoinAsTuple(Strings(fields, fields + 1)));
}

TEST(JoinAsTupleTest, JoinsTwoTuple) {
  const char* fields[] = { "1", "a" };
  EXPECT_EQ("(1, a)", JoinAsTuple(Strings(fields, fields + 2)));
}

TEST(JoinAsTupleTest, JoinsTenTuple) {
  const char* fields[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };
  EXPECT_EQ("(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)",
            JoinAsTuple(Strings(fields, fields + 10)));
}

// Tests FormatMatcherDescription().

TEST(FormatMatcherDescriptionTest, WorksForEmptyDescription) {
  EXPECT_EQ("is even",
            FormatMatcherDescription("IsEven", "", Interpolations(),
                                     Strings()));

  const char* params[] = { "5" };
  EXPECT_EQ("equals 5",
            FormatMatcherDescription("Equals", "", Interpolations(),
                                     Strings(params, params + 1)));

  const char* params2[] = { "5", "8" };
  EXPECT_EQ("is in range (5, 8)",
            FormatMatcherDescription("IsInRange", "", Interpolations(),
                                     Strings(params2, params2 + 2)));
}

TEST(FormatMatcherDescriptionTest, WorksForDescriptionWithNoInterpolation) {
  EXPECT_EQ("is positive",
            FormatMatcherDescription("Gt0", "is positive", Interpolations(),
                                     Strings()));

  const char* params[] = { "5", "6" };
  EXPECT_EQ("is negative",
            FormatMatcherDescription("Lt0", "is negative", Interpolations(),
                                     Strings(params, params + 2)));
}

TEST(FormatMatcherDescriptionTest,
     WorksWhenDescriptionStartsWithInterpolation) {
  const char* params[] = { "5" };
  const char* const desc = "%(num)s times bigger";
  const Interpolation interp[] = { Interpolation(desc, desc + 7, 0) };
  EXPECT_EQ("5 times bigger",
            FormatMatcherDescription("Foo", desc,
                                     Interpolations(interp, interp + 1),
                                     Strings(params, params + 1)));
}

TEST(FormatMatcherDescriptionTest,
     WorksWhenDescriptionEndsWithInterpolation) {
  const char* params[] = { "5", "6" };
  const char* const desc = "is bigger than %(y)s";
  const Interpolation interp[] = { Interpolation(desc + 15, desc + 20, 1) };
  EXPECT_EQ("is bigger than 6",
            FormatMatcherDescription("Foo", desc,
                                     Interpolations(interp, interp + 1),
                                     Strings(params, params + 2)));
}

TEST(FormatMatcherDescriptionTest,
     WorksWhenDescriptionStartsAndEndsWithInterpolation) {
  const char* params[] = { "5", "6" };
  const char* const desc = "%(x)s <= arg <= %(y)s";
  const Interpolation interp[] = {
    Interpolation(desc, desc + 5, 0),
    Interpolation(desc + 16, desc + 21, 1)
  };
  EXPECT_EQ("5 <= arg <= 6",
            FormatMatcherDescription("Foo", desc,
                                     Interpolations(interp, interp + 2),
                                     Strings(params, params + 2)));
}

TEST(FormatMatcherDescriptionTest,
     WorksWhenDescriptionDoesNotStartOrEndWithInterpolation) {
  const char* params[] = { "5.2" };
  const char* const desc = "has %(x)s cents";
  const Interpolation interp[] = { Interpolation(desc + 4, desc + 9, 0) };
  EXPECT_EQ("has 5.2 cents",
            FormatMatcherDescription("Foo", desc,
                                     Interpolations(interp, interp + 1),
                                     Strings(params, params + 1)));
}

TEST(FormatMatcherDescriptionTest,
     WorksWhenDescriptionContainsMultipleInterpolations) {
  const char* params[] = { "5", "6" };
  const char* const desc = "in %(*)s or [%(x)s, %(y)s]";
  const Interpolation interp[] = {
    Interpolation(desc + 3, desc + 8, kTupleInterpolation),
    Interpolation(desc + 13, desc + 18, 0),
    Interpolation(desc + 20, desc + 25, 1)
  };
  EXPECT_EQ("in (5, 6) or [5, 6]",
            FormatMatcherDescription("Foo", desc,
                                     Interpolations(interp, interp + 3),
                                     Strings(params, params + 2)));
}

TEST(FormatMatcherDescriptionTest,
     WorksWhenDescriptionContainsRepeatedParams) {
  const char* params[] = { "9" };
  const char* const desc = "in [-%(x)s, %(x)s]";
  const Interpolation interp[] = {
    Interpolation(desc + 5, desc + 10, 0),
    Interpolation(desc + 12, desc + 17, 0)
  };
  EXPECT_EQ("in [-9, 9]",
            FormatMatcherDescription("Foo", desc,
                                     Interpolations(interp, interp + 2),
                                     Strings(params, params + 1)));
}

TEST(FormatMatcherDescriptionTest,
     WorksForDescriptionWithInvalidInterpolation) {
  const char* params[] = { "9" };
  const char* const desc = "> %(x)s %(x)";
  const Interpolation interp[] = { Interpolation(desc + 2, desc + 7, 0)  };
  EXPECT_EQ("> 9 %(x)",
            FormatMatcherDescription("Foo", desc,
                                     Interpolations(interp, interp + 1),
                                     Strings(params, params + 1)));
}

}  // namespace gmock_matchers_test
}  // namespace testing
