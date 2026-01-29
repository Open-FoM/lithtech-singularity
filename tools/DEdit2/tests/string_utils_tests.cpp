#include "app/string_utils.h"
#include "editor_state.h"

#include <gtest/gtest.h>

TEST(StringUtils, LowerCopy)
{
  EXPECT_EQ("hello", LowerCopy("HELLO"));
  EXPECT_EQ("mixed", LowerCopy("MiXeD"));
  EXPECT_EQ("already", LowerCopy("already"));
}

TEST(StringUtils, TrimLine)
{
  EXPECT_EQ("", TrimLine(""));
  EXPECT_EQ("hello", TrimLine("hello"));
  EXPECT_EQ("hello", TrimLine("  hello"));
  EXPECT_EQ("hello", TrimLine("hello  "));
  EXPECT_EQ("hello", TrimLine("\t  hello\n"));
}

TEST(StringUtils, TrimQuotes)
{
  EXPECT_EQ("hello", TrimQuotes("\"hello\""));
  EXPECT_EQ("hello", TrimQuotes("'hello'"));
  EXPECT_EQ("hello", TrimQuotes("  \"hello\"  "));
  // Mismatched quotes are not trimmed
  EXPECT_EQ("'hello\"", TrimQuotes("'hello\""));
}

TEST(StringUtils, TryGetRawPropertyString)
{
  NodeProperties props;
  props.raw_properties.emplace_back("Name", "\"Value\"");
  props.raw_properties.emplace_back("Other", "Plain");

  std::string out;
  EXPECT_TRUE(TryGetRawPropertyString(props, "name", out));
  EXPECT_EQ("Value", out);

  EXPECT_TRUE(TryGetRawPropertyString(props, "OTHER", out));
  EXPECT_EQ("Plain", out);

  EXPECT_FALSE(TryGetRawPropertyString(props, "missing", out));
}

TEST(StringUtils, TryGetRawPropertyStringAny)
{
  NodeProperties props;
  props.raw_properties.emplace_back("Path", "\"C:/data\"");

  std::string out;
  EXPECT_TRUE(TryGetRawPropertyStringAny(props, {"missing", "path"}, out));
  EXPECT_EQ("C:/data", out);

  EXPECT_FALSE(TryGetRawPropertyStringAny(props, {"missing", "other"}, out));
}
