Docker-Image aktualisieren (falls Programmcode geändert wurde):

docker rmi mein-image-name # Altes Images entfernen (um Platz zu sparen)

docker build -t mein-image-name . # Neues Image mit Codeänderungen bauen

docker stop mein-container-name # 1. Alten Container stoppen & löschen
docker rm mein-container-name # 2. Container löschen
docker rmi mein-container-name # 3. Docker Image entfernen

docker start mein-container-name # Lässt einen gestoppten Container wieder laufen

docker run -d --name mein-container-name mein-image-name # Neuen Container mit aktualisiertem Image starten

docker run -d --restart=always --name mein-container-name mein-image-name # Startet den Container bei jedem Systemstart

docker logs mein-container-name # Zeigt Ausgaben der Anwendungen an

docker exec -it mein-container-name /bin/sh # Öffnet eine Konsole direkt im Container
