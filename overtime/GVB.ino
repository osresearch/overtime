/*
 * GVB message parser
 */
#include <ArduinoJson.h>
#include "time.h"
#include "GVB.h"
#include "config.h"

// GVB websocket address
static const char ws_url[] = "wss://maps-wss.gvb.nl";
static const char * const ws_queries[] = { GVB_HALTS };
static const int ws_num_queries = sizeof(ws_queries)/sizeof(*ws_queries);

static unsigned long last_ping;
static unsigned long last_pong;
#define ping_interval 10000
#define pong_timeout 15000


train_t * train_list;

train_t * train_find(int id)
{
	train_t * t = train_list;
	while(t)
	{
		if(t->id == id)
			return t;

		t = t->next;
	}

	return NULL;
}


void train_remove(train_t *t)
{
	if (t->next)
		t->next->prev = t->prev;
	*(t->prev) = t->next;

	t->next = NULL;
	t->prev = NULL;
}

void train_clear()
{
	train_t * t = train_list;
	while(t)
	{
		train_t * n = t->next;
		free(t);
		t = n;
	}

	train_list = NULL;
}

train_t * train_create(int id)
{
	train_t * t = (train_t*) calloc(1, sizeof(*t));
	t->id = id;
	t->next = NULL;
	t->prev = NULL;
	t->departure = 0;
	t->delay_sec = 0;
	t->status = '?';
	t->type = '?';

	return t;
}


void train_insert(train_t * nt)
{
	train_t **t = &train_list;
	while(*t)
	{
		if((*t)->departure > nt->departure)
			break;

		t = &(*t)->next;
	}

	// found our spot or the list is empty
	nt->next = *t;
	nt->prev = t;
	*t = nt;
	if (nt->next)
		nt->next->prev = &nt->next;
}


static int gvb_parse(String payload)
{
	//Serial.println(payload);

	//StaticJsonBuffer<1024*2> jsonBuffer;
	DynamicJsonDocument doc(4096);

	if (deserializeJson(doc, payload) != 0)
	{
		Serial.println("Unable to create a root object");
		return 0;
	}
	//Serial.println("parsed!");

	// The format is '[id,url,details]', where details is a string
	// that packs another json object.
        if (!doc.is<JsonArray>())
	{
		Serial.println("payload is not a json array");
		return 0;
	}
	//Serial.println("array!");

	const char * trip_str = doc[2];
	//Serial.println(trip_str);
/*
	if (trip_str == "1")
	{
		Serial.println("handshake done");
		doc.clear();
		return;
	}
*/

	if (deserializeJson(doc, trip_str) != 0)
	{
		Serial.println("Unable to parse trip object");
		doc.clear();
		return 0;
	}

	// see response.txt for sample object
	//JsonObject journey = doc["journey"];
	const char * line_number = doc["journey"]["publicLineNumber"];
	const char * destination = doc["journey"]["destination"];
	const char * vehicle = doc["journey"]["vehicletype"];

	// should use "id" : "gvb:3:zkg:243:2017-12-16",
	// but for now using the short number
	const int trip_id = doc["trip"]["number"];
	const char * date = doc["trip"]["operatingDate"];

	JsonObject departure = doc["calls"][0];
	int delay_sec = 0;
	const char * departure_time = departure["liveDepartureAt"];
	const char * stop = departure["stopName"];

	// if there is no live departure time, don't fetch the delay
	// and instead just fetch the planned time
	if(departure_time)
	{
		delay_sec = departure["delay"];
	} else {
		departure_time = departure["plannedDepartureAt"];
	}

	if (!departure_time)
		departure_time = "N/A";

	const char * status = departure["status"];
	if (strcmp(status, "Unknown") == 0)
		status = "Passed";

	time_t now = time(NULL);
	struct tm *tm = localtime(&now);
/*
	strptime(date, "%Y-%m-%d", &tm);
	strptime(departure_time, "%H-%M-%S", &tm);
*/
	sscanf(date, "%d-%d-%d", &tm->tm_year, &tm->tm_mon, &tm->tm_mday);
	sscanf(departure_time, "%d:%d:%d", &tm->tm_hour, &tm->tm_min, &tm->tm_sec);
	tm->tm_year -= 1900; // years since 1900
	tm->tm_mon -= 1; // months go from 0 - 11
	time_t at = mktime(tm);

	printf("%4d %8s %s %+4d: %s %-3s %s %d\r\n",
		trip_id,
		status,
		departure_time,
		delay_sec,
		vehicle,
		line_number,
		destination,
		at
	);

	if (departure_time[0] == 'N')
	{
		// we can't parse a nonexistant time
		// or this was way in the past
		Serial.print(payload);
		Serial.println("'");
		return 0;
	}

	// figure out where to insert this into our list

	train_t * t = train_find(trip_id);
	if (t)
	{
		// this train already exists in the list;
		// remove it from the list so that we can resort it
		train_remove(t);
	} else {
		// new train! do we care?
		if (status[0] == 'P')
		{
			// "Passed": we have never heard of this train,
			// so let's not bother adding it
			return 0;
		}

		t = train_create(trip_id);
		t->type = strcmp(vehicle, "Boat") == 0 ? 'F' :
			strcmp(vehicle, "Bus") == 0 ? 'B' :
			strcmp(vehicle, "Tram") == 0 ? 'T' :
			'?';

		// fix up some long names
		if (strcmp(destination, "Centraal Station") == 0)
			destination = "Centraal";
		if (strcmp(stop, "Centraal Station") == 0)
			stop = "Centraal";

		strlcpy(t->destination, destination, sizeof(t->destination));
		strlcpy(t->line_number, line_number, sizeof(t->line_number));
		strlcpy(t->departure_time, departure_time, sizeof(t->departure_time));
		strlcpy(t->stop, stop, sizeof(t->stop));
	}

	if (status[0] == 'P')
	{
		// "Passed": we've already removed it from the list
		// so we release the trip
		free(t);
		return 0;
	} else
	if (status[0] == 'A' || status[0] == 'U')
	{
		// "Arriving", "Upcoming" or "Unknown"
		// we'll update the departure time
	} else {
		printf("%d: Unknown status: '%s'\r\n",
			trip_id,
			status
		);
	}
 
	// update the departure time
	t->status = status[0];
	t->departure = at;
	t->delay_sec = delay_sec;

	// re-insert it into the sorted list
	train_insert(t);

	doc.clear();

	return 1;
}

