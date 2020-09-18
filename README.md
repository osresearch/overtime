![Image of the public transit tracker on a counter](images/header.jpg)

# OVertime

Track the GVB public transit using their websocket interface and display
the results on an E-Ink screen.  Shows three upcoming events for a given
stop.

To customize it, edit `config.h` to add your WiFi ESSID and password,
and update the halt numbers with the ones from the https://maps.gvb.nl
website.

## Building

* [Adafruit Feather ESP8266](https://www.adafruit.com/product/3046)
* [Adafruit 2.13" monochrome E-Ink screen](https://www.adafruit.com/product/4195)

The ESP8266 is older and less featured than the ESP32, although it was
what I had on hand.  The code should be portable with a few fixes to the
newer architecture.

The E-Ink display works fine, although it doesn't have partial refresh,
so the entire screen flashes when updating.  Not as bad as the tri-color,
but still a bit messy.  The code could be updated for a different display
as well.


