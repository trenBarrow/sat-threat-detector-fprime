cmake_minimum_required(VERSION 3.13)

# Optional: convert XML -> FPP at configure time using fpp-from-xml, else use checked-in FPP
option(DETECTOR_USE_XML "Generate FPP from XML at configure time" OFF)
set(GEN_FPP ${CMAKE_CURRENT_LIST_DIR}/Detector.fpp)

if (DETECTOR_USE_XML)
  find_program(FPP_FROM_XML fpp-from-xml)
  if (FPP_FROM_XML)
    set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
    file(MAKE_DIRECTORY ${GENERATED_DIR})
    set(XML_SRC ${CMAKE_CURRENT_LIST_DIR}/DetectorComponentAi.xml)
    set(GEN_XML_FPP ${GENERATED_DIR}/Detector.fromxml.fpp)
    add_custom_command(
      OUTPUT ${GEN_XML_FPP}
      COMMAND ${FPP_FROM_XML} ${XML_SRC} > ${GEN_XML_FPP}
      DEPENDS ${XML_SRC}
      VERBATIM
      COMMENT "Converting Detector XML to FPP via fpp-from-xml"
    )
    add_custom_target(detector_xml2fpp ALL DEPENDS ${GEN_XML_FPP})
    set(GEN_FPP ${GEN_XML_FPP})
  else()
    message(WARNING "DETECTOR_USE_XML=ON but fpp-from-xml not found; falling back to checked-in Detector.fpp")
  endif()
endif()

set(DETECTOR_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/DetectorComponentAi.cpp
    ${CMAKE_CURRENT_LIST_DIR}/DetectorComponentImpl.cpp
)
set(DETECTOR_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/DetectorComponentAi.hpp
    ${CMAKE_CURRENT_LIST_DIR}/DetectorComponentImpl.hpp
    ${CMAKE_CURRENT_LIST_DIR}/Detector.hpp
)

register_fprime_library(
    Detector
    AUTOCODER_INPUTS
        ${GEN_FPP}
    SOURCES
        ${DETECTOR_SOURCES}
    HEADERS
        ${DETECTOR_HEADERS}
)
