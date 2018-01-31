# ATtiny-Theremin
a [theremin](https://en.wikipedia.org/wiki/Theremin) like music instrument based on an ATTiny Microcontroller and HC-SR04 distance sensor

<img src="https://raw.githubusercontent.com/yoook/ATTiny-Theremin/master/Breadboard.JPG" width="300">

a small project to learn how to use the ATtiny (including Timer, PWM, software UART for debugging) and the HC-SR04 ultrasonic distance measurement module.

The Makefile should build out of the box with avr-gcc and transmit the program with avrdude and mySmartUSB light AVR ISP programmer on linux. Of course with small adaptions of the Makefile other setups are possible as well.

See [schema.txt](schema.txt) and the image for details of the setup and the datasheet of the ATtiny and the HC-SR04 module for the programming
