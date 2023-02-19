# ESP Sunblind's device

This is a part of the entire IoT smart home project. ESP controls sunblinds through a stepper motor *hot glued* to a balcony stick. ESP supports API to configure, reset and info calls.



# Electro parts
|                       |Model                                        |
|-----------------------|---------------------------------------------|
|ESP32                  |                                             |
|Power supply           |9V/1A (ideally a reduction for the connector)|
|Stepper motor          |28BYJ-48                                     |
|Stepper motor driver   |ULN2003                                      |
|2x Resistors           |10k ohms                                     |
|Terminal block         |2 pins for a cable from PSU                  |
|2x Socket bar          |2.54 mm, 19 pins                             |
|Protoboard PCB         |ideally 5x7 cm or bigger                     |
|2x Buttons             |TC-1212T 12x12x7.3mm                         |
|Down stepper           |MINI-360 MP1484                              |

# API
Sunblinds can be closed either facing inside or outside due this fact there is a mechanism which supports both options, open is in the middle of it - 0 value.

There is **maxOpen** and **maxClose** limits which has to be set properly, this has to be done once **after the very first run**,  this prevents to damaging parts and sunblinds - stepper motor will stop rotating when step counter will reach these limits.

## Api calls
Call examples are done in python 3 with requests and json libraries.

### Switch on right rotation
```
json_data = {
	"switch":  "on"
}

req = requests.post(f"http://{ip_address}:{port}/right",  json=json_data)
```

### Switch on left rotation
```
json_data = {
	"switch":  "on"
}

req = requests.post(f"http://{ip_address}:{port}/left",  json=json_data)
```

### Switch off rotation
Doesn't matter if you use right or left in the call ESP handles both equally.
```
json_data = {
	"switch":  "off"
}

req = requests.post(f"http://{ip_address}:{port}/left",  json=json_data)
```

### Set configuration
```
json_data = {
    'stepsPerRevolution': 2048,
    'steppingSpeed': 10,
    'maxOpen': 20000,
    'maxClose': -20000,
    'step': 1,
    'stepsCounter': 0
}

req = requests.post(f"http://{ip_addr}:{port}/configure", json=json_data)
```

### Information of current settings and device status
```
req = requests.get(f"http://{ip_addr}:{port}/info")
status = json.loads(req.content)
print(status )
```
