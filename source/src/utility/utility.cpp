/*-------------------------------------------------------------------------------
 This file is part of Ranger.

 Ranger is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Ranger is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Ranger. If not, see <http://www.gnu.org/licenses/>.

 Written by:

 Marvin N. Wright
 Institut für Medizinische Biometrie und Statistik
 Universität zu Lübeck
 Ratzeburger Allee 160
 23562 Lübeck

 http://www.imbs-luebeck.de
 wright@imbs.uni-luebeck.de
 #-------------------------------------------------------------------------------*/

#include <math.h>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "utility.h"
#include "globals.h"
#include "Data.h"

void equalSplit(std::vector<uint>& result, uint start, uint end, uint num_parts) {

  result.reserve(num_parts + 1);

  // Return range if only 1 part
  if (num_parts == 1) {
    result.push_back(start);
    result.push_back(end + 1);
    return;
  }

  // Return vector from start to end+1 if more parts than elements
  if (num_parts > end - start + 1) {
    for (uint i = start; i <= end + 1; ++i) {
      result.push_back(i);
    }
    return;
  }

  uint length = (end - start + 1);
  uint part_length_short = length / num_parts;
  uint part_length_long = (uint) ceil(length / ((double) num_parts));
  uint cut_pos = length % num_parts;

  // Add long ranges
  for (uint i = start; i < start + cut_pos * part_length_long; i = i + part_length_long) {
    result.push_back(i);
  }

  // Add short ranges
  for (uint i = start + cut_pos * part_length_long; i <= end + 1; i = i + part_length_short) {
    result.push_back(i);
  }
}

void loadDoubleVectorFromFile(std::vector<double>& result, std::string filename) {

  // Open input file
  std::ifstream input_file;
  input_file.open(filename);
  if (!input_file.good()) {
    throw std::runtime_error("Could not open file: " + filename);
  }

  // Read the first line, ignore the rest
  std::string line;
  getline(input_file, line);
  std::stringstream line_stream(line);
  double token;
  while (line_stream >> token) {
    result.push_back(token);
  }
}

double mostFrequentValue(std::unordered_map<double, size_t>& class_count, std::mt19937_64 random_number_generator) {
  std::vector<double> major_classes;

  // Find maximum count
  size_t max_count = 0;
  for (auto& class_value : class_count) {
    if (class_value.second > max_count) {
      max_count = class_value.second;
      major_classes.clear();
      major_classes.push_back(class_value.first);
    } else if (class_value.second == max_count) {
      major_classes.push_back(class_value.first);
    }
  }

  if (major_classes.size() == 1) {
    return major_classes[0];
  } else {
    // Choose randomly
    std::uniform_int_distribution<size_t> unif_dist(0, major_classes.size() - 1);
    return major_classes[unif_dist(random_number_generator)];
  }
}

double computeConcordanceIndex(Data* data, std::vector<double>& sum_chf, size_t dependent_varID, size_t status_varID,
    std::vector<size_t>& sample_IDs) {

  // Compute concordance index
  double concordance = 0;
  double permissible = 0;
  for (size_t i = 0; i < sum_chf.size(); ++i) {
    size_t sample_i = i;
    if (!sample_IDs.empty()) {
      sample_i = sample_IDs[i];
    }
    double time_i = data->get(sample_i, dependent_varID);
    double status_i = data->get(sample_i, status_varID);

    for (size_t j = i + 1; j < sum_chf.size(); ++j) {
      size_t sample_j = j;
      if (!sample_IDs.empty()) {
        sample_j = sample_IDs[j];
      }
      double time_j = data->get(sample_j, dependent_varID);
      double status_j = data->get(sample_j, status_varID);

      if (time_i < time_j && status_i == 0) {
        continue;
      }
      if (time_j < time_i && status_j == 0) {
        continue;
      }
      if (time_i == time_j && status_i == status_j) {
        continue;
      }

      permissible += 1;

      if (time_i < time_j && sum_chf[i] > sum_chf[j]) {
        concordance += 1;
      } else if (time_j < time_i && sum_chf[j] > sum_chf[i]) {
        concordance += 1;
      } else if (sum_chf[i] == sum_chf[j]) {
        concordance += 0.5;
      }

    }
  }

  return (concordance / permissible);

}

std::string uintToString(uint number) {
  return std::to_string(number);
}

std::string beautifyTime(uint seconds) {
  std::string result;

  // Add seconds, minutes, hours, days if larger than zero
  uint out_seconds = (uint) seconds % 60;
  result = uintToString(out_seconds) + " seconds";
  uint out_minutes = (seconds / 60) % 60;
  if (seconds / 60 == 0) {
    return result;
  } else if (out_minutes == 1) {
    result = "1 minute, " + result;
  } else {
    result = uintToString(out_minutes) + " minutes, " + result;
  }
  uint out_hours = (seconds / 3600) % 24;
  if (seconds / 3600 == 0) {
    return result;
  } else if (out_hours == 1) {
    result = "1 hour, " + result;
  } else {
    result = uintToString(out_hours) + " hours, " + result;
  }
  uint out_days = (seconds / 86400);
  if (out_days == 0) {
    return result;
  } else if (out_days == 1) {
    result = "1 day, " + result;
  } else {
    result = uintToString(out_days) + " days, " + result;
  }
  return result;
}

size_t roundToNextMultiple(size_t value, uint multiple) {

  if (multiple == 0) {
    return value;
  }

  int remainder = value % multiple;
  if (remainder == 0) {
    return value;
  }

  return value + multiple - remainder;
}

void splitString(std::vector<std::string>& result, std::string input, char split_char) {

  std::istringstream ss(input);
  std::string token;

  while (std::getline(ss, token, split_char)) {
    result.push_back(token);
  }
}

std::string checkUnorderedVariables(Data* data, std::vector<std::string> unordered_variable_names) {
  size_t num_rows = data->getNumRows();
  std::vector<size_t> sampleIDs(num_rows);
  std::iota(sampleIDs.begin(), sampleIDs.end(), 0);

  // Check for all unordered variables
  for (auto& variable_name : unordered_variable_names) {
    size_t varID = data->getVariableID(variable_name);
    std::vector<double> all_values;
    data->getAllValues(all_values, sampleIDs, varID);

    // Check level count
    size_t max_level_count = 8 * sizeof(size_t) - 1;
    if (all_values.size() > max_level_count) {
      return "Too many levels in unordered categorical variable " + variable_name + ". Only "
          + uintToString(max_level_count) + " levels allowed on this system.";
    }

    // Check positive integers
    if (!checkPositiveIntegers(all_values)) {
      return "Not all values in unordered categorical variable " + variable_name + " are positive integers.";
    }
  }
  return "";
}

bool checkPositiveIntegers(std::vector<double>& all_values) {
  for (auto& value : all_values) {
    if (value < 1 || !(floor(value) == value)) {
      return false;
    }
  }
  return true;
}