# ATtiny-Theremin
a [theremin](https://en.wikipedia.org/wiki/Theremin) like music instrument based on an ATTiny Microcontroller and HC-SR04 distance sensor

<img src="https://raw.githubusercontent.com/yoook/ATTiny-Theremin/master/Breadboard.JPG" width="300">

a small project to learn how to use the ATtiny (including Timer, PWM, software UART for debugging) and the HC-SR04 ultrasonic distance measurement module.

The Makefile should build out of the box with avr-g++ and transmit the program with avrdude and mySmartUSB light AVR ISP programmer on Linux. Of course with small adaptions of the Makefile other setups are possible as well.

See [schema.txt](schema.txt) and the image (the grey cable is for in system programming, the black cable is for serial communication) for details of the setup and the datasheet of the ATtiny and the HC-SR04 module for the programming

This project includes a small library that implements a software UART for debugging purposes. Debug output can be enabled by defining DEBUG_OUTPUT in main.c. I use a MAX232 based level converter to connect the output pins to the computers COM port. showdata.py can be used to receive and print the output on the computer.

The readout of the ultrasonic sensor is somewhat noisy and one out of three methods for denoising can be selected by uncommenting line 6-8 in [main.cpp](main.cpp) accordingly:
- Channeling Measurement Interpreter, provided by @Necktschnagge (see [https://github.com/Necktschnagge/Fusselsoft-Home-Controller/blob/master/src/Fussl-01/Fussl-01/f_cmi.h](link) for details)
- weighted average averages the current readout and the previous readout with a certain weight
- moving average calculates the (evenly weighted) average on the last few readouts