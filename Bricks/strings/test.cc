/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2014 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#include <chrono>
#include <cmath>
#include <set>
#include <string>
#include <vector>

#include "strings.h"

#include "../../3rdparty/gtest/gtest-main.h"

using current::strings::Printf;
using current::strings::FixedSizeSerializer;
using current::strings::PackToString;
using current::strings::UnpackFromString;
using current::strings::CompileTimeStringLength;
using current::strings::Trim;
using current::strings::ToLower;
using current::strings::ToUpper;
using current::strings::Join;
using current::strings::Split;
using current::strings::SplitIntoKeyValuePairs;
using current::strings::EmptyFields;
using current::strings::KeyValueParsing;
using current::strings::KeyValueNoValueException;
using current::strings::KeyValueMultipleValuesException;
using current::strings::ByWhitespace;
using current::strings::ByLines;
using current::strings::SlowEditDistance;
using current::strings::FastEditDistance;
using current::strings::Chunk;
using current::strings::UniqueChunk;
using current::strings::ChunkDB;
using current::strings::RoundDoubleToString;
using current::strings::TimeIntervalAsHumanReadableString;
using current::strings::EscapeForCPlusPlus;
using current::strings::EscapeForMarkdown;
using current::strings::is_string_type;

TEST(StringPrintf, SmokeTest) {
  EXPECT_EQ("Test: 42, 'Hello', 0000ABBA", Printf("Test: %d, '%s', %08X", 42, "Hello", 0xabba));
  EXPECT_EQ(1024u * 5, Printf("%s", std::string(10000, 'A').c_str()).length());
}

TEST(FixedSizeSerializer, UInt16) {
  EXPECT_EQ(5, FixedSizeSerializer<uint16_t>::size_in_bytes);
  // Does not fit signed 16-bit, requires unsigned.
  EXPECT_EQ("54321", FixedSizeSerializer<uint16_t>::PackToString(54321));
  EXPECT_EQ(54321, FixedSizeSerializer<uint16_t>::UnpackFromString("54321"));
}

TEST(FixedSizeSerializer, UInt32) {
  EXPECT_EQ(10, FixedSizeSerializer<uint32_t>::size_in_bytes);
  // Does not fit signed 32-bit, requires unsigned.
  EXPECT_EQ("3987654321", FixedSizeSerializer<uint32_t>::PackToString(3987654321));
  EXPECT_EQ(3987654321, FixedSizeSerializer<uint32_t>::UnpackFromString("3987654321"));
}

TEST(FixedSizeSerializer, UInt64) {
  EXPECT_EQ(20, FixedSizeSerializer<uint64_t>::size_in_bytes);
  uint64_t magic = static_cast<uint64_t>(1e19);
  magic += 42;
  // Does not fit signed 64-bit.
  EXPECT_EQ("10000000000000000042", FixedSizeSerializer<uint64_t>::PackToString(magic));
  EXPECT_EQ(magic, FixedSizeSerializer<uint64_t>::UnpackFromString("10000000000000000042"));
}

TEST(FixedSizeSerializer, ImplicitSyntax) {
  {
    uint32_t x;
    EXPECT_EQ(42u, UnpackFromString("42", x));
  }
  {
    uint16_t x;
    EXPECT_EQ(10000u, UnpackFromString("10000", x));
  }
  {
    uint16_t x = 42;
    EXPECT_EQ("00042", PackToString(x));
  }
  {
    uint64_t x = static_cast<int64_t>(1e18);
    EXPECT_EQ("01000000000000000000", PackToString(x));
  }
}

static const char global_string[] = "magic";

TEST(Util, CompileTimeStringLength) {
  const char local_string[] = "foo";
  static const char local_static_string[] = "blah";
  EXPECT_EQ(3u, CompileTimeStringLength(local_string));
  EXPECT_EQ(4u, CompileTimeStringLength(local_static_string));
  EXPECT_EQ(5u, CompileTimeStringLength(global_string));
}

TEST(Util, Trim) {
  EXPECT_EQ("one", Trim(" one "));
  EXPECT_EQ("one", Trim(std::string(" one ")));
  EXPECT_EQ("two", Trim("   \t\n\t\n\t\r\n   two   \t\n\t\n\t\r\n   "));
  EXPECT_EQ("two", Trim(std::string("   \t\n\t\n\t\r\n   two   \t\n\t\n\t\r\n   ")));
  EXPECT_EQ("3 \t\r\n 4", Trim("   \t\n\t\n\t\r\n   3 \t\r\n 4   \t\n\t\n\t\r\n   "));
  EXPECT_EQ("3 \t\r\n 4", Trim(std::string("   \t\n\t\n\t\r\n   3 \t\r\n 4   \t\n\t\n\t\r\n   ")));
  EXPECT_EQ("", Trim(""));
  EXPECT_EQ("", Trim(std::string("")));
  EXPECT_EQ("", Trim(" \t\r\n\t "));
  EXPECT_EQ("", Trim(std::string(" \t\r\n\t ")));
}

