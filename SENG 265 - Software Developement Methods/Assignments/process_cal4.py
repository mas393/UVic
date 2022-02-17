#!/usr/bin/env python3

# SENG 265 Assignment 4
# Author: Matthew Stephenson

'''
The process_cal4 module contains the process_cal class and the Event class.
The process_cal class is implemented such that the file tester4.py can create process_cal objects
to parse events from ical files, and display formatted output describing the events that
occur on a specific day. The implementation of the process_cal class uses Event objects to
store information of each event encountered when parsing an ical file.
'''

import datetime
import re

class process_cal:
    ''' The process_cal class can be used to parse ical files into individual events that are stored
    in an process_cal object. The process_cal object can then be asked to output a formatted
    string describing the events that occur in the original ical file on a specific day using the 
    get_events_for_day method, which takes a datetime object of the queried day.
    '''
    
    def __init__(self, filename: str):
        ''' A process_cal object is initialized using a single varible, filename, which is a \
        string of the iCal file name which will be parsed by the process_cal object. The input \
        file is parsed during initialization, so multiple process_cal objects initialized with \ 
       the same file can exist without risk of file corruption.
        '''
        self.filename = filename
        self.events = {} # Event objects are stored as dicts with the key being a specific day and
                         # the value being a list of Event objects for events that occur on that day
                         # i.e. {Event.start.datetime.date(): [Event1, Event2 ...., Event_n]}

        self.parse_file() # We parse the file during initialization to avoid conflicts with
                          # other process_cal objects using the same ical file.
        
    def get_events_for_day(self, d: datetime.datetime):
        ''' The get_events_for_day method takes a single datetime object as input and returns
        a formated string describing the ical events that take place on the input day from the
        file the process_cal object was initialized with. If no events occur on the input date,
        a Nonetype object is returned.
        '''
        date_str = None
        
        if d.date() in self.events.keys():
            date_str = d.strftime("%B %d, %Y (%a)") + "\n"
            date_str += '-' * len(d.strftime("%B %d, %Y (%a)"))
            event_l = self.events[d.date()]
            event_l.sort(key = lambda x: x.start) # Event objects may have been added out of order
            for e in event_l:
                date_str += "\n" + str(e)

        return date_str
    
    def parse_file(self):
        ''' A helper method that is called during the initialization of a process_cal object.
        The method reads over the file whos filename is provided during initialization and
        saves all ical events to the procal_cal object's events property as key value pairs,
        where the key is a datetime object of the event's start date, and the value is an 
        Event object that is added to a list of events that occur on that date.
        '''
        with open(self.filename, 'r') as file:
            event_strs = re.split(r"BEGIN:VEVENT\n", file.read())
            
        for e in event_strs:
            if (e[0:7] == "DTSTART"): # splitting events from the ical file above based on "BEGIN:VEVENT"
                                      #  may have included some non-event strings in the list
                event = Event(e)
                self.add_to_event_dict(event)
                while (event.has_rule()):
                    event = event.parse_rule()
                    if (event == None): break # exits loop when event.parse_rule() detects the start date
                                              #  is beyond the event's rrule until date
                    self.add_to_event_dict(event)
                    
    def add_to_event_dict(self, new_event):
        ''' A helper method that is called during the parsing of the input file.
        The method takes an Event object, and places that object in the process_cal 
        object events dict under the key describing the Event object's start date.
        '''
        k = new_event.start.date()
        if (k in self.events.keys()):
            self.events[k].append(new_event)
        else:
            self.events[k] = [new_event]
                                
                        
class Event:
    ''' The Event class stores the properties of an ical event. The string representation of
    an Event object displays the ical event properties in the format desired by process_cal Class
    to satisfy the SENG 265 Assignment 4 requirements. 
    '''
    
    def __init__(self, icalData: str):
        ''' The Event class is inizialized with a single input varible of a stirng
        of data describing an ical event. The input string must contain start, end, location
        and summary properties. Those properties, in addition to a rrule property if it exists are
        parsed and stored as properties in an Event object.
        '''
        self.data = icalData # store raw data to facilitate copying of event to parse events with rules
        self.start = self.to_datetime(re.search(r'(DTSTART:(.*))', icalData)[2])
        self.end = self.to_datetime(re.search(r'(DTEND:(.*))', icalData)[2])
        self.location = re.search(r'(LOCATION:(.*))', icalData)[2]
        self.summary = re.search(r'(SUMMARY:(.*))', icalData)[2]
        
        self.rrule = None
        rrule = re.search(r'(RRULE:(.*))', icalData)
        if rrule:
            self.rrule = rrule[2]
            
    def __str__(self):
        ''' The string representation of an Event class outputs details of an
        Event object's start, end, location, and summary properties. The output
        is formatted to conform with the event representation requirements
        described in the SENG 265 Assignment 4 description.
        '''
        return "{} to {}: {}".format(self.start.strftime("%l:%M %p"),\
                                      self.end.strftime("%l:%M %p"),\
                                      self.summary + " {{" + self.location + "}}")
    
    def to_datetime(self, ical_str: str) -> datetime.datetime:
        '''  A helper method that is used during Event object initialization to convert
        start and end date strings in ical format to a datetime object. The helper method 
        takes one input variable of the ical date string.
        '''
        return datetime.datetime.strptime(ical_str, "%Y%m%dT%H%M%S")

    def has_rule(self) -> bool:
        ''' Checks if the Event object has a rule  '''
        return (self.rrule != None)

    def parse_rule(self):
        ''' Returns a copy of the Event object it is called on, with the start date
        property incremented by a week. It is assumed this method is called on Event
        objects that have a rrule (and thus the has_rule method returns true).
        If incrementing the Event start date by a week places the start date beyond the
        Event object's rrule until date, a NoneType object is returned.
        '''
        until = re.search(r'(UNTIL=(.*);)', self.rrule)[2]
        until = self.to_datetime(until);
        nextdate = self.start + datetime.timedelta(weeks = 1)
        temp_event = None
        if (nextdate < until):
            temp_event = Event(self.data)
            temp_event.start = nextdate

        return temp_event
