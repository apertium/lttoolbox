#include <lttoolbox/xml_walk_util.h>
#include <iostream>

children::children(xmlNode* node_)
  : node(node_), cur(node->children)
{
  while (cur && cur->type != XML_ELEMENT_NODE) {
    cur = cur->next;
  }
}

children::children(const children& it)
  : node(it.node), cur(it.cur)
{}

children::~children()
{} // we don't own the pointers, so we don't delete them

children&
children::operator++()
{
  if (node && cur) {
    cur = cur->next;
    while (cur && cur->type != XML_ELEMENT_NODE) {
      cur = cur->next;
    }
  }
  return *this;
}

children
children::begin()
{
  return children(node);
}

children
children::end()
{
  children ret(node);
  ret.cur = nullptr;
  return ret;
}

bool
children::operator!=(const children& other) const
{
  return node != other.node || cur != other.cur;
}

bool
children::operator==(const children& other) const
{
  return node == other.node && cur == other.cur;
}

xmlNode*
load_xml(const char* fname)
{
  xmlDoc* doc = xmlReadFile(fname, NULL, 0);
  if (doc == nullptr) {
    std::cerr << "Error: Could not parse file '" << fname << "'." << std::endl;
    exit(EXIT_FAILURE);
  }
  return xmlDocGetRootElement(doc);
}

void
error_and_die(xmlNode* node, const char* fmt, ...)
{
  UFILE* err_out = u_finit(stderr, NULL, NULL);
  u_fprintf(err_out, "Error in %S on line %d: ",
            to_ustring((char*) node->doc->URL).c_str(), node->line);
  va_list argptr;
  va_start(argptr, fmt);
  u_vfprintf(err_out, fmt, argptr);
  va_end(argptr);
  u_fputc('\n', err_out);
  exit(EXIT_FAILURE);
}

UString
getattr(xmlNode* node, const char* attr)
{
  for (xmlAttr* i = node->properties; i != NULL; i = i->next) {
    if (!xmlStrcmp(i->name, (const xmlChar*) attr)) {
      return to_ustring((const char*) i->children->content);
    }
  }
  return ""_u;
}

UString
getattr(xmlNode* node, UStringView attr, UStringView fallback)
{
  for (xmlAttr* i = node->properties; i != NULL; i = i->next) {
    if (to_ustring((const char*) i->name) == attr) {
      return to_ustring((const char*) i->children->content);
    }
  }
  return US(fallback);
}
