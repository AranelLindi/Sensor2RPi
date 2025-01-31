#include <sqlite3.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Konfigurationsdatei
#define CONFIG_FILE "config.ini"

// ThingSpeak API
#define THINGSPEAK_API_KEY "W05VA6SAZ06IM1EA"
#define THINGSPEAK_URL "https://api.thingspeak.com/update"

// Funktion zum Laden der Sensor-Zuordnung aus der Konfigurationsdatei
void load_sensor_mapping(const char *sensor_id, char *room, char *field, size_t size) {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (!file) {
        fprintf(stderr, "Warnung: Konnte Konfigurationsdatei nicht öffnen (%s). Standardwerte werden verwendet.\n", CONFIG_FILE);
        snprintf(room, size, "%s", sensor_id);
        snprintf(field, size, "field1");  // Standardfeld, falls nichts gefunden wird
        return;
    }

    char line[128], key[50], value1[50], value2[50];
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%49s = %49[^,], %49s", key, value1, value2) == 3) {
            if (strcmp(sensor_id, key) == 0) {
                snprintf(room, size, "%s", value1);
                snprintf(field, size, "%s", value2);
                fclose(file);
                return;
            }
        }
    }

    // Falls kein Mapping gefunden wird, nutze Sensor-ID als Fallback
    snprintf(room, size, "%s", sensor_id);
    snprintf(field, size, "field1");
    fclose(file);
}

// Funktion zum Senden aller verfügbaren Messwerte an ThingSpeak
void send_all_to_thingspeak(sqlite3 *db) {
    sqlite3_stmt *stmt;
    char url[512];
    snprintf(url, sizeof(url), "%s?api_key=%s", THINGSPEAK_URL, THINGSPEAK_API_KEY);

    // Suche alle ungesendeten Messwerte
    const char *query = "SELECT id, device_id, value FROM measurements WHERE sent = 0 ORDER BY id ASC;";
    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK) {
        int found = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const char *sensor_id = (const char *)sqlite3_column_text(stmt, 1);
            double value = sqlite3_column_double(stmt, 2);

            // Lade das zugehörige ThingSpeak-Feld
            char room[50], field[50];
            load_sensor_mapping(sensor_id, room, field, sizeof(room));

            // Füge Wert in die URL ein
            char temp[64];
            snprintf(temp, sizeof(temp), "&%s=%.2f", field, value);
            strncat(url, temp, sizeof(url) - strlen(url) - 1);

            // Markiere den Eintrag als gesendet
            char update_sql[128];
            snprintf(update_sql, sizeof(update_sql), "UPDATE measurements SET sent = 1 WHERE id = %d;", id);
            sqlite3_exec(db, update_sql, NULL, NULL, NULL);

            found = 1;  // Mindestens ein Wert wurde verarbeitet
        }
        sqlite3_finalize(stmt);

        //printf("URL: %s\n", url);

        // Wenn mindestens ein Wert gefunden wurde, sende den API-Request
        if (found) {
            CURL *curl;
            CURLcode res;
            curl = curl_easy_init();
            if (curl) {
                curl_easy_setopt(curl, CURLOPT_URL, url);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                res = curl_easy_perform(curl);
                if (res != CURLE_OK) {
                    fprintf(stderr, "ThingSpeak Upload Fehler: %s\n", curl_easy_strerror(res));
                }
                curl_easy_cleanup(curl);
            }
        }
    }
}

int main() {
    sqlite3 *db;

    if (sqlite3_open("sensor_data.db", &db) != SQLITE_OK) {
        printf("Fehler beim Öffnen der SQLite-Datenbank!\n");
        return 1;
    }

    while (1) {
        printf("Prüfe auf ungesendete Messwerte...\n");
        send_all_to_thingspeak(db);
        sleep(300); // ⏳ Warte 5 Minuten
        //sleep(60);
    }

    sqlite3_close(db);
    return 0;
}