enum class StringConversionTestEnum : int { TheAnswer = 42 };

TEST(Util, FromString) {
  EXPECT_EQ(1, current::FromString<int>("1"));

  EXPECT_EQ(32767, static_cast<int>(current::FromString<int16_t>("32767")));
  EXPECT_EQ(65535, static_cast<int>(current::FromString<uint16_t>("65535")));

  double tmp;
  EXPECT_EQ(0.5, current::FromString("0.5", tmp));
  EXPECT_EQ(0.5, tmp);

  EXPECT_EQ(0u, current::FromString<size_t>(""));
  EXPECT_EQ(0u, current::FromString<size_t>("foo"));
  EXPECT_EQ(0u, current::FromString<size_t>("\n"));

  EXPECT_EQ(0.0, current::FromString<double>(""));
  EXPECT_EQ(0.0, current::FromString<double>("bar"));
  EXPECT_EQ(0.0, current::FromString<double>("\t"));

  EXPECT_EQ("one two", current::FromString<std::string>("one two"));
  EXPECT_EQ("three four", current::FromString<std::string>(std::string("three four")));

  EXPECT_TRUE(current::FromString<bool>("true"));
  EXPECT_TRUE(current::FromString<bool>("1"));
  EXPECT_FALSE(current::FromString<bool>("false"));
  EXPECT_FALSE(current::FromString<bool>("0"));

  EXPECT_EQ(100042ll, current::FromString<std::chrono::milliseconds>("100042").count());
  EXPECT_EQ(100000042ll, current::FromString<std::chrono::microseconds>("100000042").count());

  EXPECT_TRUE(StringConversionTestEnum::TheAnswer == current::FromString<StringConversionTestEnum>("42"));
  EXPECT_EQ(0, static_cast<int>(current::FromString<StringConversionTestEnum>("")));
}

TEST(ToString, SmokeTest) {
  EXPECT_EQ("foo", current::ToString("foo"));
  EXPECT_EQ("bar", current::ToString(std::string("bar")));
  EXPECT_EQ("one two", current::ToString("one two"));
  EXPECT_EQ("three four", current::ToString(std::string("three four")));
  EXPECT_EQ("42", current::ToString(42));
  EXPECT_EQ("0.500000", current::ToString(0.5));
  EXPECT_EQ("c", current::ToString('c'));
  EXPECT_EQ("true", current::ToString(true));
  EXPECT_EQ("false", current::ToString(false));
  EXPECT_EQ("42", current::ToString(StringConversionTestEnum::TheAnswer));
  EXPECT_EQ("100042", current::ToString(std::chrono::milliseconds(100042)));
  EXPECT_EQ("100000042", current::ToString(std::chrono::microseconds(100000042)));
}

TEST(Util, ToUpperAndToLower) {
  EXPECT_EQ("test passed", ToLower("TeSt pAsSeD"));
  EXPECT_EQ("TEST PASSED", ToUpper("TeSt pAsSeD"));
}

TEST(JoinAndSplit, Join) {
  EXPECT_EQ("one,two,three", Join({"one", "two", "three"}, ','));
  EXPECT_EQ("onetwothree", Join({"one", "two", "three"}, ""));
  EXPECT_EQ("one, two, three", Join({"one", "two", "three"}, ", "));
  EXPECT_EQ("one, two, three", Join({"one", "two", "three"}, std::string(", ")));
  EXPECT_EQ("", Join({}, ' '));
  EXPECT_EQ("", Join({}, " "));

  EXPECT_EQ("1 3 2 3", Join(std::vector<int>({1, 3, 2, 3}), " "));
  EXPECT_EQ("1 2 3", Join(std::set<int>({1, 3, 2, 3}), " "));
  EXPECT_EQ("1 2 3 3", Join(std::multiset<int>({1, 3, 2, 3}), " "));

  EXPECT_EQ("a,b,c,b", Join(std::vector<std::string>({"a", "b", "c", "b"}), ','));
  EXPECT_EQ("a,b,c", Join(std::set<std::string>({"a", "b", "c", "b"}), ','));
  EXPECT_EQ("a,b,b,c", Join(std::multiset<std::string>({"a", "b", "c", "b"}), ','));

  EXPECT_EQ("x->y->z", Join(std::set<char>({'x', 'z', 'y'}), "->"));
  EXPECT_EQ("0.500000<0.750000<0.875000<1.000000", Join(std::multiset<double>({1, 0.5, 0.75, 0.875}), '<'));
}

