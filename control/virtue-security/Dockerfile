FROM python:3.6
ENV PYTHONUNBUFFERED=0

WORKDIR /usr/src/app
COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

COPY gen_cert.sh ./
COPY tls_cert.conf ./
RUN ./gen_cert.sh

COPY . .

CMD ["python", "./virtue-security"]