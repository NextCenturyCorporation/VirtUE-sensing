FROM ubuntu:xenial as build
MAINTAINER Patrick Dwyer <patrick.dwyer@twosixlabs.com>

# Build and install Chrony
RUN apt-get update
RUN apt-get install -y coreutils build-essential wget bison asciidoctor

RUN mkdir -p /tmp/chrony
WORKDIR /tmp/chrony
RUN wget https://git.tuxfamily.org/chrony/chrony.git/snapshot/chrony-3.3.tar.gz

RUN tar -xvzf chrony-3.3.tar.gz

WORKDIR /tmp/chrony/chrony-3.3
RUN ./configure
RUN make
RUN make install

# Now rebuild from a stock Xenial
FROM ubuntu:xenial
COPY --from=build /usr/local/sbin /usr/local/sbin

RUN mkdir -p /var/lib/chrony/

COPY chronyd.sh /usr/bin/
COPY chrony-config.conf /var/lib/chrony/chrony.conf

RUN chmod +x /usr/bin/chronyd.sh

ENTRYPOINT ["/usr/bin/chronyd.sh"]