TEST(JoinAndSplit, Split) {
  EXPECT_EQ("one two three", Join(Split("one,two,three", ','), ' '));
  EXPECT_EQ("one two three four", Join(Split("one,two|three,four", ",|"), ' '));
  EXPECT_EQ("one two three four", Join(Split("one,two|three,four", std::string(",|")), ' '));
  EXPECT_EQ("one,two three,four", Join(Split("one,two|three,four", '|'), ' '));
  EXPECT_EQ("one,two three,four", Join(Split("one,two|three,four", "|"), ' '));
  EXPECT_EQ("one,two three,four", Join(Split("one,two|three,four", std::string("|")), ' '));

  EXPECT_EQ("one two three", Join(Split(",,one,,,two,,,three,,", ','), ' '));
  EXPECT_EQ("  one   two   three  ", Join(Split(",,one,,,two,,,three,,", ',', EmptyFields::Keep), ' '));

  EXPECT_EQ("one two three", Join(Split<ByWhitespace>("one two three"), ' '));
  EXPECT_EQ("one two three", Join(Split<ByWhitespace>("\t \tone\t \ttwo\t \tthree\t \t"), ' '));

  EXPECT_EQ("one two|three", Join(Split<ByLines>("one two\nthree"), '|'));
  EXPECT_EQ("one|two three", Join(Split<ByLines>("\r\n\n\r\none\n\r\n\n\r\ntwo three"), '|'));

  // Note that `Split` on a predicate splits on the characters for which the predicate returns `false`,
  // and keeps the characters where the predicate returns `true`.
  // This way, `Split` on `::isalpha` or `::isalnum` makes perfect sense.
  EXPECT_EQ("1 2 3 4 5", Join(Split("1 a2b\n3\n\n4\n\n&5$", [](char c) { return ::isdigit(c); }), ' '));
  EXPECT_EQ("ab c d e123", Join(Split("ab'c d--e123", ::isalnum), ' '));
}

TEST(JoinAndSplit, FunctionalSplit) {
  {
    std::string result;
    Split("one,two,three", ',', [&result](const std::string& s) { result += s + '\n'; });
    EXPECT_EQ("one\ntwo\nthree\n", result);
  }
  {
    std::string result;
    Split("one,two,three", ',', [&result](std::string&& s) { result += s + '\n'; });
    EXPECT_EQ("one\ntwo\nthree\n", result);
  }
  {
    std::string result;
    struct Helper {
      std::string& result_;
      explicit Helper(std::string& result) : result_(result) {}
      void operator()(const std::string& s) const { result_ += s + '\n'; }
      Helper(Helper&) = delete;
      Helper(Helper&&) = delete;
      void operator=(const Helper&) = delete;
      void operator=(Helper&&) = delete;
    };
    Helper helper(result);
    Split("one,two,three", ',', helper);
    EXPECT_EQ("one\ntwo\nthree\n", result);
  }
  {
    std::string result;
    struct Helper {
      std::string& result_;
      explicit Helper(std::string& result) : result_(result) {}
      void operator()(std::string&& s) const { result_ += s + '\n'; }
      Helper(Helper&) = delete;
      Helper(Helper&&) = delete;
      void operator=(const Helper&) = delete;
      void operator=(Helper&&) = delete;
    };
    Helper helper(result);
    Split("one,two,three", ',', helper);
    EXPECT_EQ("one\ntwo\nthree\n", result);
  }
}

TEST(JoinAndSplit, SplitIntoKeyValuePairs) {
  const auto result = SplitIntoKeyValuePairs("one=1,two=2", '=', ',');
  ASSERT_EQ(2u, result.size());
  EXPECT_EQ("one", result[0].first);
  EXPECT_EQ("1", result[0].second);
  EXPECT_EQ("two", result[1].first);
  EXPECT_EQ("2", result[1].second);
}

TEST(JoinAndSplit, SplitIntoKeyValuePairsWithWhitespaceBetweenPairs) {
  const auto result = SplitIntoKeyValuePairs("\t\n \tone=1\t\n \ttwo=2\t\n \t", '=');
  ASSERT_EQ(2u, result.size());
  EXPECT_EQ("one", result[0].first);
  EXPECT_EQ("1", result[0].second);
  EXPECT_EQ("two", result[1].first);
  EXPECT_EQ("2", result[1].second);
}

