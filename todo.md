# In Progress

 - [ ] PR for pinned actuation after verifying that everything works
 - [ ] documentation update for API
  - [ ] How to start everything
   - what are normal errors
   - what should you see if things work?
   - what if things don't work?
  - [ ] installing sensor configurations
  - [ ] interacting with sensors
   - [ ] dockerized-inspect
   - [ ] dockerized-stream
   - [ ] dockerized-run
   - [ ] dockerized-observe ( ./bin/dockerized-run.sh observe --username root --level adversarial)
  - [ ] toggling sensors
  - [ ] installing sensors
  - [ ] defining a target virtue
  - [ ] adding targets with ./bin/add-target.sh
  - [ ] creating a new target
  - [ ] defining a sensor
  - [ ] defining sensor configurations
  - [ ] listing sensor configurations
  - [ ] adding a sensor
  - [ ] developing sensors
   - [ ] update_tools.sh
  - [ ] readme for ./bin/*
 - [ ] Next documentation update
  - [ ] configuring the various services
   - [ ] sensor pruning
   - [ ] kafka
   - [ ] postgres

# Sensing Architecture

 - [ ] stub out BEAM scaling of Sensing API


# Dummy Sensor

 - new sensor: fake ps (elide by process pattern)
 - new sensor: syscall for process list


# infrastructure

 - [ ] Sensing API needs to be resilient to Kafka going away
 - [ ] test deploy on EC2


# CA

 - [ ] document how we could do key delivery with libvmi
 - [ ] stub out CFSSL scaling
 
  
# virtue-security


- virtue-security CLI
  - [ ] add support for all commands and link to routes
  - [ ] move command sets into classes and modules
  - [ ] document the `monitor` action
- [ ] update tests to reflect current architecture and expectations
  - [ ] streaming response error when no sensors match
  - [ ] how to mock a stream when testing?
 - Testing
  - [ ] Parallelize `virtue-security` test mode
  
  
# API SERVER

 - Authentication
  - [ ] fix special case authentication of the registration API by adding client certificate auth plug
  - [ ] fix failing registration tests now that we have registration ping callbacks
 - General API
  - [ ] reject streaming requests for sensors that aren't registered
  - [x] add general "is sensor registered" method to database utils
  - [ ] catch all query params and path params for validation
   - [ ] we need to standardize time. right now the `since` query param is in ISO format, while everything in the Sensing
         API is in UTC common format.
 - Sensors
  - [ ] Integrate toy sensor
   - [ ] Observation level
   - [ ] Trust level
   - [x] Streaming
   - [ ] Inspection
   - [ ] validation
 
# Sensing API Documentation Updates

