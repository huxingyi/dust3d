PROJECT(nvcore)

SET(CORE_SRCS
	nvcore.h
	DefsGnucDarwin.h
	DefsGnucLinux.h
	DefsGnucWin32.h
	DefsVcWin32.h
	Ptr.h
	RefCounted.h
	BitArray.h
	Memory.h
	Memory.cpp
	Debug.h
	Debug.cpp
	StrLib.h
	StrLib.cpp
	Stream.h
	StdStream.h
	FileSystem.h
	FileSystem.cpp
	RadixSort.h
	RadixSort.cpp)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# For Windows64 in MSVC we need to add the assembly version of vsscanf
IF(MSVC AND NV_SYSTEM_PROCESSOR STREQUAL "AMD64")
  SET(VSSCANF_ASM_NAME "vsscanf_proxy_win64")
  IF(MSVC_IDE)
    # $(IntDir) is a macro expanded to the intermediate directory of the selected solution configuration
    SET(VSSCANF_ASM_INTDIR "$(IntDir)")
  ELSE(MSVC_IDE)
    # For some reason the NMake generator doesn't work properly with the generated .obj source:
	# it requires the absolute path. So this is a hack which worked as of cmake 2.6.0 patch 0
	GET_FILENAME_COMPONENT(VSSCANF_ASM_INTDIR 
	    "${nvcore_BINARY_DIR}/CMakeFiles/nvcore.dir" ABSOLUTE)
  ENDIF(MSVC_IDE)
  
  SET(VSSCANF_ASM_SRC "${CMAKE_CURRENT_SOURCE_DIR}/${VSSCANF_ASM_NAME}.masm")
  SET(VSSCANF_ASM_OBJ "${VSSCANF_ASM_INTDIR}/${VSSCANF_ASM_NAME}.obj")

  # Adds the assembly output to the sources and adds the custom command to generate it
  SET(CORE_SRCS
	${CORE_SRCS}
	${VSSCANF_ASM_OBJ}
  )
  ADD_CUSTOM_COMMAND(OUTPUT ${VSSCANF_ASM_OBJ}
					 MAIN_DEPENDENCY ${VSSCANF_ASM_SRC}
					 COMMAND ml64
					 ARGS  /nologo /Fo ${VSSCANF_ASM_OBJ} /c /Cx ${VSSCANF_ASM_SRC}
  )
ENDIF(MSVC AND NV_SYSTEM_PROCESSOR STREQUAL "AMD64")
	
# targets
ADD_DEFINITIONS(-DNVCORE_EXPORTS)

IF(UNIX)
    SET(LIBS ${LIBS} ${CMAKE_DL_LIBS})
ENDIF(UNIX)

IF(NVCORE_SHARED)
    ADD_DEFINITIONS(-DNVCORE_SHARED=1)
    ADD_LIBRARY(nvcore SHARED ${CORE_SRCS})
ELSE(NVCORE_SHARED)
	ADD_LIBRARY(nvcore ${CORE_SRCS})
ENDIF(NVCORE_SHARED)

TARGET_LINK_LIBRARIES(nvcore ${LIBS})

INSTALL(TARGETS nvcore
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib/static)
