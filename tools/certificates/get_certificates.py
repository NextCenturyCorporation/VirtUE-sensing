#!/usr/local/python
"""
Directly retrieve certificates from the certificate authority
within Savior. This will only work for systems within the
container network boundary of the Sensing API and CFSSL.

This tool can be used to just retrieve the CA root public
key, and/or to generate a new public/private key pair signed
by the Savior CA.
"""
import argparse
import base64
import datetime
import hashlib
import hmac
import json
import os
import requests
import socket
import sys


def get_ca_certificate(opts):
    """
    Retrieve the CA root public certificate for the Savior network.

    The basic request looks like:

        curl -d '{"label": "primary"}' http://localhost:3030/api/v1/cfssl/info

    :param opts: argparse options
    :return: PEM encoded certificate as string
    """
    print("  %% Requesting CA public root certificate")

    res = requests.post("http://%s:%d/api/v1/cfssl/info" % (opts.cfssl_host, opts.cfssl_port), json={"label": "primary"})

    if res.status_code == 200:
        print("  * Got response from CA")
        payload = res.json()

        if payload["success"]:
            print("  + Retrieved CA root key")
            ca_key = payload["result"]["certificate"]
            print("    = %d bytes in key" % (len(ca_key),))
            return ca_key
        else:
            print("  - Problem retrieving CA root key")
            for err in payload["result"]["errors"]:
                print("    ! %s" % (err,))
    else:
        print("  ! Got an error from CA")
        print("    = status_code(%d)" % (res.status_code,))

    return None


def get_keys(opts):
    """
    Retrieve a new public/private key pair from CFSSL. The basic requests
    look like:

        # new private key
        curl -d '{"key": {"algo": "rsa", "size": 4096}, "hosts":["sensor-001.savior"], "names":[{"C":"US", "ST":"Virginia", "L":"Arlington", "O":"sensor-001.savior"}], "CN": "sensor-001.savior"}' http://localhost:3030/api/v1/cfssl/newkey

        # CSR with HMAC
        curl -H "Accept: application/json" -H "Content-Type: application/json" -d '{"request": "...", "token": "dBJlaIkSKqY+IbWFwzeM7pVDTBc6n7If6t9Scr1SBDo="}' http://localhost:3030/api/v1/cfssl/authsign

    To get a key we walk through a few steps:

        1. Request a new private key

        2. Build a Certificate Signing Request

        3. Generate HMAC for the CSR

        4. Issue the CSR call to CFSSL

    :param opts: argparse options
    :return: (success, PEM public key, PEM private key)
    """

    # structure our data for the private key request
    private_key_request = {
        "key": {
            "algo": opts.cert_algo,
            "size": opts.cert_size
        },
        "hosts": [opts.hostname],
        "names": [
            {
                "C": "US",
                "ST": "Virginia",
                "L": "Arlington",
                "O": opts.hostname
            }
        ],
        "CN": opts.hostname
    }

    priv_key_res = requests.post("http://%s:%d/api/v1/cfssl/newkey" % (opts.cfssl_host, opts.cfssl_port), json=private_key_request)

    # HTTP errors?
    if priv_key_res.status_code != 200:
        print("  ! Error retrieving new private key, status_code(%d)" % (priv_key_res.status_code,))
        return False, None, None

    # let's pull apart the response JSON
    priv_key_json = priv_key_res.json()

    # Content/request errors?
    if not priv_key_json["success"]:
        print("  ! Private key request returned errors")
        for err in priv_key_json["result"]["errors"]:
            print("    - %s" % (err,))

        return False, None, None

    print("  + Private key retrieved")

    # save the private key and CSR for later use
    private_key = priv_key_json["result"]["private_key"]
    csr = priv_key_json["result"]["certificate_request"]

    # we need a signing request and a stringified version of the signing request
    signing_request = {
        "hostname": opts.hostname,
        "hosts": [opts.hostname],
        "certificate_request": csr,
        "profile": "default",
        "label": "client auth",
        "bundle": False
    }

    # this is the fun part. I swear I had to brute force every. single. combination. of these fields,
    # terms, base64ing's, and orderings, because the CFSSL documentation on this part is actively
    # hostile.
    signing_request_string = json.dumps(signing_request).encode("utf-8")

    # create an HMAC token for this request
    token_hmac = hmac.new(bytearray.fromhex(opts.cfssl_secret), signing_request_string, hashlib.sha256)
    token = base64.b64encode(token_hmac.digest())

    # create the encoded signing request
    srs_b64 = base64.b64encode(signing_request_string)

    # setup the final request payload
    signing_payload = {
        "request": srs_b64.decode(),
        "token": token.decode()
    }
    # print("payload is: %s" % (json.dumps(signing_payload, indent=4)))
    # let's post it and see what happens
    signing_res = requests.post("http://%s:%d/api/v1/cfssl/authsign" % (opts.cfssl_host, opts.cfssl_port), json=signing_payload)

    if signing_res.status_code != 200:
        print("  ! Error retrieving signed key, status_code(%d)" % (signing_res.status_code))
        return False, None, None

    signing_res_json = signing_res.json()

    if not signing_res_json["success"]:
        print("  ! Signing request returned errors")
        for err in signing_res_json["result"]["errors"]:
            print("    - %s" % (err,))

        return False, None, None

    # ok, we probably have a good response!
    public_key = signing_res_json["result"]["certificate"]
    print("  + Signed Public key retrieved")

    return True, public_key, private_key


