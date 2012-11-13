VHS Door V2
===========

Circuit:

- Ethernet shield attached to pins 10, 11, 12, 13
- Door connected to pin 7 and ground (open=floating, closed=held low. Internal pullup is used)

Sends GET requests to a web server whenever the door opens or closes, and parses the JSON response.
