//--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--
// File: csv_parser.c
// Author: Ben Steer
//
// Description:
//  Provides essentially an "object" which acts as a CSV file parser
//  for a given CSV file. The object can be called using the provided 
//  methods to return the x and y position of a simulated object at the
//  next timestep, as defined in the file.
//
//--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--

#include "csv_parser.h"


//--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--
// CSV_Init(...):
//
//    Initialise the CSV parser, opening the CSV file and setting 
//    "class" variables to initial values.
//--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
void  CSV_Init (CSVParser * class, char* file_name, int n_planes, int n_time_steps)
{
  class->stream       = fopen(file_name, "r");
  class->n_planes     = n_planes;
  class->n_time_steps = n_time_steps;

  class->counter = 0;
}


//--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--
// getField(char* line, int num):
//
//   Gets the integer value at a CSV column given by "num" on a given 
//    line, provided in "line".
//--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
int getField (char* line, int num)
{
  char* tmp = strdup(line);
  char *token = strtok(tmp, ",");
  int cnt = 0;
 
  while(token != NULL && cnt < num) 
  {
    cnt++; 
    token = strtok(NULL, ",");
  }	  
  
  int value = atoi(token);
  free(tmp);

  return value;
}


//--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--
// CSV_GetNextReading(CSVParser * class):
//
//   Returns the position of the simulated object at the next timestep.
//--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
Vector CSV_GetNextReading(CSVParser * class)
{
  Vector pos;

  if (class->counter < class->n_time_steps) // Not at end of time
  {
    char line[512];                  // Store the line
    fgets(line, 512, class->stream); // Get the next line
    
    pos.x = getField(line, 1); // Get the x and y positions
    pos.y = getField(line, 2);
  }
  else
  {    
    pos.x = -1000; // Dummy values if at end of time
    pos.y = -1000;
  }  

  class->counter++; // Increment timestep

  return pos;
}


//--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--
// CSV_Close (CSVParser * class):
//
//   Closes the CSV file.
//--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
void CSV_Close (CSVParser * class)
{
  fclose(class->stream);
}