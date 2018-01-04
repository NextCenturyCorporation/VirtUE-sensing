FROM python:3.6
ENV PYTHONUNBUFFERED=0

# install virtue-security python dependencies
WORKDIR /usr/src/app
COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

# copy over our source and tools
COPY . .

# make sure we can run the bootstrap script
RUN chmod ugo+x /usr/src/app/run.sh

# install the gen certs modules
RUN pip install -r ./tools/get_certificates_requirements.txt
RUN mkdir /usr/src/app/certs

CMD ["bash", "./run.sh"]