/*
SENG 265 Assignment 1
Author: Matthew Stephenson
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#define MAX_LINE_LEN 132
#define MAX_EVENTS 500

/*
 * Struct Event
 * 
 * Used to hold the iCal information for each event.
 * The ruleFlag variable is a bool indicating whether
 * the event has subsequent events based on an iCal rule.
 * The time variable is used for event comparison.
 */
typedef struct {
  char startStr[MAX_LINE_LEN];
  char endStr[MAX_LINE_LEN];
  char location[MAX_LINE_LEN];
  char summary[MAX_LINE_LEN];
  char ruleUntilDate[MAX_LINE_LEN];
  int ruleflag;
  double time;
} Event;

/*
 * Function dt_format.
 * Taken from timeplay.c, provided by SENG 265 teaching team.
 *
 * Given a date-time, creates a more readable version of the
 * calendar date by using some C-library routines. For example,
 * if the string in "dt_time" corresponds to:
 *
 *   20190520T111500
 *
 * then the string stored at "formatted_time" is:
 *
 *   May 20, 2019 (Mon).
 *
 */

void dt_format(char *formatted_time, const char *dt_time, const int len)
{
    struct tm temp_time;
    time_t    full_time;
    char      temp[5];

    /*  
     * Ignore for now everything other than the year, month and date.
     * For conversion to work, months must be numbered from 0, and the 
     * year from 1900.
     */  
    memset(&temp_time, 0, sizeof(struct tm));
    sscanf(dt_time, "%4d%2d%2d",
        &temp_time.tm_year, &temp_time.tm_mon, &temp_time.tm_mday);
    temp_time.tm_year -= 1900;
    temp_time.tm_mon -= 1;
    full_time = mktime(&temp_time);
    strftime(formatted_time, len, "%B %d, %Y (%a)", 
        localtime(&full_time));
}

/*
 * Function dt_increment:
 * Taken from timeplay.c, provided by SENG 265 teaching team.
 *
 * Given a date-time, it adds the number of days in a way that
 * results in the correct year, month, and day. For example,
 * if the string in "before" corresponds to:
 *
 *   20190520T111500
 *
 * then the datetime string stored in "after", assuming that
 * "num_days" is 100, will be:
 *
 *   20190828T111500
 *
 * which is 100 days after May 20, 2019 (i.e., August 28, 2019).
 *
 */

void dt_increment(char *after, const char *before, int const num_days)
{
    struct tm temp_time, *p_temp_time;
    time_t    full_time;
    char      temp[5];

    memset(&temp_time, 0, sizeof(struct tm));
    sscanf(before, "%4d%2d%2d", &temp_time.tm_year,
        &temp_time.tm_mon, &temp_time.tm_mday);
    temp_time.tm_year -= 1900;
    temp_time.tm_mon -= 1;
    temp_time.tm_mday += num_days;

    full_time = mktime(&temp_time);
    after[0] = '\0';
    strftime(after, 9, "%Y%m%d", localtime(&full_time));
    strncpy(after + 8, before + 8, MAX_LINE_LEN - 8);
    after[MAX_LINE_LEN - 1] = '\0';
}

/*
 * Function tm_format.
 *
 * Given a iCal formated event date, produces the time in 12 hr
 * HH:MM format with the appropriate AM/PM at the end.
 *
 */ 
void tm_format(char *formatted_time, const char *dt_time, const int len)
{
    struct tm temp_time;
    time_t    full_time;
    char      temp[5];

    sscanf(dt_time, "%*8dT%2d%2d%2d",&temp_time.tm_hour ,&temp_time.tm_min, &temp_time.tm_sec);
 
    full_time = mktime(&temp_time);

    strftime(formatted_time, 10, "%l:%M %p", 
	     localtime(&full_time));
      
}

/*
 * Function time_calc
 * 
 * Produces a double that represents a date in years.
 * A double has enough digits that dates differing by
 * a second produce different values.
 *
 */
double time_calc(const char *dateStr){
    double res;
    double year, month, day, hour, minute, second;

    sscanf(dateStr, "%4lf%2lf%2lfT%2lf%2lf%2lf",
	   &year, &month, &day, &hour, &minute, &second);
    res = year + month/12 + day/365 + hour/365/24 + minute/365/24/60 + second/365/24/60/60;
  
    return res;
}

/*
 * Function time_string_create.
 *
 * Converts an input date into a date in the 
 * iCal format that is saved into an input char array.
 *
 */
