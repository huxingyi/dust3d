PROJECT(nvmath)

SET(MATH_SRCS
	nvmath.h
    Basis.h Basis.cpp
    Box.h
    Color.h
    ConvexHull.h ConvexHull.cpp
    Fitting.h Fitting.cpp
    KahanSum.h
    Matrix.h
    Plane.h Plane.cpp
    ProximityGrid.h ProximityGrid.cpp
    Quaternion.h
	Random.h Random.cpp
    Solver.h Solver.cpp
	Sparse.h Sparse.cpp
    TypeSerialization.h TypeSerialization.cpp
    Vector.h)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# targets
ADD_DEFINITIONS(-DNVMATH_EXPORTS)

IF(NVMATH_SHARED)
    ADD_DEFINITIONS(-DNVMATH_SHARED=1)
    ADD_LIBRARY(nvmath SHARED ${MATH_SRCS})
ELSE(NVMATH_SHARED)
    ADD_LIBRARY(nvmath ${MATH_SRCS})
ENDIF(NVMATH_SHARED)

TARGET_LINK_LIBRARIES(nvmath ${LIBS} nvcore)

INSTALL(TARGETS nvmath
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static)
