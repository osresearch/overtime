#ifndef _gvb_h_
#define _gvb_h_

// linked list of train structures
struct train_t {
	train_t * next;
	train_t ** prev;
	int id;
	char status; // U or A (upcoming or arriving)
	char type; // B, T, or F (bus, tram, ferry)
	unsigned long departure;
	int delay_sec;
	char line_number[5];
	char destination[32];
	char departure_time[16];
};

extern train_t * train_list;

#endif
