/*
 * process_cal3.c
 * SENG 265 Assignment 3 
 * Author: Matthew Stephenson
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "emalloc.h"
#include "ics.h"
#include "listy.h"

void dt_format(char *formatted_time, const char *dt_time, const int len);
void dt_increment(char *after, const char *before, int const num_days);
void tm_format(char *formatted_time, const char *dt_time, const int len);
double time_calc(const char *dateStr);
void time_string_create(char *out, int year, int month,
			int day, int hour, int min, int sec);
void print_event(node_t *e);
void print_events(int from_yy, int from_mm, int from_dd,
		  int to_yy, int to_mm, int to_dd,
		  node_t *list);
int rule_check(event_t *event);
void rule_parse(event_t *event, event_t *nextevent);
void read_event(FILE *fin, event_t *event);
node_t *read_file(char *file, node_t *list);
void mem_deallocate(node_t *list);

int main(int argc, char *argv[])
{
  // argument parsing code taken from assignment 1 process_cal.c base file provided by SENG 265 teaching team
  
  int from_y = 0, from_m = 0, from_d = 0;
  int to_y = 0, to_m = 0, to_d = 0;
  char *filename = NULL;
    
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

  node_t *eventlist=NULL;

  //the head of the sorted list containing parsed events is returned from read_file
  eventlist = read_file(filename, eventlist);

  print_events(from_y, from_m, from_d,
  	       to_y, to_m, to_d,
    	       eventlist);
  
  mem_deallocate(eventlist);

  exit(0);
}

/*
  * Function dt_format.
 * Taken from timeplay.c, provided by SENG 265 teaching team for assignemnt 1.
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
 * Taken from timeplay.c, provided by SENG 265 teaching team for assignemnt 1.
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
void dt_increment(char *after, const char *before, const int num_days)
{
    struct tm temp_time;
    time_t    full_time;
    int buffsize = 100; //chosen to be larger than array len defines in ics.h

    memset(&temp_time, 0, sizeof(struct tm));
    sscanf(before, "%4d%2d%2d", &temp_time.tm_year,
        &temp_time.tm_mon, &temp_time.tm_mday);
    temp_time.tm_year -= 1900;
    temp_time.tm_mon -= 1;
    temp_time.tm_mday += num_days;

    full_time = mktime(&temp_time);
    after[0] = '\0';
    strftime(after, 9, "%Y%m%d", localtime(&full_time));
    strncpy(after + 8, before + 8, buffsize - 8);
    after[buffsize - 1] = '\0';
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
double time_calc(const char *dateStr)
{
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
    int buffsize = 100; //chosen to be larger than array len defines in ics.h

    temp_time.tm_year = year - 1900;
    temp_time.tm_mon = month -1;
    temp_time.tm_mday = day;
    temp_time.tm_hour = hour;
    temp_time.tm_min = min;
    temp_time.tm_sec = sec;
  
    full_time = mktime(&temp_time);
    out[0] = '\0';
    strftime(out, 18, "%Y%m%dT%H%M%S", localtime(&full_time));
    out[buffsize-1] = '\0';
}

/*
 * Function print_events.
 *
 * Takes a list node containing the information for a given event (in the val ptr)
 * and prints the event in the format specified by the assignment description.
 *
 */
void print_event(node_t *e)
{
  int buffsize = 100; //chosen to be larger than array len defines in ics.h
  char start[buffsize];
  char end[buffsize];

  tm_format(start, e->val->dtstart, buffsize);
  tm_format(end, e->val->dtend, buffsize);
  printf("%s to %s: %s {{%s}}\n", start, end, e->val->summary, e->val->location);
}

/*
 * Funciton print_events
 *
 * Takes the start and end date parameters specified at runtime and loops through
 * the list which stores events, printing unique dates and calling print_event on list nodes
 * that fit within the time range.
 *
 */
void print_events(int from_yy, int from_mm, int from_dd,
		  int to_yy, int to_mm, int to_dd,
		  node_t *list)
{
  int buffsize = 100; //chosen to be larger than array len defines in ics.h
  char fromDate[buffsize], toDate[buffsize];
  
  time_string_create(fromDate, from_yy, from_mm, from_dd, 0, 0, 0);
  time_string_create(toDate, to_yy, to_mm, to_dd, 23, 59, 59);

  // time doubles are easier to compare against
  double fromTimeComp = time_calc(fromDate);
  double toTimeComp = time_calc(toDate);
  
  char lastDate[buffsize];
  strcpy(lastDate, "");

  // temp to store next node in list
  node_t *temp = NULL;

  for ( ; list != NULL; list = temp)
    {
      temp = list -> next;
      // if past start date
      if (time_calc(list -> val -> dtstart) > fromTimeComp)
	{
	  // if after end date
	  if (time_calc(list -> val -> dtstart) > toTimeComp) break;

	  char eventDate[buffsize];
	  dt_format(eventDate, list -> val -> dtstart, buffsize);

	  // only print formated date if date has changed
	  if (strcmp(eventDate, lastDate) != 0)
	    {
	      if (strcmp(lastDate, "") != 0) printf("\n");
	      printf("%s\n", eventDate);
	      for (int i=0; i < strlen(eventDate); i++) printf("-");
	      printf("\n");
	      strcpy(lastDate, eventDate);
	    }
	  print_event(list);	    
	}      
    }
}

/*
 * Function rule_check
 *
 * Given an event the fxn checks whether a week from the event's dtstart
 * member is prior to the date specified in the event's rrule member
 * returnting 1 if true, implying the rule still applies, else zero.
 *
 */
