cmake_minimum_required(VERSION 3.10)
project(MQTTclient C)

# C-Standard setzen
set(CMAKE_C_STANDARD 11)

# Bibliotheken suchen
find_package(PkgConfig REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)
pkg_check_modules(SQLITE REQUIRED sqlite3)

# Executable definieren
add_executable(mqtt_client src/mqtt_client.c)

# Bibliotheken einbinden
target_link_libraries(mqtt_client PRIVATE ${MOSQUITTO_LIBRARIES} ${SQLITE_LIBRARIES})
target_include_directories(mqtt_client PRIVATE ${MOSQUITTO_INCLUDE_DIRS} ${SQLITE_INCLUDE_DIRS})

# Nach dem Build: Executable in ../bin kopieren
add_custom_command(TARGET mqtt_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/../bin
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:mqtt_client> ${CMAKE_CURRENT_BINARY_DIR}/../bin/
)
