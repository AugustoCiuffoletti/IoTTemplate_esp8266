# Edge component for the IoT template: ESP8266 sketch

This is a sketch for use with the Arduino IDE.
The sketch manages the registration of persistent data in the REDIS database and performs deep-sleep/activity cycles.
Variants are implemented in distinct branches.

- main: the initial registration of the device (PUT) requires the presence of an operator that authorizes the key creation using a
smartphone app (http://ai2.appinventor.mit.edu/#5906520616599552)
- unattended: the initial registration is performed by accessing a secret key, which returns a free key.
