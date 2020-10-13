// Copyright (c) 2019
// Utrecht University (The Netherlands),
// ETH Zurich (Switzerland),
// INRIA Sophia-Antipolis (France),
// Max-Planck-Institute Saarbruecken (Germany),
// and Tel-Aviv University (Israel).  All rights reserved.
//
// This file is part of CGAL (www.cgal.org)
//
// $URL: https://github.com/CGAL/cgal/blob/v5.1/Number_types/include/CGAL/Exact_algebraic.h $
// $Id: Exact_algebraic.h 66040cb 2020-07-20T17:13:01+02:00 Laurent Rineau
// SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Andreas Fabri

#include <CGAL/number_type_basic.h>

#ifdef CGAL_USE_LEDA
#  include <CGAL/leda_integer.h>
#  include <CGAL/leda_rational.h>
#  include <CGAL/leda_real.h>
#endif
#ifdef CGAL_USE_CORE
#  include <CGAL/CORE_Expr.h>
#endif


namespace CGAL {

/*!
\ingroup nt_cgal

`Exact_algebraic` is an exact algebraic number type, constructible from `double`.

It is a typedef of another number type. Its exact definition depends on
the availability the third-party libraries %CORE, and %LEDA. %CGAL must
be configured with at least one of those libraries.

\cgalModels `FieldWithSqrt`
\cgalModels `RealEmbeddable`
\cgalModels `Fraction`
\cgalModels `FromDoubleConstructible`

*/
#if DOXYGEN_RUNNING

typedef unspecified_type Exact_algebraic;

#else // not DOXYGEN_RUNNING

#ifdef CGAL_USE_LEDA
typedef leda_real Exact_algebraic;
#elif defined CGAL_USE_CORE
  typedef CORE::Expr Exact_algebraic;
#endif

#endif

} /* end namespace CGAL */