def have_shared_secret(opts):
    """
    Check for the cfssl shared secret in our CLI options. If it hasn't been
    specified, check the environment variable CFSSL_SHARED_SECRET. If that
    doesn't exist, we've got a problem.

    :param opts: argparse options
    :return: True if we have a shared secret, otherwise false
    """

    if opts.cfssl_secret is None:

        # is it in the ENV?
        if "CFSSL_SHARED_SECRET" in os.environ:
            opts.cfssl_secret = os.environ["CFSSL_SHARED_SECRET"]
            return True
        else:
            return False
    else:
        return True


def have_hostname(opts):
    """
    Check that we have a hostname for generating and using TLS certs. We'll
    default to the hostname bound by a local socket if one isn't specified
    on the CLI.

    :param opts: argparse options
    :return: True if we have a hostname, otherwise False
    """

    if opts.hostname is None:
        opts.hostname = socket.gethostname()
        if opts.hostname is None:
            return False
        else:
            return True
    else:
        return True


def options():
    """
    Parse out the command line options.

    :return:
    """
    parser = argparse.ArgumentParser(description="Retrieve TLS certificates from the Savior CA")

    # top level control

    # communications
    parser.add_argument("--cfssl-host", dest="cfssl_host", default="cfssl", help="CFSSL hostname")
    parser.add_argument("--cfssl-port", dest="cfssl_port", default=3030, type=int, help="CFSSL port")

    # CA only mode
    parser.add_argument("--ca-only", dest="ca_only", default=False, action="store_true", help="Only retrieve the CA public root")

    # host for certificate - we'll check this later and default to the socket hostname
    parser.add_argument("--hostname", dest="hostname", default=None, help="Hostname to issue certificate for")

    # directory for certificates
    parser.add_argument("-d", "--certificate-directory", dest="cert_dir", default="./certs", help="Directory in which to place certificates")

    # certificate control
    #  algo
    #  size
    parser.add_argument("--certificate-algorithm", dest="cert_algo", default="rsa", help="Certificate algorithm")
    parser.add_argument("--certificate-size", dest="cert_size", default=4096, type=int, help="Certificate bit size")

    # check for our shared key for signing requests. If we don't have this on the CLI, we'll check
    # the ENV as well. Barring that, we'll bail. This is all handled in a separate method (see have_shared_secret
    # method) that is only run if we're actually requesting new keys
    parser.add_argument("--cfssl-shared-secret", dest="cfssl_secret", default=None, help="Shared secret used to generate HMAC verified requests to CFSSL. Will default to environment variable CFSSL_SHARED_SECRET if not specific")

    return parser.parse_args()


if __name__ == "__main__":

    opts = options()

    # make sure our directory exists for key storage
    cert_path = os.path.realpath(opts.cert_dir)
    if not os.path.exists(cert_path):
        print("  @ Certificate directory doesn't yet exist - creating")
        os.makedirs(cert_path)

    # track which keys we'll write out
    keys_to_write = {}

    # time tracking
    st = datetime.datetime.now()

    # retrieve the ca certificate
    ca_key = get_ca_certificate(opts)
    if ca_key is None:
        sys.exit(1)
    keys_to_write["ca.pem"] = ca_key

    # retrieve a new pub/priv key pair
    if not opts.ca_only:

        # gotta have a shared secret
        if not have_shared_secret(opts):
            print("  ! No CFSSL shared secret available - cannot generate a CSR")
            sys.exit(1)

        if not have_hostname(opts):
            print("  ! No hostname found - cannot generate a CSR")
            sys.exit(1)

        # let's get us some keys
        success, pub_key, priv_key = get_keys(opts)

        if success:
            keys_to_write["cert.pem"] = pub_key
            keys_to_write["cert-key.pem"] = priv_key
        else:
            print("  ! There was an error generating a public/private key pair - aborting")
            sys.exit(1)

    # write out our keys
    for filename, key_data in keys_to_write.items():
        print("  > writing [%s]" % (filename,))
        with open(os.path.join(cert_path, filename), "w") as keyfile:
            keyfile.write(key_data)

    ed = datetime.datetime.now()
    print("  ~ completed CA requests in %s" % (str(ed - st),))