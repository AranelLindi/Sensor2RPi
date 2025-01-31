#!/bin/bash

echo "Starte alle Dienste..."

# MQTT Broker starten
mosquitto -d

# Alle Anwendungen ausführen
/app/mqtt_client &
/app/thingspeak_uploader &
/app/webserver &

# Falls ein Prozess abstürzt, stoppt der Container
wait
