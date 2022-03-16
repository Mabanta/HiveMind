from dv import AedatFile
import numpy as np
import cv2

SCALE_FACTOR = 0.7 #Determines how long old events will have an influence on the time surface
#A scale factor of 0 means no accumulation
#Scale factors close to 1 mean accumulation for a long time

cluster_init_time = 5000
cluster_init_threshold = 30

cluster_monitor_time = 10000
cluster_destroy_threshold = 20
print(cluster_monitor_time*cluster_destroy_threshold)
cluster_max_length = 10

alpha = 0.02

frame_rate = 200;
DELAY_TIME = int(1000000/frame_rate)
#Frame rate is used to control the display
#The actual algorithm won't use frames, but we have to use frames if we want to see the data

start_time = 0
FIRST_EVENT = 1060000
#The first few seconds are just the camera being shaky, so we skip them

class Cluster():

    def __init__(self,x,y,init_time):
        self.x = x
        self.y = y
        self.radius = 10
        self.event_count = 1
        self.init_time = init_time

    def find_distance(self,x,y):
        return ((x-self.x)**2 + (y-self.y)**2)**0.5

    def in_range(self,x,y):
        if ( ((x-self.x)**2 + (y-self.y)**2)**0.5 <= self.radius):
            return True
        return False


with AedatFile('CanadaGeese_2.aedat4') as f:
    # list all the names of streams in the file
    print(f.names)

    # Access dimensions of the event stream
    height, width = f['events'].size
    print(f'Height: {height}')
    print(f'Width: {width}')

    ts= np.zeros((width,height,3)) + 0.5 #Initializes a grayscale screen
    next_time = DELAY_TIME

    events = np.hstack([packet for packet in f['events'].numpy()])
    #Puts the event data into an indexable array
    
    start_time = events[0]['timestamp'] #Sets the start time to the time that first event occurs

    potential_clusters = np.zeros((int(width/10)+1,int(height/10)+1))
    clusters = []

    test_cluster = Cluster(100,64,start_time)
    clusters.append(test_cluster)

    prev_time = 0
    prev_time_filler = 0

    for i in range(FIRST_EVENT,len(events)):
        e = events[i]
        
        event_time = e['timestamp'] - start_time #Assigns a time to the event based off of the initial time
            
        ts[int(e['y']),int(e['x'])][:] = e['polarity']*1 #Turns a pixel in the time surface white or black depending on whether it was a positive or negative spike

        event_added = False
        distances = []
        
        for cluster in clusters:
            distances.append(cluster.find_distance(e['x'],e['y']))

        if (len(distances) > 0):
            closest_cluster = clusters[distances.index(min(distances))]
            if (closest_cluster.in_range(e['x'],e['y']) and len(clusters) < cluster_max_length):
                if (closest_cluster.event_count < cluster_monitor_time*cluster_destroy_threshold):
                    closest_cluster.event_count += cluster_monitor_time
                
                closest_cluster.x = (1-alpha) * closest_cluster.x + alpha*e['x']
                closest_cluster.y = (1-alpha) * closest_cluster.y + alpha*e['x']
                
                event_added = True

        for cluster in clusters:
            if (cluster.event_count < 0):
                clusters.remove(cluster)

            cluster.event_count -= (event_time - prev_time)
            '''if ((event_time - prev_time_filler) > cluster_init_time):
                prev_time_filler = event_time
                cluster.event_count = 0'''

        if not event_added:
            potential_clusters[int(e['x']/10)][int(e['y']/10)] += 1

        '''if not event_added:
            for cluster in potential_clusters:
                if cluster.in_range(e['x'],e['y']):
                    cluster.event_count += 1
                    event_added = True

                if ((event_time - cluster.init_time) > cluster_init_time):
                    if (cluster.event_count > cluster_init_threshold):
                        clusters.append(cluster)
                        cluster.event_count = cluster_monitor_time*cluster_destroy_threshold
                
                    potential_clusters.remove(cluster)

        if not event_added:
            new_cluster = Cluster(e['x'],e['y'],event_time)
            potential_clusters.append(new_cluster)'''

        prev_time = event_time

        
        if (event_time > next_time): #Updates the display after one frame has passed
            next_time += DELAY_TIME;
            
            track_img = ts.copy()

            
            for cluster in clusters:
                cv2.circle(track_img,(int(cluster.y),int(cluster.x)),10,(0,0,255))

            for x in range(int(width/10)+1):
                for y in range(int(height/10)+1):
                    if (potential_clusters[x][y] > cluster_init_threshold and (len(clusters) < cluster_max_length)):
                        new_cluster = Cluster(x+5,y+5,event_time)
                        clusters.append(new_cluster)

                    #print(f"{x},{y}: {potential_clusters[x][y]}")
                    potential_clusters[x][y] = 0

            #cv2.circle(track_img,(64,100),10,(0,0,255))
            #print(f"Event Count: {test_cluster.event_count}")
                
            cv2.imshow('Time_Surface',cv2.resize(track_img,(300,300))) #Displays the time surface to the screen
            cv2.waitKey(1)

            ts = ts*SCALE_FACTOR + 0.5*(1-SCALE_FACTOR) #Updates the time surface


