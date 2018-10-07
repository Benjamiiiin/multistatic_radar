//--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--
// File: main.c
// Author: Ben Steer
//
// Description:
//  Uses OpenMPI to simulate a multistatic radar array, representing 
//  each sensor as an individual process in the grid of a Cartesian
//  communicator. A simulated object is passed through the grid and
//  radars provide sensor-fused position readings to a master process
//  to log the trajectory of the object.
//
//--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--

#include "mpi.h"    // Standard includes
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "csv_parser.h" // Include custom header file for CSV parsing

const int NODE_SEP    = 125; // Separation in km between radar sensors
const int RADAR_RANGE = 170; // Range of a single radar station
const int MAX_NOISE   = 30;  // Max noise (in km) to apply to sensor reading


//--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--
// getAdjacencies(int* my_coord, int adj[4][2], int* n_adjs):
//
//    Calculates the grid positions of the 4 sensors adjacent to the 
//      sensor provided in "my_coord" and stores them in "adj[4][2]".
//      The number of adjacent sensors is recorded in "n_adjs", and
//      invalid adjacencies are recorded in "adj[4][2]" as -1. Grid
//      positions are in Cartesian communicator coordinates (not in
//      kilometers).
//--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
void getAdjacencies(int* my_coord, int adj[4][2], int* n_adjs)
{
    adj[0][0] = my_coord[0] - 1; // Sensor ABOVE
    adj[0][1] = my_coord[1];

    adj[1][0] = my_coord[0];     // Sensor RIGHT
    adj[1][1] = my_coord[1] + 1;

    adj[2][0] = my_coord[0] + 1; // Sensor BELOW
    adj[2][1] = my_coord[1];

    adj[3][0] = my_coord[0];     // Sensor LEFT
    adj[3][1] = my_coord[1] - 1;

    if (my_coord[0] == 0) // We are in TOP ROW
    {
        adj[0][0] = -1;   // Sensor ABOVE is invalid
        adj[0][1] = -1;  
        (*n_adjs)--;
    }   
    if (my_coord[1] == 4) // We are in RIGHT COLUMN
    {
        adj[1][0] = -1;   // Sensor to RIGHT is invalid
        adj[1][1] = -1;  
        (*n_adjs)--;
    }
    if (my_coord[0] == 3) // We are in BOTTOM ROW
    {
        adj[2][0] = -1;   // Sensor BELOW is invalid
        adj[2][1] = -1;
        (*n_adjs)--;  
    }
    if (my_coord[1] == 0) // We are in LEFT COLUMN
    {
        adj[3][0] = -1;   // Sensor to LEFT is invalid
        adj[3][1] = -1;  
        (*n_adjs)--;
    }
}


//--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--
// inProximity(Vector target_pos, Vector my_xy, Vector *reading):
//
//    Simulates the sensing of an object from a single radar station. If 
//      the simulated object is within range of the sensor, the function
//      returns a 1 and stores a noisy reading of the object's position
//      in the field "reading". If object is outside radar range, 0 is
//      returned. The object's actual position is provided in 
//      "target_pos" and the radar station's position in "my_xy".
//--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
int inProximity(Vector target_pos, Vector my_xy, Vector *reading)
{
    // Calculate distance from this sensor to the target object
    int distance =  sqrt(pow(target_pos.x - my_xy.x, 2)
                       + pow(target_pos.y - my_xy.y, 2));

    if (distance < RADAR_RANGE) // If the target is within sensing range...
    {
        // Create a noisy radar reading from the actual target position.

        int noise = (rand() % MAX_NOISE) - MAX_NOISE/2; // Generate noise
        reading->x = target_pos.x                       // Apply noise
            + round( pow(((float) distance / RADAR_RANGE), 2)*noise);

        noise = (rand() % MAX_NOISE) - MAX_NOISE/2; // Generate noise
        reading->y = target_pos.y                   // Apply noise
            + round( pow(((float) distance / RADAR_RANGE), 2)*noise);

        return 1; // Return True, object detected
    }
    else
        return 0; // Return False, object out of range
}