TEST(JoinAndSplit, SplitIntoKeyValuePairsExceptions) {
  const auto default_is_to_not_throw = SplitIntoKeyValuePairs("test,foo=bar=baz,one=1,two=2,passed", '=', ',');
  ASSERT_EQ(2u, default_is_to_not_throw.size());
  EXPECT_EQ("one", default_is_to_not_throw[0].first);
  EXPECT_EQ("1", default_is_to_not_throw[0].second);
  EXPECT_EQ("two", default_is_to_not_throw[1].first);
  EXPECT_EQ("2", default_is_to_not_throw[1].second);
  const auto correct_case = SplitIntoKeyValuePairs("one=1,two=2", '=', ',', KeyValueParsing::Throw);
  ASSERT_EQ(2u, correct_case.size());
  EXPECT_EQ("one", correct_case[0].first);
  EXPECT_EQ("1", correct_case[0].second);
  EXPECT_EQ("two", correct_case[1].first);
  EXPECT_EQ("2", correct_case[1].second);
  ASSERT_THROW(SplitIntoKeyValuePairs("foo", '=', ',', KeyValueParsing::Throw), KeyValueNoValueException);
  ASSERT_THROW(SplitIntoKeyValuePairs("foo=bar=baz", '=', ',', KeyValueParsing::Throw),
               KeyValueMultipleValuesException);
  ASSERT_THROW(SplitIntoKeyValuePairs("foo", '=', KeyValueParsing::Throw), KeyValueNoValueException);
  ASSERT_THROW(SplitIntoKeyValuePairs("foo=bar=baz", '=', KeyValueParsing::Throw),
               KeyValueMultipleValuesException);
}

TEST(JoinAndSplit, EfficientlySplitsMutableStringIntoTemporaryChunks) {
  const std::string original_input = "foo bar baz";

  struct Processor {
    std::vector<std::string> results;
    void operator()(const std::string& string) { results.push_back("String(" + string + ')'); }
    void operator()(Chunk chunk) { results.push_back("Chunk(" + static_cast<std::string>(chunk) + ')'); }
  };

  const std::string immutable_input = original_input;
  std::string mutable_input = original_input;

  std::vector<char> mutable_vector_char(original_input.begin(), original_input.end());
  mutable_vector_char.push_back('\0');

  const Chunk chunk(immutable_input);

  {
    Processor p;
    Split<ByWhitespace>(immutable_input, p);
    EXPECT_EQ("String(foo),String(bar),String(baz)", Join(p.results, ','));
  }

  {
    Processor p;
    Split<ByWhitespace>(mutable_input, p);
    EXPECT_EQ("Chunk(foo),Chunk(bar),Chunk(baz)", Join(p.results, ','));
    EXPECT_EQ(original_input, mutable_input);
  }

  {
    Processor p;
    Split<ByWhitespace>(&mutable_vector_char[0], p);
    EXPECT_EQ("Chunk(foo),Chunk(bar),Chunk(baz)", Join(p.results, ','));
  }

  {
    Processor p;
    Split<ByWhitespace>(chunk, p);
    EXPECT_EQ("Chunk(foo),Chunk(bar),Chunk(baz)", Join(p.results, ','));
  }

  EXPECT_EQ(original_input, mutable_input);
}

TEST(EditDistance, SmokeTest) {
  EXPECT_EQ(0u, SlowEditDistance("foo", "foo"));
  EXPECT_EQ(3u, SlowEditDistance("foo", ""));
  EXPECT_EQ(3u, SlowEditDistance("", "foo"));
  EXPECT_EQ(3u, SlowEditDistance("foo", "bar"));
  EXPECT_EQ(1u, SlowEditDistance("foo", "zoo"));
  EXPECT_EQ(1u, SlowEditDistance("foo", "fwo"));
  EXPECT_EQ(1u, SlowEditDistance("foo", "foe"));
  EXPECT_EQ(1u, SlowEditDistance("zoo", "foo"));
  EXPECT_EQ(1u, SlowEditDistance("fwo", "foo"));
  EXPECT_EQ(1u, SlowEditDistance("foe", "foo"));
  EXPECT_EQ(1u, SlowEditDistance("foo", "fo"));
  EXPECT_EQ(1u, SlowEditDistance("foo", "oo"));

  EXPECT_EQ(0u, FastEditDistance("foo", "foo", 10u));
  EXPECT_EQ(3u, FastEditDistance("foo", "", 10u));
  EXPECT_EQ(3u, FastEditDistance("", "foo", 10u));
  EXPECT_EQ(3u, FastEditDistance("foo", "bar", 10u));
  EXPECT_EQ(1u, FastEditDistance("foo", "zoo", 10u));
  EXPECT_EQ(1u, FastEditDistance("foo", "fwo", 10u));
  EXPECT_EQ(1u, FastEditDistance("foo", "foe", 10u));
  EXPECT_EQ(1u, FastEditDistance("zoo", "foo", 10u));
  EXPECT_EQ(1u, FastEditDistance("fwo", "foo", 10u));
  EXPECT_EQ(1u, FastEditDistance("foe", "foo", 10u));
  EXPECT_EQ(1u, FastEditDistance("foo", "fo", 10u));
  EXPECT_EQ(1u, FastEditDistance("foo", "oo", 10u));
}