static int got_message;

static void ws_message(WebsocketsMessage message)
{
	String payload = message.data();
	gvb_parse(payload);
	got_message = 1;
}

static void gvb_subscribe()
{
	char query[32];
	for(int i = 0 ; i < ws_num_queries ; i++)
	{
		snprintf(query, sizeof(query),
			"[5,\"/stops/%s\"]",
			ws_queries[i]
		);
		//Serial.println(query);

		ws.send(query);
	}
}

static void ws_event(WebsocketsEvent event, String data)
{
	if (event == WebsocketsEvent::ConnectionOpened) {
		Serial.println("Connnection Opened");
	} else if(event == WebsocketsEvent::ConnectionClosed) {
		Serial.println("Connnection Closed");
	} else if(event == WebsocketsEvent::GotPing) {
		Serial.println("Got a Ping!");
	} else if(event == WebsocketsEvent::GotPong) {
		//Serial.println("Got a Pong!");
		last_pong = millis();
	} else {
		printf("ws: event=%d str='%s'\r\n", (int) event, data.c_str());
	}
}


void gvb_setup(void)
{
	// avoid re-triggering on the pong timeout
	last_pong = millis();

	// throw away any state that we have
	train_clear();

	Serial.print("attempting connection: ");
	Serial.println(ws_url);
	ws.connect(ws_url);
	ws.onMessage(ws_message);
	ws.onEvent(ws_event);
	//ws.setReconnectInterval(query_interval_ms);
}

 
int gvb_loop()
{
	// every so often send a ping to ensure liveliness
	const unsigned long now_ms = millis();

	if (now_ms - last_ping > ping_interval)
	{
		ws.ping();
		last_ping = now_ms;

		// also resubscribe, just in case
		gvb_subscribe();
	}

	if (now_ms - last_pong > pong_timeout)
	{
		// connection has gone down? try to restart the
		// websocket connection
		gvb_setup();
	}

	got_message = 0;
	ws.poll();
	return got_message;
}
