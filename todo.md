# In Progress
 
 - [x] wire up inspect route for sensor lookup
  
 - [x] expand inspect with targeting utility to convert targeting data to mnesia lookup
  - [x] build out all of our combos from precedence order
 - [x] pruning appears to be broken...
 - [x] basic logging of targeting and selection scope
 - [x] convert targeting and scope into mnesia query
 - [ ] finish INSPECT
 - [ ] update STREAM with targeting
  - [ ] stream from single
  - [ ] stream from multiple
 - [ ] query mnesia
 - [ ] expand queries based on the scope (see INSPECT below)
 - [ ] how to expand queries to values we don't yet have? (application, resource)
  - [ ] new tables in mnesia (id, sensor, application) (id, sensor, resource)
 - [ ] new values in sync (add/remove tracking of applications and resource monitored)
 - [ ] we can expand querying by Username and Address and Virtue across more of the scopes
 
Inspect returns a list of sensors including
 - Virtue - all virtues in use by user, or limited to a virtue
  - username
  - virtue_id
 - VM - all vms in use by user or at address or support virtue
  - username
  - address
  - virtue_id
 - Application - all applications being used by user, or in a virtue, or specific app
  - username
  - virtue_id
  - application_id
 - Resource - all resources being used by user, or mounted by a virtue, or used by application, or specific resource
  - username
  - resource_id
  - virtue_id
  - application_id
 - User - all sensors observing user
  - username
  
 - [ ] username (in db)
 - [ ] virtue_id (in db)
 - [ ] address (in db) (partial, interpolate address into ipv4 and hostname, and lookup against both)
 - [ ] cidr buh. more complicated. we could binary encode it? i guess? guh. we could use a custom function, but isn't that like a table scan?
 - [ ] application_id (part of register and sync)
 - [ ] resource_id (part of register and sync)
 - [ ] sensor_id (in db)
 
 - document order of priority/precedence when fields exist for targeting
 
# Sensing Architecture


# Dummy Sensor


Punting on CA - brain just not in that space at the end of the week.

 - [ ] CA integration
 - [ ] where is CA?
 
# infrastructure

 - [ ] Sensing API needs to be resilient to Kafka going away

# CA
 
 - [ ] find basic CA tools
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

