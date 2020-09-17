#include <ArduinoWebsockets.h> // 0.4.8
#include <ESP8266WiFi.h>
#include "time.h"
//#include <Wire.h>
//#include <SSD1306.h>
//#include <OLEDDisplayUi.h>
#include "Adafruit_EPD.h" // 2.5.3
#include "Adafruit_GFX.h" // 1.10.1
#include "GVB.h"
#include "config.h"

//#include <Fonts/FreeMonoBold18pt7b.h>
//#include <Fonts/FreeSans12pt7b.h>
#include "DinFont60pt7b.h"
//#include "DinFont36p7.h"
#include "DinFont12pt7b.h"

#ifdef ESP8266
#define SD_CS 2
#define SRAM_CS 16
#define EPD_CS 0
#define EPD_DC 15
#endif

#define EPD_RESET   -1 // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY    -1 // can set to -1 to not use a pin (will wait a fixed delay)
#define COLOR1 EPD_BLACK
#define COLOR2 EPD_RED

/* Uncomment the following line if you are using 2.13" monochrome 250*122 EPD */
Adafruit_SSD1675 display(250, 122, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);


extern const char Lato_Bold_21[];

using namespace websockets;
WebsocketsClient ws;

#define TZ_OFFSET (1 * 3600)

// query parameters
//const unsigned long query_interval_ms = 60 * 1000; // once per minute
const unsigned long query_interval_ms = 10 * 1000; // debug
unsigned long last_query_ms;
time_t last_update_sec;


#if 0
void
draw_train(
	train_t *t,
	time_t now,
	OLEDDisplay *display,
	OLEDDisplayUiState* state,
	int16_t x,
	int16_t y
)
{
	char buf[32];

	if (t->status == 'A')
	{
		// make it inverse video for this train
		display->fillRect(x,y+1, 120, 21);
		display->setColor(INVERSE);
	}

	//display->setFont(ArialMT_Plain_16); // 24 almost works
	if (strlen(t->line_number) >= 3)
		display->setFont(ArialMT_Plain_16);
	else
		display->setFont(Lato_Bold_21);
	display->setTextAlignment(TEXT_ALIGN_RIGHT);
	display->drawString(x+25, y, t->line_number);

	display->setFont(ArialMT_Plain_10);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(x+30, y, t->destination);

	int delta = t->departure - now - TZ_OFFSET; // fix for UTC to NL
	if (delta < -120)
	{
		// we seem to have forgotten about this one
		train_remove(t);
		free(t);
		return;
	}

	char neg = ' ';
	if (delta < 0)
	{
		neg = '-';
		delta = -delta;
	}

	snprintf(buf, sizeof(buf), "%c%2d:%02d",
		neg,
		delta / 60,
		delta % 60
	);

	if (t->status == 'A')
	{
		display->drawString(x+30, y+10, "ARRIVING");
	} else {
		display->drawString(x+30, y+10, buf);
	}

	if (t->delay_sec != 0)
	{
		snprintf(buf, sizeof(buf), "%+4d",
			t->delay_sec
		);
		display->setTextAlignment(TEXT_ALIGN_RIGHT);
		display->drawString(x+110, y+10, buf);
	}

	// always set the display back to normal
	display->setColor(WHITE);
}

void draw_indicator(OLEDDisplay *display, int i)
{
	const int frameCount = 5;
	//display->drawLine(i*64/frameCount, 63, (i+1)*64/frameCount, 63);
}

void draw_frame(int start, OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
	time_t now = time(NULL);

	// round now to the nearest few seconds
	now = (now / 4) * 4;

	train_t * t = train_list;

	// throw away up to the starting train
	for(int i = 0 ; t && i < start ; i++, t = t->next)
		;

	// and draw the next one (side slide)
	for(int i = 0 ; t && i < 1 ; i++)
	{
		train_t * nt = t->next; // t might be deleted
		draw_train(t, now, display, state, x, y + i*21);
		t = nt;
	}
}

void draw_frame0(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
	draw_frame(2, display, state, x, y + 2*21);
	draw_indicator(display, 1);
}

void draw_frame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
	draw_frame(3, display, state, x, y + 2*21);
	draw_indicator(display, 2);
}

void draw_frame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
	draw_frame(4, display, state, x, y + 2*21);
	draw_indicator(display, 3);
}

void draw_frame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
	draw_frame(5, display, state, x, y + 2*21);
	draw_indicator(display, 4);
}

