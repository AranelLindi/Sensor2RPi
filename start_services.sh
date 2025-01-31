#!/bin/bash

echo "Starte alle Dienste..."
/app/mqtt_client &
/app/thingspeak_uploader &
/app/simple_webserver &

# Falls ein Prozess abstürzt, stoppt der Container
wait
