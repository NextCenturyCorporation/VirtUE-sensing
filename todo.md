# In Progress
 
!! - how do we visualize what virtues are active and what they're doing?
    - how are we going to visualize state of the world
 
 Friday:  client certs everywhere & cert pinning?
  - [ ] sensors talking to Sensing API
  - [x] sensors talking to Kafka
  - [x] Sensing API talking to Kafka
  - [ ] Sensing API talking to Sensor
  - [ ] virtue-security talking to Sensing API
  - [ ] virtue-security talking to Kafka
  
 
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

