#!"c:\program files\python\python36\python.exe"
__VERSION__ = "1.20171117"


import argparse
import asyncio
import base64
from Crypto.PublicKey import RSA
from curio import subprocess, Queue, TaskGroup, run, tcp_server, CancelledError, SignalEvent, sleep, check_cancellation, ssl, spawn
import curequests
import email
import hashlib
from io import StringIO
import json
from kafka import KafkaProducer
import os
import pwd
import requests
from routes import Mapper
import signal
import socket
import sys
import time
from urllib.parse import urlparse
from uuid import uuid4


#
# Crazy monkey-patching so we can get access to the peer cert when we
# initiate an HTTPS connection somewhere. Because the peer certificate
# is attached at the SSLSocket level, which is transient in the HTTP Pool
# for requests and curequests, this is the best we can really do.
#
# Cribbed from: https://stackoverflow.com/questions/16903528/how-to-get-response-ssl-certificate-from-requests-in-python
#
# We're going to patch the curequests and requests libs directly so we can access
# the peer cert during any of our async connections.

## curequests - peercert acquisition
cuHTTPAdapter = curequests.adapters.CuHTTPAdapter
cu_orig_HTTPAdapter_build_response = cuHTTPAdapter.build_response


def cu_new_HTTPAdapter_build_response(self, request, resp, conn):
    response = cu_orig_HTTPAdapter_build_response(self, request, resp, conn)
    try:
        response.peercert = conn.sock.getpeercert(binary_form=True)
    except AttributeError:
        pass
    return response
cuHTTPAdapter.build_response = cu_new_HTTPAdapter_build_response


## requests - peercert acquisition
HTTPResponse = requests.packages.urllib3.response.HTTPResponse
orig_HTTPResponse__init__ = HTTPResponse.__init__


def new_HTTPResponse__init__(self, *args, **kwargs):
    orig_HTTPResponse__init__(self, *args, **kwargs)
    try:
        self.peercert = self._connection.sock.getpeercert()
    except AttributeError:
        pass
HTTPResponse.__init__ = new_HTTPResponse__init__

HTTPAdapter = requests.adapters.HTTPAdapter
orig_HTTPAdapter_build_response = HTTPAdapter.build_response


def new_HTTPAdapter_build_response(self, request, resp):
    response = orig_HTTPAdapter_build_response(self, request, resp)
    try:
        response.peercert = resp.peercert
    except AttributeError:
        pass
    return response
HTTPAdapter.build_response = new_HTTPAdapter_build_response

#
# A dummy sensor that streams log messages based off of following the
# `lsof` command.
#
# This sensor tracks the output of a subprocess running `lsof` as well
# as repsonds to HTTP requests for sensor configuration and actuation.
# Because of this, the sensor makes heavy use of Python asyncio to run
# tasks concurrently.
#
#


