#!/bin/bash

echo "Starte alle Dienste..."

# Vor Mosquitto gestartet wird müssen noch Einstellungen geändert werden, sodass von allen Geräten im Netzwerk gelauscht wird
# Sicherstellen, dass das Mosquitto Verzeichnis existiert:
mkdir -p /mosquitto/config

echo "listener 1883 0.0.0.0" > /mosquitto/config/mosquitto.conf
echo "allow_anonymous true" >> /mosquitto/config/mosquitto.conf

# MQTT Broker starten
echo "Starte Mosquitto mit geänderter Konfiguration..."
exec /usr/sbin/mosquitto -c /mosquitto/config/mosquitto.conf &

# 1 Sekunde warten bis Mosquitto läuft
sleep 1

# Alle meine Anwendungen ausführen
echo "Starte MQTT Client..."
/app/mqtt_client &

echo "Starte ThingSpeak Uploader..."
/app/thingspeak_uploader &

echo "Starte Webserver..."
/app/webserver &

# Falls ein Prozess abstürzt, stoppt der Container
wait
