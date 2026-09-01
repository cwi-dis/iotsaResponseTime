# iotsaResponseTime - web server to measure Stimulus-Response times

This is a wifi http server that allows triggering a digital signal, and measuring how long it takes for another digital signal to respond. It is built using the iotsa framework.

Home page is <https://github.com/cwi-dis/iotsaResponseTime>.
This software is licensed under the [MIT license](LICENSE.txt) by the   CWI DIS group, <http://www.dis.cwi.nl>.

As distributed the service sends a 3.3v trigger signal on GPIO 4. This signal can be configured as `"rise"` (low to high transition), `"fall"` (high to low transition) or `"toggle"` (inverting the output). It then measures the time it takes until the expected response state change is detected on GPIO5. This expected response can be `"rise"`, `"fall"`, `"same"` (same transition as output) or `"reverse"` (reverse transition as output).

Configuration is done manually at _/rtconfig_ or through the REST api at _/api/rtconfig_, which expects a JSON object with three fields:

- `"stimulus"` one of the stimulus strings explained above,
- `"response"` one of the response strings explained above,
- `"duration"` duration in milliseconds after which the output is reset to the other state (mainly for rise or fall stimuli).

A stimulus is generated after a GET access to _/api/stimulus_. This call will fail for rise/fall stimuli for which the _duration_ has not run out yet.

Response time is read with GET access to _/api/response_. If no response has been received yet this returns an empty object. If a response has been received the time difference between stimulus and response is returned in `micros` (in microseconds). Because this value is only recorded in 32 bits a less precise measurement (in milliseconds) is also returned, in `millis`.

Note that response detection is polled from the main loop, not interrupt-driven, so the practical resolution is milliseconds, not microseconds. That was fine for what this was built for (timing experiments on pneumatic logic). A microsecond-grade or statistics-gathering instrument would need a redesign.

## Software requirements

* PlatformIO (recommended). `pio run` builds every environment in `platformio.ini`.
* Or the Arduino IDE, with the iotsa framework from <https://github.com/cwi-dis/iotsa>.

## Hardware requirements

* a iotsa board (ESP8266). Alternatively any other ESP8266 board, but then you may have to adapt the GPIO pins used.
