
# Sensor Registration Flow

The current registration flow for a sensor includes contacting the Sensing API
to register and verify the sensor, as well as retrieve configuration information
that is then used when connecting the log output of the sensor to a Kafka instance.

This registration flow will also include the issuance of TLS client certificates
as a precursor to sensor registration.

```
Sensing API                                                      Sensor
===========                                                   =============

                                                            Spawn HTTP Endpoint
															        |																	
Receive Registration     <------------------------------    regsiter with API
       |
Verify Sensor over HTTP   <----------------------------->    HTTP Endpoint Responder
       |
Record Sensor + Issue Config  -------------------------->   Connect to Kafka
                                                                     |
                                                            Instrument Sensor 
															          /---------------\
																	  |               |
																	  \/              |
															Data streaming            |
                                                                                      |
Sync + Re-record Sensor    <----------------------------    Send heartbeat to API     |
                                                                      |               |
																	  |               |
																	  \---------------/

Remove Sensor Record      <-----------------------------    Graceful shutdown

Remove Sensor Record       (sustained missing heartbeat)

```

