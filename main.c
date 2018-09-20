#include "mpi.h"
#include <stdio.h>
#include <math.h>

#include "csv_parser.h"

#define NODE_SEP 125;
#define RADAR_RANGE 170;

// mpicc main.c csv_parser.c -lm

void getAdjacencies(int* my_coord, int adj[4][2], int* n_adjs)
{
    adj[0][0] = my_coord[0] - 1; // ABOVE
    adj[0][1] = my_coord[1];
    adj[1][0] = my_coord[0];     // RIGHT
    adj[1][1] = my_coord[1] + 1;
    adj[2][0] = my_coord[0] + 1; // BELOW
    adj[2][1] = my_coord[1];
    adj[3][0] = my_coord[0];     // LEFT
    adj[3][1] = my_coord[1] - 1;

    if (my_coord[0] == 0) // We are in TOP ROW
    {
        adj[0][0] = -1;   
        adj[0][1] = -1;  
        (*n_adjs)--;
    }   
    if (my_coord[1] == 4) // We are in RIGHT COLUMN
    {
        adj[1][0] = -1;   
        adj[1][1] = -1;  
        (*n_adjs)--;
    }
    if (my_coord[0] == 3) // We are in BOTTOM ROW
    {
        adj[2][0] = -1;   
        adj[2][1] = -1;
        (*n_adjs)--;  
    }
    if (my_coord[1] == 0) // We are in LEFT COLUMN
    {
        adj[3][0] = -1;   
        adj[3][1] = -1;  
        (*n_adjs)--;
    }
}


int inProximity(Vector target_pos, Vector my_xy)
{
    int distance =  sqrt(pow(target_pos.x - my_xy.x, 2)
                         + pow(target_pos.y - my_xy.y, 2));
    int result = distance < RADAR_RANGE;
    //printf("target (%d, %d) vs me (%d, %d). Close? %d.\n", target_pos.x, target_pos.y, my_xy.x, my_xy.y, result); 
    return distance < RADAR_RANGE;
}


int main(int argc, char *argv[])
{
    int n_time_steps = 21; // Number of time steps in simulation
    int n_planes = 1; // Number of planes in simulation
    int counter = 0; // Number of time steps elapsed

    int size, rank;

    MPI_Comm cart_comm; // Create Cartesian grid for MPI communicator
    int dim[] = {4, 5};
    int period[] = {0, 0};
    int reorder = 0;

    MPI_Init(&argc,&argv);
    MPI_Cart_create(MPI_COMM_WORLD, 2, dim, period, reorder, &cart_comm);

    MPI_Comm_size(MPI_COMM_WORLD, &size); 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int my_coord[2]; // Get the coordinate of the current rank in the Cartesian grid
    MPI_Cart_coords(cart_comm, rank, 2, my_coord);

    Vector my_xy; // Coordinates in worlds
    my_xy.x = my_coord[1]*NODE_SEP;
    my_xy.y = (3-my_coord[0])*NODE_SEP; 
    //printf("(%d, %d) has world coords (%d, %d).\n", my_coord[0], my_coord[1], my_xy.x, my_xy.y); 

    int adj[4][2]; // Get adjacency of sensors, [top, right, bottom, left]
    int n_adjs = 4;    // Number of valid adjacent nodes
    getAdjacencies(my_coord, adj, &n_adjs);

    CSVParser csv_parser; // Create a CSV parsing "object"
    CSV_Init(&csv_parser, "test_plane.csv", n_planes, n_time_steps);

    MPI_Request reqs[n_adjs*2];
    MPI_Status stats[n_adjs*2];

    int rcv_buf[4] = {0, 0, 0, 0};
    int snd_buf = 0;

    while (counter < n_time_steps)
    {        
        Vector pos = CSV_GetNextReading(&csv_parser);
        snd_buf = inProximity(pos, my_xy);

        for (int i=0; i<4; i++) rcv_buf[i] = 0;

        int adj_cnt = 0; // For accessing reqs with right index
        // Send messages to each adjacent node
        for (int i=0; i<4; i++) 
        {                
            if (adj[i][0] >= 0) // If valid adjacent node
            {
                int this_adj[2] = {adj[i][0], adj[i][1]};
                int adj_rank;
                MPI_Cart_rank(cart_comm, this_adj, &adj_rank);

                MPI_Irecv(&rcv_buf[i], 1, MPI_INT, adj_rank, MPI_ANY_TAG, MPI_COMM_WORLD, &reqs[2*adj_cnt]);
                MPI_Isend(&snd_buf,    1, MPI_INT, adj_rank, i, MPI_COMM_WORLD, &reqs[2*adj_cnt +1]);
                adj_cnt++;
            }                
        }  

        MPI_Waitall(n_adjs*2, reqs, stats);
        //printf("(%d, %d) got %d.\n", my_coord[0], my_coord[1], rcv_buf[0]);   

        MPI_Barrier(MPI_COMM_WORLD); // Wait for all nodes to catch up
        
        int n_activations = 0;
        for (int i=0; i<4; i++) n_activations += rcv_buf[i];

        if (n_activations > 2)
        {
            printf("%d: (%d, %d) got %d.\n", counter, my_coord[0], my_coord[1], n_activations); 
        }

        MPI_Barrier(MPI_COMM_WORLD); // Wait for all nodes to catch up
        
        counter ++; // run once
    }

    
    
    CSV_Close(&csv_parser);

    MPI_Finalize();
}


