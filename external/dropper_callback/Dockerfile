FROM python:3.6

WORKDIR /opt/callback/

COPY loader.sh /opt/callback/loader.sh
COPY c2.sh /opt/callback/c2.sh

CMD ["python","-m","http.server","8080"]
