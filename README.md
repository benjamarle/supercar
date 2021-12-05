| Supported Targets | ESP32 | 
| ----------------- | ----- | 
# Super car

This is a firmware for ESP32 to control a Power Wheels type kids ride.
It allows you to take control of the car with an XBox bluetooth control and to configure the car through a locally hosted web interface.
It also integrates parking sensors to detect obstacles. It stops the car and reverses it. Your child will have to press the accelerator again to start the car in reverse direction.

### Hardware Required

* A development board with any Espressif SoC ESP32
* A USB cable for Power supply and programming
* A Power wheels car (obviously)
* A dual channel motor driving board to transfer pwm signal into driving signal. I used an MDD10A which is appropriate for the 380 motors I have.
* A relay for the power supply to the motors
* A relay for the sway mode (if you have it)
* Parking sensor (Optional: The brandless one you find everywhere with 4 waterproof sensor)
* Buck converter 12v -> 5v

### XBox controller usage
* Right trigger: accelerate forward
* Left trigger: accelerate backward
* Right d-pad: turn right
* Left d-pad: turn left
* Right bumper: increase speed limit
* Left bumper: decrease speed limit
* Y: toggle control type (remote/local)
* B: reverse the car's direction
* A: toggle mode (sway/motion)


### Remote control mode
When you press on the trigger or on Y the car will go into remote control type. 
This means that the gas pedal will not be active anymore and you will have full control over the car until you press Y again.
You are not limited in speed nor affected by the distance sensor when using the remote control so be careful.
Keep in mind the steering is usually not handled by a servo and there is usually not end switch so do not keep your finger on the steering control because it will stall the motor and peak the current.
### Local control mode
This is the mode where your kid drives the car. As long as you do not touch the triggers or the Y button, your kid keeps control of the car. You can still use the remote to help.
For example you can correct the trajectory of the car with the remote. Or use B to reverse the car's direction. If you do so, it will not correspond to the physical switch anymore until it's flipped again.
And that is the same if you press the A button to change the mode of the car.
By the way, the mode will only switch when the pedal is depressed because the firmware will only switch from the motion motor to the sway motor and vice versa if the throttle is at zero to avoid sudden accelerations.

### Distance sensor
I had a lot of fun designing this feature with a cheap parking sensor kit (~8USD). The esp32 decodes the signal sent to the screen which is a sort of 1-wire protocol with different timing. The RMT feature came in very handy for this. I put two in front and two on the back.
That said, do not expect this to be the perfect obstacle avoidance system. The sensors are not very consistent nor they are reactive. On top of that they are blind when closer than ~25cm to an obstacle so if the threshold you choose for the car to stop is too short, the car might miss it and the car will keep on spinning the wheels against the wall. The stopping distance must also be taken into account in order to determin the threshold.
You can configure this in the web UI.
### Schema

![alt schema](https://github.com/benjamarle/supercar/blob/master/schema/schema.png?raw=true)

### Build and Flash

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.
