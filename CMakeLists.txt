cmake_minimum_required(VERSION 3.10)
project(Sensor2RPi C)

# Alle Unterverzeichnisse hinzufügen
add_subdirectory(Webserver)
add_subdirectory(ThingSpeakUploader)
add_subdirectory(MQTTclient)
