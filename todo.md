
# Sensing Architecture


# Dummy Sensor

 - [ ] http endpoint
  - [x] basic http request cycle
3  - [ ] layout what the different actions would do
  !- [ ] add periodic sensor sync with API (touch timestamp) to keep records alive
  !- [ ] geborked testing with sensor registration/deregistration
4 - [ ] kafka integration
   - [ ] write message formatter for log data coming from sensor
   - [ ] ? destroy topics after sensor deregistration?
   - [ ] connect stream/* actions to kafka
    - [ ] configure Sensing API to connect to kafka
    - [ ] spin up new worker
	- [ ] batch process messages 
   - [ ] write a simple serializer from kafka channels to JSONL on disk
    - [ ] what should this be written in? Elixir? Java?
	- [ ] or just write a simple monitor
	 - [ ] active topics
	 - [ ] message sizes/counts
   - [ ] sensing API needs a way to find active sensors beyond just basic targeting
    - [ ] user/sensor enumeration
   - [ ] sensor registration needs additional fields
    - [ ] sensor name
   - [ ] sensor registration needs to include default configuration


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

 - Authentication
  - [ ] fix special case authentication of the registration API by adding client certificate auth plug
  - [ ] fix failing registration tests now that we have registration ping callbacks
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