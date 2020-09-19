#include <ArduinoWebsockets.h> // 0.4.18
#include <ESP8266WiFi.h>
#include "time.h"
#include "Adafruit_EPD.h" // 2.5.3
#include "Adafruit_GFX.h" // 1.10.1
#include "GVB.h"
#include "config.h"

#include <Fonts/Org_01.h>
#include "DinBold44pt7b.h"
#include "DinBold12pt7b.h"
#define small_font Org_01
#define med_font Org_01
#define med_bold_font DinBold12pt7b
#define large_font DinBold44pt7b


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

#ifndef ARRIVAL_THRESHOLD
#define ARRIVAL_THRESHOLD 0
#endif

extern const char Lato_Bold_21[];

using namespace websockets;
WebsocketsClient ws;

#define TZ_OFFSET (1 * 3600)

// watchdog: if we don't get a packet for this long, reboot
const unsigned long query_interval_ms = 600 * 1000; // ten minutes
//const unsigned long query_interval_ms = 10 * 1000; // debug
unsigned long last_query_ms;

/* 250x122 => 83 wide per entry
   BIG sec  BIG sec  BIG sec
   MIN      MIN      MIN sec
   Dest     Dest     Dest
*/
static const uint8_t icon_train[] = {
// 'directions_train_black_24x24(1)', 24x24px
0x00, 0x00, 0x00, 0x01, 0xff, 0x80, 0x07, 0xff, 0xe0, 0x0f, 0xff, 0xf0, 0x0f, 0xff, 0xf0, 0x0c, 
0x00, 0x30, 0x0c, 0x00, 0x30, 0x0c, 0x00, 0x30, 0x0c, 0x00, 0x30, 0x0c, 0x00, 0x30, 0x0f, 0xff, 
0xf0, 0x0f, 0xff, 0xf0, 0x0f, 0xff, 0xf0, 0x0f, 0xe7, 0xf0, 0x0f, 0xc3, 0xf0, 0x0f, 0xc3, 0xf0, 
0x0f, 0xe7, 0xf0, 0x07, 0xff, 0xe0, 0x03, 0xff, 0xc0, 0x01, 0xff, 0x80, 0x03, 0xff, 0xc0, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
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

static const uint8_t icon_gvb[] = {
// 'Screenshot from 2020-09-18 17-13-18', 72x24px
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x80, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0x87, 
0xff, 0xc1, 0xfe, 0x7c, 0x79, 0xfe, 0x7f, 0xff, 0x8f, 0xff, 0xc3, 0xff, 0x7c, 0xf9, 0xff, 0x7f, 
0xff, 0x3f, 0xff, 0xc7, 0xff, 0x3c, 0xf9, 0xff, 0x7f, 0xfc, 0xff, 0xff, 0xc7, 0xcf, 0x3c, 0xf9, 
0xef, 0x7f, 0xf3, 0xff, 0xff, 0xc7, 0x80, 0x3e, 0xf1, 0xff, 0x00, 0x0f, 0xff, 0x00, 0x0f, 0xbf, 
0x1f, 0xf1, 0xff, 0x00, 0x3f, 0xfc, 0x00, 0x0f, 0xbf, 0x1f, 0xe1, 0xff, 0x7f, 0xff, 0xf1, 0xff, 
0xc7, 0xbf, 0x1f, 0xe1, 0xff, 0x7f, 0xff, 0xe7, 0xff, 0xc7, 0xcf, 0x0f, 0xe1, 0xef, 0x7f, 0xff, 
0x9f, 0xff, 0xc7, 0xff, 0x0f, 0xc1, 0xff, 0x7f, 0xfe, 0x7f, 0xff, 0xc3, 0xff, 0x0f, 0xc1, 0xff, 
0x7f, 0xf8, 0x7f, 0xff, 0xc1, 0xff, 0x07, 0xc1, 0xfe, 0x00, 0x00, 0x7f, 0x80, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


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

	return 1;
}
 

void setup() { 
  Serial.begin(115200);

  display.begin();
  display.setRotation(0);
  display.clearBuffer();

  display.setFont(&med_bold_font);
  display.setCursor(0, 20);
  display.setTextColor(COLOR1);
  display.setTextWrap(true);
  display.print("OVertime starting");
  display.drawBitmap(10, 90, icon_bus, 24, 24, COLOR1);
  display.drawBitmap(50, 90, icon_train, 24, 24, COLOR1);
  display.drawBitmap(90, 90, icon_ferry, 24, 24, COLOR1);
  display.drawBitmap(130, 90, icon_bus, 24, 24, COLOR1);
  display.drawBitmap(170, 90, icon_train, 24, 24, COLOR1);
  display.drawBitmap(210, 90, icon_ferry, 24, 24, COLOR1);

	display.setCursor(35, 73);
	display.print("Data from");
	display.drawBitmap(120, 55, icon_gvb, 72, 24, COLOR1);

	display.setCursor(0, 45);
        display.print(WIFI_SSID);
	display.print(": ");
	display.display();

	Start_WiFi(WIFI_SSID, WIFI_PASSWORD);

	display.print(WiFi.localIP());
        display.display();

	printf("configuring ntp\r\n");
	configTime(1, 3600, "pool.ntp.org");

	gvb_setup();
}

#define LEFT	0x00
#define RIGHT	0x01
#define CENTER	0x03
#define BOTTOM	0x00
#define TOP	0x10
#define VCENTER	0x30

void drawtext(int x, int y, int center, const char * fmt, ...)
{
	char buf[40];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	int16_t x1, y1;
	uint16_t w, h;
	display.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
	//printf("'%s' %d %d %d %d\r\n", buf, x1, y1, w, h);

	if ((center & 0x0F) == RIGHT)
		x = x - w;
	else
	if ((center & 0x0F) == CENTER)
		x = x - w/2;

	if ((center & 0xF0) == TOP)
		y = y + h;
	else
	if ((center & 0xF0) == VCENTER)
		y = y + h/2;

	display.setCursor(x, y);
	display.write(buf);
}

static int draw_train(train_t * t, int count)
{
	static int row;

	// starting a new display
	if (count == 0)
		row = 0;

	const int width = 83; // each time has 83 pixels for its display
	const time_t now = time(NULL) + TZ_OFFSET;
	const int delta = t->departure - now;
	const int delta_min = delta / 60;
	const int delta_sec = delta % 60;

	// if the bus is too soon, don't bother displaying it
	// since the travel time to the stop means we don't care
	if (delta < ARRIVAL_THRESHOLD)
		return 0;

	// only show three upcoming trips
	if (row >= 3)
		return 0;

	// ok, this one is going to be displayed...
	const int cur_row = row++;
	const int base_x =
		cur_row == 0 ? -4 :
		cur_row == 1 ? 81 :
		cur_row == 2 ? 166 :
		cur_row * width;

	display.setRotation(0);

	display.setFont(&large_font);
	drawtext(base_x + width - 17, 95, RIGHT, "%2d", delta_min);

	// draw the icon and line name at the top left
	if (t->type == 'B')
		display.drawBitmap(base_x + 4, 2, icon_bus, 24, 24, COLOR1);
	else
	if (t->type == 'T')
		display.drawBitmap(base_x + 4, 2, icon_train, 24, 24, COLOR1);
	else
	if (t->type == 'F')
		display.drawBitmap(base_x + 4, 2, icon_ferry, 24, 24, COLOR1);

	display.setFont(&med_bold_font);
	display.setCursor(base_x + 4 + 24, 20);
	display.print(t->line_number);

	// stop and destination
#if 0
	display.drawFastVLine(base_x + width - 7, 20, 60, COLOR1);
	display.setRotation(3);
	display.setFont(&small_font);
	//display.setFont();

	drawtext((122-20)/2 + 20, base_x + width - 9, CENTER, "%s >>", t->stop);
	drawtext((122-20)/2 + 20, base_x + width - 1, CENTER, ">> %s", t->destination);
#else
	display.setFont(&small_font);
	drawtext(base_x + 4, 31, LEFT, "%s", t->stop);
	drawtext(base_x + width, 105, RIGHT, ">> %s", t->destination);
#endif

	// if nothing has changed, we don't need a refresh
	if (t->last_row == cur_row && t->last_delta == delta_min)
		return 0;

	t->last_row = cur_row;
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

	unsigned long now_ms = millis();

	if (!gvb_loop())
	{
		if (now_ms - last_query_ms < query_interval_ms)
			return;

		// it has been too long without some sort of update;
		// re-initiate the wifi and connection and everything
		setup();
		return;
	}

	// we have a packet! reset the watchdog
	last_query_ms = now_ms;

	// dump the list, drawing the first few to the display
	display.clearBuffer();
	static int invert = 0;
	display.invertDisplay(invert);
	invert = !invert;

	// draw two lines to separate the three columns
	//display.drawFastVLine(1*83, 0, 110, COLOR1);
	//display.drawFastVLine(2*83, 0, 110, COLOR1);

	train_t * t = train_list;
	int count = 0;
	int need_refresh = 0;

	while(t)
	{
		if (draw_train(t, count))
			need_refresh = 1;

		printf("%3d %c %d %-4d: %c %-3s %s -> %s\r\n",
			t->id,
			t->status,
			t->departure,
			t->delay_sec,
			t->type,
			t->line_number,
			t->stop,
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

	time_t now = time(NULL) + TZ_OFFSET;
	struct tm *tm = localtime(&now);

	display.setRotation(0);
	//display.setFont(&small_font);
	display.setFont();
	drawtext(250/2, 121-8, CENTER,
		"%04d-%02d-%02d %02d:%02d:%02d",
		tm->tm_year + 1900,
		tm->tm_mon + 1,
		tm->tm_mday,
		tm->tm_hour + 1,
		tm->tm_min,
		tm->tm_sec
	);

	if (need_refresh)
		display.display();

/*
	Serial.println("sending query");
	if (!ws.sendTXT(ws_query))
		Serial.println("FAILED");
*/
}
