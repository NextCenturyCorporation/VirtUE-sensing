The Savior Certificate Authority and Sensing API provide a workflow for service and
system level sensor to automate the generation and verification of Client certificates
using an [HTTP-01 challenge protocol](https://tools.ietf.org/html/draft-ietf-acme-acme-08#page-48) like
workflow.


# Key Pair Workflow

In development contexts the initial key distribution of the CA Root public key is handled
via an unsecured API call to the Sensing API. In a production or contested environment,
the initial key distribution needs to be handled as part of Virtue instantiation. In both
cases, the interactions between the Sensor and Sensing API take place over a TLS connection
using the CA Root public key as a non-standard Certificate Authority in the Sensor http/s
library.

```
 Sensing API                                  Sensor
 ===========                                  ==========
                                              get or have CA Root Public Key (context dependent)
											     |
											  Spawn HTTP endpoint
											     |											  
Generate Private Key  <--------------------   Request new Private Key
      |
Send Private Key +
 Verification JSON     -------------------->  Store Private Key
											     |
											  Create endpoint with verification path and nonce
											     |
Validate Endpoint     <--------------------   Request signed public key
 Response
      |
	  +  <--------------------------------->  Receive and respond to verification request
	  |
Generate and Sign
 Public Key            -------------------->  Receive signed public key
											     |
											  Stop HTTP endpoint
											     |
										      Start HTTPS endpoint
											     |
											  *Begin Sensing*
											     |
												 *
```

# Acquiring the CA Public Root

Currently there is an API endpoint for acquiring a copy of the CA Root public certificate
while in _development_ mode:

```bash
> curl http://localhost:17141/api/v1/ca/root/public
{
    "timestamp":"2017-12-15 20:12:07.691819Z",
    "error":false,
    "certificate":"-----BEGIN CERTIFICATE-----\nMII...Okz/AIJw==\n-----END CERTIFICATE-----"
}
``` 

Loading the Root Public Certificate in a _production_ environment is still unsolved.

# Using the Certificate Tools

In some cases it is impractical to run the full Certificate Cycle in whatever native code/architecture you are implementing. In these cases, you can use the [certificate tools](https://github.com/twosixlabs/savior/tree/master/tools/certificates) to handle certificate registration and requests for you. There are caveats (which may change in the future):

 1. Requires Python 3.6
 2. Requires access to the trusted CFSSL_SHARED_SECRET
 3. Should not be used on systems with active users (i.e.; directly on user Virtues)

 

# Implementing the Certificate Cycle

This section will walk through the process of requesting a public/private key pair through the API, including the URIs and example JSON structures. This walk through will highlight the following steps:

 1. Make sure the API is ready
 2. Retrieve the CA public key
 3. Request a private key and Certificate Signing Request
 4. Serve the Challenge Token over HTTP
 5. Request a public key

## 1. Make sure the API is ready

The SAVIOR Sensing infrastructure is built around asynchronous communication, and for various reasons some services may not immediately be available at all times, for instance when restarting or updating the Sensing API. To this end, the Sensing API provides a simple, unauthenticated end point that can be queried to determine the ready state of the API.

```
GET http://api:17141/api/v1/ready
```

Calling this endpoint will either return a connection error, an `HTTP 400` error, or an `HTTP 200` response with the JSON payload:

```json
{
    "ready": true,
    "timestamp": "2018-01-30 14:54:15.679406Z"
}
```

_If and only if_ the response is an `HTTP 200` and the JSON payload includes `ready: true` is the API ready to service requests.

## 2. Retrieve the CA public key

During development the public key of the Sensing Certificate Authority can be retrieved over an unauthenticated HTTP connection. In our production architecture the public key will be pre-positioned using trusted mechanisms.

```
GET http://api:17141/api/v1/ca/root/public
```

Call this endpoint will return a JSON payload:

```json
{
    "certificate": "-----BEGIN CERTIFICATE-----\nMI...PEM ENCODED CERTIFICATE...HN5w==\n-----END CERTIFICATE-----",
    "error": false,
    "timestamp": "2018-01-30 14:57:13.818473Z"
}
```

The certificate is a PEM encoded public key for the Certificate Authority, and can (and _should_) be used as the trusted root certificate when doing trust chain validation on all certificates within the SAVIOR Sensing infrastructure.

## 3. Request a private key and CSR

The Sensing API and Certificate Authority handle private key generation over an HTTPS authenticated connection (but not a _mutually authenticated_ connection). When retrieving a private key, you also receive a Certificate Signing Request and SAVIOR-HTTP-01 Challenge data. The challenge data will be used during the Public Key request for the API to verify our identity. 

When making this request you will need to use the CA public key retrieved in Step #2 as the trusted root, otherwise the TLS connection will fail.

Requesting a private key is an HTTPS PUT request:

```
PUT https://api:17141/api/v1/ca/register/private_key/new
```

with a JSON payload in the request body:

```json
{
	"hostname": <your canonical and routable hostname>,
	"port": <port you'll serve on for SAVIOR-HTTP-01 challenge>
}
```

The response, if registration for a private key was successful, will be a JSON object:

```json
{
	"error": false,
   "timestamp": "2018-01-30 14:57:13.818473Z",
   "private_key": "----BEGIN PRIVATE CERTIFICATE----...PEM ENCODED PRIVATE KEY...",
   "certificate_request": "...PEM ENCODED CSR...",
   "challenge": {
   		"url": "http://<your hostname>:<your selected port>/path/to/challenge",
   		"token": <512 bit cryptographically random nonce>,
   		"type": "http-savior"
   }
}
```

The `private_key` can be stored, this will be your private key in your eventual public/private key pair. The `certificate_request` needs to be kept, as this will be used when requesting your public key. The `challenge` object will be used when we configure our own temporary HTTP server to respond to the verification challenge the Sensing API will issue asynchronously as we request a public key in step #5.

## 4. Serve the Challenge Token

The Sensing API uses an HTTP based verification mechanism, wherein the certificate requestor serves up specific content in response to an HTTP request. This provides, at the very least, a way for the API to verify that a certificate requestor controls the hostname for which they are requesting a certificate. Steps 4 and 5 of the request cycle happen asynchronously: once we issue our request for a public key, the API will issue an HTTP challenge, verifying our repsonse to the challenge before returning results for our initial HTTP request. For this reason, we need to serve the challenge token data in a separate thread or by other asynchronous means.

Taking apart the challenge data we saved in Step #3, we need to start a temporary HTTP server on our chosen port, and respond to an HTTP GET request:

```
GET http://<our hostname>:<our port specified in step 2>/path/to/challenge
```

The actual path to which we need to respond may vary. Our response to the HTTP request will be the challenge object from step 3:

```json
{
	"url": "http://<your hostname>:<your selected port>/path/to/challenge",
	"token": <512 bit cryptographically random nonce>,
	"type": "http-savior"
}
```

The structure and format of this data may change at a future point.

With our HTTP service running and ready to respond, we can move on to Step 5 and request a public key.

## 5. Request a Public Key

To request our public key, we'll send the Certificate Signing Request from step 3 in a PUT request to the API:

```
PUT https://api:17141/api/v1/ca/register/public_key/signed
```

and our JSON payload will be in the PUT body:

```json
{
	"certificate_request": <CSR from step 3>
}
```

If successful, we'll receive a JSON payload in response, which will contain our public key:

```json
{
	"error": false,
	"timestamp": "2018-01-30 14:57:13.818473Z",
	"certificate": "----BEGIN CERTIFIACTE----... PEM ENCODED PUBLIC KEY"
}
```

After a successful response we can:

 1. Record the public key from this step as the public half of our public/private key pair.
 2. Record the private key from step 3 as the private half of our public/private key pair.
 3. Stop our HTTP server that was serving our SAVIOR-HTTP-01 Challenge response
 4. Store the CA Public Key for use in all future HTTPS requests as the trusted CA root

Thus ends the Certificate Registration cycle.