#!/usr/bin/python3.7

#------------------------------------------
#--- Author: Markus Kollarik
#--- Date: 20th May 2022
#--- Version: 1.0
#--- Python Ver: 3.7
#------------------------------------------

import schedule
import time
import socket
import sys
import sqlite3

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

####################mail####################

import smtplib

#Email Variables
SMTP_SERVER = 'smtp.gmail.com' #Email Server (don't change!)
SMTP_PORT = 587 #Server Port (don't change!)
GMAIL_USERNAME = 'name@gmail.com' #change this to match your gmail account
GMAIL_PASSWORD = 'xxx'  #change this to match your gmail app password

class Emailer:
    def sendmail(self, recipient, subject, content):

        #Create Headers
        headers = ["From: " + GMAIL_USERNAME, "Subject: " + subject, "To: " + recipient,
                   "MIME-Version: 1.0", "Content-Type: text/html"]
        headers = "\r\n".join(headers)

        #Connect to Gmail Server
        session = smtplib.SMTP(SMTP_SERVER, SMTP_PORT)
        session.ehlo()
        session.starttls()
        session.ehlo()

        #Login to Gmail
        session.login(GMAIL_USERNAME, GMAIL_PASSWORD)

        #Send Email & Exit
        session.sendmail(GMAIL_USERNAME, recipient, headers + "\r\n\r\n" + content)
        session.quit

sender = Emailer()

sendTo = 'name@student.tuwien.ac.at' #change this to match your desired mailaccount for sensorupdates

emailSubjectD = "Daily Sensor Update - RaspberryPi"
emailSubjectA = "Alert - RaspberryPi"

emailContentD = "Dailyupdate"
emailContentA = "Scheduler started"

#Sends an email to the "sendTo" address with the specified "emailSubject" as the subject and "emailContent" as the email content.
sender.sendmail(sendTo, emailSubjectA, emailContentA)

############################################

#################scheduler##################
timercounter=0

def dailyjob(t): #change function according to your preferences / this function sends an email every day at 06:00 which contains the last read sensor temperature
	print("dailyjob",t)
	con = sqlite3.connect("IoT.db")

	cur = con.cursor()
	
	comp=cur.execute('SELECT Temperature FROM ATMEGA8_Data ORDER BY rowid DESC LIMIT 1')
	for row in comp.fetchone():
		print("Current Temperature "+str(int(float(row)))+" deg C.")	
		emailContentD="Current Temperature "+str(int(float(row)))+" deg C." #change text according to your preferences e.g. calculate mean temperature over night etc.

	#be sure to close the connection
	con.close()
	sender.sendmail(sendTo, emailSubjectD, emailContentD)
	return

def alert():
	global timercounter
	
	print("alert")
	#create a SQL connection to our SQLite database
	con = sqlite3.connect("/home/pi/Store_MQTT_Data_in_Database/IoT.db")

	cur = con.cursor()
	
	comp=cur.execute('SELECT Temperature FROM ATMEGA8_Data ORDER BY rowid DESC LIMIT 1')
	for row in comp.fetchone():
		if(int(float(row))>=21):
			print(int(float(row)))
			print("Check temperature in room xx!")
			if(timercounter):
				timercounter+=1
				if(timercounter>=5): #change value according to your preferences (1^= 1min) / after x minutes send another mail if value still to high
					timercounter=0
			else:
				emailContentA="Check temperature in room xx" #change text according to your preferences
				sender.sendmail(sendTo, emailSubjectA, emailContentA)
				timercounter+=1
			
		else:
			print(int(float(row)))
			print("Everything fine!")
			timercounter=0

	#be sure to close the connection
	con.close()

schedule.every().day.at("06:00").do(dailyjob,'It is 06:00') #change time according to your preferences for daily updates
schedule.every(1).minutes.do(alert) #change time according to your preferences for alerts

while True:
    schedule.run_pending() #run scheduler
    time.sleep(60) #wait one minute

############################################