//
// Created by st-li on 27.01.2025.
//

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "pthread.h"
#include "arpa/inet.h"
#include "sqlite3.h"

#define MAX_MAPPINGS 10
#define CONFIG_FILE "config.ini"

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 5
pthread_mutex_t connection_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char key[100];   // z. B. sensor1:temperature
    char room[32];   // z. B. room1
} RoomMapping;

RoomMapping room_mappings[MAX_MAPPINGS];
int room_mapping_count = 0;

// Globale Variablen
int active_connections = 0;

// Liest die config.ini und entnimmt ihr die Raumzuordnung für jeden Sensor
void load_room_mappings() {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (!file) {
        fprintf(stderr, "WARNUNG: config.ini konnte nicht geladen werden.\n");
        return;
    }

    char line[128], key[50], room[32], field[32];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '[') continue;

        if (sscanf(line, "%49s = %31[^,], %31s", key, room, field) == 3) {
            if (room_mapping_count < MAX_MAPPINGS) {
                strncpy(room_mappings[room_mapping_count].key, key, sizeof(room_mappings[0].key));
                strncpy(room_mappings[room_mapping_count].room, room, sizeof(room_mappings[0].room));
                room_mapping_count++;
            }
        }
    }

    fclose(file);
}

void get_room_for_sensor(const char *device_id, const char *sensor_type, char *room_out, size_t room_size) {
    char key[100];
    snprintf(key, sizeof(key), "%s:%s", device_id, sensor_type);

    for (int i = 0; i < room_mapping_count; i++) {
        if (strcmp(key, room_mappings[i].key) == 0) {
            strncpy(room_out, room_mappings[i].room, room_size);
            return;
        }
    }

    snprintf(room_out, room_size, "unknown");
}


// Funktion: Sensordaten als JSON aus SQLite abrufen
void get_sensor_data(char *response, size_t response_size) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    const char *query =
        "SELECT device_id, sensor_type, value, strftime('%s', timestamp) AS timestamp "
        "FROM measurements "
        "WHERE timestamp = ("
        "  SELECT MAX(timestamp) "
        "  FROM measurements AS sub "
        "  WHERE sub.device_id = measurements.device_id "
        "  AND sub.sensor_type = measurements.sensor_type"
        ")";
    
    if (sqlite3_open("sensor_data.db", &db) != SQLITE_OK) {
        snprintf(response, response_size, "{\"error\": \"Datenbank konnte nicht geöffnet werden\"}");
        return;
    }

    strncat(response, "[", response_size - strlen(response) - 1);

    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK) {
        int first = 1;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (!first) strncat(response, ",", response_size - strlen(response) - 1);
            first = 0;

            const char *device_id = (const char *)sqlite3_column_text(stmt, 0);
            const char *sensor_type = (const char *)sqlite3_column_text(stmt, 1);
            double value = sqlite3_column_double(stmt, 2);
            sqlite3_int64 timestamp = sqlite3_column_int64(stmt, 3);
            
            char room[32];
            get_room_for_sensor(device_id, sensor_type, room, sizeof(room));
            
            char entry[256];
            snprintf(entry, sizeof(entry),
                "{\"device_id\": \"%s\", \"sensor_type\": \"%s\", \"room\": \"%s\", \"value\": %.2f, \"timestamp\": %lld}",
                device_id, sensor_type, room, value, timestamp);
            

            strncat(response, entry, response_size - strlen(response) - 1);
        }
        sqlite3_finalize(stmt);
    }
    
    strncat(response, "]", response_size - strlen(response) - 1);
    sqlite3_close(db);
}

