#!/usr/bin/env python

# SENG 265 Assignment 2
# Author: Matthew Stephenson

import sys
from datetime import datetime, timedelta


def input_to_iso_date(date:str) -> str:
    """ Takes a date of the form YYYY/M/D and returns the date in the form
        YYYY/MM/DD (iso format) for datetime functions.
    """
    
    add_zero = lambda s: "0"+s if (len(s) == 1) else s
    iso_date = date.split('/')
    iso_date[1] = add_zero(iso_date[1])
    iso_date[2] = add_zero(iso_date[2])
    return "-".join(iso_date)


def ical_to_iso_date(date: str) -> str:
    """ Takes a date in the ical format and returns a date in the iso format for datetime functions """
    return date[0:4] + '-' + date[4:6] + '-' + date[6:8] + date[8:11] + ':' + date[11:13] + ':' + date[13:]


def parse_events(FileIn) -> list:
    """ Reads an iso event from FileIn and returns a dict of the event properties

    Assumes FileIn is at the line following a BEGIN:VEVENT line
    Converts DTSTART and DTEND properties to datetime objects
    All other properties are stored as strings how they are read
    """
    
    event = []
    for line in FileIn:
        line = line[:-1] #trim newline char
        if line == "END:VEVENT":
            break
        variable, value = line.split(':')
        if variable in ["DTSTART", "DTEND"]:
            value = ical_to_iso_date(value)
            value = datetime.fromisoformat(value)
        event.append(value)
    return event


def rule_parse(Events: list):
    """ Given a list of events, produces a new event which is a copy of the last event with dates shifted
        by a week if the subsequent event will occur prior to the UNTIL date specified by the ical RRULE property
        This function recursively calls itself until the subseqent event will not occur before the UNTIL date.

    Assumes the last entry to the Events input list has the RRULE property.
    Will append all subseqent events before the UNTIL date at the end of the Events list
    """
    
    until = ical_to_iso_date(Events[-1][2].split("UNTIL=")[1].split(";")[0])
    until = datetime.fromisoformat(until)
    nextdate = Events[-1][0] + timedelta(weeks = 1)
    if (nextdate < until):
        eventcpy = Events[-1].copy() #if we do not produce a deep copy for the subsequent event, bad things happen
        eventcpy[0] = nextdate
        eventcpy[1] = nextdate + (Events[-1][1] - Events[-1][0])
        Events.append(eventcpy)
        rule_parse(Events)

        
def truncate_events(StartDate: datetime, EndDate: datetime, Events: list) -> list:
    """ Takes a list of events and a start and end date, returning the events that occur between
        the start and end, sorted by the event start dates in chronological order.
    """
    
    #sorting events by start date
    E = Events.copy()
    E.sort(key=lambda x: x[0])

    #removing the events outside the start to end date range
    event_range = lambda start, end, event: event if (event[0] > start and event[0] < end) else None
    E = [event_range(StartDate, EndDate, e) for e in E]
    while True:
        try:
            E.remove(None) #lambda expression above replaced events outside of range with None entries
        except ValueError: #value error occurs when no None entries are left
            break

    return E

        
def print_events(Events: list):
    """ Prints the events in the input event list in the format specified by the assignment
    """
    
    #need to make this check because truncating the events may have removed all events
    if len(Events) == 0:
        return

    #lambda functions are too good
    printDate = lambda d: print(d.strftime("%B %d, %Y (%a)"))
    printDash = lambda d: print('-' * len(d.strftime("%B %d, %Y (%a)")))
    printEvent = lambda e: print("{} to {}: {} ".format(e[0].strftime("%l:%M %p"),e[1].strftime("%l:%M %p"),e[-1]) + "{{" + e[-2] + "}}")
    
    previousDate = Events[0][0]
    printDate(previousDate)
    printDash(previousDate)

    for e in Events:
        if (e[0].date() != previousDate.date()): #need to print new date if true
            previousDate = e[0]
            print() 
            printDate(previousDate)
            printDash(previousDate)
        printEvent(e)

        
def main():
    # assuming well formed input
    startDate = sys.argv[1].split('=')[1]
    startDate = input_to_iso_date(startDate)
    startDate = datetime.fromisoformat(startDate)

    endDate = sys.argv[2].split('=')[1]
    endDate = input_to_iso_date(endDate)+"T23:59:59"
    endDate = datetime.fromisoformat(endDate)

    fileName = sys.argv[3].split('=')[1]

    events = []

    with open(fileName, 'r') as file:
        file.readline() # read off the initial BEGIN:VCALENDAR line
        for line in file:
            if line[:-1] == "END:VCALENDAR":
                #end of file
                break
            if line[:-1] == "BEGIN:VEVENT":
                events.append(parse_events(file))
                if len(events[-1]) > 4: #not a very robust method for checking for rrule if other ical properties exist
                    rule_parse(events)
            
    events = truncate_events(startDate, endDate, events)
    print_events(events)
            
if __name__ == "__main__":
    main()
    
