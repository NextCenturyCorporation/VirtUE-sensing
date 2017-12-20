The **tools/certificates** project contains tools for creating and manipulating
server and client TLS certificates from the Savior Certificate Authority.


# CLI Certificate Generation

The `gen_certificates.py` tool can be used to work directly with the CFSSL
API to generate TLS certificates. This tool is run, and operates, in an implicitly
trusted environment - no effort is made to verify that the system or services
using `gen_certificates.py` are who they say they are. This means that the
tool should only be used within the trust perimeter of the Sensing API
infrastructure core.

## Examples

Retrieve just the CA root public key:

```bash
python gen_certificates.py --cfssl-host localhost --ca-only
```

Create a public/private key pair, placing them by default in the relative `./certs`
directory:

```bash
> python get_certificates.py --cfssl-host localhost --cfssl-shared-secret **redacted**
  %% Requesting CA public root certificate
  * Got response from CA
  + Retrieved CA root key
    = 1987 bytes in key
  + Private key retrieved
  + Signed Public key retrieved
  > writing [ca.pem]
  > writing [cert.pem]
  > writing [cert-key.pem]
  ~ completed CA requests in 0:00:04.029550
```

Create a 2048bit RSA key pair, placing the keys into `/etc/certificates`:

```bash
 python get_certificates.py --cfssl-host localhost --certificate-size 2048 -d /etc/certificates --cfssl-shared-secret **redacted**
  %% Requesting CA public root certificate
  * Got response from CA
  + Retrieved CA root key
    = 1987 bytes in key
  + Private key retrieved
  + Signed Public key retrieved
  > writing [ca.pem]
  > writing [cert.pem]
  > writing [cert-key.pem]
  ~ completed CA requests in 0:00:04.029550
```
 

## Command Line Options

 - `--cfssl-host`: (default `localhost`) Hostname of the CFSSL server
 - `--cfssl-port`: (default `3030`) Listening port of the CFSSL server
 - `--ca-only`: (defaults to false) Only retrieve the CA Root public certificate, doesn't require the CFSSL Shared Secret
 - `--hostname`: (defaults to system socket hostname) Hostname to use when creating certificates
 - `--certificate-directory`, `-d`: (default to relative `./certs` directory) Directory for storing generated certificates
 - `--certificate-algorithm`: (defaults to `rsa`) Algorithm to use when generating certificates
 - `--certificate-size`: (defaults to `4096`) Bit size to use when generating certificates
 - `--cfssl-shared-secret`: (no default) Shared secret to use when generating authenticated requests to CFSSL
 
## Environment Variables

 - `CFSSL_SHARED_SECRET`: If the `--cfssl-shared-secret` CLI option is not used, this environment variable is checked
  for a shared secret value