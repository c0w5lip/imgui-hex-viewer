#include "parser.h"

#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <fstream>
#include <vector>
#include <sstream>

void getRunningProcesses(std::string processes[], const char* processes_cstr[], int n_processes) {
	HANDLE process_snapshot = 0;
	PROCESSENTRY32 pe32 = { 0 };

	pe32.dwSize = sizeof(PROCESSENTRY32);
	process_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	Process32First(process_snapshot, &pe32);

	int i = 0;

	do {
		if (i >= n_processes) {
			break;
		}

		std::string process_name = pe32.szExeFile;
		processes[i] = process_name;
		processes_cstr[i] = processes[i].c_str();

		i++;

	} while (Process32Next(process_snapshot, &pe32));
	CloseHandle(process_snapshot);
}


void readOpenedProcess(const char* file_path, std::string bytes[], const char* bytes_cstr[], const std::size_t n_bytes) {
	using byte = std::uint8_t;

	if (std::ifstream file{ file_path, std::ios::binary })
	{
		std::vector<char> buff(n_bytes);

		if (file.read(buff.data(), buff.size()))
		{
			const auto nread = file.gcount();
			std::vector<byte> bytes_read(buff.begin(), buff.begin() + nread);
			int i = 0;
			for (byte b : bytes_read) {

				std::string current_byte = std::to_string(b);
				bytes[i] = current_byte;
				bytes_cstr[i] = bytes[i].c_str();
				i++;
			}
		}
	}
}

void readAttachedProcess(const char* process_name, std::string bytes[], const char* bytes_cstr[], const std::size_t n_bytes) {
	HANDLE process_snapshot = 0;
	HANDLE module_snapshot = 0;
	PROCESSENTRY32 pe32 = { 0 };
	MODULEENTRY32 me32;

	DWORD exitCode = 0;

	pe32.dwSize = sizeof(PROCESSENTRY32);
	me32.dwSize = sizeof(MODULEENTRY32);

	process_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	Process32First(process_snapshot, &pe32);

	do {
		if (strcmp(pe32.szExeFile, process_name) == 0) {
			module_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pe32.th32ProcessID);

			HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, true, pe32.th32ProcessID);

			Module32First(module_snapshot, &me32);
			do {
				if (strcmp(me32.szModule, process_name) == 0) {
					unsigned char* buffer = (unsigned char*)calloc(1, me32.modBaseSize);

					SIZE_T dwRead;
					ReadProcessMemory(process, (void*)me32.modBaseAddr, buffer, me32.modBaseSize, &dwRead);

					for (int i = 0; i < n_bytes; i++) {
						std::string current_byte = std::to_string(buffer[i]);
						bytes[i] = current_byte;
						bytes_cstr[i] = bytes[i].c_str();
					}

					free(buffer);
				}
			} while (Module32Next(module_snapshot, &me32));

			CloseHandle(process);
			break;
		}
	} while (Process32Next(process_snapshot, &pe32));
}
