cmake_minimum_required(VERSION 3.10)

# Projektname
project(WebServer C)

# Setze die C-Standardversion
set(CMAKE_C_STANDARD 11)

# Erstelle die ausführbare Datei
add_executable(webserver src/webserver.c)

# Benötigte Bibliotheken hinzufügen (pthread und SQLite3)
target_link_libraries(webserver pthread sqlite3)

# Installationsziel festlegen
install(TARGETS webserver DESTINATION bin)
install(FILES index.html DESTINATION bin)

# Automatischer Kopiervorgang nach Build (optional)
add_custom_command(
    TARGET webserver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/src/index.html
    ${CMAKE_BINARY_DIR}/../bin/index.html
)
