
# Sensing Architecture

 - [ ] Outline sensor/log/streaming architecture
 
# API SERVER

 - [ ] Move all request handling out of StubController
 - [ ] Parallelize `virtue-security` test mode
 - [ ] Migrate non-JSON validating endpoints to return HTTP 200
 - [ ] Write basic JSON validation routines
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