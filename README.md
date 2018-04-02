# micro-remote
A tiny remote for the Blackmagic Micro Cinema Camera with a large number of functions. See also the project site at https://micro-remote.org.

## Project state
The state of the code on GitHub is currently for the Arduino Micro Pro hardware. There are four directories which contain the code for the mRemote, a mRemote receiver as well as an older project for a remote follow focus.

## The concept
### The old version
While working on the project in the last year there had been some fundamental changes. The primary idea was to built a controller for the Blackmagic Micro using the remote connector to change the parameters in an easy way. By adding a rf module it was possible to separate the controller powered by a battery and connect a separate receiver directly to the camera.
This is working and the code can be found here.

### The new version
To simplify the wiring of the mRemote I changed the hardware to and Adafruit module with an integrated Bluetooth receiver and a colour display as well as a LiIo battery. To use this new mRemote a new receiver, to be plugged in the remote input of the camera, has to be built.

I didn't start the altered code for this version till now. The me code and the hardware specifications and schemes will follow.
