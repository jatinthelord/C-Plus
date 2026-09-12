cmake_minimum_required(VERSION 3.20)
project(csp VERSION 1.0.0 LANGUAGES CXX)
find_program(GO_EXECUTABLE NAMES go)
if(GO_EXECUTABLE)
  if(WIN32)
    set(CSX_BINARY ${CMAKE_CURRENT_BINARY_DIR}/csx.exe)
  else()
    set(CSX_BINARY ${CMAKE_CURRENT_BINARY_DIR}/csx)
  endif()
  add_custom_command(OUTPUT ${CSX_BINARY}
    COMMAND ${GO_EXECUTABLE} build -buildvcs=false -trimpath -ldflags=-s -o ${CSX_BINARY} ./cmd/csx
    DEPENDS cmd/csx/main.go go.mod
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building compiled CSX package manager")
  add_custom_target(csx ALL DEPENDS ${CSX_BINARY})
  if(WIN32)
    set(CSP_CODEGEN_BINARY ${CMAKE_CURRENT_BINARY_DIR}/csp-codegen.exe)
  else()
    set(CSP_CODEGEN_BINARY ${CMAKE_CURRENT_BINARY_DIR}/csp-codegen)
  endif()
  add_custom_command(OUTPUT ${CSP_CODEGEN_BINARY}
    COMMAND ${GO_EXECUTABLE} build -buildvcs=false -trimpath -ldflags=-s -o ${CSP_CODEGEN_BINARY} ./cmd/csp-codegen
    DEPENDS cmd/csp-codegen/main.go go.mod
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building CSP regression runner")
  add_custom_target(csp-codegen ALL DEPENDS ${CSP_CODEGEN_BINARY})
  if(WIN32)
    set(CSPWEB_BINARY ${CMAKE_CURRENT_BINARY_DIR}/cspweb.exe)
  else()
    set(CSPWEB_BINARY ${CMAKE_CURRENT_BINARY_DIR}/cspweb)
  endif()
  add_custom_command(OUTPUT ${CSPWEB_BINARY}
    COMMAND ${GO_EXECUTABLE} build -buildvcs=false -trimpath -ldflags=-s -o ${CSPWEB_BINARY} ./cmd/cspweb
    DEPENDS cmd/cspweb/main.go go.mod
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building CSP Foundation web server")
  add_custom_target(cspweb ALL DEPENDS ${CSPWEB_BINARY})
  if(WIN32)
    set(CSP_BENCH_BINARY ${CMAKE_CURRENT_BINARY_DIR}/csp-bench.exe)
  else()
    set(CSP_BENCH_BINARY ${CMAKE_CURRENT_BINARY_DIR}/csp-bench)
  endif()
  add_custom_command(OUTPUT ${CSP_BENCH_BINARY}
    COMMAND ${GO_EXECUTABLE} build -buildvcs=false -trimpath -ldflags=-s -o ${CSP_BENCH_BINARY} ./cmd/csp-bench
    DEPENDS cmd/csp-bench/main.go go.mod
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building honest CSP benchmark runner")
  add_custom_target(csp-bench ALL DEPENDS ${CSP_BENCH_BINARY})
else()
  message(WARNING "Go was not found: the csx package-manager target is unavailable")
endif()
add_executable(cspc cspc.cpp src/cpasm.cpp src/ir.cpp src/thir.cpp src/tooling.cpp src/lexer.cpp src/lowering.cpp src/parser.cpp src/semantic.cpp src/transpiler.cpp
  include/cp/networking.h)
target_compile_features(cspc PRIVATE cxx_std_20)
target_include_directories(cspc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(CTest)
if(BUILD_TESTING)
  add_executable(csp_frontend_tests tests/frontend_tests.cpp src/cpasm.cpp src/ir.cpp src/thir.cpp src/tooling.cpp src/lexer.cpp src/lowering.cpp src/parser.cpp src/semantic.cpp src/transpiler.cpp)
  target_compile_features(csp_frontend_tests PRIVATE cxx_std_20)
  target_include_directories(csp_frontend_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  add_test(NAME csp.frontend COMMAND csp_frontend_tests)
  add_executable(csp_header_smoke tests/header_smoke.cpp)
  target_compile_features(csp_header_smoke PRIVATE cxx_std_20)
  target_include_directories(csp_header_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  add_test(NAME csp.headers COMMAND csp_header_smoke)
  if(GO_EXECUTABLE)
    add_test(NAME csp.codegen.manifest COMMAND ${CSP_CODEGEN_BINARY} --validate-only)
  endif()
  add_test(NAME csp.semantic.valid-overloads
    COMMAND cspc ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic/valid_overloads.csp --check)
  add_test(NAME csp.semantic.valid-bugemoan
    COMMAND cspc ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic/valid_bugemoan.csp --check)
  add_test(NAME csp.semantic.valid-hopa
    COMMAND cspc ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic/valid_hopa.csp --check)
  add_test(NAME csp.semantic.valid-match
    COMMAND cspc ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic/valid_match.csp --check)
  add_test(NAME csp.semantic.reject-non-exhaustive-match
    COMMAND cspc ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic/invalid_non_exhaustive_match.csp --check)
  set_tests_properties(csp.semantic.reject-non-exhaustive-match PROPERTIES WILL_FAIL TRUE)
  add_test(NAME csp.semantic.valid-ctfe
    COMMAND cspc ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic/valid_ctfe.csp --check)
  add_test(NAME csp.semantic.reject-ctfe
    COMMAND cspc ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic/invalid_ctfe.csp --check)
  set_tests_properties(csp.semantic.reject-ctfe PROPERTIES WILL_FAIL TRUE)
  add_test(NAME csp.features.zero-cost
    COMMAND cspc ${CMAKE_CURRENT_SOURCE_DIR}/tests/features/zero_cost.csp --check)
  add_test(NAME csp.features.safe-defaults
    COMMAND cspc ${CMAKE_CURRENT_SOURCE_DIR}/tests/features/safe_defaults.csp --check)
  add_test(NAME csp.features.low-level
    COMMAND cspc ${CMAKE_CURRENT_SOURCE_DIR}/tests/features/low_level.csp --check)
  add_test(NAME csp.semantic.reject-optional-dereference
    COMMAND cspc ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic/invalid_optional_dereference.csp --check)
  set_tests_properties(csp.semantic.reject-optional-dereference PROPERTIES WILL_FAIL TRUE)
  add_test(NAME csp.semantic.reject-immutable-assignment
    COMMAND cspc ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic/invalid_immutable_assignment.csp --check)
  set_tests_properties(csp.semantic.reject-immutable-assignment PROPERTIES WILL_FAIL TRUE)
  add_test(NAME csp.semantic.reject-cross-function-goto
    COMMAND cspc ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic/invalid_cross_function_goto.csp --check)
  set_tests_properties(csp.semantic.reject-cross-function-goto PROPERTIES WILL_FAIL TRUE)
  add_test(NAME csp.semantic.reject-duplicate-default
    COMMAND cspc ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic/invalid_duplicate_default.csp --check)
  set_tests_properties(csp.semantic.reject-duplicate-default PROPERTIES WILL_FAIL TRUE)
  add_test(NAME csp.semantic.reject-duplicate-case
    COMMAND cspc ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic/invalid_duplicate_case.csp --check)
  set_tests_properties(csp.semantic.reject-duplicate-case PROPERTIES WILL_FAIL TRUE)
endif()

include(GNUInstallDirs)
install(TARGETS cspc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if(GO_EXECUTABLE)
  install(PROGRAMS ${CSX_BINARY} DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(PROGRAMS ${CSP_CODEGEN_BINARY} ${CSPWEB_BINARY} ${CSP_BENCH_BINARY} DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(DIRECTORY packages/csx/ DESTINATION ${CMAKE_INSTALL_DATADIR}/csx)
install(FILES README.md syntax.md LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})

set(CPACK_PACKAGE_NAME "csp")
set(CPACK_PACKAGE_VENDOR "CSP Foundation")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Configuration Senior Programming compiler and libraries")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
include(CPack)
if(MSVC)
  target_compile_options(cspc PRIVATE /W4)
else()
  target_compile_options(cspc PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(MINGW)
  target_link_options(cspc PRIVATE -static-libgcc -static-libstdc++)
  if(BUILD_TESTING)
    target_link_options(csp_frontend_tests PRIVATE -static-libgcc -static-libstdc++)
  endif()
endif()
