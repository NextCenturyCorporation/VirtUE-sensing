FROM python:3.6
ENV PYTHONUNBUFFERED=0

# base system setup and requirements
RUN apt-get update
WORKDIR /usr/src/app

# REQUIERMENTS.TXT install and run
RUN mkdir /usr/src/app/requirements
COPY requirements/ /usr/src/app/requirements/
RUN pip install -r /usr/src/app/requirements/requirements_master.txt

# SENSOR LIBRARIES
RUN mkdir /usr/src/app/sensor_libraries
COPY sensor_libraries/ /usr/src/app/sensor_libraries/
WORKDIR /usr/src/app/sensor_libraries
RUN chmod a+x install.sh
RUN ./install.sh

# Run the apt-get install script for required OS support in sensors
RUN chmod a+x /usr/src/app/sensor_libraries/apt_get_install.sh
RUN /bin/bash /usr/src/app/sensor_libraries/apt_get_install.sh

# SENSORS
RUN mkdir /opt/sensors
COPY sensors/ /opt/sensors/

# RUN SCRIPTS
RUN mkdir /opt/sensor_startup
COPY sensor_startup/ /opt/sensor_startup/
RUN chmod a+x /opt/sensor_startup/*

# Service components
COPY dropper.sh /tmp/dropper.sh
COPY run.sh /usr/src/app

CMD ["/bin/bash", "/usr/src/app/run.sh"]