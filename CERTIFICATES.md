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