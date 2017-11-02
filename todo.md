
# Sensing Architecture

0- [ ] Outline sensor/log/streaming architecture

# virtue-security


6- virtue-security CLI
  - [ ] add support for all commands and link to routes
 
# API SERVER

2. basic readme/getting started for API Server

 - General API
  - [ ] validation plug methods need to catch missing param cases (write tests for this)
  - [ ] inline test cases with exercised routes
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
5 - [ ] inject unauthenticated tests for every test that is normally a 200 response, replace CODE and RESULTS_VALIDATORS
3 - [ ] create fuzzing tests
4 - [ ] add failure cases to all endpoints with targeting
  - [ ] Add fuzzing tests with known bad inputs
  - [ ] streaming JSONL in elixir
   - [ ] create an infinite streaming source for log messages in server, spawn and attach when doing stream/follow
 - Documentation
  - [ ] up and running documentation
  - [ ] versioning
  - [ ] container vs local run
  - [ ] development guide
 
# Sensing API Documentation Updates

 - [ ] validate/check now returns additional fields (see resource/validate )
 - [ ] validate/trigger now returns additional fields (see resource/validate )
 - [ ] inspect now returns additional fields (see application/inspect )
 - [ ] trust routes now return additional fields (see application/trust )
 - [ ] stream routes now return additional fields (see application/stream )