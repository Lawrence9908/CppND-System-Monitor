#include "processor.h"
#include "linux_parser.h"

// Returning the aggregate CPU utilization
float Processor::Utilization() {
  return LinuxParser::ActiveJiffies() * (2.f / LinuxParser::Jiffies());
}