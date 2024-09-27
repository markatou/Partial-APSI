# FindGMP.cmake
#
# Finds the GNU Multiple Precision Arithmetic Library (GMP)
# See http://gmplib.org/
#
# This will define the following variables::
#
#    GMP_FOUND
#    GMP_VERSION
#    GMP_DEFINITIONS
#    GMP_INCLUDE_DIR
#    GMP_LIBRARY
#    GMP_GMPXX_DEFINITIONS
#    GMP_GMPXX_INCLUDE_DIR
#    GMP_GMPXX_LIBRARY
#
# and the following imported targets::
#
#     GMP::GMP
#     GMP::GMPXX

find_package(PkgConfig QUIET)
pkg_check_modules(PC_GMP QUIET gmp gmpxx)

set(GMP_VERSION ${PC_GMP_gmp_VERSION})

find_library(GMP_LIBRARY
	NAMES gmp libgmp
	HINTS
		${PC_GMP_gmp_LIBDIR}
		${PC_GMP_gmp_LIBRARY_DIRS})

find_path(GMP_INCLUDE_DIR
	NAMES gmp.h
	HINTS
		${PC_GMP_gmp_INCLUDEDIR}
		${PC_GMP_gmp_INCLUDE_DIRS})

find_library(GMP_GMPXX_LIBRARY
	NAMES gmpxx libgmpxx
	HINTS
		${PC_GMP_gmpxx_LIBDIR}
		${PC_GMP_gmpxx_LIBRARY_DIRS})

find_path(GMP_GMPXX_INCLUDE_DIR
	NAMES gmpxx.h
	HINTS
		${PC_GMP_gmpxx_INCLUDEDIR}
		${PC_GMP_gmpxx_INCLUDE_DIRS})


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GMP
	REQUIRED_VARS
		GMP_INCLUDE_DIR
		GMP_LIBRARY
	GMP_GMPXX_INCLUDE_DIR
	GMP_GMPXX_LIBRARY
	VERSION_VAR GMP_VERSION)

if(GMP_FOUND)
	set(GMP_LIBRARIES ${GMP_LIBRARY})
	set(GMP_INCLUDE_DIRS ${GMP_INCLUDE_DIR})
	set(GMP_DEFINITIONS ${PC_GMP_gmp_CFLAGS_OTHER})
	set(GMP_GMPXX_LIBRARIES ${GMP_GMPXX_LIBRARY})
	set(GMP_GMPXX_INCLUDE_DIRS ${GMP_GMPXX_INCLUDE_DIR})
	set(GMP_GMPXX_DEFINITIONS ${PC_GMP_gmpxx_CFLAGS_OTHER})

	if(NOT TARGET GMP::GMP)
		add_library(GMP::GMP UNKNOWN IMPORTED)
		set_target_properties(GMP::GMP PROPERTIES
			INTERFACE_COMPILE_OPTIONS "${PC_GMP_gmp_CFLAGS_OTHER}"
			INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}"
			IMPORTED_LOCATION "${GMP_LIBRARY}")
	endif()

	if(NOT TARGET GMP::GMPXX)
		add_library(GMP::GMPXX UNKNOWN IMPORTED)
		set_target_properties(GMP::GMPXX PROPERTIES
			INTERFACE_COMPILE_OPTIONS "${PC_GMP_gmpxx_CFLAGS_OTHER}"
			INTERFACE_INCLUDE_DIRECTORIES "${GMP_GMPXX_INCLUDE_DIR}"
			INTERFACE_LINK_LIBRARIES GMP::GMP
			IMPORTED_LOCATION "${GMP_GMPXX_LIBRARY}")
	endif()
endif()

mark_as_advanced(GMP_FOUND GMP_INCLUDE_DIR GMP_LIBRARY)
mark_as_advanced(GMP_GMPXX_INCLUDE_DIR GMP_GMPXX_LIBRARY)
