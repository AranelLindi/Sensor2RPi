# Basis Image mit Build Tools
FROM ubuntu:minimal

# Nötige Pakete installieren
RUN apt-get update && apt-get install -y \
	sqlite3 libsqlite3-0 libcurl4 mosquitto \
	&& rm -rf /var/lib/apt/lists/*


# Setzt das Arbeitsverzeichnis auf /app
WORKDIR /app

# Kopiert alle kompilierten Dateien in das Verzeichnis /app im Container
COPY webserver /app/webserver
COPY mqtt_client /app/mqtt_client
COPY thingspeak_uploader /app/thingspeak_uploader
COPY index.html /app/index.html
COPY config.ini /app/config.ini
COPY sensor_data.db /app/sensor_data.db

# Stellt sicher, dass die Binärdatei ausführbar ist
RUN chmod +x /app/webserver /app/mqtt_client /app/thingspeak_uploader

# Exponiert Ports für Webserver & MQTT
EXPOSE 8080 1883

# Startet alle Programme parallel mit supervisord
CMD ["/app/start_services.sh"]
