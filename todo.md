# In Progress
 
!! - how do we visualize what virtues are active and what they're doing?
    - how are we going to visualize state of the world
 
# Sensing Architecture

 - [ ] 6 start on CA related issues from GH
 - [ ] 3 stub out Sensing API extensions for certificates
 - [ ] 2 fix authentication routes in Sensing API to use over-lapping scopes and a non-auth-api plug
 - [ ] 4 CA integration with Sensor wrapper
 - [ ] 5 Cert generation for Kafka
 - [ ] 1 Cert generation for Sensing API
 - [ ] sensor deregistration needs to cause certificate revocation
 - [ ] stub out BEAM scaling of Sensing API
 - [ ] stub out CFSSL scaling
 - [ ] how do we get the root public into the trust stores of the other containers?
 - [ ] update tests to reflect current architecture and expectations
  - [ ] streaming response error when no sensors match
  - [ ] how to mock a stream when testing?
- test deploy on EC2


# Dummy Sensor


Punting on CA - brain just not in that space at the end of the week.

 - [ ] CA integration
 - [ ] where is CA?
 
# infrastructure

 - [ ] Sensing API needs to be resilient to Kafka going away

# CA
 
  
# virtue-security


- virtue-security CLI
  - [ ] add support for all commands and link to routes
  - [ ] move command sets into classes and modules
  - [ ] document the `monitor` action
  
# API SERVER

 - Authentication
  - [ ] fix special case authentication of the registration API by adding client certificate auth plug
  - [ ] fix failing registration tests now that we have registration ping callbacks
 - Registration
  - [ ] sensor registration workflow needs to be cleaned up before we have really deep nesting
 - General API
  - [ ] reject streaming requests for sensors that aren't registered
  - [x] add general "is sensor registered" method to database utils
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

