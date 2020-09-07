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

# haarlemmerplein
#halts = [ "02133", "02114" ]

# ponts Buiksloterweg, Distelweg, NDSM
halts = [ "02133", "02114", "09906", "09901", "09902" ]
#halts = [  "09902" ]

trips = {}

logfile = open("bustime.log", "a")

def on_message(ws, message):
	try:
		#print(message, file=logfile)
		(t,halt,jsmsg) = json.loads(message)
		if t != 8:
			print("ID", t, "?", message)
			return
		j = json.loads(jsmsg)
	except:
		print("FAIL:", message)
		return

	try:
		print(halt, "=", json.dumps(j, indent=4, sort_keys=True), file=logfile)
		dest = j["journey"]["destination"]
		line = j["journey"]["publicLineNumber"]
		#if line != "18":
			#return
		vehicle = j["journey"]["vehicletype"]
		trip = j["trip"]
		num = trip["number"]
		trip_id = trip["id"]
		lastcall = trip["lastCallMade"]
		call = j["calls"][0]
		delay = call["delay"]
		if delay is None:
			delay = 0
		depart = call["liveDepartureAt"]
		if depart is None:
			depart = call["plannedDepartureAt"] + "(?)"
		halt = call["stopName"]
		status = call["status"]

		if status == "Unknown" or status == "Passed":
			# this trip is no longer interesting
			if trip_id not in trips:
				#print("%s: %s" % (trip_id, status))
				return
			print("%s: DEPARTED" % (num))
			del trips[trip_id]
			return

		trips[trip_id] = call

		# format the delay into +/-mm:ss
		delay_str = "%+3d:%02d" % (delay / 60, abs(delay) % 60)

		#print(json.dumps(j, indent=4, sort_keys=True))
		print("%s: %s %s: %s%s %s -> %s (%s %d stops away)" % (
			num,
			vehicle,
			line,
			depart,
			delay_str,
			halt,
			dest,
			status,
			call["callOrder"] - lastcall["callOrder"]
		))
	except Exception as e:
		print(str(e))
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
