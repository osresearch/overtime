#!/usr/bin/env python3
#
# time: seconds since epoch for thi supdate
# trip: high level about the entire trip;
#   lastCallMade: where this specific bus is right now
#     callOrder: which stop number the bus is at
#     status: "Passed" for a traveling bus, "Arrived" for a terminus
#   operatingDate: do we care?
#   id: a unique id for this trip
# journey: somewhat redudant with trip
#   destination: name of final stop
#   publicLineNumber: name of the line
#   vehicletype: Bus or Boat
# calls: array of where this trip is going. for a halt it has 1 entry
#   callOrder: which stop number this is (can compute how many away)
#   delay: minutes? seconds? can be negative?
#   liveDepartureAt: when it is leaving
#   status: "Upcoming", "Arrived" & "Passed" for buses, "Unknown" for ferries
import websocket
import json
import gvbparser
import logging
logging.basicConfig(filename='debug.log',level=logging.DEBUG)

# haarlemmerplein
#halts = [ "02133", "02114" ]

# ponts Buiksloterweg, Distelweg, NDSM
halts = [ "02133", "02114", "09906", "09901", "09902" ]
#halts = [  "09906", "09901", "09902" ]

trips = {}

logfile = open("bustime.log", "a")

def on_message(ws, message):
	gvbparser.parse(message)
def on_error(ws, error):
	print(error)
def on_close(ws):
	print("--- closed ---")
	exit(0)
def on_open(ws):
	print("connected: adding stops")
	for halt in halts:
		ws.send('[5,"/stops/%s"]' % (halt))

ws = websocket.WebSocketApp(
	"wss://maps-wss.gvb.nl/",
	on_message = on_message,
	on_error = on_error,
	on_close = on_close,
)
ws.on_open = on_open
ws.run_forever()
