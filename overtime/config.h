#ifndef _overtime_config_h_
#define _overtime_config_h_

/*
 * Add your wifi credentials so the ESP can get on your network.
 */
#define WIFI_SSID "NETWORKNAME"
#define WIFI_PASSWORD "PASSWORD"

/*
 * Add the stops that you want to track.
 * These can be found on the maps.gvb.nl website by clicking
 * on the stop.  Note that the leading 0 is important, so they
 * are strings, not numbers.
 */
#define GVB_HALTS "02113", "02114", "09906", "09901", "09902"

/*
 * Ignore buses/trams/ferries that are too close to reach in time.
 * This only shows trips that are more than five minutes away.
 */
#define ARRIVAL_THRESHOLD 300

#endif
