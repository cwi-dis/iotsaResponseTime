# iotsaResponseTime - web server to measure Stimulus-Response times

This is a wifi http server that allows triggering a digital signal, and measuring how long it takes for another digital signal to respond. It is built using the iotsa framework.

Home page is <https://github.com/cwi-dis/iotsaResponseTime>.
This software is licensed under the [MIT license](LICENSE.txt) by the   CWI DIS group, <http://www.dis.cwi.nl>.

As distributed the service sends a 3.3v stimulus on GPIO 4 and measures the time until the expected response arrives on GPIO 5.

## Settings

The settings are on the web page _/rtconfig_ and in REST at _/api/rtconfig_:

- `"stimulus"`: `"rise"` (low to high), `"fall"` (high to low) or `"toggle"` (inverting the output).
- `"response"`: the edge expected on the input: `"rise"`, `"fall"`, `"same"` (the same edge as the stimulus) or `"reverse"` (the opposite edge).
- `"duration"`: milliseconds after which a rise or fall goes back to its rest level (low for rise, high for fall). Toggle has no rest level.
- `"timeout"`: milliseconds after which a measurement gives up, 0 to wait forever. The default is 10000.

The device is a bench instrument: everything, settings included, is open to everyone on the network, and the settings need no configuration mode. They are saved.

## Measuring

Each stimulus gets a number, `count`, counting from 1 since boot. A client compares the `count` of its stimulus with the one in the response, so it never mistakes an older measurement, or someone else's, for its own.

- `PUT /api/stimulus` with `{"fire": true}` fires a stimulus and returns its `{"count": n}`. It is refused with 409 while the previous stimulus is still waiting for its response, or while the output is not at its rest level yet (the duration hasn't run out).
- `PUT /api/stimulus` with `{"reset": true}` cancels a waiting measurement and puts the output at its rest level. It is always allowed. `{"reset": true, "fire": true}` does both, in that order.
- `GET /api/stimulus` returns whether a stimulus can fire now (`ready`) and the output level (`output`).
- `GET /api/response` returns the last measurement: its `count` (0: no stimulus yet), its `state` (`waiting`, `done`, `timeout` or `cancelled`), and the `stimulus`, `response`, `duration` and `timeout` it was measured with. When `done`, it has the response time in `millis`, and in `micros` when shorter than 71 minutes (where `micros()` wraps).

The web page shows the last measurement and has Fire and Reset buttons.

Note that response detection is polled from the main loop, not interrupt-driven, so the practical resolution is milliseconds, not microseconds. That was fine for what this was built for (timing experiments on pneumatic logic). A response that is already there when the stimulus fires counts as immediate.

## Software requirements

* PlatformIO (recommended). `pio run` builds every environment in `platformio.ini`.
* Or the Arduino IDE, with the iotsa framework from <https://github.com/cwi-dis/iotsa>.

## Hardware requirements

* a iotsa board (ESP8266). Alternatively any other ESP8266 board, but then you may have to adapt the GPIO pins used.
