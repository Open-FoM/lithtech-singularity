#include "path_utils.h"

#include <gtest/gtest.h>

TEST(PathUtils, TrimWhitespace)
{
  EXPECT_EQ("", path_utils::TrimWhitespace(""));
  EXPECT_EQ("hello", path_utils::TrimWhitespace("hello"));
  EXPECT_EQ("hello", path_utils::TrimWhitespace("  hello"));
  EXPECT_EQ("hello", path_utils::TrimWhitespace("hello  "));
  EXPECT_EQ("hello", path_utils::TrimWhitespace("\t  hello\n"));
}

TEST(PathUtils, SanitizePath)
{
  EXPECT_EQ("a/b/c", path_utils::SanitizePath("a\\b//c"));
  EXPECT_EQ("a/b", path_utils::SanitizePath("a///b"));
  EXPECT_EQ("a/b", path_utils::SanitizePath("a/\n/b"));
  EXPECT_EQ("a/b", path_utils::SanitizePath("a/\r/b"));
  EXPECT_EQ("a/b", path_utils::SanitizePath("a/\t/b"));
}

TEST(PathUtils, NormalizePathSeparators)
{
  EXPECT_EQ("a/b", path_utils::NormalizePathSeparators("  a\\b  "));
  EXPECT_EQ("a/b/c", path_utils::NormalizePathSeparators("\t a///b//c \n"));
}

TEST(PathUtils, HasExtension)
{
  EXPECT_TRUE(path_utils::HasExtension("file.txt"));
  EXPECT_TRUE(path_utils::HasExtension("dir/file.txt"));
  EXPECT_FALSE(path_utils::HasExtension("dir/file"));
  EXPECT_FALSE(path_utils::HasExtension("dir.with.dot/file"));
}
