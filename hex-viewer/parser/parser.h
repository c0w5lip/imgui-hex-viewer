#pragma once

#include <string>

void getRunningProcesses(std::string processes[], const char* processes_cstr[], int n_processes);

void readOpenedProcess(const char* file_path, std::string bytes[], const char* bytes_cstr[], const std::size_t n_bytes);
void readAttachedProcess(const char* process_name, std::string bytes[], const char* bytes_cstr[], const std::size_t n_bytes);