//--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--
// main(int argc, char *argv[]):
//
//    Main function to run the simulation using OpenMPI. The number of
//      time steps for the simulation is provided in the first command 
//      line argument.
//    The position of a simulated object is read from a CSV file over a 
//      number of time steps, and an array of radar sensors (each 
//      represented by a different process) attempt to detect the object
//      at each time step. If three sensors adjacent to a station 
//      detect the object, sensor fusion is performed on the detected 
//      positions to provide a better estimate, and the result is sent
//      to the master process. The master keeps a log of all sensor
//      reports to track the object's trajectory.
//--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
int main(int argc, char *argv[])
{
    int n_time_steps = atoi(argv[1]); // Number of time steps in simulation
    int n_planes = 1;                 // Number of planes in simulation
    int counter = 0;                  // Number of time steps elapsed

    int size, rank;        // MPI rank and size in world
    int dim[] = {4, 5};    // Dimensions of Cartesian grid for new communicator
    int period[] = {0, 0}; // No periodicity
    int reorder = 0;       // Do not reorder the processes

    MPI_Init(&argc,&argv); 

    MPI_Comm_size(MPI_COMM_WORLD, &size); // Get size of world
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get process rank in world

    Vector my_xy;    // Coordinates of this sensor in world
    int my_coord[2]; // Get coordinate of this sensor in the Cartesian communicator
    int adj[4][2];   // Get Cart. coord. of adjacent sensors: [top, right, bottom, left]
    int n_adjs = 4;  // Number of valid adjacent sensors

    FILE *log;          // Log file for master process
    int master_msg[13]; // Data to send to master
    int msg_cnt = 0;    // Cumulative number of messages sent to master
    
    srand((time(NULL) << rank)); // Seed random number generator for noise generation

    MPI_Group group_world;   // Separate processes into communicators using groups
    MPI_Group group_master; 
    MPI_Group group_sensors;

    int rank_master[1] = {20}; // Define process ranks belonging to each communicator
    int rank_sensors[20]; for (int i=0; i<20; i++) rank_sensors[i] = i;

    MPI_Comm_group(MPI_COMM_WORLD, &group_world);                  // Get world group
    MPI_Group_incl(group_world, 1, rank_master, &group_master);    // Create subgroups
    MPI_Group_incl(group_world, 20, rank_sensors, &group_sensors);

    // Create the two new communicators from the subgroups
    MPI_Comm comm_master; // Master communicator
    MPI_Comm comm_sensor; // Sensor communicator   
    MPI_Comm_create(MPI_COMM_WORLD, group_master, &comm_master);
    MPI_Comm_create(MPI_COMM_WORLD, group_sensors, &comm_sensor);

    MPI_Comm cart_comm; // Create Cartesian grid for sensor communicator
    
    if (rank == 20) // If we are the Master process
    {
        log = fopen("master_log.csv", "w"); // Create the log file
        fprintf(log, "time, x, y, src_y, src_x, sensors (y1, x1, y2, x2, ...)\n");

        printf("Starting multistatic radar simulation.\n");
        printf("-------------------------------------------------------------\n");
    }
    else // If we are one of the Sensor processes
    {   
        // Create the Cartesian communicator
        MPI_Cart_create(comm_sensor, 2, dim, period, reorder, &cart_comm);

        // Get my Cartesian coord in the grid
        MPI_Cart_coords(cart_comm, rank, 2, my_coord); 

        my_xy.x = my_coord[1]*NODE_SEP;     // Calculate my real coords (in km)
        my_xy.y = (3-my_coord[0])*NODE_SEP; 

        getAdjacencies(my_coord, adj, &n_adjs); // Get coordinates of adjacent sensors
    }

    CSVParser csv_parser; // Create a CSV parsing "object" and initialise it
    CSV_Init(&csv_parser, "test_plane.csv", n_planes, n_time_steps);

    MPI_Request reqs[n_adjs*2]; // Requests + status objects for each adjacent sensor
    MPI_Status stats[n_adjs*2];
    MPI_Status stat; 

    // Buffers for receiving info from adjacent sensors and sending readings to sensors
    int rcv_buf[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int snd_buf[2] = {0, 0};

    // Begin the simulation, iterating for each time step
    while (counter < n_time_steps)
    {        
        if (rank != 20) // If we are a Sensor process
        {
            Vector pos = CSV_GetNextReading(&csv_parser); // Read line of CSV file
            Vector reading;

            int inProx = inProximity(pos, my_xy, &reading); // Simulate radar reading

            if (inProx) // If this radar station detects an object
            {
                snd_buf[0] = reading.x; // Report object to all neighbouring sensors
                snd_buf[1] = reading.y;
            }
            else // No object detected
            {
                snd_buf[0] = 0;  // Report nothing to neighbours
                snd_buf[1] = 0;
            }

            for (int i=0; i<8; i++) rcv_buf[i] = 0; // Clear receive buffer

            int adj_cnt = 0; // Counter for accessing requests with the correct index

            // Send and receive readings from each adjacent node
            for (int i=0; i<4; i++) 
            {                
                if (adj[i][0] >= 0) // If valid adjacent node
                {
                    // Get world rank of adjacent node
                    int this_adj[2] = {adj[i][0], adj[i][1]}; 
                    int adj_rank;
                    MPI_Cart_rank(cart_comm, this_adj, &adj_rank);

                    MPI_Irecv(&rcv_buf[2*i], 2, MPI_INT, adj_rank, // Receive readings
                        MPI_ANY_TAG, comm_sensor, &reqs[2*adj_cnt]);

                    MPI_Isend(&snd_buf[0],   2, MPI_INT, adj_rank, i, // Send readings
                        comm_sensor, &reqs[2*adj_cnt +1]);

                    adj_cnt++; // Increment adjacent sensor counter
                }                
            }  

            MPI_Waitall(n_adjs*2, reqs, stats); // Wait for all messages to be passed 
            MPI_Barrier(comm_sensor); // Wait for all sensors to catch up to this point
            
            int n_activations = 0; // Count the number of adjacent sensor activations

            for (int i=0; i<4; i++) // For each neighbouring sensor...
            {
                if (rcv_buf[2*i] > 0) // Has neigbour sensed something?
                {
                    n_activations += 1;

                    // Get position of neighbour
                    int target_coord[2] = {adj[i][0], adj[i][1]};

                    // Record coordinate of activated sensor for master
                    master_msg[2*i]   = target_coord[0]; 
                    master_msg[2*i+1] = target_coord[1]; 
                }
                else // Neighbour has sensed nothing
                {
                    master_msg[2*i] = -1;   // Mark neighbour as not sensing for master
                    master_msg[2*i+1] = -1; // Mark neighbour as not sensing for master
                }
            } 

            if (n_activations > 2) // 3 or more neighbour detections is an activation
            {
                Vector fused_reading; // Store result of sensor fusion of readings
                fused_reading.x = 0;
                fused_reading.y = 0;

                for (int i=0; i<4; i++) // Perform sensor fusion (simply an average)
                {
                    fused_reading.x += rcv_buf[2*i];   // Sum
                    fused_reading.y += rcv_buf[2*i+1];
                }

                // Divide for average
                fused_reading.x = fused_reading.x / n_activations;
                fused_reading.y = fused_reading.y / n_activations;

                master_msg[8] = my_coord[0]; // Record position of this radar station
                master_msg[9] = my_coord[1]; 

                master_msg[10] = fused_reading.x; // Record fused radar reading x and y
                master_msg[11] = fused_reading.y;
                master_msg[12] = counter;         // Record current timestep
                
                // Send activation info to master process
                MPI_Send(&master_msg, 13, MPI_INT, 20, 0, MPI_COMM_WORLD); 
            }

            MPI_Barrier(comm_sensor); // Wait for all sensor nodes to catch up
            
            counter ++; 
        }
        else // We are the Master process
        {
            MPI_Recv(&master_msg, 13, MPI_INT, MPI_ANY_SOURCE, // Receive activation msg
                MPI_ANY_TAG, MPI_COMM_WORLD, &stat);

            if (master_msg[0] == -2) // Received exit message
            {
                printf("-------------------------------------------------------------\n");
                printf("Simulation finished with total of %d messages sent to master.\n", msg_cnt);

                counter = n_time_steps; // Exit while loop
                fclose(log);            // Close log file
            }
            else // Continue reading activation messages
            {
                printf("Time %d: Radar (%d, %d) detected object at x = %dkm, y = %dkm.\n", 
                         master_msg[12], master_msg[8], master_msg[9], master_msg[10], master_msg[11]);

                msg_cnt++; // Mark as having received another message 

                // Log timestep, x and y readings
                fprintf(log, "%d, %d, %d, ", master_msg[12], master_msg[10], master_msg[11]); 

                // Log activating sensor coords
                fprintf(log, "%d, %d, ", master_msg[8], master_msg[9]); 

                // Log the neighbouring sensor coords
                for (int i=0; i<8; i++) 
                {
                    fprintf(log, " %d,", master_msg[i]);
                }
                
                fprintf(log, "\n"); // End the log line
            }
        }
    }    

    if (rank != 20) // If Sensor processes are all finished
    {
        master_msg[0] = -2; // Send exit msg to master
        MPI_Send(&master_msg, 13, MPI_INT, 20, 0, MPI_COMM_WORLD);
    }
    
    CSV_Close(&csv_parser); // Close the CSV file

    MPI_Finalize();
}
