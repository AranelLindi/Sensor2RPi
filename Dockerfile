# Basis Image mit Build Tools
FROM ubuntu:latest

# Nötige Pakete installieren
RUN apt-get update && apt-get install -y \
	sqlite3 libsqlite3-0 libcurl4 mosquitto \
	mosquitto mosquitto-clients \
	&& rm -rf /var/lib/apt/lists/*


# Setzt das Arbeitsverzeichnis auf /app
WORKDIR /app

# Kopiert alle kompilierten Dateien in das Verzeichnis /app im Container
COPY bin/webserver /app/webserver
COPY bin/mqtt_client /app/mqtt_client
COPY bin/thingspeak_uploader /app/thingspeak_uploader
COPY bin/index.html /app/index.html
COPY bin/config.ini /app/config.ini
COPY bin/sensor_data.db /app/sensor_data.db
COPY start_services.sh /app/start_services.sh

# Stellt sicher, dass die Binärdatei ausführbar ist
RUN chmod +x /app/webserver /app/mqtt_client /app/thingspeak_uploader /app/start_services.sh

# Exponiert Ports für Webserver & MQTT
EXPOSE 8080 1883

# Startet alle Programme parallel mit supervisord
CMD ["/app/start_services.sh"]
