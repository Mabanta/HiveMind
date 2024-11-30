from dv import AedatFile
import numpy as np
import cv2
import math

math_e = math.e

alpha = 0.15 #This factor controls how sensitive a cluster is to location change based on new spikes
#A higher value will cause the cluster to adapt more quickly, but it will also move more sporadically

SCALE_FACTOR = 0.995 #A scale factor of 0 means no accumulation
IMG_SCALE_FACTOR = 0.4
#Scale factors close to 1 mean accumulation for a long time

frame_rate = 100;
DISPLAY_TIME = int(1000000/frame_rate)
#Frame rate is used to control the display
#The actual algorithm won't use frames, but we have to use frames if we want to see the data

update_rate = 150;
DELAY_TIME = int(1000000/update_rate)
#This controls how often certain costly procedures are performed, such as checking for new clusters

start_time = 0
FIRST_EVENT = 200000
#The first few seconds are just the camera being shaky, so we skip them

BLUR_SCALE = 20
#This algorithm uses a "blur" to make it easier to detect a lot of events occurring in the same region
#The algorithm breaks the time surface into 20 x 20 regions and keeps track of how many events have occurred in each region
#The blur scale controls the size of each region
increase_factor_blurred = 0.18
#This controls how much each event contributes to the regions in the blurred time surface
#A higher number will make each region more sensitive to individual events


max_clusters = 20 #This puts a limit on how many clusters can be formed
cluster_init_threshold = 0.9 #This is the value that a region in the blurred time surface must reach in order to initiate a cluster
cluster_sustain_threshold = 18 #This is the number of events that must occur within a certain time inside a cluster in order for it to survive
cluster_sustain_time = 40000 #This is the amount of time that the program waits before checking if a cluster needs to be removed



total_clusters = 0
total_cross = 0
net_cross = 0

radius_growth_factor = 1.0008
radius_shrink_factor = 0.998

colors = [(0,0,255),(0,255,0),(255,0,0),(255,255,0),(255,0,255)]
color_index = 0

class Cluster():

    def __init__(self,x,y,color):
        self.x = x
        self.y = y
        self.radius = 25.0
        self.vel_x = 0
        self.vel_y = 0
        self.prev_x = x
        self.prev_y = y
        self.color = color
        #This controls how large the clusters are
        self.count = 0

        self.side = 1

    def distance(self,x,y):
        return ((x - self.x)**2 + (y-self.y)**2)**0.5
    #This function determines how far an event is from a cluster

    def inRange(self,x,y):
        if self.distance(x,y) < self.radius:
            return True
        return False
    #This determines whether an event happened inside the cluster

    def borderRange(self,x,y):
        if self.distance(x,y) < self.radius * 1.33:
            return True
        return False

    def otherClusterRange(self,x,y):
        if self.distance(x,y) < self.radius * 2:
            return True
        return False

    def shift(self,x,y):
        self.x = (1-alpha)*self.x + alpha*x
        self.y = (1-alpha)*self.y + alpha*y
    #This shifts the location of the cluster based on events that happen inside it

def sigmoid(x):
    #return (math_e**x-math_e**(-x))/(math_e**x+math_e**(-x))
    return x / (1 + np.abs(x))
    #This function limits each pixel in a time surface to a max value of 1

def change_side(x):
    normalized = x - int(width/2)
    if (abs(normalized) > 10):
        return int(normalized / abs(normalized))
    else:
        return 0

