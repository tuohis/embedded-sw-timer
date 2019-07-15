set(LIB_SOURCES
  sw_timer.c
)

set(HEADERS
  ${CONFIG_HEADER}
  sw_timer.h
)

add_library(timer STATIC ${LIB_SOURCES} ${HEADERS})

install(TARGETS timer
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/sw_timer.h DESTINATION include)
