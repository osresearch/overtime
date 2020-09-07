#!/usr/bin/env python3
import websocket
import json

# haarlemmerplein
#halts = [ "02133", "02114" ]

# ponts Buiksloterweg, Distelweg, NDSM
halts = [ "02133", "02114", "09906", "09901", "09902" ]

trips = {}

def on_message(ws, message):
	try:
		(t,halt,jsmsg) = json.loads(message)
		if t != 8:
			print("ID", t, "?", message)
			return
		j = json.loads(jsmsg)
	except:
		print("FAIL:", message)
		return

	try:
		#print(halt, "=", json.dumps(j, indent=4, sort_keys=True))
		dest = j["journey"]["destination"]
		line = j["journey"]["publicLineNumber"]
		vehicle = j["journey"]["vehicletype"]
		trip = j["trip"]
		num = trip["number"]
		trip_id = trip["id"]
		call = trip["lastCallMade"]
		delay = call["delay"]
		if delay is None:
			delay = 0
		depart = call["liveDepartureAt"]
		if depart is None:
			depart = call["plannedDepartureAt"] + "(?)"
		halt = call["stopName"]
		status = call["status"]

		if status == "Uknown":
			# this trip is no longer interesting
			if trip_id not in trips:
				return
			print("%s: DEPARTED" % (num))
			del trips[trip_id]
			return

		print(json.dumps(j, indent=4, sort_keys=True))
		print("%s: %s %s: %s -> %s %s + %d" % (num, vehicle, line, halt, dest, depart, delay))
	except Exception as e:
		print(e)
		print("FAIL:", json.dumps(j, indent=4, sort_keys=True))
		return

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
