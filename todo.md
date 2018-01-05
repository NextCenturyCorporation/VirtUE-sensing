# In Progress

 3. [x] Determine what needs to happen for new sensors
 4. [ ] Genericize the sensor wrapper #65
  - create template Dockerfile that runs any requirements files, spins up any sensor run scripts, installs sensor_wrapper library, etc
   - [x] install all requirements.txt files
   - [x] run all run_scripts
   - [x] install sensor_wrapper library
   - [x] copy all sensors into /opt/sensors
   - [x] SENSOR SPECIFIC OS ADDITIONS (like lsof for the lsof-sensor)
   - [x] Make sure everything runs
 5. [ ] Create new sensors #47 #48
  - [ ] dropped PS
  - [ ] system level PS
  - [ ] syscall/kernellog PS
 5.1 [ ] work on hostname discovery in sensor wrapper (coming through as garbage OIDs)
 6. [x] Integrate dropper/malware into instrumented container #64 #66
 7. [ ] script demo 
  - backup screen shots
  - backup video
  - screen shot summary of:
   - differences between two sensors
   - registration process
   - cli calls
  - show which CLI calls are working
  - show reg process
  - show results of adding a new host
 8. [ ] Invalidate a sensor? (can we do run time revocation?) (this is an open issue)
 9. [ ] Add a script for generating and attaching new instrumented hosts during run time (like, add-host name) #67
 10. Simple scripts to show which hosts have which sensors registered
 11. simple script to pull the different PS logs and compare

 
!! - how do we visualize what virtues are active and what they're doing?
    - how are we going to visualize state of the world


 - [ ] start planning dom0 sensors
 - [ ] start planning configuration DB

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

