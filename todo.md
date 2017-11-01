
# Sensing Architecture

 - [ ] Outline sensor/log/streaming architecture
 
# API SERVER

 - [x] Move all request handling out of StubController
 - [ ] Migrate non-JSON validating endpoints to return HTTP 200
 - [ ] Write basic JSON validation routines
  - tackling the Migrate and validation controller by controller
   - [ ] Configure Controller
 - [ ] Add tests to check all endpoints without authenticating
 - [ ] add failure cases to all endpoints with targeting
 - [ ] Add fuzzing tests with known bad inputs
 - [ ] Write targeting Plug
  - [ ] Determine target inline to request processing, stash in "target" on connection
  - [ ] break out some methods based on targeting as method guards
 - [ ] write/wire toy dummy monitoring sensor
  - longer term this will be the sensor that testing hits
 - [ ] wire first basic sensor in a VM/container
  - this will be our target sensor for the sprint
  - [ ] basic inline controller (python?)
 - [ ] Integrate toy sensor
  - [ ] Observation level
  - [ ] Trust level
  - [ ] Streaming
  - [ ] Inspection
  - [ ] validation
 - [ ] Parallelize `virtue-security` test mode
 - [ ] client certificates for sensors in api
 - [ ] CA/issuer API
 - [ ] streaming JSONL in elixir
 
 
# Sensing API Documentation Updates

 - [ ] validate/check now returns additional fields (see resource/validate )
 - [ ] validate/trigger now returns additional fields (see resource/validate )