// Copyright (c) 2010  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Author(s)     : Sophie Fei Che <fei@cis.udel.edu>
//
// File Description : Demo of CGAL 3D Triangulation package

#include "MainWindow.h"
#include <QApplication>

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  app.setOrganizationDomain("inria.fr");
  app.setOrganizationName("INRIA");
  app.setApplicationName("3D Triangulation Demo");
  //for windows
#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0))
  app.setAttribute(Qt::AA_UseDesktopOpenGL);
#endif

  MainWindow mw;
  mw.show();

  return app.exec();
}