class SensorWrapper(object):

    def __init__(self, name, sensing_methods=[]):

        # log message queueing
        self.log_messages = Queue()
        self.sensor_name = name
        self.setup_options()
        self.opts = None

        # all of our sensing methods that are called - this is the
        # sensor specific implementation
        self.sensing_methods = sensing_methods

        # track the different task groups and async tasks that
        # we need to interact with throughout the sensor life cycle.
        # Because we start/stop/restart multiple tasks in these
        # groups, we can't just use anonymous or Context task groups
        self.sensing_method_task_group = None
        self.wrapper_task_group = None
        self.log_drain_task = None

        # Keep track of the public certificate of the API server
        self.api_public_certificate = None

        print("Configured with sensing methods:")
        print(sensing_methods)

    async def get_root_ca_pubkey(self):
        """
        Get the public key of the CA root. During development, this is an unauthenticated
        and insecure connection to the Sensing API.

        Returns look like:

            True, "PEM data"
            False, "error message"

        The PEM will also be written to the "ca_key_path" directory as ca.pem.

        :param opts: argparse options
        :return: success boolean, PEM encoded CA public key or error message
        """
        print("  @ Retrieving CA Root public key")

        uri = self.construct_api_uri("/ca/root/public", secure=False)

        res = requests.get(uri)

        if res.status_code == 200:
            res_json = res.json()

            if not res_json["error"]:
                print("  + got PEM encoded certificate")

                with open(os.path.join(self.opts.ca_key_path, "ca.pem"), "w") as ca_pem:
                    ca_pem.write(res_json["certificate"])
                    print("  < PEM written to [%s]" % (os.path.join(self.opts.ca_key_path, "ca.pem")))
                return True, res_json["certificate"]
            else:
                print("  ! encountered an error retrieving the certificate: %s" % (res_json["message"],))
                return False, res_json["message"]
        else:
            print("  ! Encountered an HTTP error retrieving the certificate: status_code(%d)" % (res.status_code,))
            return False, "HTTP(%d)" % (res.status_code,)

    async def wait_for_sensor_api(self):
        """
        We may spin up before our configured Sensor API endpoint
        is ready to work. We need to wait (a configurable amount
        of time) for the API to be ready.

        This method loops on calls to the API /ready endpoint, either
        waiting for a ready: True response, or killing the sensor
        if we exceed a defined retry time limit.

        :param opts: argparse options
        :return: Return if ready, otherwise raise ConnectionError
        """

        print("  @ Waiting for Sensing API")

        uri = self.construct_api_uri("/ready", secure=False)

        st = time.time()

        while True:

            try:

                # try to connect to the ready endpoint
                res = requests.get(uri)

                # did we get a status 200?
                if res.status_code == 200:
                    res_json = res.json()

                    # can we break free?
                    if "ready" in res_json and res_json["ready"]:
                        print("    + Sensing API is ready")
                        return
            except Exception as e:
                print("  ! Exception while waiting for the Sensing API (%s)" % (str(e),))

            # have we exceeded our retry time limit?
            if (time.time() - st) > self.opts.api_retry_max:
                raise ConnectionError("Cannot connect to Sensing API")

            # The endpoint isn't ready, we need to sleep on it
            print("    ~ retrying in %0.2f seconds" % (self.opts.api_retry_wait,))

            time.sleep(self.opts.api_retry_wait)

    async def sync_sensor(self, pub_key):
        """
        Synchronize the sensor with the Sensing API so we stay registered.

        :param opts: argparse parse parameters
        :return:
        """

        # define our payload for registration sync
        pubkey_b64 = str(base64.urlsafe_b64encode(pub_key.encode()),
                         encoding="iso-8859-1")
        uri = self.construct_api_uri("/sensor/%s/sync" % (self.opts.sensor_id,))

        payload = {
            "sensor": self.opts.sensor_id,
            "public_key": pubkey_b64,
        }

        while True:

            # try and hit the sync end point
            print("syncing with [%s]" % (uri,))

            ca_path = os.path.join(self.opts.ca_key_path, "ca.pem")

            reg_res = await curequests.put(uri, json=payload, verify=ca_path, cert=(self.opts.public_key_path, self.opts.private_key_path))

            if reg_res.status_code == 200:
                if not self.is_pinned_api(reg_res):
                    sys.exit(1)
                print("Synced sensor with Sensing API")
            else:
                print("Couldn't sync sensor with Sensing API")
                print("  status_code == %d" % (reg_res.status_code,))
                print(reg_res.json())

            # sleep
            await sleep(60 * 2)

            # bail if we're being cancelled
            if await check_cancellation():
                break

    async def get_private_key_and_csr(self):
        """
        Request a new private key from the Sensing API. This will get us both
        the private key (RSA 4096 by default) and the Certificate Signing Request.


        :param opts: argparse program options
        :param root_cert: Savior CA public root
        :return: success, private_key, csr, challenge data
        """
        print("  @ Requesting Private Key and Certificate Signing Request from the Sensing API")

        uri = self.construct_api_uri("/ca/register/private_key/new")
        payload = {
            "hostname": self.opts.sensor_hostname,
            "port": self.opts.sensor_port
        }
        ca_path = os.path.join(self.opts.ca_key_path, "ca.pem")

        pki_priv_res = await curequests.put(uri, json=payload, verify=ca_path)

        if pki_priv_res.status_code == 200:
            print("  + got private key response from Sensing API")
            pki_priv_json = pki_priv_res.json()

            self.api_public_certificate = pki_priv_res.peercert
            print("  + storing API public certificate for pinning (%d bytes)" % (len(self.api_public_certificate),))

            return True, pki_priv_json["private_key"], pki_priv_json["certificate_request"], pki_priv_json["challenge"]
        else:
            print("  ! got status_code(%d) from the Sensing API" % (pki_priv_res.status_code,))
            res_json = pki_priv_res.json()
            if "messages" in res_json:
                for msg in res_json["messages"]:
                    print("    - %s" % (msg,))
            return False, "", "", {}

    async def get_signed_public_key(self, csr):
        """
        Request a signed public certificate that corresponds to the CSR that we received
        with our private key.

        Return will be:

            - True, key
            - False

        :param opts: argparse program options
        :param csr: certificate signing request PEM data
        :return: success, key
        """
        print("  @ Requesting Signed Public Key from the Sensing API")

        uri = self.construct_api_uri("/ca/register/public_key/signed")
        payload = {
            "certificate_request": csr
        }
        ca_path = os.path.join(self.opts.ca_key_path, "ca.pem")

        pub_key_res = await curequests.put(uri, json=payload, verify=ca_path)

        if pub_key_res.status_code == 200:

            # verify the API public key
            if not self.is_pinned_api(pub_key_res):
                return False, ""

            print("  + got a signed public key from the Sensing API")
            pub_key_json = pub_key_res.json()

            return True, pub_key_json["certificate"]

        else:
            print("  ! got status_code(%d) from the Sensing API" % (pub_key_res.status_code,))
            res_json = pub_key_res.json()
            if "messages" in res_json:
                for msg in res_json["messages"]:
                    print("    - %s" % (msg,))
            return False, ""

    def is_pinned_api(self, response):
        """
        Verify the public key of the HTTPS response against the pinned API public key.

        :param response:
        :return:
        """

        if response.peercert == self.api_public_certificate:
            print("  + pinned API certificate match")
            return True
        else:
            print("  - pinned API certificate mismatch")
            return False

    async def register_sensor(self, pub_key):
        """
        Register this sensor with the Sensing API.

        :param opts: argparse parsed parameters
        :return:
        """
        uri = self.construct_api_uri("/sensor/%s/register" % (self.opts.sensor_id,))
        pubkey_b64 = str(base64.urlsafe_b64encode(pub_key.encode()), encoding="iso-8859-1")

        payload = {
            "sensor": self.opts.sensor_id,
            "virtue": self.opts.virtue_id,
            "user": self.opts.username,
            "public_key": pubkey_b64,
            "hostname": self.opts.sensor_hostname,
            "port": self.opts.sensor_port,
            "name": "%s-%s" % (self.sensor_name, __VERSION__,)
        }

        print("registering with [%s]" % (uri,))

        ca_path = os.path.join(self.opts.ca_key_path, "ca.pem")
        client_cert_paths = (os.path.abspath(self.opts.public_key_path), os.path.abspath(self.opts.private_key_path))
        print("  client certificate public(%s), private(%s)" % client_cert_paths)
        reg_res = await curequests.put(uri, json=payload, verify=ca_path, cert=client_cert_paths)

        if reg_res.status_code == 200:
            if not self.is_pinned_api(reg_res):
                sys.exit(1)
            print("  = Got registration data: ")
            print(reg_res.json())
            return reg_res.json()
        else:
            print("Couldn't register sensor with Sensing API")
            print("  status_code == %d" % (reg_res.status_code,))
            print(reg_res.text)
            sys.exit(1)

    def deregister_sensor(self, pub_key):
        """
        Deregister this sensor from the sensing API.

        :param opts: argparse parsed parameters
        :return:
        """
        uri = self.construct_api_uri("/sensor/%s/deregister" % (self.opts.sensor_id,))
        pubkey_b64 = str(base64.urlsafe_b64encode(pub_key.encode()), encoding="iso-8859-1")

        payload = {
            "sensor": self.opts.sensor_id,
            "public_key": pubkey_b64
        }

        ca_path = os.path.join(self.opts.ca_key_path, "ca.pem")

        res = requests.put(uri, data=payload, verify=ca_path, cert=(self.opts.public_key_path, self.opts.private_key_path))

        if res.status_code == 200:

            if not self.is_pinned_api(res):
                print("  !!! API cert pinning failed during deregistration")
            print("Deregistered sensor with Sensing API")
        else:
            print("Couldn't deregister sensor with Sensing API")
            print("  status_code == %d" % (res.status_code,))
            print(res.text)

    async def call_sensing_method(self, sensor_id, default_config):
        """
        We call into our loaded sensor method, passing in the basic message
        stub that needs to be in every message, the configuration for the
        sensor, and the log_message queue.

        :param sensor_id: ID of this sensor
        :param default_config: Configuration for this sensor as retrieved from the Sensing API
        :return: -
        """

        if self.sensing_method_task_group is not None:
            try:
                await self.sensing_method_task_group.cancel_remaining()
            except CancelledError as ce:
                print("  ^ ate CancelledError when cancelling stale sensing_method task group")
                self.sensing_method_task_group = None

        # setup our message stub
        msg_stub = {
            "sensor_id": sensor_id,
            "sensor_name": "%s-%s" % (self.sensor_name, __VERSION__,)
        }

        self.sensing_method_task_group = TaskGroup()
        async with self.sensing_method_task_group as g:
            for lm in self.sensing_methods:
                await g.spawn(lm, msg_stub, default_config, self.log_messages)

    async def log_drain(self, kafka_bootstrap_hosts=None, kafka_channel=None):
        """
        Pull messages off of the reporting queue, and splat them out on STDOUT
        for now.

        :param kafka_bootstrap_hosts: Bootstrap host list for kafka
        :param kafka_channel:
        :return:
        """
        print(" ::starting log_drain")
        print("  ? configuring KafkaProducer(bootstrap=%s, topic=%s)" % (",".join(kafka_bootstrap_hosts), kafka_channel))

        producer = KafkaProducer(
            bootstrap_servers=kafka_bootstrap_hosts,
            retries=5,
            max_block_ms=10000,
            value_serializer=str.encode,
            ssl_cafile=os.path.join(self.opts.ca_key_path, "ca.pem"),
            security_protocol="SSL",
            ssl_certfile=self.opts.public_key_path,
            ssl_keyfile=self.opts.private_key_path
        )

        # basic channel tracking
        msg_count = 0
        msg_bytes = 0

        try:

            # just read messages as they become available in the message queue
            async for message in self.log_messages:

                msg_count += 1
                msg_bytes += len(message)

                producer.send(kafka_channel, message)

                # progress reporting
                if msg_count % 5000 == 0:
                    print("  :: %d messages received, %d bytes total" % (msg_count, msg_bytes))

        except CancelledError as ce:
            print(" () stopping log-drain")

    def configure_http_handler(self, routes, secure=False):
        """
        Expose the run time options to the http handler which
        will be spawned by the tcp server.

        The `routes` list contains dictionaries, with each dictionary defining
        a route that the HTTP(s) server should handle.

        :param opts: argparse options
        :param routes: List of dicts
        :param secure: Secure the server with an SSL/TLS certificate
        :return: async http_handler function
        """

        # Configure our HTTP routes
        mapper = Mapper()
        # mapper.connect(None, "/sensor/{uuid}/registered", controller="route_registration_ping")
        # mapper.connect(None, "/sensor/status", controller="route_sensor_status")
        #
        # # and which methods handle what?
        mapper_funcs = {}
        #     "route_registration_ping": route_registration_ping,
        #     "route_sensor_status": route_sensor_status
        # }

        for route in routes:
            mapper.connect(None, route["path"], controller=route["name"])
            mapper_funcs[route["name"]] = route["handler"]

        async def http_handler(client, addr):
            """
            Handle an HTTP request from a remote client.

            :param client: tcp_server client object
            :param addr: Client address
            :return: -
            """
            print(" -> connection from ", addr)
            s = client.as_stream()
            try:

                # read everything in. we're parsing HTTP, so
                # we'll look for the trailing \r\n\r\n after
                # which we'll jump into header parsing and then
                # response
                buff = b''

                async for line in s:
                    buff += line
                    if buff[-4:] == b'\r\n\r\n':
                        break

                # now we need to parse the request
                path, headers = buff.decode('iso-8859-1').split('\r\n', 1)
                message = email.message_from_file(StringIO(headers))
                headers = dict(message.items())

                # break out the request string into it's pieces, as it
                # is currently something like:
                #
                #       GET /sensor/status HTTP/1.1
                #
                (r_method, r_path, r_version) = path.strip().split(" ")

                # handle the request, figuring out where it goes with the mapper
                print("[info] got [%s] request path [%s]" % (r_method, r_path,))
                handler = mapper.match(r_path)

                if handler is not None:
                    print("[%(controller)s] > handling request" % handler)
                    await mapper_funcs[handler["controller"]](s, headers, handler)
                else:
                    # whoops
                    print("   :: No route available for request [%s]" % (path,))

                    # send an error
                    await send_json(s, {"error": True, "msg": "no such route"}, status_code=404)

            except CancelledError:

                # ruh-roh, connection broken
                print("connection goes boom")

            # request cycle done
            print(" <- connection closed")

        return http_handler

    def create_challenge_handler(self, outer_opts, challenge):
        """
        Create the HTTP handler that will be used during the HTTP-01/SAVIOR verification
        challenge.

        :param opts: argparse program options
        :param challenge: challenge configuration data from Sensing API
        :return:
        """

        async def challenge_handler(stream, headers, params):
            """
            Handle the HTTP-01/Savior certificate challenge
            :param opts:
            :param stream:
            :param headers:
            :param params:
            :return:
            """
            print("  | sending HTTP-01/SAVIOR certificate challenge response")
            await send_json(stream, challenge)

        return challenge_handler

    async def route_registration_ping(self, stream, headers, params):
        """
        Handle a request for:

            /sensor/{uuid}/registered

        :param opts: argparse parsed options
        :param stream: Async client stream
        :param headers: HTTP headers
        :param params: URI path parameters and controller id
        :return: None
        """

        # do our UUIDs match?
        if self.opts.sensor_id == params["uuid"]:
            print("  | sensor ID is a match")
            await send_json(stream, {"error": False, "msg": "ack"})
        else:
            print("  | sensor ID is NOT A MATCH")
            await send_json(stream, {"error": True, "msg": "Sensor ID does not match registration ID"}, status_code=401)

    async def route_sensor_actuate(self, stream, headers, params):
        """
        Handle an actuation request. Depending on the value of the "actuation" parameter,
        this wil be handled in different ways.

        :param stream:
        :param headers:
        :param params:
        :return:
        """
        body = await self.wait_for_json(stream)

        print("=> Sensor actuation received action(%s)" % (body["actuation"]))
        print("   config=<%s>" % (body["payload"]["configuration"],))

        if body["actuation"] == "observe":

            # rebuild our sensing method
            csm_t = await spawn(self.call_sensing_method, self.opts.sensor_id, json.loads(body["payload"]["configuration"]))
            await self.wrapper_task_group.add_task(csm_t)

            # bounce the log drain
            await self.log_drain_task.cancel()
            ld_t = await spawn(self.log_drain, body["payload"]["kafka_bootstrap_hosts"], body["payload"]["sensor_topic"])
            await self.wrapper_task_group.add_task(ld_t)
            self.log_drain_task = ld_t

            print("  = observe actuation triggered")

            # respond
            await send_json(stream, {"error": False})
        else:

            # if we don't know what it is, we can't really do anything. Respond
            # to the API with an error, and carry on
            print("  ! unknown actuation(%s) - dropping" % (body["actuation"],))
            await send_json(stream, {"error": True, "msg": "Unknown actuation"})

    async def route_sensor_status(self, stream, headers, params):
        """
        Handle a request for:

            /sensor/status

        :param opts:
        :param stream:
        :param headers:
        :param params:
        :return:
        """
        print("  | reporting status ")
        await send_json(stream,
                        {
                            "error": False,
                            "sensor": self.opts.sensor_id,
                            "virtue": self.opts.virtue_id,
                            "user": self.opts.username
                        }
                        )

    async def wait_for_json(self, stream):
        """
        Because everything is broken, nothing works, and the internet only stays up
        because everything is coded to assume everyone else is a brain-dead mole rat
        with ADHD.

        Fuh.

        If we try and do a normal read of the request body from a stream, curio will
        let us happily block until the connection is timed out by the client. So, we
        need to detect when we've reached the end of the request body. In this case,
        that means repeatedly checking to see if our results are JSON parse-able. If
        we can parse it, return the decoded JSON data.

        :param stream: Curio SocketStream
        :return: decoded JSON data
        """

        body = ""
        decoded = None
        while True:
            line = await stream.read(1024)
            body += line.decode("utf-8")
            print(body)
            try:
                decoded = json.loads(body)
                break
            except json.decoder.JSONDecodeError as jde:
                pass
        return decoded

    async def main(self):
        """
        Primary task spin-up. we're going to coordinate and launch all of
        our asyncio co-routines from here.

        :return: -
        """
        Goodbye = SignalEvent(signal.SIGINT, signal.SIGTERM)

        self.wrapper_task_group = TaskGroup()
        async with self.wrapper_task_group as g:

            # first thing first - let's make sure the Sensing API is ready for us
            api_ready = await g.spawn(self.wait_for_sensor_api)
            await api_ready.join()

            # now we need to get the root public key
            ca_root_cert_future = await g.spawn(self.get_root_ca_pubkey)
            rc_success, root_cert = await ca_root_cert_future.join()

            if not rc_success:
                print("  ! Couldn't get the CA root certificate - no way to verify secure communications")
                sys.exit(1)

            # now the certificate cycle, where we get our pub/priv key pair
            pki_private_future = await g.spawn(self.get_private_key_and_csr)
            pki_priv_success, priv_key, csr, challenge_data = await pki_private_future.join()
            if not pki_priv_success:
                print("  ! Encountered an error when retrieving a private key for the sensor, aborting")
                sys.exit(1)

            print("  %% private key fingerprint(%s)" % (self.rsa_private_fingerprint(priv_key),))
            print("  %% CA http-savior challenge url(%s) and token(%s)" % (challenge_data["url"], challenge_data["token"]))

            # Our Verification and Challenge cycle has two components:
            #
            #  1. HTTP-01-SAVIOR challenge for issuing a signed public certificate
            #  2. Sensor control verification and configuration
            #
            # We've already retrieved a private key and Certificate Signing Request from
            # the Sensing API. Now the Challenge cycle requires us to
            #
            #  1. spin up the http challenge responder
            #  2. call the public key signer of the Sensing API
            #  3. Store our public and private keys
            #
            # Once we have our public and private key, we can continue with the sensor
            # control challenge, where we our certificates for:
            #
            #  - https server for registration responder
            #  - client certs for registration request
            #  - client certs for all heartbeats

            # layout the routes we'll serve during the certificate challenge
            http_routes = [
                {"path": urlparse(challenge_data["url"]).path, "name": "http_01_savior_challenge", "handler": self.create_challenge_handler(self.opts, challenge_data)}
            ]

            # now spin up the http server so we can respond to the challenge
            challenge_server = await g.spawn(
                tcp_server(
                    self.opts.sensor_hostname,
                    self.opts.sensor_port,
                    self.configure_http_handler(http_routes, secure=False)
                )
            )

            # request a public cert
            pki_public_future = await g.spawn(self.get_signed_public_key, csr)
            pub_key_success, pub_key = await pki_public_future.join()
            if not pub_key_success:
                print("  ! Encountered an error when retrieving a public key for the sensor, aborting")
                sys.exit(1)

            # spin down the http server
            await challenge_server.cancel()

            # write out our keys to the pub_key/priv_key paths in our options, and make
            # sure we set the correct permissions (0x600)
            with open(self.opts.public_key_path, "w") as public_key_file:
                public_key_file.write(pub_key)
            os.chmod(self.opts.public_key_path, 0o600)

            with open(self.opts.private_key_path, "w") as private_key_file:
                private_key_file.write(priv_key)
            os.chmod(self.opts.private_key_path, 0x600)

            # we can now spin up our HTTPS actuation listener using our new keys
            ssl_context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
            ssl_context.load_cert_chain(certfile=self.opts.public_key_path, keyfile=self.opts.private_key_path)

            # layout the routes we'll be serving persistently for actuation/inspection over HTTPS
            https_routes = [
                {"path": "/sensor/{uuid}/registered", "name": "route_registration_ping", "handler": self.route_registration_ping},
                {"path": "/sensor/status", "name": "route_sensor_status", "handler": self.route_sensor_status},
                {"path": "/actuation", "name": "route_sensor_actuate", "handler": self.route_sensor_actuate}
            ]
            await g.spawn(
                tcp_server(
                    self.opts.sensor_hostname,
                    self.opts.sensor_port,
                    self.configure_http_handler(https_routes, secure=True),
                    ssl=ssl_context
                )
            )

            # now we register and wait for the results
            reg = await g.spawn(self.register_sensor, pub_key)

            print("  @ waiting for registration cycle")
            reg_data = await reg.join()
            print(reg_data)
            print("  = got registration")

            self.log_drain_task = await g.spawn(self.log_drain, reg_data["kafka_bootstrap_hosts"], reg_data["sensor_topic"])
            await g.spawn(self.call_sensing_method, self.opts.sensor_id, json.loads(reg_data["default_configuration"]))
            await g.spawn(self.sync_sensor, pub_key)

            await Goodbye.wait()
            print("Got SIG: deregistering sensor and shutting down")
            await g.cancel_remaining()

            # don't run this async - we're happy to block on deregistration
            self.deregister_sensor(pub_key)

            print("Stopping.")

    def load_public_key(self, key_path):
        """
        Load and validate a public key for use with the API.

        An exception is raised if the public key is invalid

        :param key_path: Path to the public key.
        :return: RSAKEY object, PEM String
        """

        # validate the key
        try:

            if key_path is None:
                raise ValueError("Public key path is not specified.")

            key_string = open(key_path, 'r').read()

            rsakey = RSA.importKey(key_string)

            # make sure it's a pubkey
            if rsakey.has_private():
                raise ValueError("specified key is an RSA private key")

        except ValueError as ve:

            print("Couldn't import RSA public key at %s - %s" % (key_path, str(ve)))
            print("  If this is encountered during testing, run the gen_cert.sh script to create a key pair, and try testing again")
            sys.exit(1)

        return rsakey, key_string

    def load_private_key(self, key_path):
        """
        Load and validate a private key for use with the API and
        stream encryption.

        An exception is raised if the key is not a private key.

        :param key_path: Path to the public key
        :return: RSAKEY object, PEM STring
        """

        # validate the key
        try:

            if key_path is None:
                raise ValueError("Private key path is not specified")

            key_string = open(key_path, 'r').read()

            rsakey = RSA.importKey(key_string)

            # make sure it's a pubkey
            if not rsakey.has_private():
                raise ValueError("specified key is not an RSA private key")

        except ValueError as ve:

            print("Couldn't import RSA private key at %s - %s" % (key_path, str(ve)))
            print("  If this is encountered during testing, run the gen_cert.sh script to create a key pair, and try testing again")
            sys.exit(1)

        return rsakey, key_string

    def insert_char_every_n_chars(self, string, char='\n', every=64):
        return char.join(
            string[i:i + every] for i in range(0, len(string), every))

    def rsa_public_fingerprint(self, pem_data):
        """
        Generate a hex digest in the form ##:##:##...## of the given
        public key. The fingerprint will be a 47 character string, with
        32 characters of data.

        The public key fingerprint is an md5 digest.

        :param key: RSA Public Key object
        :return:
        """
        key = RSA.importKey(pem_data)
        md5digest = hashlib.md5(key.exportKey("DER")).hexdigest()
        fingerprint = self.insert_char_every_n_chars(md5digest, ':', 2)
        return fingerprint

    def rsa_private_fingerprint(self, pem_data):
        """
        Generate a hex digest of the form ##:##:##..## of the given
        private key. The finger print will be a 59 character string,
        with 40 characters of data.

        The private key fingerprint is a sha1 digest.

        :param key: PEM string
        :return:
        """
        key = RSA.importKey(pem_data)
        sha1digest = hashlib.sha1(key.exportKey("DER", pkcs=8)).hexdigest()
        fingerprint = self.insert_char_every_n_chars(sha1digest, ':', 2)
        return fingerprint

    def construct_api_uri(self, uri_path, secure=True):
        """
        Build a full URI for a request to the API.

        :param opts: Command configuration options
        :param uri_path: Full path for the API
        :return: String URI
        """

        # setup the host
        host = self.opts.api_host
        port = self.opts.api_https_port
        if not secure:
            port = self.opts.api_http_port

        if not host.startswith("http"):
            if secure:
                host = "https://%s" % (host,)
            else:
                host = "http://%s" % (host,)

        # setup the full uri
        return "%s:%d/api/%s%s" % (host, port, self.opts.api_version, uri_path)

    def setup_options(self):
        """
        TODO: Make this extensible, and separate it out from actually parsing the options.

        Parse out the command line options.

        :return:
        """
        self.argparser = argparse.ArgumentParser(description="LSOF sensor")

        # top level control

        # key management
        self.argparser.add_argument("--public-key-path", dest="public_key_path", default=None, help="Path to the public key to use")
        self.argparser.add_argument("--private-key-path", dest="private_key_path", default=None, help="Path to the private key to use")
        self.argparser.add_argument("--ca-key-path", dest="ca_key_path", default="./cert", help="Directory path at which CA public keys can be written")

        # communications
        self.argparser.add_argument("-a", "--api-host", dest="api_host", default="localhost", help="API host URI")
        self.argparser.add_argument("--api-https-port", dest="api_https_port", default=17504, type=int, help="API host secure port")
        self.argparser.add_argument("--api-http-port", dest="api_http_port", default=17141, type=int,
                            help="API host insecure port")
        self.argparser.add_argument("--api-version", dest="api_version", default="v1", help="API version being called")

        # identification
        self.argparser.add_argument("--sensor-id", dest="sensor_id", default=None, help="ID of the sensor, auto-generated if absent")
        self.argparser.add_argument("--virtue-id", dest="virtue_id", default=None, help="ID of the current Virtue, auto-generated if absent")
        self.argparser.add_argument("--username", dest="username", default=None, help="Name of the observed user, inferred if absent")
        self.argparser.add_argument("--sensor-hostname", dest="sensor_hostname", default=None, help="Addressable name of the sensor host")
        self.argparser.add_argument("--sensor-port", dest="sensor_port", type=int, default=11000, help="Port on sensor host where sensor is listening for API actuations")

        # for testing and time management/dependency management
        self.argparser.add_argument("--delay-start", dest="delay_start", type=int, default=0, help="Number of seconds to delay before startup")
        self.argparser.add_argument("--api-connect-retry-max", dest="api_retry_max", type=float, default=30.0, help="How many seconds should we wait for the Sensing API to be ready before we shutdown")
        self.argparser.add_argument("--api-connect-retry-wait", dest="api_retry_wait", type=float, default=0.5, help="Delay, in seconds, between Sensign API connection retries")

    def parse_options(self):
        self.opts = self.argparser.parse_args()

    def check_identification(self):
        """
        Check the identification fields in the options and build values if none
        exist. The fields checked are:

            sensor_id - auto-generated as UUID if missing
            virtue_id - pulled from ENV if available, auto-generated as UUID if missing
            username - pulled from OS if available, auto-generated as user@hostname if missing
            public-key-path - check for valid RSA pubkey
            private-key-path - check for valid RSA private key
            sensor-hostname - fill in a valid hostname

        :param opts: run time options (argparse parsed options)
        :return: None
        """

        if self.opts.sensor_id is None:
            self.opts.sensor_id = str(uuid4())

        if self.opts.virtue_id is None:
            if "VIRTUE_ID" is os.environ:
                self.opts.virtue_id = os.environ["VIRTUE_ID"]
            else:
                self.opts.virtue_id = str(uuid4())

        if self.opts.username is None:

            # try and resolve the user from the PWD
            pstruct = pwd.getpwuid(os.getuid())
            if pstruct is not None:
                self.opts.username = pstruct[0]

            # no? maybe it's in the environment
            if self.opts.username is None:
                if "USERNAME" is os.environ:
                    self.opts.username = os.environ["USERNAME"]

            # really? Maybe the process controller?
            if self.opts.username is None:
                self.opts.username = os.getlogin()

            # ok. now we're getting desperate. Let's make something up
            if self.opts.username is None:
                self.opts.username = "user@%s" % (socket.getfqdn(),)

            # we're out of options at this point
            if self.opts.username is None:
                raise ValueError("Can't identify the current username, bailing out")

        if self.opts.sensor_hostname is None:

            self.opts.sensor_hostname = socket.getfqdn()

            # bork bork bork
            if self.opts.sensor_hostname is None:
                raise ValueError("Can't identify the current hostname, bailing out")

        # report
        print("Sensor Identification")
        print("\tsensor_id  == %s" % (self.opts.sensor_id,))
        print("\tvirtue_id  == %s" % (self.opts.virtue_id,))
        print("\tusername   == %s" % (self.opts.username,))
        print("\thostname   == %s" % (self.opts.sensor_hostname,))

        print("Sensing API")
        print("\thostname   == %s" % (self.opts.api_host,))
        print("\thttp port  == %d" % (self.opts.api_http_port,))
        print("\thttps port == %d" % (self.opts.api_https_port,))
        print("\tversion    == %s" % (self.opts.api_version,))

        print("Sensor Interface")
        print("\thostname  == %s" % (self.opts.sensor_hostname,))
        print("\tport      == %d" % (self.opts.sensor_port,))

    def start(self):
        """
        Start up the sensor wrapper.

        :return:
        """
        # good morning.
        print("Starting %s(version=%s)" % (self.sensor_name, __VERSION__,))

        self.parse_options()

        self.check_identification()

        if self.opts.delay_start > 0:
            print("Delaying startup of lsof_sensor %d seconds" % (self.opts.delay_start,))
            time.sleep(self.opts.delay_start)

        # let's jump right into async land
        run(self.main)


