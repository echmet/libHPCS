#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/libhpcs.h"

int read_data(const char* path)
{
	struct HPCS_MeasuredData* mdata;
	enum HPCS_RetCode hret;
	size_t di;

	mdata = hpcs_alloc_mdata();
	if (mdata == NULL) {
		printf("Out of memory\n");
		return EXIT_FAILURE;
	}

	hret = hpcs_read_mdata(path, mdata);
	if (hret != HPCS_OK) {
		printf("Cannot parse file: %s\n", hpcs_error_to_string(hret));
		return EXIT_FAILURE;
	}

	printf("Sample info: %s\n"
		  "Operator name: %s\n"
		  "Method name: %s\n"
		  "Y units: %s\n",
		  mdata->sample_info,
		  mdata->operator_name,
		  mdata->method_name,
		  mdata->y_units);

	for (di = 0; di < mdata->data_count; di++)
		printf("Time: %.17lg, Value: %.17lg\n", mdata->data[di].time, mdata->data[di].value);

	hpcs_free_mdata(mdata);

	return EXIT_SUCCESS;
}

int read_info(const char* path)
{
	struct HPCS_MethodInfo* minfo;
	enum HPCS_RetCode hret;
	size_t di;

	minfo = hpcs_alloc_minfo();
	if (minfo == NULL) {
		printf("Out of memory\n");
		return EXIT_FAILURE;
	}

	hret = hpcs_read_minfo(path, minfo);
	if (hret != HPCS_OK) {
		printf("Cannot parse file: %s\n", hpcs_error_to_string(hret));
		return EXIT_FAILURE;
	}

	for (di = 0; di < minfo->count; di++) {
		struct HPCS_MethodInfoBlock* b = &minfo->blocks[di];
		printf("Name: %s = Value: %s\n", b->name, b->value);
	}

	hpcs_free_minfo(minfo);

	return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
	if (argc < 3) {
		printf("Not enough arguments\n");
		printf("Usage: test_tool MODE FILE\n");
		printf("MODE: d - read data file\n"
		       "      i - method info\n"
		       "FILE: path\n");
		return EXIT_FAILURE;
	}

	if (strcmp(argv[1], "d") == 0)
		return read_data(argv[2]);
	else if (strcmp(argv[1], "i") == 0)
		return read_info(argv[2]);
	else {
		printf("Invalid mode argument\n");
		return EXIT_FAILURE;
	}
}
