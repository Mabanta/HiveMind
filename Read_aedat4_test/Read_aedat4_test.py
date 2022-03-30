from dv import AedatFile
import numpy as np
import cv2

SCALE_FACTOR = 0.4 #Determines how long old events will have an influence on the time surface
#A scale factor of 0 means no accumulation
#Scale factors close to 1 mean accumulation for a long time

frame_rate = 500;
DELAY_TIME = int(1000000/frame_rate)
#Frame rate is used to control the display
#The actual algorithm won't use frames, but we have to use frames if we want to see the data

start_time = 0
FIRST_EVENT = 200000
#The first few seconds are just the camera being shaky, so we skip them

with AedatFile('tossing_multiple-2022_03_14_16_31_50.aedat4') as f:
    # list all the names of streams in the file
    print(f.names)

    # Access dimensions of the event stream
    height, width = f['events'].size
    print(f'Height: {height}')
    print(f'Width: {width}')

    ts= np.zeros((height,width)) #Initializes a grayscale screen
    next_time = DELAY_TIME

    events = np.hstack([packet for packet in f['events'].numpy()])
    #Puts the event data into an indexable array
    
    start_time = events[0]['timestamp'] #Sets the start time to the time that first event occurs

    for i in range(FIRST_EVENT,len(events)):
        e = events[i]
        
        event_time = e['timestamp'] - start_time #Assigns a time to the event based off of the initial time

        #if e['polarity']:
        #    ts[int(e['y']),int(e['x'])] = 1 #Turns a pixel in the time surface white or black depending on whether it was a positive or negative spike
        ts[int(e['y']),int(e['x'])] = e['polarity'];

        
        if (event_time > next_time): #Updates the display after one frame has passed
            next_time += DELAY_TIME;
            
            track_img = ts.copy()
            cv2.imshow('Time_Surface',cv2.resize(track_img,(300,300))) #Displays the time surface to the screen
            cv2.waitKey(1)

            ts = ts*SCALE_FACTOR + (1-SCALE_FACTOR)*0.5#Updates the time surface


#Ignore all this stuff, its old code that we probably won't need

    # loop through the "events" stream
    '''for e in f['events']:
        print(e.timestamp)
        print(e.polarity)'''

    # loop through the "frames" stream
    
    '''for frame in f['events']:
        print(frame.timestamp)
        cv2.imshow('out', frame.image)
        cv2.waitKey(1)'''

'''
with AedatFile('bee1_test_file.aedat4') as f:
    # events will be a named numpy array
    events = np.hstack([packet for packet in f['events'].numpy()])

    # Access information of all events by type
    timestamps, x, y, polarities = events['timestamp'], events['x'], events['y'], events['polarity']
    # Access individual events information
    print(events[123]['x'])
    print(x[123])
    # Slice events
    first_100_events = events[:100]'''

