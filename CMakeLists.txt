cmake_minimum_required(VERSION 3.10)
project(Sensor2RPi)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_C_STANDARD 17)

# Unterverzeichnisse einbinden
add_subdirectory(MQTTclient)
add_subdirectory(ThingSpeakUploader)
add_subdirectory(Webserver)

# Kopiere index.html, config.ini und sensor_data.db in "bin/"
add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/Webserver/src/index.html ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/index.html
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/ThingSpeakUploader/src/config.ini ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config.ini
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/sensor_data.db ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sensor_data.db
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/sensor_data.db ${CMAKE_SOURCE_DIR}/bin/sensor_data.db
)