TEST(EditDistance, MaxOffset1) {
  // Max. offset of 1 is fine, max. offset 0 is per-char comparison.
  EXPECT_EQ(2u, SlowEditDistance("abcde", "bcdef"));
  EXPECT_EQ(2u, FastEditDistance("abcde", "bcdef", 1u));
  EXPECT_EQ(5u, FastEditDistance("abcde", "bcdef", 0u));
}

TEST(EditDistance, MaxOffset2) {
  // Max. offset of 2 is fine, max. offset of 1 is same as max. offset of 0, which is per-char comparison.
  EXPECT_EQ(4u, SlowEditDistance("01234567", "23456789"));
  EXPECT_EQ(4u, FastEditDistance("01234567", "23456789", 2u));
  EXPECT_EQ(8u, FastEditDistance("01234567", "23456789", 1u));
  EXPECT_EQ(8u, FastEditDistance("01234567", "23456789", 0u));
}

TEST(EditDistance, StringsOfTooDifferentLength) {
  // When the strings are of too different lengths, `FastEditDistance` can't do anything.
  EXPECT_EQ(6u, SlowEditDistance("foo", "foobarbaz"));
  EXPECT_EQ(6u, SlowEditDistance("foobarbaz", "baz"));
  EXPECT_EQ(6u, FastEditDistance("foo", "foobarbaz", 6u));
  EXPECT_EQ(6u, FastEditDistance("foobarbaz", "baz", 6u));
  EXPECT_EQ(static_cast<size_t>(-1), FastEditDistance("foo", "foobarbaz", 5u));
  EXPECT_EQ(static_cast<size_t>(-1), FastEditDistance("foobarbaz", "baz", 5u));
}

