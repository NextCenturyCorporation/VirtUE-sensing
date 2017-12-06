The `savior-ca` is the Certificate Authority used within Savior to issue server and
client certificates used for authorization and authentication of sensors and infra-
structure components of the Sensing API.

[CFSSL](https://github.com/cloudflare/cfssl) from CloudFlare is used as the heart of the Certificate Authority. CFSSL
is a stand-alone CLI application, as well as a server, providing key generation,
signing, and validation. CFSSL is the certificate engine at the heart of [Let's Encrypt](https://letsencrypt.org), which
faces a similar problem as Savior when automating certificate issuance in a contested
trust environment.

Three API routes of CFSSL are used during the course of issuing certificates for
sensors and infrastructure in Savior:

 - [/info](https://github.com/cloudflare/cfssl/blob/master/doc/api/endpoint_info.txt) - share the CA public root key
 - [/newkey](https://github.com/cloudflare/cfssl/blob/master/doc/api/endpoint_newkey.txt) - generate a new private key and Certificate Signing Request
 - [/authsign](https://github.com/cloudflare/cfssl/blob/master/doc/api/endpoint_authsign.txt) - generate and sign a new public key from a CSR
 
Non-transient tools in the infrastructure can use the [certificates](https://github.com/twosixlabs/savior/tools/certificates) tools to generate
new TLS certificates via the command line. Transient components (like sensors) should use the
[certificate services]() provided by the Sensing API. These two different methods of
certificate generation use two different trust models:

 - The command line tools for certificate generation assume the requestor is implicitly trusted, and no
   effort is made to verify that the certificate requestor controls the entity name for which a
   certificate is issued.
  
 - The Sensing API `certificate service` explicitly distrusts the requestor, and uses
   an [HTTP-01 challenge protocol](https://tools.ietf.org/html/draft-ietf-acme-acme-08#page-48) to verify that a requestor controls the entity for which
   they are requesting a certificate.  

# Running

## As Infrastructure

CFSSL is automatically built and run as part of the Sensing API docker-compose environment, with
it's API port mapped to the host port `3030`.

## Stand-alone

The CFSSL server can be run in a stand-alone mode by first building the Docker container:

```bash
> ./build.sh
```

And then running the container with the proper ports exposed:

```bash
> docker run -p "3030:3030" -rm virtue-savior/savior-ca 
```

You can verify the server is running with `curl`:

```bash
> curl -d '{"label": "primary"}' http://localhost:3030/api/v1/cfssl/info
```

which will return the CA's root public key:

```json
{
    "success":true,
    "result": {
      "certificate": "-----BEGIN CERTIFICATE-----\nMIIFjjCCA3agAwIBAgIUPSyN1x0mwtiePlfDpQ3Yrv02qsEwDQYJKoZIhvcNAQEN\nBQAwXzELMAkGA1UEBhMCVVMxCzAJBgNVBAgTAlZBMRIwEAYDVQQHEwlBcmxpbmd0\nb24xDzANBgNVBAoTBlNBVklPUjEeMBwGA1UECxMVQ2VydGlmaWNhdGUgQXV0aG9y\naXR5MB4XDTE3MTIwNjE5MDQwMFoXDTIyMTIwNTE5MDQwMFowXzELMAkGA1UEBhMC\nVVMxCzAJBgNVBAgTAlZBMRIwEAYDVQQHEwlBcmxpbmd0b24xDzANBgNVBAoTBlNB\nVklPUjEeMBwGA1UECxMVQ2VydGlmaWNhdGUgQXV0aG9yaXR5MIICIjANBgkqhkiG\n9w0BAQEFAAOCAg8AMIICCgKCAgEA4saI+UsX9HZ0/AhgGDEAWt+tRYQvBlevHmXY\nkbZ7UZydq1wyUQ/k1LfH0O6RBlbdMXPkMkBDr7p9U1m5yRCnhwlaHvyDadzamudm\n1KHU2S0WTVH/aavm5f+bpXNlHamdUkY/XJNZe/RcQ2OEnc0iLu/r3Uuor8eHmT+r\nYSoBbc6x+cDjNggOOMZrIKstfuZaWKcPQBLVDFmxeFNTbITQ+Wj531zxNkweeL1Q\nM5EWmaatHCFdAZcTwGBhYSZcHwG+YEBRUPwwJj+ktqm/q/BJWKQUTiVFM7TgX3G1\neCY3WqKso9ZHf7YJYwYv9yqkUW+Ja9HBNvUeMU0DpCUWw1+aqxeLKcO40pDiSjEc\nsNqKu/qimcgj+ncilTIj7fxgVOTRnzlwqPwQGIyHRUHNA1OfceoJONqs0Ps/r2EW\nDKCLOVsT1XlXe+SlR6u1ZHPfDJKlaevZHvpjR0zGtDPyDh7q4Xmla5Nq9HZ/aIFt\nTA5P1cJNkNstFp9spzilzBQkKx1mICAZnPPOr18TsXJHhWElB475a/e3H2UP/EE3\nr/xcMTc65HFh5dAIvbySpAS1wtTPLpe2OQT0TdSqhTVr8QDr5dJDGecMxnCRwiII\nLr1DVPDYAl9B0+bHho8VsJBG4cz+VJeTIH1+evNguBKYrVYeX3J0+S+x0Spfl/vl\nV0248K0CAwEAAaNCMEAwDgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8w\nHQYDVR0OBBYEFOCSEHWaaxgCa71lNEkOmCiUFRUxMA0GCSqGSIb3DQEBDQUAA4IC\nAQDZshb7dFNd5NbhELaSeULPEKe5S6FZ+q35SSxR7zCwOIAxnqmhIruw+KTDc0KR\n+div9fPTHbGIKfz7b/W9wk1B6tmilsoTSxnJFZ060ICZFHhPh9uX72TVoj3Myl4i\nxj5Ylsbm7yKfLvqNQgvKIqC9l1l107iNJdtBq0umx7RyLyLzflXuFf4DwVrZtOuC\ndFo0JT3JAoieFuYVjuBfrrmc8kIfpFFcNrhjz5p59n4bXiMBhHZhvH0VhLv5Izc7\nbnkZThT71b3xkDpUgryMB8D+aKnJWJWON/PniL3uGdc9GV520w2RfnCYviNLmY/X\nEcJspthkWDNt866F5rJMP1h7ws/yJ+OHAMbue7yK6sGlw+IGYAXlsCUtsZ2tPjhc\n8pQX0sL6LSo2nmhMqoOeLLwV5GnUNlBSfJOw1yNk4/yl2c1AXSqzNx7zH3EXKl8Q\naexydHE5TplMUchDpT+7uHVy1LeCCxKTRtcLT1Nh7W/WCY05VwfNJhR1nQmgyJu+\niI+B0fVGk1xQ2a4Ees9vNN7j3isQ/fl3zrx+6CIqHCBfMe0xtMaEqd2I/wyDE1a6\nVIGhx2iBtQKXtscHjSmvtgB8+yY/1eyFIxQtWJIp+RxIWcUouQI6peIbsmqkqrwM\nzqIcx12EPYaKV3fd16mbQHU5ZtUoPXlIkjobH4ftILcMgA==\n-----END CERTIFICATE-----",
      "usages":
        ["signing","key encipherment","server auth","client auth"],
      "expiry":"8760h"
    },
    "errors":[],
    "messages":[]
}
```


# Configuration

The CFSSL Certificate Authority is configured with three JSON files in the `./config` directory:

 - `ca.json` - Configuration for generating the CA root key pair
 - `config.json` - Server and API configuration, including signing capabilities
 - `db.json` - Certificate Request persistence, allowing Certificate Revocation and Certificate Revocation Lists

# Development and Logging Patch

The `patch` directory contains modifications to the upstream CFSSL codebase that are
applied at Docker build time. This allows us to instrument sections of the CFSSL
code, without maintaining a fork of the codebase.