void time_string_create(char *out, int year, int month,
			int day, int hour, int min, int sec) {
    time_t full_time;
    struct tm temp_time;

    temp_time.tm_year = year - 1900;
    temp_time.tm_mon = month -1;
    temp_time.tm_mday = day;
    temp_time.tm_hour = hour;
    temp_time.tm_min = min;
    temp_time.tm_sec = sec;
  
    full_time = mktime(&temp_time);
    out[0] = '\0';
    strftime(out, 18, "%Y%m%dT%H%M%S", localtime(&full_time));
    out[MAX_LINE_LEN-1] = '\0';
}

/*
 * Function print_events.
 *
 * Takes an event structure contianing a single iCal event
 * and prints the event in the format specified by the
 * assignment description.
 *
 */
void print_event(const Event event) {
    char startTime[MAX_LINE_LEN], endTime[MAX_LINE_LEN];
    tm_format(startTime, event.startStr, MAX_LINE_LEN);
    tm_format(endTime, event.endStr, MAX_LINE_LEN);
    printf("%s to %s: %s {{%s}}\n", startTime, endTime, event.summary, event.location);
}

/*
 * Funciton print_events
 *
 * Takes the start and end date parameters specified at runtime and loops through
 * EventArr, printing unique dates and calling print_event for Events that fit 
 * within the time range.
 *
 */
void print_events(int from_yy, int from_mm, int from_dd,
		  int to_yy, int to_mm, int to_dd,
		  Event *EventArr, int EventArrLen) {

    char from_date[MAX_LINE_LEN], to_date[MAX_LINE_LEN];
    time_string_create(from_date, from_yy, from_mm, from_dd, 0, 0, 0);
    time_string_create(to_date, to_yy, to_mm, to_dd, 23, 59, 59);
    // the time doubles are easier to compare
    double from_time_comp = time_calc(from_date);
    double to_time_comp = time_calc(to_date);

    // need to track the last date in order to print the date
    // when it changes
    char lastDate[MAX_LINE_LEN] = "";

    // worst case loop over all events
    for (int i=0; i < EventArrLen; i++) {
    
        if (EventArr[i].time > from_time_comp) {

	    //break out of loop if eventarray[i].time > end time
  	    if (EventArr[i].time > to_time_comp) {
  	        break;
	    }

	    char eventDate[MAX_LINE_LEN];
	    dt_format(eventDate, EventArr[i].startStr, MAX_LINE_LEN);
      
	    if (strcmp(eventDate,lastDate) != 0) {
	        if (strcmp(lastDate,"") != 0) {
		    printf("\n");
		}
		printf("%s\n", eventDate);
		for (int i=0; i < strlen(eventDate); i++) printf("-");
		printf("\n");
		strcpy(lastDate, eventDate);
	    }
	    print_event(EventArr[i]);
	}    
    }
}
		 
/*
 * Function sort_events
 *
 * Takes an Event array and performs insertion sort on the 
 * Events based on the time property.
 *
 */
void sort_events(Event *EventArr, int EventArrLen){
    // insertion sort for events based on time variable
    Event temp;
    for (int i = 1; i < EventArrLen; i++) {
        for (int j = i; j > 0; j--) {
  	    if (!(EventArr[j].time < EventArr[j-1].time)) {
	        break;
	    }
	    temp = EventArr[j];
	    EventArr[j] = EventArr[j-1];
	    EventArr[j-1] = temp;
	}
    }
}
/*
 * Function rule_check
 *
 * Given an event (that assumably has an iCal rule),
 * the fxn checks whether a week from the event date
 * is prior to the event's ruleUntilDate,
 * returnting 1 if true, implying the rule still applies.
 *
 */
int rule_check(const Event EventWithRule) {
    int ruleValid = 0;

    double ruleUntilTime = time_calc(EventWithRule.ruleUntilDate);

    // produce a hypothetical next event to compare against
    char nextDate[MAX_LINE_LEN];
    dt_increment(nextDate, EventWithRule.startStr, 7);
    double nextTime = time_calc(nextDate);
  
    if (nextTime < ruleUntilTime) {
        ruleValid = 1;
    }
				
    return ruleValid;
}


/*
 * Function apply_rule.
 *
 * Given an event with a rule, the fxn produces
 * new Event a week from the start date.
 *
 */
