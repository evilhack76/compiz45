
INCLUDE_DIRECTORIES(  
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}../logmessage/include
  
  ${compiz_SOURCE_DIR}/include
    
  ${Boost_INCLUDE_DIRS}
)

SET( 
  PUBLIC_HEADERS 
  ${CMAKE_CURRENT_SOURCE_DIR}/include/core/pluginclasses.h 
  ${CMAKE_CURRENT_SOURCE_DIR}/include/core/pluginclasshandler.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/core/valueholder.h
)

SET( 
  PRIVATE_HEADERS 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/valueholder.h
)

SET( 
  SRCS 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/valueholder.cpp 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pluginclasses.cpp
)

ADD_LIBRARY( 
  compiz_pluginclasshandler STATIC
  
  ${SRCS}
  
  ${PUBLIC_HEADERS}
  ${PRIVATE_HEADERS}
)

ADD_SUBDIRECTORY( ${CMAKE_CURRENT_SOURCE_DIR}/tests )

SET_TARGET_PROPERTIES(
  compiz_pluginclasshandler PROPERTIES
  PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

INSTALL(
  TARGETS                                 compiz_pluginclasshandler
  RUNTIME             DESTINATION         bin
  LIBRARY             DESTINATION         lib
  ARCHIVE             DESTINATION         lib
  PUBLIC_HEADER       DESTINATION         include/compiz/core
)