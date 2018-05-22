FROM alpine:3.3
MAINTAINER Patrick Dwyer <patrick.dwyer@twosixlabs.com>


RUN apk add --no-cache bash coreutils chrony
RUN mkdir -p /var/lib/chrony/

COPY chronyd.sh /usr/bin/
COPY chrony-config.conf /var/lib/chrony/chrony.conf

ENTRYPOINT ["chronyd.sh"]