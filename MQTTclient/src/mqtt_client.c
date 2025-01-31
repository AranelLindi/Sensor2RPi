#include <mosquitto.h> // MQTT Broker
#include <sqlite3.h> // SQLite Datenbankabfragen
#include <curl/curl.h> // Libcurl für HTTP-Requests (ThingSpeak)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DB_NAME "sensor_data.db"

// SQLite-Datenbank
sqlite3 *db;

// Callback-Funktion für empfangene MQTT-Nachrichten
void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
    printf("Interrupt: Neue Nachricht! Topic=%s, Payload=%s\n", message->topic, (char *)message->payload);

    // Topic zerlegen: sensors/device_id/sensor_type
    char device_id[50], sensor_type[50];
    if (sscanf(message->topic, "sensors/%49[^/]/%49s", device_id, sensor_type) != 2) {
        printf("Fehler: Konnte device_id und sensor_type nicht aus Topic extrahieren!\n");
        return;
    }

    // Payload (Messwert) in Zahl umwandeln
    double value = atof((char *)message->payload);

    char sql[256];
    snprintf(sql, sizeof(sql),
            "INSERT INTO measurements (timestamp, device_id, sensor_type, value) "
            "VALUES (CAST(strftime('%%s', 'now') AS INTEGER), '%s', '%s', %f);",
            device_id, sensor_type, value);
            // ACHTUNG! Reihenfolge der Spalten muss übereinstimmen mit der DB!
    printf("SQLite3: %s\n", sql);

    char *err_msg = NULL;
    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        printf("SQLite-Fehler: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        printf("Daten gespeichert: %s -> %s (%s = %f)\n",
        message->topic, (char *)message->payload, sensor_type, value);
    }
}

int main() {
    // SQLite Datenbank öffnen/erstellen
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        printf("Fehler beim Öffnen der SQLite-Datenbank!\n");
        return 1;
    }

    // Tabelle erstellen, falls nicht vorhanden:
    const char *create_table_sql = 
        "CREATE TABLE measurements ("
        "id INTEGER PRIMARY KEY,"
        "timestamp TEXT NOT NULL,"
        "device_id TEXT NOT NULL,"
        "sensor_type TEXT NOT NULL,"
        "value REAL NOT NULL,"
	"sent INTEGER DEFAULT 0"
        ");";
    sqlite3_exec(db, create_table_sql, 0, 0, NULL);


    // Mosquitto Bibliothek initialisieren
    mosquitto_lib_init(); 

    // MQTT-Client erstellen
    struct mosquitto *mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq) {
        printf("Fehler beim Erstellen des MQTT-Clients\n");
        return 1;
    }

    // Callback für empfangene Nachrichten setzen
    mosquitto_message_callback_set(mosq, on_message);

    // Verbindung zum MQTT-Broker herstellen
    if (mosquitto_connect(mosq, "localhost", 1883, 60) != MOSQ_ERR_SUCCESS) {
        printf("Fehler: Verbindung zum MQTT-Broker fehlgeschlagen!\n");
        return 1;
    }

    // Abonniere alle Sensor-Topics
    mosquitto_subscribe(mosq, NULL, "sensors/#", 1); // QoS == 1 (jedes Paket wird mindestens 1 Mal empfangen)

    printf("Warte auf MQTT-Interrupts...\n");

    // Endlosschleife zum Verarbeiten von Nachrichten (ohne Blockierung)
    while (1) {
        mosquitto_loop(mosq, -1, 1);
    }

    // Speicher freigeben (wird nie erreicht)
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    sqlite3_close(db); // Datenbank schließen

    return 0;
}

// Wichtige Kommandos:
// 1. Mosquitto muss vorher gestartet sein (ist im Moment aber so konfiguriert, dass es automatisch beim Betriebsstart ausgeführt wird):
//mosquitto -d // startet den Dienst im Hintergrund

// Broker stoppen:
//sudo systemctl stop mosquitto

// Programm kompilieren:
//gcc -o mqtt_logger main.c -lmosquitto -lsqlite3

// Testnachricht mit mosquitto_pub senden:
//mosquitto_pub -t "sensors/sensor123/temperature" -m "23.5"

// Datenbank öffnen:
//sqlite3 sensor_data.db

// Alle gespeicherten Messwerte anzeigen:
//SELECT * FROM measurements;

// Nur Temperaturwerte abfragen:
//SELECT * FROM measurements WHERE sensor_type = 'temperature';

// Nur Werte eines bestimmten Sensors abfragen:
//SELECT * FROM measurements WHERE device_id = 'sensor123';

// Nur Messwerte der letzten 10 Minuten abfragen:
//SELECT * FROM measurements WHERE timestamp > strftime('%s', 'now', '-10 minutes');

// SQLite beenden:
//.exit
