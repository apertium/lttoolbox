#ifndef _XML_WALK_UTIL_
#define _XML_WALK_UTIL_

#include <lttoolbox/ustring.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

class children
{
private:
  xmlNode* node;
  xmlNode* cur;
public:
  children(xmlNode* node);
  children(const children& it);
  ~children();

  children& operator++();
  children begin();
  children end();
  inline xmlNode* operator*() const { return cur; }
  bool operator!=(const children& other) const;
  bool operator==(const children& other) const;
};

xmlNode* load_xml(const char* fname);
void error_and_die(xmlNode* node, const char* fmt, ...);

UString getattr(xmlNode* node, const char* attr);
UString getattr(xmlNode* node, UStringView attr, UStringView fallback = u"");

#endif
