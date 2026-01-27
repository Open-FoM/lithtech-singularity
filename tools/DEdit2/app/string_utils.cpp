#include "app/string_utils.h"

#include "editor_state.h"

#include <cctype>

std::string LowerCopy(std::string value)
{
  for (char& ch : value)
  {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}

std::string TrimLine(const std::string& value)
{
  size_t start = 0;
  while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
  {
    ++start;
  }
  size_t end = value.size();
  while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
  {
    --end;
  }
  return value.substr(start, end - start);
}

std::string TrimQuotes(const std::string& value)
{
  std::string trimmed = TrimLine(value);
  if (trimmed.size() >= 2)
  {
    const char first = trimmed.front();
    const char last = trimmed.back();
    if ((first == '\"' && last == '\"') || (first == '\'' && last == '\''))
    {
      trimmed = trimmed.substr(1, trimmed.size() - 2);
    }
  }
  return trimmed;
}

bool TryGetRawPropertyString(const NodeProperties& props, const char* key, std::string& out)
{
  if (key == nullptr)
  {
    return false;
  }
  const std::string key_lower = LowerCopy(key);
  for (const auto& entry : props.raw_properties)
  {
    if (LowerCopy(entry.first) == key_lower)
    {
      out = TrimQuotes(entry.second);
      return true;
    }
  }
  return false;
}

bool TryGetRawPropertyStringAny(
  const NodeProperties& props,
  const std::initializer_list<const char*>& keys,
  std::string& out)
{
  for (const char* key : keys)
  {
    if (TryGetRawPropertyString(props, key, out))
    {
      return true;
    }
  }
  return false;
}
