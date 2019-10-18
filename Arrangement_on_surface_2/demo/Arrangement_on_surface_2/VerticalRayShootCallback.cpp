// Copyright (c) 2012  Tel-Aviv University (Israel).
// All rights reserved.
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Author(s)     : Alex Tsui <alextsui05@gmail.com>

#include "VerticalRayShootCallback.h"

VerticalRayShootCallbackBase::VerticalRayShootCallbackBase(QObject* parent_) :
  CGAL::Qt::Callback( parent_ ),
  shootingUp( true )
{ }

void VerticalRayShootCallbackBase::setShootingUp( bool isShootingUp )
{
  this->shootingUp = isShootingUp;
}

