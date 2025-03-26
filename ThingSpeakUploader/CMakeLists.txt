project(ThingSpeakUploader C)

# Setzt die minimale CMake-Version und aktiviere C11-Standard
cmake_minimum_required(VERSION 3.10)
set(CMAKE_C_STANDARD 11)

# Findet benötigte Bibliotheken (SQLite3 und cURL)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(CURL REQUIRED libcurl)

# Fügt das ausführbare Programm hinzu
add_executable(thingspeak_uploader src/thingspeak_uploader.c)

# Linkt die notwendigen Bibliotheken
target_link_libraries(thingspeak_uploader PRIVATE ${SQLITE3_LIBRARIES} ${CURL_LIBRARIES})
target_include_directories(thingspeak_uploader PRIVATE ${SQLITE3_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})

# Installiert das Binary im übergeordneten Verzeichnis (z. B. `Sensor2RPi/bin/`)
install(TARGETS thingspeak_uploader DESTINATION bin)

# Installiert die Konfigurationsdatei (config.ini) im gleichen Verzeichnis wie das Binary
install(FILES config.ini DESTINATION bin)

# Automatischer Kopiervorgang nach Build
add_custom_command(
	TARGET thingspeak_uploader POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	${CMAKE_SOURCE_DIR}/src/config.ini
	${CMAKE_BINARY_DIR}/../bin/config.ini
)
