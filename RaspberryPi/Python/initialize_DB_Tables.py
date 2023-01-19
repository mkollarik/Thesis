#!/usr/bin/python3.7

#------------------------------------------
#--- Author: Pradeep Singh
#--- Date: 20th January 2017
#--- Version: 1.0
#--- Python Ver: 2.7
#--- Details At: https://iotbytes.wordpress.com/store-mqtt-data-from-sensors-into-sql-database/
#---
#--- Modified by: Markus Kollarik
#--- Date: 20th May 2022
#------------------------------------------

import sqlite3

# SQLite DB Name
DB_Name =  "IoT.db"

# SQLite DB Table Schema
# Change Table Name and Values according to your preferences
TableSchema="""
drop table if exists ATMEGA8_Data ;
create table ATMEGA8_Data (
  id integer primary key autoincrement,
  Date text,
  Temperature text,
  Pressure text,
  ADCValue text
);
"""

#Connect or Create DB File
conn = sqlite3.connect(DB_Name)
curs = conn.cursor()

#Create Tables
sqlite3.complete_statement(TableSchema)
curs.executescript(TableSchema)

#Close DB
curs.close()
conn.close()
