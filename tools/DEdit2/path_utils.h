#pragma once

#include <string>

/// Shared path utilities for DEdit2.
/// Provides path normalization routines used by both texture and model loading.
namespace path_utils
{

/// Removes leading and trailing whitespace from a string.
std::string TrimWhitespace(std::string name);

/// Sanitizes a file path by normalizing separators (backslash to forward slash),
/// removing control characters, and collapsing duplicate separators.
std::string SanitizePath(const std::string& name);

/// Combines TrimWhitespace + SanitizePath for full path normalization.
std::string NormalizePathSeparators(std::string name);

/// Checks if a path has a file extension (e.g., ".dtx", ".ltb").
bool HasExtension(const std::string& value);

} // namespace path_utils
