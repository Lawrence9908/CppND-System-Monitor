#include <dirent.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm> // for
#include <numeric>

#include "linux_parser.h"

using std::string;
using std::vector;
using  std::replace;
using std::ifstream;
using std::stoi;
using std::stol;
using std::istringstream;
using std::stof;
using std::accumulate;
using std::to_string;
// Read and return the name of the Operating System from the filesystem
string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

// Read and return the Kernel version from the filesystem
string LinuxParser::Kernel() {
  string os, version, kernel;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

// BONUS: Update this to use std::filesystem
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}
// Read and return the system memory utilization
float LinuxParser::MemoryUtilization() {
  string MemFree, MemTotal, value, key, line;
  std::ifstream stream(kProcDirectory + kMeminfoFilename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "MemTotal:")
          MemTotal = value;
        if (key == "MemFree:")
          MemFree = value;
      }
    }
  }
  return (stof(MemTotal)-stof(MemFree)) / stof(MemTotal);
}

// Read and return the system uptime
long LinuxParser::UpTime() {
  string line; long uptime;
  std::ifstream stream(kProcDirectory + kUptimeFilename);
  if (stream.is_open()) {
     stream >> uptime;
     return uptime;
  }
  return uptime;
}

// Read and return the number of jiffies for the system
long LinuxParser::Jiffies() {
  vector<string> values = LinuxParser::CpuUtilization();
  vector<CPUStates> all = {kUser_, kNice_, kSystem_, kIdle_, kIOwait_, kIRQ_, kSoftIRQ_, kSteal_};
  long total_number = 0;
  for (CPUStates state : all) {
    total_number += stol(values[state]);
  }
  return total_number;
}

// Read and return the number of active jiffies for a PID
long LinuxParser::ActiveJiffies(int pid) {
  string line, value;
  vector<string> values;
  std::ifstream stream(kProcDirectory + to_string(pid) + kStatFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    while (linestream >> value) {
      values.push_back(value);
    }
  }
  return stol(values[13] + values[14]);
}

// Read and return the number of active jiffies for the system
long LinuxParser::ActiveJiffies() {
  vector<string> jiffies = CpuUtilization();
  const vector<int> activeStates = {CPUStates::kUser_, CPUStates::kNice_, CPUStates::kSystem_,
                                    CPUStates::kIRQ_, CPUStates::kSoftIRQ_, CPUStates::kSteal_};

  long total = 0;
  for (int state : activeStates) {
    total += stol(jiffies[state]);
  }
  return total;
}


// Read and return the number of idle jiffies for the system
long LinuxParser::IdleJiffies() {
  vector<string> jiffies = CpuUtilization();
  return stol(jiffies[CPUStates::kIdle_]) + stol(jiffies[CPUStates::kIOwait_]);
}

// Read and return CPU utilization
vector<string> LinuxParser::CpuUtilization() {
  string line, value, key;
  vector<string> values;
  std::ifstream stream(kProcDirectory + kStatFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> key;
    while (linestream >> value) {
      values.push_back(value);
    };
  }
  return values;
}

// Read and return the total number of processes
int LinuxParser::TotalProcesses() {
  std::string total_processes = LinuxParser::GetStat(kStatFilename, "processes");
  return stoi(total_processes);
}

// Read and return the number of running processes
int LinuxParser::RunningProcesses() {
  string running_processes = LinuxParser::GetStat(kStatFilename, "procs_running");
  return stoi(running_processes);
}

// Read and return the command associated with a process
string LinuxParser::Command(int pid) {
  string command;
  std::ifstream stream(kProcDirectory + to_string(pid) + kCmdlineFilename);
  if (stream.is_open())
    std::getline(stream, command);
  if(command.length() >= 30)
    command = command.substr(0, 30).append("...");
  return command;
}

// Read and return the memory used by a process
string LinuxParser::Ram(int pid) {
  long value  = stol(LinuxParser::GetStat(to_string(pid) + kStatusFilename, "VmSize:")) / 1024;
  return std::to_string(value);
}

// Read and return the user ID associated with a process
string LinuxParser::Uid(int pid) {
  return LinuxParser::GetStat(to_string(pid) + kStatusFilename, "Uid:");
}

// Read and return the user associated with a process
string LinuxParser::User(int pid) {
  string user_name{"unknown user"}, x, line, key;
  std::ifstream stream(kPasswordPath);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      while (linestream >> user_name >> x >> key) {
        if (key == LinuxParser::Uid(pid)) {
          return user_name;
        }
      }
    }
  }
  return user_name; 
}

// Read and return the uptime of a process
long LinuxParser::UpTime(int pid) {
  string uptime, line;
  long clock_ticks;
  std::ifstream stream(kProcDirectory + to_string(pid) + kStatFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    for (int x = 0; x < 22; ++x){
      linestream >> uptime;
    }
  }
  clock_ticks = std::stol(uptime)/sysconf(_SC_CLK_TCK);
  return LinuxParser::UpTime() - clock_ticks;
}

string LinuxParser::GetStat(std::string filename, std::string key_value){
  std::string value{"0"}, key, line;
  std::ifstream stream(kProcDirectory +filename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == key_value) {
          return value;
        }
      }
    }
  }
  return value;
}
 