// Copyright (c) 2012  Tel-Aviv University (Israel).
// All rights reserved.
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Author(s)     : Alex Tsui <alextsui05@gmail.com>

#include "ArrangementDemoWindow.h"
#include <QApplication>

int main( int argc, char* argv[] )
{
  QApplication app( argc, argv );

  ArrangementDemoWindow demoWindow;
  demoWindow.show( );

  return app.exec( );
}
