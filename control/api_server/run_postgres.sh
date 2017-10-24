#!/bin/sh
docker run -d --rm --name api_server_postgres -p 5432:5432 -v "$(pwd)/pgdata:/var/lib/postgresql/data/pgdata" -e "POSTGRES_PASSWORD=postgres" -e "POSTGRES_USER=postgres" -e "PGDATA=/var/lib/postgresql/data/pgdata" -e "POSTGRES_DB=api_server_dev" postgres:9.3