
# Sensing Architecture


# Dummy Sensor

 - [ ] http endpoint
  - [x] basic http request cycle
7  - [ ] clean up the http request cycle into a class
3  - [ ] layout what the different actions would do
 - [ ] sensor registration
1  - [x] api endpoint
1.1 - [x] actually write is_public_key validator
2  - [ ] what does a registration do? virtue/user/sensor IDs
    - [x] persistence
	- [x] pub key validation
	- [x] actually register dummy sensor
	- [ ] how do we un/de-register?
	 - [x] as an API call
	 - [ ] as a result of trust actions
	 - [ ] by timeout? - if we haven't seen a heartbeat from a sensor in X minutes/hours/days?
	- [x] !! persist the pub key of the sensor
	- [x] finish dereg in registration controller
	- [x] write deregister in db utils
	- [x] add invalid_deregistration method in registration_controller
	- [x] !! Still getting 500 error when deregistering -> investigate
	- [x] full-cycle ack back to sensor post registration
   - [ ] fix special case authentication of the registration API by adding client certificate auth plug
4 - [ ] kafka integration
5 - [ ] sensor config
6  - [ ] filters as lockable objects, or processed at the kafka emitter
 - [ ] CA integration
 - [ ] where is Sensing API
 - [ ] where is Kafka
 - [ ] where is CA?
8 - [ ] how do we get default config?
 
# infrastructure

 - [ ] setup kafka
 - [ ] spool reader from channel

# CA
 
 - [ ] find basic CA tools
 - [ ] root certs
 - [ ] how to generate client/server certs
  
# virtue-security


- virtue-security CLI
  - [ ] add support for all commands and link to routes
  - [ ] move command sets into classes and modules
  
# API SERVER

 - General API
  - [ ] catch all query params and path params for validation
   - [ ] we need to standardize time. right now the `since` query param is in ISO format, while everything in the Sensing
         API is in UTC common format.
 - Sensors
  - [ ] write/wire toy dummy monitoring sensor
   - [ ] longer term this will be the sensor that testing hits
  - [ ] wire first basic sensor in a VM/container
   - this will be our target sensor for the sprint
   - [ ] basic inline controller (python?)
  - [ ] Integrate toy sensor
   - [ ] Observation level
   - [ ] Trust level
   - [ ] Streaming
   - [ ] Inspection
   - [ ] validation
 - TLS/Encryption
  - [ ] client certificates for sensors in api
  - [ ] CA/issuer API
 - Testing
  - [ ] Parallelize `virtue-security` test mode
  - [ ] Add fuzzing tests with known bad inputs
  - [ ] streaming JSONL in elixir
   - [ ] create an infinite streaming source for log messages in server, spawn and attach when doing stream/follow
 
# Sensing API Documentation Updates

 - [ ] validate/check now returns additional fields (see resource/validate )
 - [ ] validate/trigger now returns additional fields (see resource/validate )
 - [ ] inspect now returns additional fields (see application/inspect )
 - [ ] trust routes now return additional fields (see application/trust )
 - [ ] stream routes now return additional fields (see application/stream )