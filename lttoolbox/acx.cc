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
#include <iostream>
#include <unicode/ustream.h>
#include <i18n.h>

const xmlChar* CHAR_NODE = (const xmlChar*)"char";
const xmlChar* EQUIV_NODE = (const xmlChar*)"equiv-char";
const char* VALUE_ATTR = "value";

int32_t get_val(xmlNode* node)
{
  I18n i18n {LTTB_I18N_DATA, "lttoolbox"};
  UString s = getattr(node, VALUE_ATTR);
  if (s.empty()) {
    i18n.error("LTTB1001", {"node_doc_url", "line_number"},
                           {(char*)node->doc->URL, node->line}, true);
  }
  std::vector<int32_t> v;
  ustring_to_vec32(s, v);
  if (v.size() > 1) {
    i18n.error("LTTB1002", {"node_doc_url", "line_number", "value_size"},
                           {(char*)node->doc->URL, node->line, std::to_string(v.size()).c_str()}, true);
  }
  return v[0];
}

std::map<int32_t, sorted_vector<int32_t>> readACX(const char* file)
{
  I18n i18n {LTTB_I18N_DATA, "lttoolbox"};
  std::map<int32_t, sorted_vector<int32_t>> acx;
  xmlNode* top_node = load_xml(file);
  for (auto char_node : children(top_node)) {
    if (!xmlStrEqual(char_node->name, CHAR_NODE)) {
      i18n.error("LTTB1003", {"node_doc_url", "line_number", "node_name"},
                             {(char*)char_node->doc->URL, char_node->line, (const char*)char_node->name}, true);
    }
    int32_t key = get_val(char_node);
    sorted_vector<int32_t> vec;
    for (auto equiv_node : children(char_node)) {
      if (!xmlStrEqual(equiv_node->name, EQUIV_NODE)) {
        i18n.error("LTTB1004", {"node_doc_url", "line_number", "node_name"},
                               {(char*)char_node->doc->URL, char_node->line, (const char*)equiv_node->name}, true);
      }
      vec.insert(get_val(equiv_node));
    }
    if (!vec.empty()) {
      acx.insert({key, vec});
    }
  }
  return acx;
}
