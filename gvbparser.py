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

# haarlemmerplein
#halts = [ "02133", "02114" ]

# ponts Buiksloterweg, Distelweg, NDSM
halts = [ "02133", "02114", "09906", "09901", "09902" ]
#halts = [  "09906", "09901", "09902" ]

trips = {}

import json

def parse(message):
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
		if __debug__: print(halt+ "="+ json.dumps(j, indent=4, sort_keys=True))
		dest = j["journey"]["destination"]
		if dest == "Centraal Station":
			dest = "CS"
		elif dest == "Station Sloterdijk":
			dest = "Sloterdijk"
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

		# format the delay into +/-mm:ss
		delay_str = "%+3d:%02d" % (delay / 60, abs(delay) % 60)
		call["stopsAway"] = call["callOrder"] - lastcall["callOrder"]

		if status == "Unknown":
			status = "Passed"
		if status == "Upcoming" and call["stopsAway"] != 0:
			status = "%d stops away" % (call["stopsAway"])

		if trip_id in trips:
			# skip printing if nothing has changed
			old_call = trips[trip_id]

			if  old_call is not None \
			and old_call["delay"] == call["delay"] \
			and old_call["status"] == call["status"] \
			and old_call["stopsAway"] == call["stopsAway"] \
			and True:
				return
		elif status == "Passed":
			# never heard of this one, and it is already gone...
			return

		#print(json.dumps(j, indent=4, sort_keys=True))
		print("%s: %-8s: %s%s %s -> %s (%s)" % (
			num,
			vehicle + " " + line,
			depart,
			delay_str,
			halt,
			dest,
			status,
		))

		if status != "Passed":
			trips[trip_id] = call
		elif trip_id in trips:
			del trips[trip_id]

	except Exception as e:
		print(e)
		print("FAIL:", json.dumps(j, indent=4, sort_keys=True))
		return