def which_file(name):
    """
    Find a file on the path.

    :param name:
    :return:
    """
    paths = os.environ["PATH"].split(os.pathsep)

    for path in paths:
        if os.path.isfile(os.path.join(path, name)):
            return os.path.join(path, name)

    return None


async def report_on_file(filename):
    """
    Run a report on a file, and return data about the file. This is effectively the
    check-summing and file inspection capability of a sensor.

    :param filename: Absolute path to a file resource
    :return: Dictionary
    """
    if not os.path.exists(filename):
        return None

    # let's do the basics
    data = {
        "is_file": os.path.isfile(filename),
        "is_dir": os.path.isdir(filename),
        "is_link": os.path.islink(filename),
        "real_path": os.path.realpath(filename),
        "exists": os.path.exists(filename)
    }

    # the remainder of the actions depend on the file existing
    if data["exists"]:

        # size and time stamps
        data.update(
            {
                "size": os.path.getsize(filename),
                "atime": os.path.getatime(filename),
                "mtime": os.path.getmtime(filename),
                "ctime": os.path.getctime(filename)
            }
        )

        # sha512
        filename_flo = open(filename, 'rb')
        flo_sha512 = hashlib.sha512()
        for chunk in iter(lambda: filename_flo.read(4096), b''):
            flo_sha512.update(chunk)

        data["sha512"] = flo_sha512.hexdigest()
    else:
        data.update(
            {
                "size": 0,
                "atime": 0,
                "mtime": 0,
                "ctime": 0,
                "sha512": ""
            }
        )

    return data




