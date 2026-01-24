#include "path_utils.h"

#include <cctype>

namespace path_utils
{

std::string TrimWhitespace(std::string name)
{
  auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
  while (!name.empty() && is_space(static_cast<unsigned char>(name.front())))
  {
    name.erase(name.begin());
  }
  while (!name.empty() && is_space(static_cast<unsigned char>(name.back())))
  {
    name.pop_back();
  }
  return name;
}

std::string SanitizePath(const std::string& name)
{
  std::string out;
  out.reserve(name.size());
  bool last_was_sep = false;
  for (unsigned char raw : name)
  {
    if (std::iscntrl(raw) != 0)
    {
      continue;
    }
    char c = static_cast<char>(raw);
    if (c == '\\' || c == '/')
    {
      if (!last_was_sep)
      {
        out.push_back('/');
        last_was_sep = true;
      }
      continue;
    }
    last_was_sep = false;
    out.push_back(c);
  }
  return out;
}

std::string NormalizePathSeparators(std::string name)
{
  name = TrimWhitespace(std::move(name));
  return SanitizePath(name);
}

bool HasExtension(const std::string& value)
{
  const size_t slash = value.find_last_of("/\\");
  const size_t dot = value.find_last_of('.');
  return dot != std::string::npos && (slash == std::string::npos || dot > slash);
}

} // namespace path_utils
