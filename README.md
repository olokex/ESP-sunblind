# ESP Sunblind's Device

This is part of an entire IoT smart home project. The ESP controls sunblinds through a stepper motor hot glued to a balcony stick. For API calls, see the [API Calls Overview](#api-calls-overview) section. This project also supports OTA updates.


# Electro Parts
|                       | Model                                      |
|-----------------------|--------------------------------------------|
| ESP32                 |                                            |
| Power supply          | 9V/1A (ideally with a reduction for the connector) |
| Stepper motor         | 28BYJ-48                                   |
| Stepper motor driver  | ULN2003                                    |
| 2x Resistors          | 10k ohms                                   |
| Terminal block        | 2 pins for a cable from PSU                |
| 2x Socket bar         | 2.54 mm, 19 pins                           |
| Protoboard PCB        | Ideally 5x7 cm or larger                   |
| 2x Buttons            | TC-1212T 12x12x7.3mm                       |
| Down stepper          | MINI-360 MP1484 (set to 5V to power the ESP) |

The motor is powered by 9V, and a down stepper is used to drop the voltage to 5V, which powers up the ESP. From the ESP, 3.3V is used for button inputs. Avoid using 5V for inputs as the ESP does **NOT** have protection.

# API
The sunblinds can be closed either facing inside or outside; due to this, there is a mechanism that supports both options. The open position is at the 0 value.

There are **maxOpen** and **maxClose** limits that must be set properly after the very first run. This prevents damage to parts and sunblinds—the stepper motor will stop rotating when the step counter reaches these limits.

## API Calls
Examples of calls are made in Python 3 using the requests and json libraries.

### Switch on right rotation
```python
json_data = {
	"switch":  "on"
}

req = requests.post(f"http://{ip_address}:{port}/right",  json=json_data)
```

### Switch on left rotation
```python
json_data = {
	"switch":  "on"
}

req = requests.post(f"http://{ip_address}:{port}/left",  json=json_data)
```

### Switch off rotation
It doesn't matter if you use right or left in the call; the ESP handles both equally.
```python
json_data = {
	"switch":  "off"
}

req = requests.post(f"http://{ip_address}:{port}/left",  json=json_data)
```

### Set configuration
```python
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

### Information on Current Settings and Device Status
```python
req = requests.get(f"http://{ip_addr}:{port}/info")
status = json.loads(req.content)
print(status )
```

## ⚠️ Be Aware ⚠️
Boundaries must be set properly; otherwise, these API calls might be harmful to the sunblinds. There is **NO** sensor to check their position.

### Close to left fully
Opens sunblinds to the maxOpen boundary.
```python
req = requests.post(f"http://{ip_addr}:{port}/close/left")
```

### Close to right fully
Opens sunblinds to the maxClose boundary.
```python
req = requests.post(f"http://{ip_addr}:{port}/close/right")
```

### Open fully
Opens sunblinds to the 0 position.
```python
req = requests.post(f"http://{ip_addr}:{port}/open")
```

# API Calls Overview

| API Call            | Method | Endpoint                 | Payload Example                                 | Description                                      |
|---------------------|--------|--------------------------|-------------------------------------------------|--------------------------------------------------|
| Switch on Right     | POST   | `/right`                 | `{ "switch": "on" }`                            | Activates the sunblind to rotate to the right.   |
| Switch on Left      | POST   | `/left`                  | `{ "switch": "on" }`                            | Activates the sunblind to rotate to the left.    |
| Switch Off Rotation | POST   | `/left` or `/right`      | `{ "switch": "off" }`                           | Stops any ongoing rotation.                      |
| Set Configuration   | POST   | `/configure`             | `{ "stepsPerRevolution": 2048, "steppingSpeed": 10, "maxOpen": 20000, "maxClose": -20000, "step": 1, "stepsCounter": 0 }` | Configures various parameters of the sunblind.  |
| Get Device Info     | GET    | `/info`                  | N/A                                             | Retrieves the current settings and device status.|
| Close to Left Fully | POST   | `/close/left`            | N/A                                             | Closes the sunblinds to the maximum left position.|
| Close to Right Fully| POST   | `/close/right`           | N/A                                             | Closes the sunblinds to the maximum right position.|
| Open Fully          | POST   | `/open`                  | N/A                                             | Opens the sunblinds to the neutral (0) position.  |
