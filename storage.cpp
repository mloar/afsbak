#include <string>
#include <map>
#include "storage.h"

std::map<std::pair<std::string, int>, std::string> m_entries;
void add(const char *dir, int vnode, const char *file)
{
  m_entries.insert(std::make_pair(std::make_pair(dir, vnode), file));
}

const char *get(const char *dir, int vnode)
{
  return m_entries.find(std::make_pair(dir, vnode))->second.c_str();
}
