#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define a 2-dimensional vector.
typedef struct
{
  int x;
  int y;
} Vector;

// Define an "object" representing the CSV parser.
typedef struct
{
    FILE* stream;     // File stream for CSV file
    int n_planes;     // Max number of planes
    int n_time_steps; // Number of time steps in CSV

    int counter;

} CSVParser;

// Define "class" methods. See C file for descriptions.
void   CSV_Init (CSVParser* class, char* file_name, int n_planes, int n_time_steps);
Vector CSV_GetNextReading(CSVParser * class);
void   CSV_Close (CSVParser * class);
int    getField (char* line, int num);