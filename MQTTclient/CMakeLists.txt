# Setzt die minimale CMake-Version und aktiviere C11-Standard
cmake_minimum_required(VERSION 3.10)
project(MQTTclient C)

set(CMAKE_C_STANDARD 11)

# Findet benötigte Bibliotheken (Mosquitto und SQLite3)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)
pkg_check_modules(SQLITE REQUIRED sqlite3)

# Fügt das ausführbare Programm hinzu
add_executable(mqtt_client src/mqtt_client.c)

# Linkt die notwendigen Bibliotheken
target_link_libraries(mqtt_client PRIVATE ${MOSQUITTO_LIBRARIES} ${SQLITE3_LIBRARIES})
target_include_directories(mqtt_client PRIVATE ${MOSQUITTO_INCLUDE_DIRS} ${SQLITE3_INCLUDE_DIRS})

# Installiert das Binary im übergeordneten Verzeichnis (z. B. `Sensor2RPi/bin/`)
install(TARGETS mqtt_client DESTINATION ../bin)