async def send_json(stream, json_data, status_code=200):
    """
    Serialize a data structure to JSON and send it as a response
    to an HTTP request. This includes sending HTTP headers and
    closing the connection (via `Connection: close` header).

    :param stream: AsyncIO writable stream
    :param json_data: Serializable data structure
    :return:
    """

    # serialize
    raw = json.dumps(json_data).encode()

    # get the raw length
    content_size = len(raw)

    # figure out the status code string given our numeric code
    #
    #       https://www.w3.org/Protocols/rfc2616/rfc2616-sec6.html
    #
    # This is an incomplete list, because we don't use all of the codes.
    # If we need additional codes, we can add them later.
    sc_reason = {
        200: "ok",
        500: "internal server error",
        400: "bad request",
        401: "unauthorized",
        404: "not found"
    }

    # write the status code
    res_header = "HTTP/1.1 %d %s\r\n" % (status_code, sc_reason[status_code])

    await stream.write(res_header.encode())

    # write the headers
    headers = {
        "Content-Type": "application/json",
        "Content-Length": str(content_size),
        "Connection": "close"
    }
    for h_k, h_v in headers.items():
        await stream.write( ("%s: %s\r\n" % (h_k, h_v)).encode())

    await stream.write("\r\n".encode())

    # write the json
    await stream.write(raw)
