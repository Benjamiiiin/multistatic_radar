#!/usr/bin/env python

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Button
import random
import csv
import time
import os

m = 4 # Number of sensors vertically
n = 5 # Number of sensors horizontally
node_sep = 125    # Distance between sensors in km (must match c file)
n_time_steps = 21 # Number of time steps in CSV file (passed to c file)

# Define sensor positions in grid
x_grid = []
y_grid = []
for m_i in range(m):
    for n_i in range(n):
        x_grid.append(n_i*node_sep)
        y_grid.append(m_i*node_sep)

# Radius and angle used to generate start and end positions of object trajectory
radius = np.sqrt(pow(m*node_sep, 2) + pow(n*node_sep, 2))/2
angle = random.uniform(0, np.pi)

# Generate random start and end positions outside sensor grid boundaries
start_pos = [0.5*n*node_sep + radius*np.cos(angle), 
    0.5*m*node_sep + radius*np.sin(angle)]
end_pos = [0.5*n*node_sep + radius*np.cos(angle + np.pi), 
    0.5*m*node_sep + radius*np.sin(angle + np.pi)]

# Clip start and end positions to sensor grid boundary
start_pos[0] = max(0, min(start_pos[0], (n-1)*node_sep))
end_pos[0] = max(0, min(end_pos[0], (n-1)*node_sep))
start_pos[1] = max(0, min(start_pos[1], (m-1)*node_sep))
end_pos[1] = max(0, min(end_pos[1], (m-1)*node_sep))

# Interpolate start and end positions
x = np.linspace(start_pos[0], end_pos[0], n_time_steps)
y = np.linspace(start_pos[1], end_pos[1], n_time_steps)

# Write generated trajectory to CSV file
with open('build/test_plane.csv', 'wb') as csvfile:

    csv_writer = csv.writer(csvfile, delimiter=',',
                            quotechar='|', quoting=csv.QUOTE_MINIMAL)

    for i in range(n_time_steps):
        csv_writer.writerow([i, int(x[i]), int(y[i])])

margin = 50 # Set margin around plot

class Index(object):

    def __init__(self):  # Initialise plot

        self.fig, self.ax = plt.subplots(nrows=1, # Figure definition
            ncols=1) 
        self.sensors = plt.scatter(x_grid, y_grid, c='white', # Plot sensor grid
            lw=0, s=30, zorder=3, marker="s", label="Sensor Positions") 

        self.plt_handle, = plt.plot(x, y, lw=2, c='white', # Trajectory plot
            zorder=2, alpha=0.5, label="Actual Object Trajectory") 

        self.sct_handle = plt.scatter([], [], c='green',  # Plot sensor readings
            lw=1, edgecolors="lime", s=50, zorder=3, 
            label="Sensor Object Detections") 

        self.fake_origin, = plt.plot([0, 0], [0, 0], lw=1, c="orange", 
            label="Originating Sensor")

        self.ax.set_axis_bgcolor((0.1, 0.1, 0.1)) # Configure plot display
        self.ax.set_xlim([-margin, (n-1)*node_sep + margin])
        self.ax.set_ylim([-margin, (m-1)*node_sep + margin])
        self.ax.set_title('Multistatic Radar Simulation\n\n\n\n')
        self.ax.set_xlabel('x position (km)')
        self.ax.set_ylabel('y position (km)')
        self.fig.tight_layout(pad=3)
        self.fig.canvas.set_window_title('Multistatic Radar Simulation')

        self.leg = plt.legend(handles=[self.sensors, self.sct_handle, 
                                       self.plt_handle, self.fake_origin], 
            bbox_to_anchor=(0., 1.02, 1., .102), loc=3,
            ncol=2, mode="expand", borderaxespad=0., frameon=1)
        frame = self.leg.get_frame()
        frame.set_facecolor('gray')

    
    def next(self, event):  # On button press
        xdata = []
        ydata = []

        # Run the simulation and allow short delay for execution
        os.system("cd build; mpirun -np 21 ./simulator " + str(n_time_steps)) 
        time.sleep(3)

        # Open the master log file and plot the radar-detected trajectory
        with open('build/master_log.csv') as f:
            reader = csv.reader(f, delimiter=',')

            line_num = 0
            for row in reader:
                if line_num > 0:
                    xdata.append(int(row[1])) # Record detected position
                    ydata.append(int(row[2]))

                    self.ax.plot( # Plot line to the originating sensor
                        [int(row[4])*node_sep, int(row[1])], 
                        [(m-1)*node_sep - int(row[3])*node_sep, int(row[2])], 
                        lw=1, c='orange', alpha=0.5) 

                line_num += 1    

        self.sct_handle.set_offsets(np.c_[xdata,ydata])
        plt.draw()

callback = Index()
axnext = plt.axes([0.81, 0.025, 0.1, 0.075]) # Create a button
bnext = Button(axnext, 'Simulate')
bnext.on_clicked(callback.next)

plt.show()
