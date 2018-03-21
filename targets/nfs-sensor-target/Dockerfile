# Dockerfile for NFS sensor, which, for the time-being, runs as a
# target so it fits with the rest of the system. The NFS sensor
# container should be run on the same host as the NFS server.

FROM python:3.6
ENV PYTHONUNBUFFERED=0

# base system setup and requirements
RUN apt-get update
WORKDIR /usr/src/app

# REQUIERMENTS.TXT install and run
RUN mkdir /usr/src/app/requirements
COPY requirements/sensor_wrapper_requirements.txt /usr/src/app/requirements/
RUN pip install -r /usr/src/app/requirements/sensor_wrapper_requirements.txt

# COMMON SENSOR LIBRARIES
RUN mkdir /usr/src/app/libs
COPY common_libraries /usr/src/app/libs/common
WORKDIR /usr/src/app/libs/common
RUN chmod a+x install.sh
RUN ./install.sh

# SENSOR-SPECIFIC LIBRARY
COPY sensor/scapy /usr/src/app/libs/scapy
WORKDIR /usr/src/app/libs
RUN ls .
RUN find ./scapy/setup.py
RUN pip install ./scapy


# Run the apt-get install script for required OS support in sensors
RUN chmod a+x /usr/src/app/libs/common/apt_get_install.sh
RUN /bin/bash /usr/src/app/libs/common/apt_get_install.sh

# SENSORS
RUN mkdir /opt/sensor
COPY sensor/* /opt/sensor/

# RUN SCRIPTS
#RUN mkdir /opt/sensor_startup
#COPY sensor_startup/ /opt/sensor_startup/
#RUN chmod a+x /opt/sensor_startup/*

# Crypto
RUN  mkdir  /opt/sensor/nfs_sensor/certs
#COPY certs/ /opt/sensor/nfs_sensor/certs/

# Service components
COPY run.sh /usr/src/app

RUN echo "127.0.0.1         api" >> /etc/hosts

CMD ["/bin/bash", "/usr/src/app/run.sh"]