Event apply_rule(Event EventWithRule) {
    Event nextEvent = EventWithRule;

    //modify the dates of the copied events
    dt_increment(nextEvent.startStr, EventWithRule.startStr, 7);
    dt_increment(nextEvent.endStr, EventWithRule.endStr, 7);
    nextEvent.time = time_calc(nextEvent.startStr);
    
    // check if in a week from the event date, the event rule still applies
    if (!rule_check(nextEvent)) {
        nextEvent.ruleflag = 0;
    }
    
    return nextEvent;
}

/*
 * Function read_event
 *
 * Given a file ptr that is presumed to point to an iCal event
 * this fxn parses the lines detailing the event properties.
 * The function returns an Event struct with the event properties 
 * copied to the appropriate struct attributes.
 *  
 */
Event read_event(FILE *file){
    Event event;
    event.ruleflag = 0;  //base case, will change below if present in ical event
  
    char line[MAX_LINE_LEN];

    fgets(line, MAX_LINE_LEN, file);
    // event ends with an "END" tag
    while (strncmp(line, "END", 3) != 0){
        char *attribute = strchr(line, ':')+1;
	// lop off the newline character at the end of attribute and replace with end of string character
	int l = strlen(attribute);
	attribute[l-1] = '\0'; 
	
	if (strncmp(line, "DTS", 3) == 0) {
	    strcpy(event.startStr, attribute);
	    event.time = time_calc(event.startStr);
	} else if (strncmp(line, "DTE", 3) == 0) {
	    strcpy(event.endStr, attribute);
	} else if (strncmp(line, "LOC", 3) == 0) {
	    strcpy(event.location, attribute);
	} else if (strncmp(line, "SUM", 3) == 0) {
 	    strcpy(event.summary, attribute);
	} else if (strncmp(line, "RRU", 3) == 0) {
 	    while (strncmp(attribute, "UNTIL=", 6) != 0) {
	        attribute = strchr(attribute, ';')+1;
	    }

	    attribute += 6;
	    attribute[15] = '\0';
	    strcpy(event.ruleUntilDate, attribute);

	    if (rule_check(event)) {
	        event.ruleflag = 1;
	    }
	}
	
	fgets(line, MAX_LINE_LEN, file);
    }
    return event;
}

int main(int argc, char *argv[])
{
    int from_y = 0, from_m = 0, from_d = 0;
    int to_y = 0, to_m = 0, to_d = 0;
    char *filename = NULL; /* assuming process_cal is run in the same folder as the .ics file,
			      therefore no absolute path attached to filename */
    int i; 

    for (i = 0; i < argc; i++) {
        if (strncmp(argv[i], "--start=", 8) == 0) {
            sscanf(argv[i], "--start=%d/%d/%d", &from_y, &from_m, &from_d);
        } else if (strncmp(argv[i], "--end=", 6) == 0) {
            sscanf(argv[i], "--end=%d/%d/%d", &to_y, &to_m, &to_d);
        } else if (strncmp(argv[i], "--file=", 7) == 0) {
            filename = argv[i]+7;
        }
    }
    
    if (from_y == 0 || to_y == 0 || filename == NULL) {
        fprintf(stderr, 
            "usage: %s --start=yyyy/mm/dd --end=yyyy/mm/dd --file=icsfile\n",
            argv[0]);
        exit(1);
    }

    FILE *filePtr = fopen(filename, "r");


    // storing all events read from input file in a array
    Event Events[MAX_EVENTS];
    int EventCount = 0;
    
    char line[MAX_LINE_LEN];
    fgets(line, MAX_LINE_LEN, filePtr); //read the first line off
    // we read the initial BEGIN line, so the file ends with the END line
    // the while loop below checks for this line
    fgets(line, MAX_LINE_LEN, filePtr);
    while (strncmp(line, "END", 3) != 0){

        // an event is preceeded by a BEGIN tag
        if (strncmp(line, "BEG", 3) == 0) {
	    EventCount++;
	    Events[EventCount-1] = read_event(filePtr);

	    // if new event has a ruleflag continue producing events
	    // and checking if the event date is before the rule ends
	    while (Events[EventCount-1].ruleflag) {
	        EventCount++;
		Events[EventCount-1] = apply_rule(Events[EventCount-2]);
	    }
	}
	fgets(line, MAX_LINE_LEN, filePtr);
    }

    // sort the events prior to printing them to output
    sort_events(Events, EventCount);
    print_events(from_y, from_m, from_d, to_y, to_m, to_d, Events, EventCount);
    
    fclose(filePtr);
    
    exit(0);
}
