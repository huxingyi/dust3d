if(CARVE_SYSTEM_BOOST)
  install(DIRECTORY carve
          DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
          FILES_MATCHING
            PATTERN "*.hpp"
            PATTERN "internal" EXCLUDE
            REGEX "external/boost" EXCLUDE
  )
else(CARVE_SYSTEM_BOOST)
  install(DIRECTORY carve
          DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
          FILES_MATCHING
            PATTERN "*.hpp"
            PATTERN "internal" EXCLUDE
  )
endif(CARVE_SYSTEM_BOOST)
