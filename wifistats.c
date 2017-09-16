/*
CITS2002 Project 1 2017
Name(s):             Jakrin Juangbhanich
Student number(s):   21789272
Date:                18/9/2017
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <ctype.h>

#define UNKNOWN_VENDOR "UNKNOWN-VENDOR"
#define UNKNOWN_ADDRESS "??:??:??"
#define BROADCAST_ADDRESS "ff:ff:ff:ff:ff:ff"
#define TEMP_COPY_FILE_NAME "_tempCopy.txt"
#define TRANSMIT_DEVICE 't'
#define RECEIVE_DEVICE 'r'
#define TAB_TOKEN "\t"
#define NULL_CHAR '\0'

#define V_COUNT 25000
#define V_NAME_LENGTH 100
#define V_ADDRESS_LENGTH 10
#define OUTPUT_COUNT 500
#define OUTPUT_LENGTH 200

extern char**environ;

void Split(char* string, const char* delimiter, char* array[])
{
	// Split a string by a given delimiter, and write the results into an array.
	int i = 0;
	char* token = strtok(string, delimiter);
	while (token != NULL)
	{
		array[i] = token;
		i++;
		token = strtok(NULL, TAB_TOKEN);
	}
}

bool Compare(char* string1, char* string2)
{
	// Compare if two strings have equal value.
	int i = 0;
	while (string1[i] != NULL_CHAR)
	{
		if (string1[i] != string2[i])
		{
			return false;
		}
		i++;
	}
	return string2[i] == NULL_CHAR ? true : false;
}

char* GetVendorAddress(char* full_address, char* partial_address, bool format)
{
	// Extract the first part of the MAC address (the vendor address).
	for (int j = 0; j < 8; j++)
	{
		// If the format is TRUE, it will change the format to match the look-up file.
		if (format)
		{
			char new_char = toupper(full_address[j]);
			partial_address[j] = new_char == ':' ? '-' : new_char;
		}
		// Otherwise, just copy the address as is.
		else
		{
			partial_address[j] = full_address[j];
		}
	}
	partial_address[8] = NULL_CHAR;
	return partial_address;
}

char* GetVendorName(char* full_address, int vendor_count, char vendor_addresses[V_COUNT][V_ADDRESS_LENGTH], char vendor_names[V_COUNT][V_NAME_LENGTH])
{
	// Find the matching vendor name for a given vendor address. If not found, will return UNKNOWN-VENDOR.
	char* name = UNKNOWN_VENDOR;
	char partial_address[9];
	GetVendorAddress(full_address, partial_address, true);

	for (int i = 0; i < vendor_count; i++)
	{
		if (Compare(partial_address, vendor_addresses[i]) == true)
			return vendor_names[i];
	}

	return name;
}

void Trim(char* target_string)
{
	// Terminate a string by replacing the newline char with the null char.
	int i = 0;
	while (target_string[i] != NULL_CHAR)
	{
		if (target_string[i] == '\n' || target_string[i] == '\r')
		{
			target_string[i] = NULL_CHAR;
			break;
		}
		i++;
	}
}

void CopyOutputToFile(char out_lines[OUTPUT_COUNT][OUTPUT_LENGTH], int line_index)
{
	// Write the results to a file, so that we can sort it using /usr/bin/sort.
	FILE* fp_out = fopen(TEMP_COPY_FILE_NAME, "w");
	if (fp_out != NULL)
	{
		for (int f = 0; f < line_index; f++)
			fputs(out_lines[f], fp_out);
		fclose(fp_out);
	}
}

void SortResults(int vendor_count)
{
	// Invoke /usr/bin/sort to sort the results.
	int pid = fork();
	int status = 0;

	if (pid == 0)
	{
		if (vendor_count == 0)
		{
			// No vendor names: Sort by package size in descending numerical order. 
			char* exe_argv[] = {"sort", TEMP_COPY_FILE_NAME, "-n", "-k2", "-r", NULL};
			execve("/usr/bin/sort", exe_argv, environ);
		}
		else
		{
			// With vendor name: Sort by package size first, then by vendor name on alphabetical order.
			char* exe_argv[] = {"sort", TEMP_COPY_FILE_NAME, "-k3,3nr", "-k2,2", NULL};
			execve("/usr/bin/sort", exe_argv, environ);
		}
	}

	if (pid != 0)
		wait(&status);
}

void DeleteTempFile()
{
	// Remove the temp. text file used for sorting.
	int pid = fork();
	int status = 0;

	if (pid == 0)
	{
		char* exe_argv[] = { "rm", TEMP_COPY_FILE_NAME, NULL };
		execve("/bin/rm", exe_argv, environ);
	}

	if (pid != 0)
		wait(&status);
}

int ExtractVendorInfo(char** argv, char vendor_addresses[V_COUNT][V_ADDRESS_LENGTH], char vendor_names[V_COUNT][V_NAME_LENGTH])
{
	// Process the Vendor Lookup information from the input text file.
	FILE* file = fopen(argv[3], "r");
	char line[OUTPUT_LENGTH];
	int i = 0;
	while (fgets(line, sizeof line, file) != NULL)
	{
		strcpy(vendor_addresses[i], strtok(line, TAB_TOKEN));
		strcpy(vendor_names[i], strtok(NULL, TAB_TOKEN));
		Trim(vendor_names[i]);
		Trim(vendor_addresses[i]);
		i++;
	}
	return i;
}

char* CopyStringToOutput(char* copy_str, char* output_str)
{
	// A helper function to append a string into the output.
	int j = 0;
	while (copy_str[j] != NULL_CHAR)
	{
		*output_str = copy_str[j];
		output_str++;
		j++;
	}
	return output_str;
}

char* AddTabToOutput(char* str)
{
	// Append a tab token to the output.
	*str = '\t';
	return ++str;
}

void ProcessSimplePackage(char* package_size, char* full_address, char** output_pointer)
{
	// Format a single line of a package, without vendor name.
	*output_pointer = CopyStringToOutput(full_address, *output_pointer);
	*output_pointer = AddTabToOutput(*output_pointer);
	*output_pointer = CopyStringToOutput(package_size, *output_pointer);
	**output_pointer = NULL_CHAR;
}

void ProcessNamedPackage(char* package_size, char* vendor_name, char partial_address[9], char** output_pointer)
{
	// Format a single line of a package, with vendor name.
	char* vendor_address = vendor_name == UNKNOWN_VENDOR ? UNKNOWN_ADDRESS : partial_address;
	*output_pointer = CopyStringToOutput(vendor_address, *output_pointer);
	*output_pointer = AddTabToOutput(*output_pointer);
	*output_pointer = CopyStringToOutput(vendor_name, *output_pointer);
	*output_pointer = AddTabToOutput(*output_pointer);
	*output_pointer = CopyStringToOutput(package_size, *output_pointer);
	**output_pointer = NULL_CHAR;
}

int ProcessInputFile(
	// Read all the lines from the input file. Format those lines to spec, and then output it into the target array.
	char* file_name,
	char device_type,
	char out_lines[OUTPUT_COUNT][OUTPUT_LENGTH],
	int vendor_count,
	char vendor_addresses[V_COUNT][V_ADDRESS_LENGTH],
	char vendor_names[V_COUNT][V_NAME_LENGTH])
{
	int line_index = 0;
	FILE* file = fopen(file_name, "r");
	char line[OUTPUT_LENGTH];
	while (fgets(line, sizeof line, file) != NULL)
	{
		char* array[4];
		Split(line, TAB_TOKEN, array);
		char* transmit_address = array[1];
		char* receive_address = array[2];
		char* package_size = array[3];

		char* full_address = device_type == TRANSMIT_DEVICE ? transmit_address : receive_address;
		char* vendor_name = GetVendorName(full_address, vendor_count, vendor_addresses, vendor_names);

		if (Compare(full_address, BROADCAST_ADDRESS) == true)
		{
			line_index++;
			continue;
		}

		char partial_address[9];
		GetVendorAddress(full_address, partial_address, false);
		char* output_pointer = out_lines[line_index];

		if (vendor_count == 0)
			ProcessSimplePackage(package_size, full_address, &output_pointer);
		else
			ProcessNamedPackage(package_size, vendor_name, partial_address, &output_pointer);

		line_index++;
	}

	return line_index;
}

int main(int argc, char* argv[])
{
	int exit_code = EXIT_SUCCESS;
	char vendor_addresses[V_COUNT][V_ADDRESS_LENGTH];
	char vendor_names[V_COUNT][V_NAME_LENGTH];
	char out_lines[OUTPUT_COUNT][OUTPUT_LENGTH];
	int vendor_count = 0;

	if (argc == 4)
	{
		vendor_count = ExtractVendorInfo(argv, vendor_addresses, vendor_names);
	}

	if (argc >= 3)
	{
		char* input_file = argv[2];
		char d_type = argv[1][0];

		int line_index = ProcessInputFile(input_file, d_type, out_lines, vendor_count, vendor_addresses, vendor_names);
		CopyOutputToFile(out_lines, line_index);
		SortResults(vendor_count);
		DeleteTempFile();
	}

	return exit_code;
}
