// Copyright (c) 2012  Tel-Aviv University (Israel).
// All rights reserved.
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Author(s)     : Alex Tsui <alextsui05@gmail.com>

#ifndef ISNAPPABLE_H
#define ISNAPPABLE_H

class ISnappable
{
public:
  virtual ~ISnappable( ) { }
  virtual void setSnappingEnabled( bool b ) = 0;
  virtual void setSnapToGridEnabled( bool b ) = 0;
}; // class ISnappable


#endif // SNAPPABLE_H
