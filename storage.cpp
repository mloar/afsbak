/*
 * Written by Matthew Loar <matthew@loar.name>
 * This work is hereby placed in the public domain by its author.
 */

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
    if (m_entries.find(vnode) != m_entries.end())
    {
        return m_entries.find(vnode)->second.c_str();
    }
    else
    {
        return NULL;
    }
}
