# In Progress
 
# Sensing Architecture

 - [x] fork letsencrypt/boulder
 - [x] port 4000 conflict
  - [x] move SensingAPI to a different default port 17504 (Unicode LEFT-POINTING MAGNIFYING GLASS is U+1F50D), numeric conversion is 128269, which is too big, so literal transpose it is
  - [x] update everywhere
 - [x] update va.json config
 - [x] update docker-compose
 - [ ] figure out certbot workflow, plan integration with python wrapper
  - [ ] start with just using certbot itself?
 - [x] run CA in separate docker-compose
 - [ ] setup named network for API infrastructure
 - [ ] attach networks for boulder/sensing API
 - [ ] change "FAKE_DNS" to either be real docker based lookups, or externalized lookups
 - [ ] how do we get the root public into the trust stores of the other containers?
 
# Dummy Sensor


Punting on CA - brain just not in that space at the end of the week.

 - [ ] CA integration
 - [ ] where is CA?
 
# infrastructure

 - [ ] Sensing API needs to be resilient to Kafka going away

# CA
 
 - [x] find basic CA tools
 - [ ] root certs
 - [ ] how to generate client/server certs
  
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

