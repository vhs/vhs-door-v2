VHS Door V2
===========

Sends GET requests to a web server whenever the door opens or closes, and parses the JSON response.

Developed with Arduino 1.0.2, and definitely won't work with anything older than 1.0.1, due to use of the Ethernet.maintain() function (there are other applicable bug fixes in 1.0.1 also).

## Circuit ##

- Ethernet shield attached to pins 10, 11, 12, 13
- Door connected to pin 7 and ground (open=floating, closed=held low. Internal pullup is used)


## Required Libraries ##
Requires the aJSON library for Arduino: [http://github.com/interactive-matter/aJson](http://github.com/interactive-matter/aJson)

See the Arduino Libraries Guide here for how to install a library: [http://arduino.cc/en/Guide/Libraries](http://arduino.cc/en/Guide/Libraries)