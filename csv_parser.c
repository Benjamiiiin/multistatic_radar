#include "csv_parser.h"

void  CSV_Init (CSVParser * class, char* file_name, int n_planes, int n_time_steps)
//void CSVParser::CSVParser (string file_name, int n_planes, int n_time_steps)
{
  class->stream       = fopen(file_name, "r");
  class->n_planes     = n_planes;
  class->n_time_steps = n_time_steps;

  class->counter = 0;
  
  char line[512]; // Get past the first line of names
  fgets(line, 512, class->stream); 
}


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


Vector CSV_GetNextReading(CSVParser * class)
{
  Vector pos;

  if (class->counter < class->n_time_steps) // Not at end of time
  {
    char line[512];           // Store the line
    fgets(line, 512, class->stream); // Get the next line
    
    pos.x = getField(line, 1);
    pos.y = getField(line, 2);
  }
  else
  {    
    pos.x = -1000; // Dummy values
    pos.y = -1000;
  }  

  class->counter++;

  return pos;
}


void CSV_Close (CSVParser * class)
{
  fclose(class->stream);
}