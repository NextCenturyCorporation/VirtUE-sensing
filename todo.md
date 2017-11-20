
# Sensing Architecture


# Dummy Sensor

5 - [ ] add periodic sensor sync with API (touch timestamp) to keep records alive
  - [x] kafka integration
3 - [ ] add documentation to Sensing API for /sensor/:sensor/stream
  - [ ] handle case of non-existent topics in stream mode
2  - [ ] sensing API needs to broadcast a message on a common Kafka channel when setting up a new sensor
   - [ ] sensor registration needs additional fields
    - [x] sensor name
    - [ ] sensor registration response needs to include default configuration
   - [ ] add a /sensor/:sensor/details route to retrieve specific config/key/stream metadata about the sensor (wait, this is what /inspect is supposed to do)
  - [ ] add simple `monitor` action to virtue-security

Punting on CA - brain just not in that space at the end of the week.

 - [ ] CA integration
 - [ ] where is CA?
 
# infrastructure

 - [ ] Sensing API needs to be resilient to Kafka going away

# CA
 
 - [ ] find basic CA tools
 - [ ] root certs
 - [ ] how to generate client/server certs
  
# virtue-security


- virtue-security CLI
  - [ ] add support for all commands and link to routes
  - [ ] move command sets into classes and modules
  
# API SERVER

 - Authentication
  - [ ] fix special case authentication of the registration API by adding client certificate auth plug
  - [ ] fix failing registration tests now that we have registration ping callbacks
 - General API
  - [ ] catch all query params and path params for validation
   - [ ] we need to standardize time. right now the `since` query param is in ISO format, while everything in the Sensing
         API is in UTC common format.
 - Sensors
  - [ ] wire first basic sensor in a VM/container
   - this will be our target sensor for the sprint
   - [ ] basic inline controller (python?)
  - [ ] Integrate toy sensor
   - [ ] Observation level
   - [ ] Trust level
   - [x] Streaming
   - [ ] Inspection
   - [ ] validation
 - TLS/Encryption
  - [ ] client certificates for sensors in api
  - [ ] CA/issuer API
 - Testing
  - [ ] Parallelize `virtue-security` test mode
 
# Sensing API Documentation Updates

 - [ ] validate/check now returns additional fields (see resource/validate )
 - [ ] validate/trigger now returns additional fields (see resource/validate )
 - [ ] inspect now returns additional fields (see application/inspect )
 - [ ] trust routes now return additional fields (see application/trust )
 - [ ] stream routes now return additional fields (see application/stream )]
 - [ ] sensor registration now includes additional fields (public key, port, hostname, sensor name, etc) (see registration_controller.ex)