void show_time(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
	// only show the time for a second
	static unsigned long start_millis;

	if (start_millis == 0)
		start_millis = millis();
	else
	if (millis() - start_millis > 1500
	&& last_update_sec != 0)
	{
		start_millis = 0;
		ui.nextFrame();
	}

	draw_indicator(display, 0);

	// just the time
	struct tm tm;
	if(!getLocalTime(&tm))
	{
		Serial.println("Failed to obtain time");
		return;
	}

	display->setTextAlignment(TEXT_ALIGN_CENTER);
	display->setFont(ArialMT_Plain_16);

	char buf[32];

	snprintf(buf, sizeof(buf),
		"%02d:%02d:%02d",
		tm.tm_hour + 1,
		tm.tm_min,
		tm.tm_sec
	);

	display->setFont(ArialMT_Plain_24);
	display->drawString(x+64-8, y+2*21, buf);


	if(last_update_sec == 0)
	{
		// if we don't have any updates, display
		// the date and time
		snprintf(buf, sizeof(buf),
			"%04d-%02d-%02d",
			tm.tm_year+1900,
			tm.tm_mon+1,
			tm.tm_mday
		);

		display->setFont(ArialMT_Plain_16);
		display->drawString(x+64-8, y+24, buf);

		display->setFont(ArialMT_Plain_10);
		display->setTextAlignment(TEXT_ALIGN_CENTER);
		display->drawString(x+64-8, y+4, WiFi.localIP().toString());
	} else {
		int delta = time(NULL) - last_update_sec;
		snprintf(buf, sizeof(buf), "%d", delta);

		display->setFont(ArialMT_Plain_10);
		display->setTextAlignment(TEXT_ALIGN_LEFT);
		if (delta > 10)
			display->drawString(x, y+54, buf);

		// two minutes without a packet, force it
		if (delta > 120)
		{
			last_update_sec = 0;
			ws.disconnect();
			ws.connect(ws_url);
		}
	}
}


// draw the top two trains
void draw_two_trains(OLEDDisplay *display, OLEDDisplayUiState* state)
{
	draw_frame(0, display, state, 0, 0);
	draw_frame(1, display, state, 0, 21);
}

// This array keeps function pointers to all frames
// frames are the single views that slide in
//FrameCallback frames[] = { show_time, draw_frame0, draw_frame1, };
FrameCallback frames[] = {
	show_time,
	draw_frame0,
	draw_frame1,
	draw_frame2,
	draw_frame3,
};

// how many frames are there?
const int frameCount = sizeof(frames) / sizeof(*frames);

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { draw_two_trains, };
const int overlaysCount = sizeof(overlays) / sizeof(*overlays);
#endif

int Start_WiFi(const char* ssid, const char* password)
{
	int connAttempts = 0;
	Serial.println("\r\nConnecting to: "+String(ssid));
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED ) {
		delay(500);
		Serial.print(".");
		if(connAttempts > 20)
			return -5;
		connAttempts++;
	}

	Serial.println("WiFi connected\r\n");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	display.setCursor(0, 24);
	display.setTextWrap(true);
	//display.setTextSize(3);
        display.print(ssid);
	display.print(": ");
	display.print(WiFi.localIP());
        display.display();

	return 1;
}
 

void setup() { 
  Serial.begin(115200);
#if 0
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high

  ui.setTargetFPS(30);
  ui.setIndicatorPosition(RIGHT);
  ui.setIndicatorDirection(LEFT_RIGHT);
  //ui.disableAllIndicators();

  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, frameCount);
  ui.setOverlays(overlays, overlaysCount);
  ui.init();
  ui.setTimePerTransition(500);
  ui.setTimePerFrame(1500);

  display.flipScreenVertically();
#endif

  display.begin();
  display.setRotation(0);
  display.clearBuffer();

  display.setCursor(0, 0);
  display.setTextColor(COLOR1);
  display.setTextWrap(true);
  display.print("OVertime starting");
  display.display();

  Start_WiFi(WIFI_SSID, WIFI_PASSWORD);

  configTime(1, 3600, "pool.ntp.org");

  gvb_setup();
}

