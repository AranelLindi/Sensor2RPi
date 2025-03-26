cmake_minimum_required(VERSION 3.10)
project(WebServer C)

# C-Standard setzen
set(CMAKE_C_STANDARD 11)

# Executable erstellen
add_executable(webserver src/webserver.c)

# Bibliotheken linken (pthread + SQLite3)
target_link_libraries(webserver pthread sqlite3)

# Nach dem Build: Executable und index.html in ../bin kopieren
add_custom_command(TARGET webserver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/../bin
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:webserver> ${CMAKE_CURRENT_BINARY_DIR}/../bin/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/index.html ${CMAKE_CURRENT_BINARY_DIR}/../bin/
)
