FROM spotify/kafka:latest

# We need python3
RUN apt-get update
RUN apt-get install -y python3 python3-pip

# install the certificate support tools
RUN mkdir -p /var/tools/
COPY tools/* /var/tools/
RUN chmod ugo+x /var/tools/*

RUN pip3 install -r /var/tools/get_certificates_requirements.txt

# place our configuration
COPY config/server.properties /opt/kafka_2.11-0.10.1.0/config

# prep for our certifictes
RUN mkdir -p /opt/kafka_2.11-0.10.1.0/certs

# Place the run command
COPY run.sh /opt/kafka_2.11-0.10.1.0/
RUN chmod ugo+x /opt/kafka_2.11-0.10.1.0/run.sh

EXPOSE 9455 9092 2181

CMD "/opt/kafka_2.11-0.10.1.0/run.sh"

