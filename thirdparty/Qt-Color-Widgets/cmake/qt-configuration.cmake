function (select_qt
    QT_SUPPORTED_VERSIONS i_qt_supported_versions
    QT_DEFAULT_VERSION i_default_qt_version)
  set (QT_VERSION ${i_default_qt_version} CACHE STRING
    "Qt version to build against")

  set_property (
    CACHE QT_VERSION
    PROPERTY STRINGS ${i_qt_supported_versions})
endfunction (select_qt)


macro (find_qt
    QT_SUPPORTED_VERSIONS i_qt_supported_versions
    REQUIRED_COMPONENTS i_required_components
    OPTIONAL_COMPONENTS i_optional_components)
  # Auto generate moc files
  if (4 EQUAL QT_VERSION)
    find_package (Qt4 4.8 REQUIRED
      COMPONENTS
      ${i_required_components}
      OPTIONAL_COMPONENTS
      ${i_optional_components})
  elseif (5 EQUAL QT_VERSION)
    find_package (Qt5 5.9 REQUIRED
      COMPONENTS
      ${i_required_components}
      OPTIONAL_COMPONENTS
      ${i_optional_components})
  else (5 EQUAL QT_VERSION)
    message (FATAL_ERROR "Unsupported version of Qt: ${QT_VERSION}")
  endif (4 EQUAL QT_VERSION)
endmacro (find_qt)


function (use_qt
    TARGET_NAME i_target_name
    QT_SUPPORTED_VERSIONS i_qt_supported_versions
    REQUIRED_COMPONENTS i_required_components
    OPTIONAL_COMPONENTS i_optional_components)
  if (4 EQUAL QT_VERSION)
    foreach (COMPONENT IN LISTS i_required_components, i_optional_components)
      target_link_libraries (${i_target_name}
        PRIVATE
        Qt4::${COMPONENT}
        )
    endforeach (COMPONENT IN LISTS i_required_components, i_optional_components)
  elseif (5 EQUAL QT_VERSION)
    foreach (COMPONENT IN LISTS i_required_components)
      target_link_libraries (${i_target_name}
        PRIVATE
        Qt5::${COMPONENT}
        )
    endforeach (COMPONENT IN LISTS i_required_components, i_optional_components)
  else (5 EQUAL QT_VERSION)
    message (FATAL_ERROR "Unsupported version of Qt: ${QT_VERSION}")
  endif (4 EQUAL QT_VERSION)
endfunction (use_qt)
