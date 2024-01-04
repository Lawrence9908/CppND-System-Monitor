#include <string>


#include "format.h"

using std::string;
using std::to_string;

string Format::ElapsedTime(long seconds) 
{ 
  int hours  = seconds/3600;
  int minutes  = (seconds % 3600)/60;
  return std::to_string(hours ) + ":" + std::to_string(minutes) + ":" + std::to_string((seconds  % 60));  
}