int rule_check(event_t *event)
{
  int res = 0;
  if (strncmp(event -> rrule, "", 1) == 0) return res;
  
  int offset = 0;
  int buffsize=100; //chosen to be larger than array len defines in ics.h
  char ruleUntilDate[buffsize];
  char nextDate[buffsize];

  //find where the until date is in the rrule string
  while (strncmp((event->rrule)+offset, "UNTIL=", 6) != 0) offset++;
  strcpy(ruleUntilDate, (event->rrule)+offset+6);
  ruleUntilDate[15] = '\0'; // the strings are of length 15

  double ruleUntilTime = time_calc(ruleUntilDate);
  dt_increment(nextDate, event->dtstart, 7);
  double nextTime = time_calc(nextDate);
  
  if (nextTime < ruleUntilTime) res = 1;
  
  //return 1 if rule applies else 0
  return res;
}

/*
 * Function rule_parse
 *
 * Given an event where a rule still applies, implying subsequent events
 * the fxn copies the event_t members of event into nextevent and
 * add a week to the dtstart member of nextevetn.
 *
 */
void rule_parse(event_t *event, event_t *nextevent)
{
  // I couldnt use a memcpy for some reason, it appeared to remove all memory buffers
  // between event_t members, and when the dtstart was changed for the nextevent
  // it would copy over other members, leaving them empty. I don't really understand
  // what was happening, but I got it to work by copying each event_t member one by one,
  // and modifying dtstart prior to copying the remaining event_t members
  strcpy(nextevent->dtstart, event->dtstart);
  dt_increment(nextevent->dtstart, event->dtstart, 7);
  strcpy(nextevent->dtend, event->dtend);
  // technically we do not need to increment dtend by 7 days using dt_increment
  // b/c it is not used in the program
  strcpy(nextevent->summary, event->summary);
  strcpy(nextevent->location, event->location);
  strcpy(nextevent->rrule, event->rrule);
}

/*
 * Function read_event
 *
 * Given a file ptr that is presumed to point at the begining of
 * an iCal event, this fxn copies the lines detailing the event properties
 * into the appropiate memebers of the event_t struct passed in.
 *  
 */
void read_event(FILE *fin, event_t *event)
{
  strcpy(event->rrule, ""); //by default rrule is empty so we can catch if it has not been filled by file
  int buffsize = 100;  //chosen to be larger than array len defines in ics.h
  char line[buffsize];
  
  fgets(line, buffsize, fin); //read off begin line

  // event is terminated with an end line
  while(strncmp(line, "END", 3) != 0) 
    {
      char *attribute = strchr(line, ':')+1; 
      int l = strlen(attribute);
      attribute[l-1] = '\0';

      if (strncmp(line, "DTS", 3) == 0) strcpy(event -> dtstart, attribute);
      else if (strncmp(line, "DTE", 3) == 0) strcpy(event -> dtend, attribute);
      else if (strncmp(line, "SUM", 3) == 0) strcpy(event -> summary, attribute);
      else if (strncmp(line, "LOC", 3) == 0) strcpy(event -> location, attribute);
      else if (strncmp(line, "RRU", 3) == 0) strcpy(event -> rrule, attribute);
       
      fgets(line, buffsize, fin);
    }
}

/*
 * Function read_file
 *
 * This fxn takes the filename to a file containing iCal events and parses the events
 * in the file, saving them as members in the node_t list which is given as an input.
 * The function inserts the events into this list in chronoligical order based on the 
 * dtstart property of each event. If events containing a repeat rule as specified by the
 * rrule property are parsed, all events implied by the repeat rule are also added to the list.
 * The function returns a pointer to the sorted list containing the event data once all 
 * events in the file are parsed.
 *
 */
node_t *read_file(char *file, node_t *list)
{
  FILE *fin = fopen(file, "r");
  int buffsize = 100; //chosen to be larger than array len defines in ics.h
  char line[buffsize];

  fgets(line, buffsize, fin); //read off first line stating the begining of an iCal file
  fgets(line, buffsize, fin);

  // file is terminated with an end line
  while (strncmp(line, "END", 3) != 0)
    {
      if (strncmp(line, "BEG", 3) ==0)
	{
	  // allocate prior to filling with data
	  event_t *temp = (event_t *)emalloc(sizeof(event_t));

	  read_event(fin, temp);
	  //events added to list in order so no need for sorting later
	  list = add_inorder(list, new_node(temp));
	  
	  // rule_check returns zero when a week from dtstart of temp > the date in rrule or when rrule is empty 
	  while (rule_check(temp))
	    {
	      // allocate prior to copying data from temp in rule_parse
	      event_t *ruletemp = (event_t *)emalloc(sizeof(event_t));
	      rule_parse(temp, ruletemp);
	      //events added to list in order so no need for sorting later
	      list = add_inorder(list, new_node(ruletemp));
	      // need to set temp to newly created event which will come after temp, so that while loop
	      // condition returns false at some point
	      temp = ruletemp;
	    }
	      
	}
      fgets(line, buffsize, fin);
    }
  
  fclose(fin);
  return list;
}

/*
 * Function mem_deallocate
 *
 * This function was taken from the the tester.c file provided by the SENG 265
 * teaching team for lab 08 part b. The function frees all memory allocated for the 
 * list containing event data pointed to by *list.
 * 
 */
void mem_deallocate(node_t *list)
{
  node_t *temp = NULL;
  for ( ; list != NULL; list = temp)
    {
      temp = list -> next;
      free(list -> val);
      free(list);
    }
}
