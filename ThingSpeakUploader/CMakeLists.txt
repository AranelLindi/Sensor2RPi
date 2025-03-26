cmake_minimum_required(VERSION 3.10)
project(ThingSpeakUploader C)

# C-Standard setzen
set(CMAKE_C_STANDARD 11)

# Bibliotheken suchen
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(CURL REQUIRED libcurl)

# Executable definieren
add_executable(thingspeak_uploader src/thingspeak_uploader.c)

# Bibliotheken einbinden
target_link_libraries(thingspeak_uploader PRIVATE ${SQLITE3_LIBRARIES} ${CURL_LIBRARIES})
target_include_directories(thingspeak_uploader PRIVATE ${SQLITE3_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})

# Nach dem Build: Executable und config.ini in ../bin kopieren
add_custom_command(TARGET thingspeak_uploader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/../bin
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:thingspeak_uploader> ${CMAKE_CURRENT_BINARY_DIR}/../bin/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/config.ini ${CMAKE_CURRENT_BINARY_DIR}/../bin/
)
