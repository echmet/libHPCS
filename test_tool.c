#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include "libhpcs.h"

int main(int argc, char** argv)
{
	struct HPCS_MeasuredData* mdata;
	enum HPCS_RetCode hret;
	size_t di;
	
	if (argc < 2) {
		printf("No filename specified\n");
		return EXIT_FAILURE;
	}

	mdata = hpcs_alloc();
	if (mdata == NULL) {
		printf("Out of memory\n");
		return EXIT_FAILURE;
	}

	hret = hpcs_read_file(argv[1], mdata);
	if (hret != HPCS_OK) {
		printf("Cannot parse file: %s\n", hpcs_error_to_string(hret));
		return EXIT_FAILURE;
	}

	for (di = 0; di < mdata->data_count; di++)
		printf("Time: %.17lg, Value: %.17lg\n", mdata->data[di].time, mdata->data[di].value);

	hpcs_free(mdata);

	return EXIT_SUCCESS;
}
