# Auflisten aller laufenden Docker Container:
docker ps

# Entfernen eines Containers:
# 1. Alten Container stoppen (falls er gerade läuft):
docker stop [name]
# 2. Container löschen:
docker rm [name]
# 3. Docker Image entfernen (kann dann nicht mehr gestartet werden):
docker rmi [name]

# Falls ein Container nur gestoppt wurde kann er wieder gestartet werden:
docker start [name]

# Erstellen eines neuen Images (erfordert Dockerfile):
docker build -t [name] .

# Starten des Containers:
docker run -it -d -p 8080:8080 -p 1883:1883 --name [Container name] [name]
docker run -it -d --restart=always --name [Container name] [name] # Startet Container bei jedem Systemstart

# Zeigt Ausgaben der Anwendungen an:
docker logs [name]
docker logs -f [name] # Live Ausgabe!

# Öffnet eine Konsole direkt im Container:
docker exec -it [name] /bin/sh

# Alle installierten Images auflisten:
docker image ls