TEST(Chunk, SmokeTest) {
  Chunk foo("foo", 3);
  EXPECT_FALSE(foo.empty());
  EXPECT_EQ(3u, foo.length());
  EXPECT_EQ(0, ::strcmp("foo", foo.c_str()));

  Chunk bar("bar\0baz", 3);
  EXPECT_FALSE(bar.empty());
  EXPECT_EQ(3u, bar.length());
  EXPECT_EQ(0, ::strcmp("bar", bar.c_str()));

  Chunk empty;
  EXPECT_TRUE(empty.empty());
  EXPECT_EQ(0u, empty.length());

  Chunk foo_copy = foo;
  Chunk bar_copy = "meh";
  bar_copy = bar;

  EXPECT_TRUE(foo_copy.HasPrefix(foo));
  EXPECT_TRUE(foo_copy.HasPrefix("foo"));
  EXPECT_TRUE(foo_copy.HasPrefix("fo"));
  EXPECT_TRUE(foo_copy.HasPrefix("f"));
  EXPECT_TRUE(foo_copy.HasPrefix(""));
  EXPECT_FALSE(foo_copy.HasPrefix(bar));
  EXPECT_FALSE(foo_copy.HasPrefix("bar"));
  EXPECT_FALSE(foo_copy.HasPrefix("ba"));
  EXPECT_FALSE(foo_copy.HasPrefix("b"));

  Chunk result;
  EXPECT_TRUE(foo_copy.ExpungePrefix(foo, result));
  EXPECT_EQ(0u, result.length());
  EXPECT_TRUE(foo_copy.ExpungePrefix("f", result));
  EXPECT_EQ(2u, result.length());
  EXPECT_EQ(std::string("oo"), result.c_str());
  EXPECT_FALSE(foo_copy.ExpungePrefix("blah", result));

  EXPECT_EQ(0, foo_copy.LexicographicalCompare(foo));
  EXPECT_EQ(0, bar_copy.LexicographicalCompare(bar));
  EXPECT_GT(foo_copy.LexicographicalCompare(bar_copy), 0);
  EXPECT_LT(bar_copy.LexicographicalCompare(foo_copy), 0);

  {
    Chunk foo("foo");
    Chunk foo1("foo1");
    EXPECT_LT(foo.LexicographicalCompare(foo1), 0);
    EXPECT_GT(foo1.LexicographicalCompare(foo), 0);
  }

  std::string new_foo;
  new_foo += 'f';
  new_foo += 'o';
  new_foo += 'o';
  Chunk foo_from_std_string(new_foo);

  EXPECT_FALSE(foo_from_std_string.empty());
  EXPECT_EQ(3u, foo_from_std_string.length());
  EXPECT_EQ(0, ::strcmp("foo", foo_from_std_string.c_str()));

  EXPECT_EQ(0, ::strcmp(foo_copy.c_str(), foo_from_std_string.c_str()));
  EXPECT_FALSE(foo_copy.c_str() == foo_from_std_string.c_str());

  ChunkDB db;

  UniqueChunk unique_foo_1 = db[foo];
  UniqueChunk unique_foo_2 = db[foo_copy];
  UniqueChunk unique_foo_3 = db[foo_from_std_string];
  EXPECT_EQ(unique_foo_1.c_str(), foo.c_str());
  EXPECT_EQ(unique_foo_2.c_str(), foo.c_str());
  EXPECT_EQ(unique_foo_3.c_str(), foo.c_str());
  EXPECT_TRUE(unique_foo_1 == unique_foo_2);
  EXPECT_TRUE(unique_foo_2 == unique_foo_3);
  EXPECT_FALSE(unique_foo_1 != unique_foo_3);
  EXPECT_FALSE(unique_foo_2 != unique_foo_1);
  EXPECT_FALSE(unique_foo_3 != unique_foo_2);
  EXPECT_FALSE(unique_foo_1 < unique_foo_2);
  EXPECT_FALSE(unique_foo_2 > unique_foo_3);
  EXPECT_TRUE(unique_foo_1 <= unique_foo_2);
  EXPECT_TRUE(unique_foo_2 >= unique_foo_3);
  EXPECT_FALSE(unique_foo_1 != unique_foo_2);

  UniqueChunk unique_bar_1 = db[bar];
  UniqueChunk unique_bar_2 = db[bar_copy];
  EXPECT_EQ(unique_bar_1.c_str(), bar.c_str());
  EXPECT_EQ(unique_bar_2.c_str(), bar.c_str());
  EXPECT_TRUE(unique_bar_1 == unique_bar_2);
  EXPECT_FALSE(unique_bar_1 != unique_bar_2);

  EXPECT_TRUE(unique_foo_1 != unique_bar_1);
  EXPECT_FALSE(unique_foo_1 == unique_bar_1);

  const bool dir = (unique_foo_1 < unique_bar_1);  // Can be either way.
  EXPECT_EQ(dir, unique_foo_1 <= unique_bar_1);
  EXPECT_EQ(!dir, unique_foo_1 > unique_bar_1);
  EXPECT_EQ(!dir, unique_foo_1 >= unique_bar_1);

  const char* pchar_meh_more_stuff = "meh\0more\0good stuff";
  const Chunk meh_1 = Chunk("meh", 3);
  const Chunk meh_2 = Chunk(pchar_meh_more_stuff, 3);
  EXPECT_EQ(0, meh_1.LexicographicalCompare(meh_2));
  EXPECT_EQ(0, meh_2.LexicographicalCompare(meh_1));

  UniqueChunk unique_meh_1 = db.FromConstChunk(meh_1);
  UniqueChunk unique_meh_2 = db.FromConstChunk(meh_2);
  EXPECT_TRUE(unique_meh_1 == unique_meh_2);

  const Chunk meh_more_1 = Chunk("meh\0more\0stuff", 8);
  const Chunk meh_more_2 = Chunk(pchar_meh_more_stuff, 8);
  EXPECT_EQ(0, meh_more_1.LexicographicalCompare(meh_more_2));
  EXPECT_EQ(0, meh_more_2.LexicographicalCompare(meh_more_1));

  EXPECT_EQ(-1, meh_1.LexicographicalCompare(meh_more_1));

  UniqueChunk unique_meh_more_1 = db.FromConstChunk(meh_more_1);
  UniqueChunk unique_meh_more_2 = db.FromConstChunk(meh_more_2);
  EXPECT_TRUE(unique_meh_more_1 == unique_meh_more_2);

  EXPECT_FALSE(unique_meh_1 == unique_meh_more_1);
  EXPECT_FALSE(unique_meh_1 == unique_meh_more_2);
  EXPECT_FALSE(unique_meh_2 == unique_meh_more_1);
  EXPECT_FALSE(unique_meh_2 == unique_meh_more_2);

  UniqueChunk unique_result;
  EXPECT_TRUE(db.Find("foo", unique_result));
  EXPECT_TRUE(unique_result == unique_foo_1);
  EXPECT_FALSE(db.Find("nope", unique_result));
}

