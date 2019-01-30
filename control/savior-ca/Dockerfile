FROM golang:1.10

WORKDIR /go/src/app

# CFSSL tools
RUN go get -u github.com/cloudflare/cfssl/cmd/cfssl

RUN go get -u github.com/cloudflare/cfssl/cmd/mkbundle

RUN go get -u github.com/cloudflare/cfssl/cmd/cfssljson

RUN go get bitbucket.org/liamstask/goose/cmd/goose

# patch our files
COPY ./patch/auth.go $GOPATH/src/github.com/cloudflare/cfssl/auth/auth.go
RUN cd $GOPATH/src/github.com/cloudflare/cfssl/cmd/cfssl && go install

# configuration data for CFSSL Certificate Authority root
RUN mkdir config
ADD ./config/* ./config/

# setup the certificate DB
RUN goose -path $GOPATH/src/github.com/cloudflare/cfssl/certdb/sqlite up

# certificate generation
RUN cfssl genkey -initca ./config/ca.json | cfssljson -bare ca
RUN mkdir /etc/cfssl
RUN mv ca.pem /etc/cfssl
RUN mv ca-key.pem /etc/cfssl
RUN chmod go-r /etc/cfssl/*.pem

# Simple tools and scripts
RUN mkdir bin
ADD ./bin/* ./bin/

# Run in serve mode
CMD ["cfssl", "serve", "-ca", "/etc/cfssl/ca.pem", "-ca-key", "/etc/cfssl/ca-key.pem", "-port", "3030", "-address", "0.0.0.0", "-db-config", "./config/db.json", "-config", "./config/config.json"]