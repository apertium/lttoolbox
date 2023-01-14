#ifndef __LT_SYMBOL_ITER_H__
#define __LT_SYMBOL_ITER_H__

#include <ustring.h>

class symbol_iter
{
private:
  UStringView str;
  UStringView::size_type sloc = 0;
  UStringView::size_type eloc = 0;
public:
  symbol_iter(UStringView s);
  symbol_iter(const symbol_iter& other);
  ~symbol_iter();
  UStringView operator*();
  symbol_iter& operator++();
  bool operator!=(const symbol_iter& other) const;
  bool operator==(const symbol_iter& other) const;
  symbol_iter begin();
  symbol_iter end();
};

#endif