TEST(Rounding, SmokeTest) {
  const double pi = 2.0 * std::acos(0.0);
  EXPECT_EQ("3.1", RoundDoubleToString(pi));
  EXPECT_EQ("3", RoundDoubleToString(pi, 1));
  EXPECT_EQ("3.1", RoundDoubleToString(pi, 2));
  EXPECT_EQ("3.14", RoundDoubleToString(pi, 3));
  EXPECT_EQ("3.142", RoundDoubleToString(pi, 4));
  EXPECT_EQ("300", RoundDoubleToString(pi * 100, 1));
  EXPECT_EQ("310", RoundDoubleToString(pi * 100, 2));
  EXPECT_EQ("314", RoundDoubleToString(pi * 100, 3));
  EXPECT_EQ("314.2", RoundDoubleToString(pi * 100, 4));
  EXPECT_EQ("0.03", RoundDoubleToString(pi * 0.01, 1));
  EXPECT_EQ("0.031", RoundDoubleToString(pi * 0.01, 2));
  EXPECT_EQ("0.0314", RoundDoubleToString(pi * 0.01, 3));
  EXPECT_EQ("0.03142", RoundDoubleToString(pi * 0.01, 4));

  const double e = exp(1);
  EXPECT_EQ("2.7", RoundDoubleToString(e));
  EXPECT_EQ("3", RoundDoubleToString(e, 1));
  EXPECT_EQ("2.7", RoundDoubleToString(e, 2));
  EXPECT_EQ("2.72", RoundDoubleToString(e, 3));
  EXPECT_EQ("2.718", RoundDoubleToString(e, 4));
  EXPECT_EQ("300", RoundDoubleToString(e * 100, 1));
  EXPECT_EQ("270", RoundDoubleToString(e * 100, 2));
  EXPECT_EQ("272", RoundDoubleToString(e * 100, 3));
  EXPECT_EQ("271.8", RoundDoubleToString(e * 100, 4));
  EXPECT_EQ("0.03", RoundDoubleToString(e * 0.01, 1));
  EXPECT_EQ("0.027", RoundDoubleToString(e * 0.01, 2));
  EXPECT_EQ("0.0272", RoundDoubleToString(e * 0.01, 3));
  EXPECT_EQ("0.02718", RoundDoubleToString(e * 0.01, 4));

  EXPECT_EQ("1", RoundDoubleToString(1.0 - 1e-7, 1));
  EXPECT_EQ("2", RoundDoubleToString(2.0 - 1e-7, 2));
  EXPECT_EQ("3", RoundDoubleToString(3.0 - 1e-7, 3));
  EXPECT_EQ("4", RoundDoubleToString(4.0 - 1e-7, 4));

  EXPECT_EQ("5", RoundDoubleToString(5.0 + 1e-7, 1));
  EXPECT_EQ("6", RoundDoubleToString(6.0 + 1e-7, 2));
  EXPECT_EQ("7", RoundDoubleToString(7.0 + 1e-7, 3));
  EXPECT_EQ("8", RoundDoubleToString(8.0 + 1e-7, 4));

  EXPECT_EQ("1000", RoundDoubleToString(1000.0 - 1e-7, 1));
  EXPECT_EQ("2000", RoundDoubleToString(2000.0 - 1e-7, 2));
  EXPECT_EQ("3000", RoundDoubleToString(3000.0 - 1e-7, 3));
  EXPECT_EQ("4000", RoundDoubleToString(4000.0 - 1e-7, 4));

  EXPECT_EQ("5000", RoundDoubleToString(5000.0 + 1e-7, 1));
  EXPECT_EQ("6000", RoundDoubleToString(6000.0 + 1e-7, 2));
  EXPECT_EQ("7000", RoundDoubleToString(7000.0 + 1e-7, 3));
  EXPECT_EQ("8000", RoundDoubleToString(8000.0 + 1e-7, 4));

  EXPECT_EQ("0.001", RoundDoubleToString(0.001 - 1e-7, 1));
  EXPECT_EQ("0.002", RoundDoubleToString(0.002 - 1e-7, 2));
  EXPECT_EQ("0.003", RoundDoubleToString(0.003 - 1e-7, 3));
  EXPECT_EQ("0.004", RoundDoubleToString(0.004 - 1e-7, 4));

  EXPECT_EQ("0.005", RoundDoubleToString(0.005 + 1e-7, 1));
  EXPECT_EQ("0.006", RoundDoubleToString(0.006 + 1e-7, 2));
  EXPECT_EQ("0.007", RoundDoubleToString(0.007 + 1e-7, 3));
  EXPECT_EQ("0.008", RoundDoubleToString(0.008 + 1e-7, 4));
}

