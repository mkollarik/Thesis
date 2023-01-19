#!/usr/bin/python3.7

#------------------------------------------
#--- Author: Markus Kollarik
#--- Date: 20th May 2022
#--- Version: 1.0
#--- Python Ver: 3.7
#------------------------------------------

import serial
import time
import socket
import sys
import struct
import sqlite3
import os

############################################
#deny multiple instances
####################lock####################

def get_lock(process_name):
    # Without holding a reference to our socket somewhere it gets garbage
    # collected when the function exits
    get_lock._lock_socket = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)

    try:
        # The null byte (\0) means the socket is created 
        # in the abstract namespace instead of being created 
        # on the file system itself.
        # Works only in Linux
        get_lock._lock_socket.bind('\0' + process_name)
        print ('I got the lock')
    except socket.error:
        print ('lock exists')
        sys.exit()

get_lock('running_test')
############################################	

#########open connection to arduino#########
from datetime import datetime

import serial.tools.list_ports
ports = serial.tools.list_ports.comports() #check for connected USB devices
for port, desc, hwid in ports:
		print("{}: {} [{}]".format(port, desc, hwid))

device=[]
desc=[]

for p in ports:
	device.append(p.device)
	desc.append(p.description)
	print (p.description)

if ("ttyACM0" or "ttyACM1") not in desc: #check if Arduino is connected to an USB port
	raise Exception("Arduino not connected")

conn = sqlite3.connect("/home/pi/Store_MQTT_Data_in_Database/IoT.db") #connect to the database
cursor = conn.cursor()

try:
	s = serial.Serial(device[0], 9600)  #connect to Arduino 9600 Baud
	
except:
	raise exception("Something went wrong")

#Arduino resets after establishing a serial connection, wait for reset to complete
time.sleep(5)

############################################

##read serial data and save into database###

try:
	while True:
		if b"receive" in s.readline():
			temp = []
			press = []
			adc = []
			#s.readline().rstrip() #skip receive
			s.readline().rstrip() #first byte represents packetnumber
			for x in range(4):
				temp.append(int(s.readline().strip(b'\r\n')))
			for x in range(4):
				press.append(int(s.readline().rstrip()))
			for x in range(2):
				adc.append(int(s.readline().rstrip()))
				
			#print(temp)
			#print(''.join(map(str,(struct.unpack('<f', bytearray(temp))))))

			date = datetime.now()
			datestamp = date.strftime("%Y-%m-%d %H:%M:%S")
			print(datestamp)
			query = """INSERT INTO ATMEGA8_Data (Date, Temperature, Pressure, ADCValue) VALUES (?,?,?,?)"""
			tuple = (datestamp, 
					''.join(map(str,(struct.unpack('<f', bytearray(temp))))),
					''.join(map(str,(struct.unpack('<l', bytearray(press))))), 
					''.join(map(str,(struct.unpack('<H', bytearray(adc))))))
			cursor.execute(query, tuple)
			conn.commit()
		
		#else:
		#	s.readline().rstrip()

except KeyboardInterrupt:
	s.close()
#	allentries = []
#	cursor.execute('SELECT * FROM ATMEGA8_Data')
#	allentries=cursor.fetchall()
#	print allentries
	cursor.close()
	conn.close()

############################################