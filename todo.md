# In Progress

0. [x] Use config db ( #115 )
1. [x] migrate sensor db to postgres ( #111 )
 - [x] create schema and migrations
 - [x] create schema methods (changeset, etc)
 - [x] migrate sensor authentication
 - [x] migrate sensor registration
  - [x] ApiServer.ControlUtils.announce_new_sensor/1
  - [x] deregistration
   - [x] ApiServer.ControlUtils.announce_deregistered_sensor/2
  - [x] sync
  - [x] auto-sync check
2. add configuration JSON for all existing sensors ( #112 )
3. [x] add config load command/script ( #113 )
4. add actuation callback to sensor wrapper ( #91 )
5. add actuation routing in API ( #90 )
 1. push new config via observe targeting in API
6. API needs to pin certificates for call to sensor ( #114 )
7. Set observe level via `virtue-security` ( #92 )

8. we need a way to check that any actions in registration match the pubkey used in auth

 - [ ] start planning dom0 sensors

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

