#!/usr/bin/python3
 
# receive data from com port and print it to the terminal.
# usefull for debugging of microcontroller by serial communication
# you might need to have root privileges to open the com port


import serial
import time
import os
 

blockSize = 2 #number of bytes belonging together

# configure serial port:
ser = serial.Serial()

#  insert your port here!!!
ser.port = '/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller_D-if00-port0'
ser.baudrate = 9600
ser.timeout = 1
 
# open port
ser.open()
print("port opened")
 
# read data
try:
	while 1:
		# read <blockSize> bytes
		bytes = ser.read(blockSize)
		string = ""
		for byte in bytes:
			string += hex(byte)
			string += " "

		print(string)
		
		
# catch keyboard interrupt, to terminate with [CTRL]+[C].
except(KeyboardInterrupt, SystemExit):
	print("\nterminating:")
	ser.close()
	print("port closed")
	print("\nEND")