FROM wurstmeister/kafka:latest

RUN apk update

# Get Python3 for certificate tools support
RUN apk add python3 openssl

# install the certificate support tools
RUN mkdir -p /var/tools/
COPY tools/* /var/tools/
RUN chmod ugo+x /var/tools/*

RUN pip3 install -r /var/tools/get_certificates_requirements.txt

# place our configuration
COPY config/server.properties /opt/kafka/config

# prep for our certifictes
RUN mkdir -p /opt/kafka/certs

# Place the run command
COPY run.sh /opt/kafka/
RUN chmod ugo+x /opt/kafka/run.sh

EXPOSE 9455

CMD "/opt/kafka/run.sh"