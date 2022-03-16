#
from dv import AedatFile
import time
import cv2
import math
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

fig = plt.figure()
ax = fig.add_subplot(111, projection ='3d')

ROI_save_fname = input ('\nROI events save filename > ')

ROIspikes = np.zeros((20000,4)) # output spikes from the ROI
ROIi = 0
ts = np.ones((128,128))  # between 0.0 and 1.0
nfilter = np.zeros((128+2,128+2))

#begin_s = 2 # in seconds 4
begin_us = 1 * 1e6
ending_us = 2 * 1e6
#color_interval_us = int((ending_us - begin_us)/7)
time_window_us = ending_us - begin_us
mycolormap = plt.cm.get_cmap("viridis")
#color_palette = ('k','g','b','c','m','r','y')

start_flag = False
ROI_X = 64.0
ROI_Y = 64.0
ROI_hW = 10
w_track = 0.002
focus = np.ones((2*ROI_hW+1, 2*ROI_hW+1))

next_frame_us = 0
n_t_thresh = 1000 # 10 ms

with AedatFile('./LeftFast.aedat4') as f:
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
            if t_us - nfilter[e.y+1, e.x+1] < n_t_thresh: # NOISE FILTER
                if (e.polarity == 1):  # ON spikes
                    zz = 0#ts[int(e.y),int(e.x)] = 1.0
                else: # OFF Spikes
                    ts[int(e.y),int(e.x)] = 0.0
                # is the event in the ROI?
                if (ROI_X - ROI_hW < e.x < ROI_X + ROI_hW) and (ROI_Y - ROI_hW < e.y < ROI_Y + ROI_hW):
                    # if e.polarity == 1:
                    #     ts[int(e.y),int(e.x)] = 1.0  #ts[int(e.x),int(e.y)] = 1.0
                    ROI_X = (1-w_track)*ROI_X + w_track*e.x
                    ROI_Y = (1-w_track)*ROI_Y + w_track*e.y
                    ROI_X = sorted([ROI_hW, ROI_X, 127-ROI_hW])[1]
                    ROI_Y = sorted([ROI_hW, ROI_Y, 127-ROI_hW])[1]
                    #print('%d : %d : %d : %d ==> (%f, %f)'%(t_us, e.x, e.y, e.polarity, ROI_X, ROI_Y))
                    ROIe_x = int(e.x - ROI_X + ROI_hW)
                    ROIe_y = int(e.y - ROI_Y + ROI_hW)
                    ROIe_polarity = e.polarity
                    #print ('%d, %d, %d, %d'%(e.timestamp-start_us, ROIe_x, ROIe_y, ROIe_polarity))
                    if t_us < ending_us:
                        ROIspikes[ROIi,:] = e.timestamp-start_us, ROIe_x, ROIe_y, ROIe_polarity
                        ROIi += 1
                        print('%d, %d, %d, %d'%(e.timestamp-start_us, ROIe_x, ROIe_y, ROIe_polarity))
                        #if e.polarity == 1:
                        #    p = 0# ax.scatter(t_us, ROIe_y, ROIe_x,c='r',marker='o')
                        #else:
                        #    graylevel = (t_us - begin_us) / time_window_us
                        #    print ('graylevel = %f'%graylevel)
                        #    c_g = [graylevel]
                        #    ax.scatter(t_us/1e6, ROIe_y, ROIe_x, c=gray, cmap=cmhot)
            #else:
                #print ('rejected.')
            nfilter[e.y:e.y+2, e.x:e.x+2] = t_us  # 1 pixel neighborhood
            # finished with noise filter event processing


        if t_us > next_frame_us: # graphics display gate
            track_img = ts.copy()
            ROIs = (int(ROI_X-ROI_hW),int(ROI_Y-ROI_hW))
            ROIe = (int(ROI_X+ROI_hW),int(ROI_Y+ROI_hW))
            cv2.rectangle(track_img,ROIs,ROIe,(0,0,0),1)
            cv2.imshow ('time surface',cv2.resize(track_img,(300,300),interpolation=cv2.INTER_NEAREST))
            cv2.waitKey(1)
            next_frame_us += 5000
            focus = track_img[int(ROI_Y-ROI_hW):int(ROI_Y+ROI_hW+1),int(ROI_X-ROI_hW):int(ROI_X+ROI_hW+1)]
            cv2.imshow('ROI',cv2.resize(focus,(100,100),interpolation=cv2.INTER_NEAREST))
            cv2.waitKey(1)
        #time.sleep(0.1)
        ts = 0.999*ts + 0.001*1.0
    np.savez (ROI_save_fname,ROIspikes,ROIi)

    s_times = ROIspikes[0:ROIi,0]/1e6
    grays_ = s_times[::-1]
    ROIe_x_list = ROIspikes[0:ROIi,1]
    ROIe_y_list = ROIspikes[0:ROIi,2]
    ax.scatter(s_times, ROIe_y_list, ROIe_x_list, c= grays_, cmap=mycolormap, marker='.')
    plt.xlabel('Time')
    plt.ylabel('Image Y')
    plt.title('Off Events')
    plt.show()
