#include <sqlite3.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>

// Konfigurationsdatei
#define CONFIG_FILE "config.ini"

// Datenbank
#define DATABASE "sensor_data.db"

// ThingSpeak API
#define THINGSPEAK_API_KEY "W05VA6SAZ06IM1EA"
#define THINGSPEAK_URL "https://api.thingspeak.com/update"

#define MAX_MAPPINGS 10

typedef struct {
    char key[100];     // z. B. sensor1:temperature
    char field[16];    // z. B. field1
} SensorMapping;

SensorMapping mappings[MAX_MAPPINGS];
int mapping_count = 0;


void load_all_sensor_mappings() {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (!file) {
        fprintf(stderr, "Fehler: Konnte %s nicht öffnen.\n", CONFIG_FILE);
        return;
    }

    char line[128], key[50], value1[50], value2[50];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '[') continue;
        if (sscanf(line, "%49s = %49[^,], %49s", key, value1, value2) == 3) {
            if (mapping_count < MAX_MAPPINGS) {
                snprintf(mappings[mapping_count].key, sizeof(mappings[mapping_count].key), "%s", key);
                snprintf(mappings[mapping_count].field, sizeof(mappings[mapping_count].field), "%s", value2);
                mapping_count++;         
            }
            else if (mapping_count >= MAX_MAPPINGS) {
                fprintf(stderr, "WARNUNG: MAX_MAPPINGS (%d) erreicht – weitere Einträge werden ignoriert.\n", MAX_MAPPINGS);
                break;
            }       
        }
    }
    
    fclose(file);
}

void get_field_for_sensor(const char *sensor_id, const char *sensor_type, char *field, size_t size) {
    char full_key[100];
    snprintf(full_key, sizeof(full_key), "%s:%s", sensor_id, sensor_type);

    for (int i = 0; i < mapping_count; i++) {
        if (strcmp(full_key, mappings[i].key) == 0) {
            snprintf(field, size, "%s", mappings[i].field);
            return;
        }
    }

    printf("WARNUNG: Kein Mapping gefunden für %s → sende an fieldX\n", full_key);
    snprintf(field, size, "fieldX");
}




// Funktion zum Laden der Sensor-Zuordnung aus der Konfigurationsdatei
/*void load_sensor_mapping(const char *sensor_id, const char *sensor_type, char *field, size_t size) {
    FILE *file = fopen(CONFIG_FILE, "r"); // Config.ini wird jedes Mal neu eingelesen! Ermöglicht Live-Aktualisierung!
    if (!file) {
        fprintf(stderr, "Warnung: Konnte Konfigurationsdatei nicht öffnen (%s). Werte werden an fieldX gesendet (Fehler!).\n", CONFIG_FILE);
        snprintf(field, size, "fieldX");  // Standardfeld
        return;
    }

    char line[128], key[50], value[50];
    char full_sensor_id[100];

    // Erzeuge den neuen Schlüssel im Format "sensor1:temperature"
    snprintf(full_sensor_id, sizeof(full_sensor_id), "%s:%s", sensor_id, sensor_type);

    while (fgets(line, sizeof(line), file)) {
        // Leerzeilen, Kommentare und sonstiges überspringen
        if (line[0] == '#' || line[0] == '\n' || line[0] == '[') continue;

        printf("Prüfe Zeile: %s", line); // DEBUG!

        char value1[50], value2[50];

        // Wir müssen zwei Werte lesen (roomX, fieldX), aber nur fieldX verwenden
        if (sscanf(line, "%49s = %49[^,], %49s", key, value1, value2) == 3) {
            if (strcmp(full_sensor_id, key) == 0) {
                snprintf(field, size, "%s", value2);  // Nur `fieldX` übernehmen
                fclose(file);
                return;
            }
        }
    }

    // Falls keine Zuordnung in config.ini gefunden wird, einen Fehler ausgeben!
    printf("Warnung: Sensor %s:%s ist nicht in config.ini eingetragen!\n", sensor_id, sensor_type);
    // und einen Fehler field verwenden:
    snprintf(field, size, "fieldX"); // Datenanfrage an ThingSpeak schlägt dann fehl aber es werden wenigstens nicht andere Frames kompromitiert mit falschen Daten!

    fclose(file);
}*/



// Funktion zum Senden aller aktuellen Messwerte an ThingSpeak
void send_all_to_thingspeak(sqlite3 *db) {
    sqlite3_stmt *stmt;
    char url[512];
    snprintf(url, sizeof(url), "%s?api_key=%s", THINGSPEAK_URL, THINGSPEAK_API_KEY);

    // Suche den neuesten ungesendeten Messwert für jede Sensor-Type-Kombination
    const char *query = 
        "SELECT id, device_id, sensor_type, value FROM measurements "
        "WHERE sent = 0 "
        "AND id IN (SELECT MAX(id) FROM measurements WHERE sent = 0 GROUP BY device_id, sensor_type);";

    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK) {
        int found = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const char *sensor_id = (const char *)sqlite3_column_text(stmt, 1);
            const char *sensor_type = (const char *)sqlite3_column_text(stmt, 2);
            double value = sqlite3_column_double(stmt, 3);

            // Lade das zugehörige ThingSpeak-Feld
            char field[50];
            //load_sensor_mapping(sensor_id, sensor_type, field, sizeof(field));
            get_field_for_sensor(sensor_id, sensor_type, field, sizeof(field));

            // DEBUG:
            printf("→ %s:%s → %s = %.2f\n", sensor_id, sensor_type, field, value);

            // Füge Wert in die URL ein
            char temp[64];
            snprintf(temp, sizeof(temp), "&%s=%.2f", field, value);
            strncat(url, temp, sizeof(url) - strlen(url) - 1);

            found = 1;  // Mindestens ein Wert wurde verarbeitet
        }
        sqlite3_finalize(stmt);

        // Wenn mindestens ein Wert gefunden wurde, sende den API-Request
        if (found) {
            CURL *curl;
            CURLcode res;
            curl = curl_easy_init();
            if (curl) {

                printf("Finale URL an ThingSpeak: %s\n", url);

                printf("%lld ThingSpeak_Uploader update url=%s\n", (long long)time(NULL), url);

                curl_easy_setopt(curl, CURLOPT_URL, url);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                res = curl_easy_perform(curl);

                if (res == CURLE_OK) {
                    // Markiere Werte als gesendet
                    char update_sql[128];
                    snprintf(update_sql, sizeof(update_sql), "UPDATE measurements SET sent = 1 WHERE sent = 0;");
                    sqlite3_exec(db, update_sql, NULL, NULL, NULL);
                } else {
                    fprintf(stderr, "ThingSpeak Upload Fehler: %s\n", curl_easy_strerror(res));
                }
                

                if (res != CURLE_OK) {
                    fprintf(stderr, "ThingSpeak Upload Fehler: %s\n", curl_easy_strerror(res));
                }
                curl_easy_cleanup(curl);
            }
        } else {
            // !found
            printf("Keine neuen Messwerte zum Hochladen gefunden. Warte auf neue Daten...\n");
        }
    }
}


int main() {
    sqlite3 *db;

    while (1) {
	if (sqlite3_open(DATABASE, &db) != SQLITE_OK) {
	  printf("Fehler beim Öffnen der SQLite-Datenbank!\n");
	  return 1;
	}

	printf("Prüfe auf ungesendete Messwerte...\n");

    load_all_sensor_mappings();

    send_all_to_thingspeak(db);

	sqlite3_close(db);

        sleep(300); // Warte 5 Minuten
        //sleep(120); // Warte 2 Minuten
    }

    return 0;
}
