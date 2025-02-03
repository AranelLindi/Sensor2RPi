#!/bin/bash

echo "Starte alle Dienste..."

# Vor Mosquitto gestartet wird müssen noch Einstellungen geändert werden, sodass von allen Geräten im Netzwerk gelauscht wird
echo "listener 1883 0.0.0.0" > /mosquitto/config/mosquitto.conf
echo "allow_anonymous true" >> /mosquitto/config/mosquitto.conf

echo "Starte Mosquitto mit geänderter Konfiguration..."
exec /usr/sbin/mosquitto -c /mosquitto/config/mosquitto.conf


# MQTT Broker starten
mosquitto -d

# Alle Anwendungen ausführen
/app/mqtt_client &
/app/thingspeak_uploader &
/app/webserver &

# Falls ein Prozess abstürzt, stoppt der Container
wait
