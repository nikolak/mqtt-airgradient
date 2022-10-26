Code for running DIY AirGradient sensor. Modified from [this original arduino code](https://github.com/airgradienthq/arduino/blob/d5c8af68a0d2b327c147e3f81c0da9141e1d1d95/examples/DIY_PRO/DIY_PRO.ino)


Differences compared to original code:

- Pushes data to mqtt server instead of arigradient
- Turns off display during the night
- Properly rotates the display (at least my unit had display rotated 180 degrees, this is easily adjustible)
- Add human readable AQI value
- PlatformIO/cpp support instead of Arduino IDE/ino format.

Clone repo, modify `src/config.h.example` with your values and rename it to `config.h`, install dependencies, and build/deploy `main.cpp`

MIT License

Copyright (c) 2022 Nikola Kovacevic

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.