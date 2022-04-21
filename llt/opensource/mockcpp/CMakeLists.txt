cmake_minimum_required(VERSION 3.14.1)
project(mockcpp)

set(MOCKCPP_PACKAGE_NAME \"mockcpp\")
set(MOCKCPP_PACKAGE_STRING \"mockcpp\")
set(MOCKCPP_PACKAGE_BUGREPORT \"dev.mockcpp@gmail.com\")
set(MOCKCPP_PACKAGE_URL \"http://code.google.com/p/mockcpp\")
set(MOCKCPP_PACKAGE \"mockcpp\")
set(MOCKCPP_VERSION \"2.6\")

# set(mockcpp_xunit "testngpp" CACHE STRING "Set to gtest/cpputest/cppunit to use other testframeworks.")
set(mockcpp_xunit "testngpp")

include(FindPythonInterp)

if (PYTHONINTERP_FOUND)
  set(PYTHON ${PYTHON_EXECUTABLE})
else ()
  message(FATAL_ERROR "No Python interpreter found")
endif (PYTHONINTERP_FOUND)

if (NOT DEFINED MOCKCPP_ALLOW_MI)
  set(MOCKCPP_ALLOW_MI TRUE)
endif (NOT DEFINED MOCKCPP_ALLOW_MI)

if (NOT DEFINED mockcpp_xunit)
  set(mockcpp_xunit STDEXCEPT)
endif ()
# Set MOCKCPP_MAX_INHERITANCE
#
if (MOCKCPP_ALLOW_MI)
  set(MOCKCPP_ALLOW_MULTI_INHERITANCE 1)
  if (NOT DEFINED MOCKCPP_MAX_INHERITANCE)
    set(MOCKCPP_MAX_INHERITANCE 5)
  endif (NOT DEFINED MOCKCPP_MAX_INHERITANCE)
else (MOCKCPP_ALLOW_MI)
  set(MOCKCPP_ALLOW_MULTI_INHERITANCE 0)
  set(MOCKCPP_MAX_INHERITANCE 1)
endif (MOCKCPP_ALLOW_MI)

if (MOCKCPP_NO_NAMESPACE)
  set(MOCKCPP_NO_NAMESPACE 1)
else (MOCKCPP_NO_NAMESPACE)
  set(MOCKCPP_NO_NAMESPACE 0)
endif (MOCKCPP_NO_NAMESPACE)
#
# Set MOCKCPP_MAX_VTBL_SIZE
#
if (NOT MOCKCPP_MAX_VTBL_SIZE)
  set(MOCKCPP_MAX_VTBL_SIZE 100)
endif (NOT MOCKCPP_MAX_VTBL_SIZE)

if (NOT MOCKCPP_MAX_PARAMETERS)
  set(MOCKCPP_MAX_PARAMETERS 12)
endif (NOT MOCKCPP_MAX_PARAMETERS)

add_definitions(
  -DPACKAGE_NAME=${MOCKCPP_PACKAGE_NAME}
  -DPACKAGE_STRING=${MOCKCPP_PACKAGE_STRING}
  -DPACKAGE_BUGREPORT=${MOCKCPP_PACKAGE_BUGREPORT}
  -DPACKAGE_URL=${MOCKCPP_PACKAGE_URL}
  -DPACKAGE=${MOCKCPP_PACKAGE}
  -DVERSION=${MOCKCPP_VERSION}
  -DMOCKCPP_NO_NAMESPACE=${MOCKCPP_NO_NAMESPACE}
  -DMOCKCPP_ALLOW_MULTI_INHERITANCE=${MOCKCPP_ALLOW_MULTI_INHERITANCE}
  -DMOCKCPP_MAX_INHERITANCE=${MOCKCPP_MAX_INHERITANCE}
  -DMOCKCPP_MAX_VTBL_SIZE=${MOCKCPP_MAX_VTBL_SIZE}
  -DMOCKCPP_MAX_PARAMETERS=${MOCKCPP_MAX_PARAMETERS}
)

if (MSVC)
  add_definitions(-DMSVC_VMG_ENABLED) #  /Z7)
  add_definitions(-wd4996 -wd4100 -wd4706 -wd4702 -wd4458 -wd4127)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /vmg")
  include_directories(BEFORE ${mockcpp_SOURCE_DIR}/3rdparty/msinttypes)
endif (MSVC)

#if (UNIX)
#message("UNIX")
#add_definitions(
#    -g -ggdb
#)
#endif (UNIX)

if (UNIX AND NOT CYGWIN)
  add_definitions(-fPIC)
endif (UNIX AND NOT CYGWIN)

if (CMAKE_CL_64)
  add_definitions(-DWIN64)
endif (CMAKE_CL_64)

set(MOCKCPP_JMPCODE_SRCS
  src/JmpCodeAARCH32.h
  src/JmpCodeAARCH64.h
  src/JmpCodeArch.h
  src/JmpCodeX64.h
  src/JmpCodeX86.h
  src/JmpOnlyApiHook.h
  src/JmpOnlyApiHook.cpp
  src/JmpCode.cpp
)

file(GLOB_RECURSE MOCKCPP_INCS
  include/mockcpp/*.h
  include/mockcpp/*.tcc
  include/mockcpp/*.hpp
)

source_group(JmpCode FILES ${MOCKCPP_JMPCODE_SRCS})

set(MOCKCPP_SRCS 
  ${MOCKCPP_INCS}
  ${MOCKCPP_JMPCODE_SRCS}
  src/AfterMatcher.cpp
  src/AnyBase.cpp
  src/AnyCast.cpp
  src/Any.cpp
  src/Asserter.cpp
  src/AssertionFailedError.cpp
  src/BeforeMatcher.cpp
  src/CallerMatcher.cpp
  src/ChainableMockMethodContainer.cpp
  src/ChainableMockMethodCore.cpp
  src/ChainableMockMethodIndexKey.cpp
  src/ChainableMockMethodNameKey.cpp
  src/ChainableMockObjectBase.cpp
  src/ChainableMockObject.cpp
  src/ChainingMockHelper.cpp
  src/ConstraintSet.cpp
  src/DecoratedConstraint.cpp
  src/DefaultMatcher.cpp
  src/DefaultStub.cpp
  src/DelegatedMethodGetter.cpp
  src/MethodIndiceChecker.cpp
  src/DieStub.cpp
  src/Exception.cpp
  src/ExpectsMatcher.cpp
  src/Formatter.cpp
  src/IdentityBuilder.cpp
  src/IgnoreResultHandler.cpp
  src/IgnoreResultHandlerFactory.cpp
  src/IgnoreReturnStub.cpp
  src/InterfaceInfo.cpp
  src/Invocation.cpp
  src/InvocationId.cpp
  src/InvocationMockBuilderGetter.cpp
  src/InvocationMocker.cpp
  src/InvocationMockerSet.cpp
  src/InvocationTimesMatcher.cpp
  src/InvokedAtLeast.cpp
  src/InvokedAtMost.cpp
  src/InvokedExactly.cpp
  src/InvokedOnce.cpp
  src/InvokedTimesMatcher.cpp
  src/IsAnythingHelper.cpp
  src/IsStringContains.cpp
  src/IsStringEndWith.cpp
  src/IsStringStartWith.cpp
  src/MismatchResultHandler.cpp
  src/MismatchResultHandlerFactory.cpp
  src/MockObjectBase.cpp
  src/NormalResultHandler.cpp
  src/NormalResultHandlerFactory.cpp
  src/OutBoundPointer.cpp
  src/PendingMatcher.cpp
  src/ProcStub.cpp
  src/RefAny.cpp
  src/RepeatStub.cpp
  src/Result.cpp
  src/ReturnObjectList.cpp
  src/ReturnStub.cpp
  src/SimpleInvocationRecorder.cpp
  src/StringConstraint.cpp
  src/StubContainer.cpp
  src/StubsMatcher.cpp
  src/TestFailureMatcher.cpp
  src/ThenStub.cpp
  src/TypelessConstraintAdapter.cpp
  src/TypelessStubAdapter.cpp
  src/TypeString.cpp
  src/VirtualTable.cpp
  src/VirtualTableUtils.cpp
  src/VoidResultHandler.cpp
  src/VoidResultHandlerFactory.cpp
  src/WillStub.cpp
  src/HookMockObject.cpp
  src/ApiHookKey.cpp
  src/GlobalMockObject.cpp
  src/JmpOnlyApiHook.cpp
  src/JmpCode.cpp
  src/ApiHook.cpp
)

if (${mockcpp_xunit} STREQUAL "GTEST" OR ${mockcpp_xunit} STREQUAL "gtest")

  if (NOT DEFINED MOCKCPP_XUNIT_HOME)
    message(FATAL_ERROR "Please specify MOCKCPP_XUNIT_HOME as the home path of googletest")
  endif ()
  find_file(GTEST_HEADER
            gtest.h 
            PATHS ${MOCKCPP_XUNIT_HOME}/include/gtest 
            NO_DEFAULT_PATH 
            NO_CMAKE_ENVIRONMENT_PATH 
            NO_CMAKE_PATH 
            NO_SYSTEM_ENVIRONMENT_PATH 
            NO_CMAKE_SYSTEM_PATH)
  if (NOT GTEST_HEADER)
    message(FATAL_ERROR "gtest.h not found in ${MOCKCPP_XUNIT_HOME}/include/gtest, please specify MOCKCPP_XUNIT_HOME as the home path of googletest correctly.")
  endif ()
  include_directories(BEFORE ${MOCKCPP_XUNIT_HOME}/include)
  set(MOCKCPP_SRCS ${MOCKCPP_SRCS} src/ports/failure/gtest_report_failure.cpp)

elseif (${mockcpp_xunit} STREQUAL "CppUTest" OR ${mockcpp_xunit} STREQUAL "cpputest")

  if (NOT DEFINED MOCKCPP_XUNIT_HOME)
    message(FATAL_ERROR "Please specify MOCKCPP_XUNIT_HOME as the home path of googletest")
  endif ()
  find_file(CPPUTEST_HEADER
            TestHarness.h 
            PATHS ${MOCKCPP_XUNIT_HOME}/include/CppUTest
            NO_DEFAULT_PATH 
            NO_CMAKE_ENVIRONMENT_PATH 
            NO_CMAKE_PATH 
            NO_SYSTEM_ENVIRONMENT_PATH 
            NO_CMAKE_SYSTEM_PATH)
  if (NOT CPPUTEST_HEADER)
    message(FATAL_ERROR "TestHarness.h not found in ${MOCKCPP_XUNIT_HOME}/include/CppUTest, please specify MOCKCPP_XUNIT_HOME as the home path of CppUTest correctly.")
  endif ()
  include_directories(BEFORE ${MOCKCPP_XUNIT_HOME}/include ${MOCKCPP_XUNIT_HOME}/include/Platforms/VisualCpp)
  set(MOCKCPP_SRCS ${MOCKCPP_SRCS} src/ports/failure/cpputest_report_failure.cpp)   
   
elseif (${mockcpp_xunit} STREQUAL "CPPUNIT" OR ${mockcpp_xunit} STREQUAL "cppunit")
  if (NOT DEFINED MOCKCPP_XUNIT_HOME)
    message(FATAL_ERROR "Please specify MOCKCPP_XUNIT_HOME as the home path of cppunit")
  endif ()
  find_file(CPPUNIT_HEADER Exception.h 
            PATHS ${MOCKCPP_XUNIT_HOME}/include/cppunit
            NO_DEFAULT_PATH 
            NO_CMAKE_ENVIRONMENT_PATH 
            NO_CMAKE_PATH 
            NO_SYSTEM_ENVIRONMENT_PATH 
            NO_CMAKE_SYSTEM_PATH)
  if (NOT ${GTEST_HEADER})
    message(FATAL_ERROR "Can't find file Exception.h, please specify MOCKCPP_XUNIT_HOME as the home path of cppunit correctly.")
  endif ()
  include_directories(BEFORE ${MOCKCPP_XUNIT_HOME}/include)
  set(MOCKCPP_SRCS ${MOCKCPP_SRCS} src/ports/failure/cppunit_report_failure.cpp)
else ()
  set(MOCKCPP_SRCS ${MOCKCPP_SRCS} src/ports/failure/stdexcept_report_failure.cpp)
endif ()

if (MSVC)
  set(MOCKCPP_SRCS 
    ${MOCKCPP_SRCS} 
    src/WinCodeModifier.cpp
  ) 
else (MSVC)
  set(MOCKCPP_SRCS 
    ${MOCKCPP_SRCS} 
    src/UnixCodeModifier.cpp
  )
endif (MSVC)
######################################################
set(MOCKCPP_HEADERS_PATH ${mockcpp_SOURCE_DIR}/include/mockcpp)

set(MOCKCPP_VTBL_RELATED_HEADERS 
    ${MOCKCPP_HEADERS_PATH}/DelegatedMethodGetDef.h 
    ${MOCKCPP_HEADERS_PATH}/DelegatedMethodGetByVptrDef.h 
    ${MOCKCPP_HEADERS_PATH}/DestructorAddrGetterDef.h 
    ${MOCKCPP_HEADERS_PATH}/MethodIndiceCheckerDef.h
    ${MOCKCPP_HEADERS_PATH}/DefaultMethodAddrGetterDef.h
)

set(MOCKCPP_ARG_RELATED_HEADER_FILES
    ${MOCKCPP_HEADERS_PATH}/DelegatedMethodDef.h 
    ${MOCKCPP_HEADERS_PATH}/ArgumentsListDef.h  
    ${MOCKCPP_HEADERS_PATH}/MethodTypeTraitsDef.h
)

if (MOCKCPP_ALLOW_MI)
  set(ALLOW_MI yes)
else (MOCKCPP_ALLOW_MI)
  set(ALLOW_MI no)
endif (MOCKCPP_ALLOW_MI)

######################################################
set(VTBL_GENERATOR ${mockcpp_SOURCE_DIR}/src/generate_vtbl_related_files.py)
set(EXTRA_VTBL_HEADERS_DEFS
    --allow-mi=${ALLOW_MI}
    --max-inheritance=${MOCKCPP_MAX_INHERITANCE}
    --max-vtbl-size=${MOCKCPP_MAX_VTBL_SIZE}
    --include-path=${MOCKCPP_HEADERS_PATH}
)
foreach(HEADER ${MOCKCPP_VTBL_RELATED_HEADERS})
    get_filename_component(HEADER_NAME ${HEADER} NAME)
    add_custom_command(
        OUTPUT ${HEADER}
        COMMAND ${PYTHON} ${VTBL_GENERATOR} ${EXTRA_VTBL_HEADERS_DEFS} ${HEADER_NAME}
    )
endforeach()

######################################################
set(ARG_GENERATOR ${mockcpp_SOURCE_DIR}/src/generate_arg_related_files.py)
set(EXTRA_ARG_HEADERS_DEFS
    --max-parameters=${MOCKCPP_MAX_PARAMETERS}
    --include-path=${MOCKCPP_HEADERS_PATH}
)
foreach(HEADER ${MOCKCPP_ARG_RELATED_HEADER_FILES})
    get_filename_component(HEADER_NAME ${HEADER} NAME)
    add_custom_command(
        OUTPUT ${HEADER}
        COMMAND ${PYTHON} ${ARG_GENERATOR} ${EXTRA_ARG_HEADERS_DEFS} ${HEADER_NAME}
    )
endforeach()

######################################################
link_directories(${mockcpp_BINARY_DIR}/src ${mockcpp_BINARY_DIR}/tests/3rdparty/testngpp/src)

add_library(mockcpp STATIC ${MOCKCPP_SRCS})

include_directories(BEFORE ${mockcpp_SOURCE_DIR} ${mockcpp_SOURCE_DIR}/include ${BOOST_INCLUDE_DIRS})

add_custom_target(vtbl_related_headers DEPENDS ${MOCKCPP_VTBL_RELATED_HEADERS})
add_custom_target(arg_related_headers DEPENDS ${MOCKCPP_ARG_RELATED_HEADER_FILES})

add_dependencies(mockcpp vtbl_related_headers arg_related_headers)

install(TARGETS mockcpp
        ARCHIVE DESTINATION lib)

install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
        DESTINATION include)
# if (MSVC)
#   install(FILES ${mockcpp_BINARY_DIR}/src/Debug/mockcpp.pdb
#           DESTINATION lib)
#   message(${mockcpp_BINARY_DIR})
# endif (MSVC)