TEST(TimeIntervalAsString, SmokeTest) {
  EXPECT_EQ("0s", TimeIntervalAsHumanReadableString(std::chrono::microseconds(0)));
  EXPECT_EQ("0s", TimeIntervalAsHumanReadableString(std::chrono::microseconds(1)));
  EXPECT_EQ("0s", TimeIntervalAsHumanReadableString(std::chrono::microseconds(static_cast<int64_t>(1e6 - 1))));

  EXPECT_EQ("1s", TimeIntervalAsHumanReadableString(std::chrono::microseconds(static_cast<int64_t>(1e6))));
  EXPECT_EQ("1s", TimeIntervalAsHumanReadableString(std::chrono::microseconds(static_cast<int64_t>(1e6 + 1))));
  EXPECT_EQ("59s",
            TimeIntervalAsHumanReadableString(std::chrono::microseconds(static_cast<int64_t>(60 * 1e6 - 1))));
  EXPECT_EQ("1m 0s",
            TimeIntervalAsHumanReadableString(std::chrono::microseconds(static_cast<int64_t>(60 * 1e6))));
  EXPECT_EQ("1m 0s",
            TimeIntervalAsHumanReadableString(std::chrono::microseconds(static_cast<int64_t>(60 * 1e6 + 1))));

  EXPECT_EQ(
      "59m 59s",
      TimeIntervalAsHumanReadableString(std::chrono::microseconds(static_cast<int64_t>(60 * 60 * 1e6 - 1))));
  EXPECT_EQ("1h 0m 0s",
            TimeIntervalAsHumanReadableString(std::chrono::microseconds(static_cast<int64_t>(60 * 60 * 1e6))));
  EXPECT_EQ(
      "1h 0m 0s",
      TimeIntervalAsHumanReadableString(std::chrono::microseconds(static_cast<int64_t>(60 * 60 * 1e6 + 1))));

  EXPECT_EQ("23h 59m 59s",
            TimeIntervalAsHumanReadableString(
                std::chrono::microseconds(static_cast<int64_t>(24 * 60 * 60 * 1e6 - 1))));
  EXPECT_EQ(
      "1d 0h 0m 0s",
      TimeIntervalAsHumanReadableString(std::chrono::microseconds(static_cast<int64_t>(24 * 60 * 60 * 1e6))));
  EXPECT_EQ("1d 0h 0m 0s",
            TimeIntervalAsHumanReadableString(
                std::chrono::microseconds(static_cast<int64_t>(24 * 60 * 60 * 1e6 + 1))));

  EXPECT_EQ("-0s", TimeIntervalAsHumanReadableString(std::chrono::microseconds(-1)));
  EXPECT_EQ("-1s", TimeIntervalAsHumanReadableString(std::chrono::microseconds(static_cast<int64_t>(-1e6))));
  EXPECT_EQ("-1s", TimeIntervalAsHumanReadableString(std::chrono::microseconds(static_cast<int64_t>(-1e6 - 1))));
  EXPECT_EQ("-0s", TimeIntervalAsHumanReadableString(std::chrono::microseconds(static_cast<int64_t>(-1e6 + 1))));

  EXPECT_EQ("-1h 0m 0s",
            TimeIntervalAsHumanReadableString(std::chrono::microseconds(static_cast<int64_t>(-60 * 60 * 1e6))));
}

TEST(Util, Escape) {
  EXPECT_EQ("Smoke.", EscapeForCPlusPlus("Smoke."));
  EXPECT_EQ("vertical|bar", EscapeForCPlusPlus("vertical|bar"));
  EXPECT_EQ("new\\nline", EscapeForCPlusPlus("new\nline"));
  EXPECT_EQ("t\\ts\\tv", EscapeForCPlusPlus("t\ts\tv"));
  EXPECT_EQ("Testing \\'quote\\' and \\\"quote\\\".", EscapeForCPlusPlus("Testing 'quote' and \"quote\"."));

  EXPECT_EQ("Smoke.", EscapeForMarkdown("Smoke."));
  EXPECT_EQ("vertical&#124;bar", EscapeForMarkdown("vertical|bar"));
  EXPECT_EQ("new<br>line", EscapeForMarkdown("new\nline"));
  EXPECT_EQ("t\ts\tv", EscapeForMarkdown("t\ts\tv"));
  EXPECT_EQ("Testing 'quote' and \"quote\".", EscapeForMarkdown("Testing 'quote' and \"quote\"."));
}

TEST(IsStringType, StaticAsserts) {
  static_assert(!is_string_type<int>::value, "");

  static_assert(is_string_type<char>::value, "");

  static_assert(is_string_type<char*>::value, "");

  static_assert(is_string_type<const char*>::value, "");
  static_assert(is_string_type<const char*&>::value, "");
  static_assert(is_string_type<const char*&&>::value, "");
  static_assert(is_string_type<char*&&>::value, "");

  static_assert(is_string_type<std::string>::value, "");
  static_assert(is_string_type<const std::string&>::value, "");
  static_assert(is_string_type<std::string&&>::value, "");

  static_assert(is_string_type<std::vector<char>>::value, "");
  static_assert(is_string_type<const std::vector<char>&>::value, "");
  static_assert(is_string_type<std::vector<char>&&>::value, "");
}