with AedatFile('summer_bees_video_2022_08_13.aedat4') as f:
    # list all the names of streams in the file
    print(f.names)

    # Access dimensions of the event stream
    height, width = f['events'].size
    print(f'Height: {height}')
    print(f'Width: {width}')

    ts= np.zeros((height,width,3)) #Initializes a screen - its grayscale but uses 3 channels so that clusters can be drawn on the screen in red
    ts_blurred = np.zeros((int(height/BLUR_SCALE),int(width/BLUR_SCALE)))
    #Initializes the blurred time surface, used to control the creation of new clusters

    next_time = DELAY_TIME
    next_frame = DISPLAY_TIME
    next_sustain_time = cluster_sustain_time

    clusters = []

    events = np.hstack([packet for packet in f['events'].numpy()])
    #Puts the event data into an indexable array
    
    start_time = events[0]['timestamp'] #Sets the start time to the time that first event occurs
    prev_time = start_time

    for i in range(FIRST_EVENT,len(events)):
        e = events[i]
        
        event_time = e['timestamp'] - start_time #Assigns a time to the event based off of the initial time

        if not e['polarity']:
            ts[int(e['y']),int(e['x'])][:] = 1 #Updates the time surface - this is purely for visualization purposes at this point
            ts_blurred[int(e['y']/BLUR_SCALE),int(e['x']/BLUR_SCALE)] += increase_factor_blurred
            #Increases the value of the corresponding region in the blurred time surface

            distances = [] #Finds the distance of the event from each existing cluster
            for cluster in clusters:
                distances.append(cluster.distance(int(e['x']),int(e['y'])))
                cluster.x += cluster.vel_x * (event_time - prev_time)
                cluster.y += cluster.vel_y * (event_time - prev_time)

            if len(clusters) > 0:
                min_cluster = clusters[distances.index(min(distances))] #Finds the closest cluster to the event
                if min_cluster.inRange(int(e['x']),int(e['y'])): #If the event is inside the closest cluster, it updates the location of that cluster
                    min_cluster.shift(int(e['x']),int(e['y']))
                    min_cluster.count += 1 #Used to track the number of events that have occurred in the cluster

                elif min_cluster.borderRange(int(e['x']),int(e['y'])):
                    min_cluster.radius *= radius_growth_factor

            prev_time = event_time
            
        ts_blurred = ts_blurred*SCALE_FACTOR
        ts_blurred[ts_blurred > 1] = 1
        
        if (event_time > next_time): #Checks for new clusters (costly computation, can't be performed every event)
            next_time += DELAY_TIME;

            for x in range(int(width/BLUR_SCALE)):
                for y in range(int(height/BLUR_SCALE)):
                    
                    if (ts_blurred[y,x] > cluster_init_threshold) and (len(clusters) < max_clusters): 
                        alreadyAdded = False
                        
                        for cluster in clusters: 
                            if cluster.otherClusterRange(x*BLUR_SCALE,y*BLUR_SCALE):
                                alreadyAdded = True

                        if not alreadyAdded:
                            new_cluster = Cluster(x*BLUR_SCALE,y*BLUR_SCALE,colors[color_index])
                            clusters.append(new_cluster)
                            
                            if (new_cluster.x < (width / 2)):
                                cluster.side = -1
                                
                            total_clusters += 1
                            
                            color_index += 1
                            if (color_index > 4):
                                color_index = 0
                            #print(f"Total Clusters: {total_clusters}")
                            #Adds a new cluster if three criteria are met:
                            #The region must be greater than the cluster initialization threshold
                            #The region can't be inside an already existing cluster
                            #There can't be more clusters than the max limit

            for cluster in clusters: #Updates velocity and size of each cluster
                cluster.vel_x = (cluster.x - cluster.prev_x) / DELAY_TIME
                cluster.vel_y = (cluster.y - cluster.prev_y) / DELAY_TIME
                cluster.prev_x = cluster.x
                cluster.prev_y = cluster.y

                new_side = change_side(cluster.x)
                if (new_side != cluster.side and new_side != 0):
                    cluster.side = new_side
                    
                    total_cross += 1
                    net_cross -= cluster.side
                    print(f"Total Crossed: {total_cross}")
                    print(f"Net Crossed: {net_cross}")
                    print()

                cluster.radius *= radius_shrink_factor


            if (event_time > next_sustain_time):
                next_sustain_time += cluster_sustain_time
                
                for cluster in clusters: #Resets the event count of each cluster and removes clusters if they didn't reach the minimum number of events
                    if cluster.count < cluster_sustain_threshold:
                        clusters.remove(cluster)
                    cluster.count = 0
            

        if (event_time > next_frame):
            next_frame += DISPLAY_TIME
            
            track_img = ts.copy()
            #img_blurred = ts_blurred.copy()
            cv2.line(track_img, (int(width/2),0), (int(width/2),height), (0,0,255), 2)
            for cluster in clusters:
                cv2.circle(track_img,(int(cluster.x),int(cluster.y)),int(cluster.radius),cluster.color)
                

            
            cv2.imshow('Time_Surface',cv2.resize(track_img,(int(width/1),int(height/1)))) #Displays the time surface to the screen
            cv2.waitKey(1)

            '''cv2.imshow('Blurred',cv2.resize(img_blurred,(int(width/1),int(height/1)))) #Displays the time surface to the screen
            cv2.waitKey(1)'''

            ts = ts*IMG_SCALE_FACTOR

            
        

