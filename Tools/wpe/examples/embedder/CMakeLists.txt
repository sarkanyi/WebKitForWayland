set(WPE_EMBEDDER_DIR "${TOOLS_DIR}/wpe/examples/embedder")

set(WPE_EMBEDDER_SOURCES
    ${WPE_EMBEDDER_DIR}/main.cpp
)

set(WPE_EMBEDDER_INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE/include
    ${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE-mesa/include
    ${FORWARDING_HEADERS_DIR}
    ${WTF_DIR}
)

set(WPE_EMBEDDER_SYSTEM_INCLUDE_DIRECTORIES
    ${GLIB_INCLUDE_DIRS}
)

set(WPE_EMBEDDER_LIBRARIES
    WebKit2
    WPE-mesa
    wayland-egl
    ${GLIB_LIBRARIES}
)

include_directories(SYSTEM ${WPE_EMBEDDER_SYSTEM_INCLUDE_DIRECTORIES})
add_executable(WPEEmbedder ${WPE_EMBEDDER_SOURCES})
target_include_directories(WPEEmbedder PUBLIC ${WPE_EMBEDDER_INCLUDE_DIRECTORIES})
target_link_libraries(WPEEmbedder ${WPE_EMBEDDER_LIBRARIES})
install(TARGETS WPEEmbedder DESTINATION "${EXEC_INSTALL_DIR}")
