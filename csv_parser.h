#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
  int x;
  int y;
} Vector;

typedef struct
{
    FILE* stream;     // File stream for csv file
    int n_planes;     // Max number of planes
    int n_time_steps; // Number of time steps in csv

    int counter;

} CSVParser;

void   CSV_Init (CSVParser* class, char* file_name, int n_planes, int n_time_steps);
Vector CSV_GetNextReading(CSVParser * class);
void   CSV_Close (CSVParser * class);

int    getField (char* line, int num); // Get the x or y coord of the plane