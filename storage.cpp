#include <string>
#include <map>
#include "storage.h"

std::map<int, std::string> m_entries;
void add(int vnode, const char *file)
{
  m_entries.insert(std::make_pair(vnode, file));
}

const char *get(int vnode)
{
  return m_entries.find(vnode)->second.c_str();
}
