# In Progress

 3. [x] Determine what needs to happen for new sensors
 4. [ ] Genericize the sensor wrapper #65
  - [x] sensor installer needs to install global sensor_wrapper library
   - [x] define library folder in target.json
  - [x] sensor installer needs to install certificate tools for each sensor
  - define a sensors.json file in a target
   - [x] define which sensors are required
  - sensor install tool
   - [x] sensors are copied into place
   - [x] certs directory created for each sensor
   - [x] sensor run scripts copied into place
    - [x] sensor run scripts are copied into place (each sensor needs to define a sensor run script)
   - [x] requirements files are copied into place
  - [x] define a sensor.json file in a sensor
   - [x] map files to locations in the target sensor folder
   - [x] defines the name of the folder for sensor on target
   - [x] defines the requirements files for script
   - [x] point to the sensor startup script
  - create template Dockerfile that runs any requirements files, spins up any sensor run scripts, installs sensor_wrapper library, etc
   - [ ] install all requirements.txt files
   - [ ] run all run_scripts
   - [ ] install sensor_wrapper library
   - [ ] copy all sensors into /opt/sensors
 5. [ ] Create new sensors #47 #48
 6. [ ] Integrate dropper/malware into instrumented container #64 #66
 7. [ ] script demo 
 8. [ ] Invalidate a sensor? (can we do run time revocation?)
 9. [ ] Add a script for generating and attaching new instrumented hosts during run time (like, add-host name) #67
 
!! - how do we visualize what virtues are active and what they're doing?
    - how are we going to visualize state of the world

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

