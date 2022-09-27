/*
 * Copyright (C) 2022 Apertium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */
#include <lttoolbox/acx.h>
#include <lttoolbox/xml_walk_util.h>

const xmlChar* CHAR_NODE = (const xmlChar*)"char";
const xmlChar* EQUIV_NODE = (const xmlChar*)"equiv-char";
const char* VALUE_ATTR = "value";

int32_t get_val(xmlNode* node)
{
  UString s = getattr(node, VALUE_ATTR);
  if (s.empty()) {
    error_and_die(node, "Missing value attribute.");
  }
  std::vector<int32_t> v;
  ustring_to_vec32(s, v);
  if (v.size() > 1) {
    error_and_die(node, "Expected a single character in value attribute, but found %d.", v.size());
  }
  return v[0];
}

std::map<int32_t, sorted_vector<int32_t>> readACX(const char* file)
{
  std::map<int32_t, sorted_vector<int32_t>> acx;
  xmlNode* top_node = load_xml(file);
  for (auto char_node : children(top_node)) {
    if (!xmlStrEqual(char_node->name, CHAR_NODE)) {
      error_and_die(char_node, "Expected <char> but found <%s>.",
                    (const char*)char_node->name);
    }
    int32_t key = get_val(char_node);
    sorted_vector<int32_t> vec;
    for (auto equiv_node : children(char_node)) {
      if (!xmlStrEqual(equiv_node->name, EQUIV_NODE)) {
        error_and_die(char_node, "Expected <equiv-char> but found <%s>.",
                      (const char*)equiv_node->name);
      }
      vec.insert(get_val(equiv_node));
    }
    if (!vec.empty()) {
      acx.insert({key, vec});
    }
  }
  return acx;
}
