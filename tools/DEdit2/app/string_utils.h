#pragma once

#include <initializer_list>
#include <string>

struct NodeProperties;

std::string LowerCopy(std::string value);
std::string TrimLine(const std::string& value);
std::string TrimQuotes(const std::string& value);

bool TryGetRawPropertyString(const NodeProperties& props, const char* key, std::string& out);
bool TryGetRawPropertyStringAny(
  const NodeProperties& props,
  const std::initializer_list<const char*>& keys,
  std::string& out);
