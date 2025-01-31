cmake_minimum_required(VERSION 3.10)
project(Sensor2RPi)

set(CMAKE_C_STANDARD 17)

# Unterverzeichnisse
add_subdirectory(MQTTclient)
add_subdirectory(ThingSpeakUploader)
add_subdirectory(Webserver)
