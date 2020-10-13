// Copyright (c) 2018
// GeometryFactory( France) All rights reserved.
//
// This file is part of CGAL (www.cgal.org)
//
// $URL: https://github.com/CGAL/cgal/blob/v5.1/Stream_support/include/CGAL/IO/write_vtk.h $
// $Id: write_vtk.h c8b73c9 2020-08-07T06:40:23+02:00 Sébastien Loriot
// SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Stephane Tayeb

#ifndef CGAL_WRITE_VTK_IO_H
#define CGAL_WRITE_VTK_IO_H

#include <fstream>
#include <vector>
template <class FT>
void
write_vector(std::ostream& os,
             const std::vector<FT>& vect)
{
  const char* buffer = reinterpret_cast<const char*>(&(vect[0]));
  std::size_t size = vect.size()*sizeof(FT);

  os.write(reinterpret_cast<const char *>(&size), sizeof(std::size_t)); // number of bytes encoded
  os.write(buffer, vect.size()*sizeof(FT));                     // encoded data
}
#endif
