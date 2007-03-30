#include <lttoolbox/SortedVector.H>
#include <cstdlib>

using namespace std;

void
SortedVector::copy(SortedVector const &o)
{
  sv = new SVNode[o.size];
  size = o.size;
  
  for(int i = 0; i != size; i++)
  {
    sv[i].tag = o.sv[i].tag;
    sv[i].dest = o.sv[i].dest;
  }
}

void
SortedVector::destroy()
{
  delete sv;
}

SortedVector::SortedVector(int const fixed_size)
{
  sv = new SVNode[fixed_size];
  size = fixed_size;
}

SortedVector::~SortedVector()
{
  destroy();
}

SortedVector::SortedVector(SortedVector const &o)
{
  copy(o);
}

SortedVector &
SortedVector::operator =(SortedVector const &o)
{
  if(this != &o)
  {
    destroy();
    copy(o);
  }
  return *this;
}

void
SortedVector::add(int tag, MatchNode *dest, int pos)
{
  sv[pos].tag = tag;
  sv[pos].dest = dest; 
}

MatchNode *
SortedVector::search(int tag)
{
  int left = 0, right = size-1;
  while(left <= right)
  {
    int mid = (left+right)/2;
    if(sv[mid].tag == tag)
    {  
      return sv[mid].dest;
    }
    if(sv[mid].tag > tag)
    {
      right = mid - 1;
    }
    else
    {
      left = mid + 1;
    }
  }
  
  return NULL;
}
