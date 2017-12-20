FROM python:3.6
ENV PYTHONUNBUFFERED=0

RUN apt-get update

# gonna need lsof if we're going to try and stream lsof...
RUN apt-get install -y lsof vim

WORKDIR /usr/src/app
COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

COPY gen_cert.sh ./
COPY tls_cert.conf ./
RUN mkdir cert
RUN ./gen_cert.sh

COPY . .

CMD ["python", "lsof_sensor.py", "--public-key-path", "./cert/rsa_key.pub", "--private-key-path", "./cert/rsa_key", "--api-host", "api"]