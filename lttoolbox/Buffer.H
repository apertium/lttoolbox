/*
 * Copyright (C) 2005 Universitat d'Alacant / Universidad de Alicante
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#ifndef _BUFFER_
#define _BUFFER_

#include <iostream>
#include <cstdlib>
#include <cstring>

using namespace std;

/**
 * Generic circular buffer class
 */
template <class T>
class Buffer
{
private:
  /**
   * Buffer size.
   */
  unsigned int size;
  
  /**
   * Buffer array.
   */
  T *buf;
   
  /**
   * Buffer current position.
   */
  unsigned int currentpos;
  
  /**
   * Last position.
   */
  unsigned int lastpos;
  
  
  void copy(Buffer const &b)
    {
      currentpos = b.currentpos;
      lastpos = b.lastpos;
      size = b.size;
      buf = new T[size];
      memcpy(buf, b.buf, size);
    }

  void destroy()
    {
      delete[] buf;
    }

public:  

  /**
   * Constructor
   * @param buf_size buffer size
   */
  Buffer(unsigned int const buf_size=2048)
    {
      if(buf_size == 0)
      {
	cerr << "Error: Cannot create empty buffer." << endl;
	exit(EXIT_FAILURE);
      }    
      buf = new T[buf_size];
      size = buf_size;
      currentpos = 0;
      lastpos = 0;
    }

  /**
   * Copy constructor.
   */
  Buffer(Buffer const &b)
    {
      copy(b);
    }

  /**
   * Destructor.
   */
  ~Buffer()
    {
      destroy();
    }

  /**
   * Assign operator.
   */
  Buffer & operator=(Buffer const &b)
    {
      if(&b != this)
      {
	destroy();
	copy(b);
      }
      return *this;
    }
    
  /**
   * Add an element to the buffer.
   * @param value the value.
   * @return reference to the stored object.
   */
  T & add(T const &value)
    {
      if(lastpos == size)
      {
	lastpos = 0;
      }
      buf[lastpos++] = value;
      currentpos = lastpos;
      return buf[lastpos - 1];
    }
  

  /**
   * Add an element to the buffer and not modify its content, getting the
   * next free reference
   * @return reference to the stored object.
   */
  T & add(void)
    {
      if(lastpos == size)
      {
	lastpos = 0;
      }
      currentpos = lastpos;
      return buf[lastpos -1];
    }


  /**
   * Consume the buffer's current value.
   * @return the current value.
   */
  T& next()
    {
      if(currentpos != lastpos)
      {
	if(currentpos == size)
	{
	  currentpos = 0;
	}
	return buf[currentpos++];
      }
      else
      {
	return last();
      }
    }
    
  /**
   * Get the last element of the buffer.
   * @return last element.
   */
  T & last() const
    {
      if(lastpos != 0)
      {
	return buf[lastpos-1];
      }
      else
      {
	return buf[size-1];
      }
    }
 
  /**
   * Get the current buffer position.
   * @return the position.
   */
  unsigned int getPos() const
    {
      return currentpos;
    }

  /**
   * Set the buffer to a new position.
   * @param newpos the new position.
   */
  void setPos(unsigned int const newpos)
    {
      currentpos = newpos;
    }

  /**
   * Return the range size between the buffer current position and a
   * outside stored given position that is previous to the current.
   * @param prevpos the given position.
   * @return the range size.
   */
  unsigned int diffPrevPos(unsigned int const prevpos) const
    {
      if(prevpos <= currentpos)
      {
	return currentpos - prevpos;
      }
      else
      {
	return currentpos + size - prevpos;
      }
    } 

  /**
   * Return the range size between the buffer current position and a
   * outside stored given position that is following to the current.
   * @param postpos the given position.
   * @return the range size.
   */
  unsigned int diffPostPos(unsigned int const postpos) const
    {
      if(postpos >= currentpos)
      {
	return postpos - currentpos;
      }
      else
      {
	return postpos + size - currentpos;
      }
    }

  /**
   * Checks the buffer for emptyness.
   * @return true if the buffer is empty.
   */
  bool isEmpty() const
    {
      return currentpos == lastpos;   
    }
  
  /**
   * Gets back 'posback' positions in the buffer.
   * @param posback the amount of position to get back.
   */
  void back(unsigned int const posback)
    {
      if(currentpos > posback)
      {
        currentpos-=posback;
      }
      else
      {
        currentpos = size - (posback-currentpos);
      }
    }
};

#endif