/* 250x122 => 83 wide per entry
   BIG sec  BIG sec  BIG sec
   MIN      MIN      MIN sec
   Dest     Dest     Dest
*/
static const uint8_t icon_train[] = {
// 'train_black_24x24', 24x24px
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0x80, 0x07, 0xff, 0xe0, 0x0f, 0xff, 0xf0, 0x0f, 
0xff, 0xf0, 0x0c, 0x18, 0x30, 0x0c, 0x18, 0x30, 0x0c, 0x18, 0x30, 0x0c, 0x18, 0x30, 0x0f, 0xff, 
0xf0, 0x0f, 0xff, 0xf0, 0x0f, 0xff, 0xf0, 0x0f, 0xff, 0xf0, 0x0c, 0x7e, 0x30, 0x0c, 0x7e, 0x30, 
0x0c, 0x7e, 0x30, 0x07, 0xff, 0xe0, 0x03, 0xff, 0xc0, 0x01, 0xc3, 0x80, 0x03, 0x81, 0xc0, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  };
static const uint8_t icon_bus[] = {
// 'directions_bus_black_24x24', 24x24px
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0x80, 0x07, 0xff, 0xe0, 0x0f, 0xff, 0xf0, 0x0f, 
0xff, 0xf0, 0x0c, 0x00, 0x30, 0x0c, 0x00, 0x30, 0x0c, 0x00, 0x30, 0x0c, 0x00, 0x30, 0x0c, 0x00, 
0x30, 0x0f, 0xff, 0xf0, 0x0f, 0xff, 0xf0, 0x0f, 0xff, 0xf0, 0x0c, 0x7e, 0x30, 0x0c, 0x7e, 0x30, 
0x0c, 0x7e, 0x30, 0x0f, 0xff, 0xf0, 0x07, 0xff, 0xe0, 0x07, 0x00, 0xe0, 0x07, 0x00, 0xe0, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  };
static const uint8_t icon_ferry[] = {
// 'directions_boat_black_24x24', 24x24px
0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x7e, 0x00, 0x07, 0xff, 0xe0, 0x0f, 
0xff, 0xf0, 0x0c, 0x00, 0x30, 0x0c, 0x00, 0x30, 0x0c, 0x3c, 0x30, 0x0d, 0xff, 0xb0, 0x0f, 0xff, 
0xf0, 0x3f, 0xff, 0xfc, 0x3f, 0xff, 0xfc, 0x3f, 0xff, 0xfc, 0x1f, 0xff, 0xf8, 0x1f, 0xff, 0xf8, 
0x1f, 0xff, 0xf8, 0x0e, 0x7e, 0x70, 0x0c, 0x3c, 0x30, 0x00, 0x00, 0x00, 0x03, 0xc3, 0xc0, 0x3f, 
0xff, 0xfc, 0x3e, 0x7e, 0x7c, 0x00, 0x00, 0x00, };

static int draw_train(train_t * t, int row)
{
	if (row >= 3)
		return 0;

	const time_t now = time(NULL) + TZ_OFFSET;
	const int delta = t->departure - now;
	const int delta_min = delta / 60;
	const int delta_sec = delta % 60;

	char buf[64];
	const int width = 83;

	display.setRotation(0);

	if (row > 0)
		display.drawFastVLine(width*row + 4, 0, 122, COLOR1);

	display.setFont(&DinFont60pt7b);
	if (delta < 0) {
		buf[0] = '-';
		buf[1] = '\0';
	} else {
		snprintf(buf, sizeof(buf),
			"%2d",
			delta_min
		);
	}

	int16_t x1, y1;
	uint16_t w, h;
	display.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
	display.setCursor(width*(row+1) - w - 12, 114);
	display.print(buf);
	//printf("'%s' = %d\n", buf, w);

/*
	// display seconds (not on the e-ink)
	display.setFont(&DinFont12pt7b);
	display.setCursor(width*row + 55, 20);
	if (delta < 0) {
		display.print("--");
	} else {
		snprintf(buf, sizeof(buf),
			"%02d",
			delta_sec
		);
		display.print(buf);
	}
*/

/*
	snprintf(buf, sizeof(buf),
		"%c%s %s",
		t->type,
		t->line_number,
		"" //t->departure_time
	);
*/
	if (t->type == 'B')
		display.drawBitmap(width*row + 24, 6, icon_bus, 24, 24, COLOR1);
	else
	if (t->type == 'T')
		display.drawBitmap(width*row + 24, 6, icon_train, 24, 24, COLOR1);
	else
	if (t->type == 'F')
		display.drawBitmap(width*row + 24, 6, icon_ferry, 24, 24, COLOR1);

	display.setFont(&DinFont12pt7b);
	display.setCursor(width*row + 48, 25);
	display.print(t->line_number);

	const int dest_len = strlen(t->destination);
	display.setRotation(3);
	display.setFont();
	display.setCursor((122 - dest_len*5)/2, width*(row+1) - 8);
	display.print(t->destination);

	// if nothing has changed, we don't need a refresh
	if (t->last_row == row && t->last_delta == delta_min)
		return 0;

	t->last_row = row;
	t->last_delta = delta_min;

	return 1;
}
 
void loop()
{
#if 0
	const int remaining_ms = ui.update();
	if (remaining_ms < 0)
		return;
#endif

	gvb_loop();

	unsigned long now_ms = millis();
	if (now_ms - last_query_ms < query_interval_ms)
	{
		//delay(remaining_ms);
		return;
	}

	// attempt a reconnect or send a new query
	last_query_ms = now_ms;

	// dump the list, drawing the first few to the display
	display.clearBuffer();
	static int invert = 0;
	display.invertDisplay(invert);
	invert = !invert;

	train_t * t = train_list;
	int count = 0;
	int need_refresh = 0;

	while(t)
	{
		if (draw_train(t, count))
			need_refresh = 1;

		printf("%3d %c %d %-4d: %-3s %c %s\r\n",
			t->id,
			t->status,
			t->departure,
			t->delay_sec,
			t->line_number,
			t->type,
			t->destination
		);

		t = t->next;

		if (count++ > 50)
		{
			// something is wrong. flush the whole list
			printf("ERROR?\r\n");
			break;
		}
	}

	if (need_refresh)
		display.display();

/*
	Serial.println("sending query");
	if (!ws.sendTXT(ws_query))
		Serial.println("FAILED");
*/
}
