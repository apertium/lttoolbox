#ifndef __LT_SYMBOL_ITER_H__
#define __LT_SYMBOL_ITER_H__

#include <ustring.h>

class symbol_iter
{
private:
  UStringView str;
public:
  symbol_iter(UStringView s) : str(s) {}
  ~symbol_iter() {}
  class iterator
  {
    friend symbol_iter;
  private:
    UStringView str;
    UStringView::size_type sloc = 0;
    UStringView::size_type eloc = 0;
  public:
    iterator(UStringView s);
    iterator(const iterator& other);
    ~iterator();
    UStringView operator*() const;
    iterator& operator++();
    bool operator!=(const symbol_iter::iterator& other) const;
    bool operator==(const symbol_iter::iterator& other) const;
  };
  iterator begin() const;
  iterator end() const;
};

#endif
