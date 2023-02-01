#
from dv import AedatFile
import time
import cv2
import pandas as pd
import numpy as np

ts = np.zeros((128,128)) + 0.5  # between 0.0 and 1.0

begin_s = 4 # in seconds 4
begin_us = begin_s * 1e6
start_flag = False
ROI_X = 40.0
ROI_Y = 75.0
ROI_hW = 15
w_track = 0.01
focus = np.zeros((2*ROI_hW, 2*ROI_hW))

next_frame_us = 0

with AedatFile('./CanadaGeese_2.aedat4') as f:
    # list all the names of streams in the file
    print(f.names)

    # Access dimensions of the event stream
    height, width = f['events'].size
    print ('height = %d'%height)
    print ('width = %d'%width)
    # loop through the "events" stream
    for e in f['events']:
        if not start_flag:
            start_us = e.timestamp
            start_flag = True
        t_us = e.timestamp - start_us
        if (t_us > begin_us):
            if (e.polarity == 1):
                ts[int(e.y),int(e.x)] = 1.0
            else:
                ts[int(e.y),int(e.x)] = 0.0
            if (ROI_X - ROI_hW < e.x < ROI_X + ROI_hW) and (ROI_Y - ROI_hW < e.y < ROI_Y + ROI_hW):
            # if e.polarity == 1:
            #     ts[int(e.y),int(e.x)] = 1.0  #ts[int(e.x),int(e.y)] = 1.0
                ROI_X = (1-w_track)*ROI_X + w_track*e.x
                ROI_Y = (1-w_track)*ROI_Y + w_track*e.y
                ROI_X = sorted([ROI_hW, ROI_X, 127-ROI_hW])[1]
                ROI_Y = sorted([ROI_hW, ROI_Y, 127-ROI_hW])[1]
                print('%d : %d : %d : %d ==> (%f, %f)'%(t_us, e.x, e.y, e.polarity, ROI_X, ROI_Y))
        if t_us > next_frame_us:
            track_img = ts.copy()
            ROIs = (int(ROI_X-ROI_hW),int(ROI_Y-ROI_hW))
            ROIe = (int(ROI_X+ROI_hW),int(ROI_Y+ROI_hW))
            cv2.rectangle(track_img,ROIs,ROIe,(255,255,255),1)
            cv2.imshow ('time surface',cv2.resize(track_img,(300,300),interpolation=cv2.INTER_NEAREST))
            cv2.waitKey(1)
            next_frame_us += 5000
            focus = track_img[int(ROI_Y-ROI_hW):int(ROI_Y+ROI_hW),int(ROI_X-ROI_hW):int(ROI_X+ROI_hW)]
            cv2.imshow('ROI',cv2.resize(focus,(100,100),interpolation=cv2.INTER_NEAREST))
            cv2.waitKey(1)
        #time.sleep(0.1)
        ts = 0.999*ts + 0.001*0.5