// Funktion zur Verarbeitung eines Clients
void *handle_client(void* socket) {
    int client_socket = *(int *)socket;
    free(socket); // Speicher für den Socket freigeben

    char buffer[BUFFER_SIZE] = {0};
    read(client_socket, buffer, BUFFER_SIZE); // HTTP-Anfrage lesen
    printf("Request:\n%s\n", buffer);

    // Einfacher parser für die angeforderte Datei (z.B. "GET /index.html")
    char method[16], path[256];
    sscanf(buffer, "%s %s", method, path);

    // Nur GET-Anfragen akzeptieren
    if (strcmp(method, "GET") != 0) {
        const char *error_response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        write(client_socket, error_response, strlen(error_response));
        close(client_socket);
        goto cleanup;
    }

    if (strcmp(path, "/sensors") == 0) {
        char json_response[BUFFER_SIZE * 4] = {0};
        get_sensor_data(json_response, sizeof(json_response));

        char header[BUFFER_SIZE];
        snprintf(header, sizeof(header),
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: %ld\r\n"
                    "Connection: close\r\n\r\n",
                    strlen(json_response));

        write(client_socket, header, strlen(header));
        write(client_socket, json_response, strlen(json_response));
        close(client_socket);
        goto cleanup;
    }

    // Pfad anpassen (entferne führendes "/")
    if (path[0] == '/') {
        memmove(path, path + 1, strlen(path));
    }
    if (strlen(path) == 0) { // Leerer Pfad führt zu index.html
        strcpy(path, "index.html");
    }

    // Datei öffnen und lesen
    FILE *file = fopen(path, "r");
    if (!file) {
        const char *error_response = "HTTP/1.1 404 Not Found\r\n\r\n";
        write(client_socket, error_response, strlen(error_response));
        close(client_socket);
        goto cleanup;
    }

    // Dateiinhalt in den Speicher laden
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *file_content = malloc(file_size + 1);
    if (!file_content) {
        perror("Memory allocation for file failed");
        goto cleanup; //error_close;
    }
    fread(file_content, 1, file_size, file);
    fclose(file);

    // HTTP-Antwort generieren
    char header[BUFFER_SIZE];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n\r\n",
             file_size);

    // Antwort senden
    write(client_socket, header, strlen(header));
    write(client_socket, file_content, file_size);

    // Ressourcen freigeben
    free(file_content);

cleanup: //error_close:
    close(client_socket);

    // Verbindung freigeben
    pthread_mutex_lock(&connection_mutex);
    active_connections--;
    pthread_mutex_unlock(&connection_mutex);

    return NULL;
}

int main() {
    printf("Webserver is starting...\n");

    load_room_mappings();

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 1. Socket erstellen
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. Adresse konfigurieren
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 3. Socket binden
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 4. Auf Verbindungen warten
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is running on http://localhost:%d\n", PORT);

    // 5. Eingehende Verbindungen akzeptieren
    while ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) >= 0) {
        pthread_mutex_lock(&connection_mutex);
        if (active_connections >= MAX_CONNECTIONS) {
            pthread_mutex_unlock(&connection_mutex);

            const char *error_response = "HTTP/1.1 503 Service Unavailable\r\n"
                                         "Content-Type: text/html\r\n\r\n"
                                         "<html><body><h1>503 Service Unavailable</h1><p>Too many connections. Try again later.</p></body></html>";
            write(new_socket, error_response, strlen(error_response));
            close(new_socket);
            continue;
        }
        active_connections++;
        pthread_mutex_unlock(&connection_mutex);

        pthread_t thread_id;
        int *client_socket = malloc(sizeof(int));
        if(!client_socket) {
            perror("Memory allocation failed");
            close(new_socket);
            continue;
        }
        *client_socket = new_socket;
        if (pthread_create(&thread_id, NULL, handle_client, client_socket) != 0) {
            perror("Thread creation failed");
            close(new_socket);
            free(client_socket);
            pthread_mutex_lock(&connection_mutex);
            active_connections--;
            pthread_mutex_unlock(&connection_mutex);
        }
        pthread_detach(thread_id);
    }

    // 7. Cleanup
    close(server_fd);
    return 0;
}

// Kompilieren mit
// gcc -o simple_webserver webserver.c -pthread -lsqlite3

// Ausführen (als Root, wenn Port 80 verwendet wird, bei 8080 geht es auch ohne sudo!)
// sudo ./simple_webserver
// ALTERNATIVE: ./simple_webserver > server.log 2>&1 &
// Leitet die Standardausgabe in server.log; leitet auch die Fehlerausgabe in die gleiche Datei

// Zugreifen:
// Öffne den Brwoser und rufe http://[Ip Adresse]:8080 auf (Die Ip Adresse findest du mit hostname -I)
