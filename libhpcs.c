#include "libhpcs.h"
#include "libhpcs_p.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct HPCS_MeasuredData* hpcs_alloc()
{
	return malloc(sizeof(struct HPCS_MeasuredData));
}

char* hpcs_error_to_string(const enum HPCS_RetCode err)
{
	char* msg;

	switch (err) {
	case HPCS_OK:
		msg = malloc(strlen(HPCS_OK_STR) + 1);
		strcpy(msg, HPCS_OK_STR);
		return msg;
	case HPCS_E_NULLPTR:
		msg = malloc(strlen(HPCS_E_NULLPTR_STR) + 1);
		strcpy(msg, HPCS_E_NULLPTR_STR);
		return msg;
	case HPCS_E_CANT_OPEN:
		msg = malloc(strlen(HPCS_E_CANT_OPEN_STR) + 1);
		strcpy(msg, HPCS_E_CANT_OPEN_STR);
		return msg;
	case HPCS_E_PARSE_ERROR:
		msg = malloc(strlen(HPCS_E_PARSE_ERROR_STR) + 1);
		strcpy(msg, HPCS_E_PARSE_ERROR_STR);
		return msg;
	case HPCS_E_UNKNOWN_TYPE:
		msg = malloc(strlen(HPCS_E_UNKNOWN_TYPE_STR) + 1);
		strcpy(msg, HPCS_E_UNKNOWN_TYPE_STR);
		return msg;
	default:
		msg = malloc(strlen(HPCS_E__UNKNOWN_EC_STR) + 1);
		strcpy(msg, HPCS_E__UNKNOWN_EC_STR);
		return msg;
	}
}

void hpcs_free(struct HPCS_MeasuredData* const mdata)
{
	if (mdata == NULL)
		return;
	free(mdata->file_description);
	free(mdata->sample_info);
	free(mdata->operator_name);
	free(mdata->method_name);
	free(mdata->cs_ver);
	free(mdata->cs_rev);
	free(mdata->y_units);
	free(mdata->data);
	free(mdata);
}

enum HPCS_RetCode hpcs_read_file(const char* filename, struct HPCS_MeasuredData* mdata)
{
	FILE* datafile;
	enum HPCS_ParseCode pret;
	enum HPCS_RetCode ret;

	if (mdata == NULL)
		return HPCS_E_NULLPTR;

	datafile = fopen(filename, "rb");
	if (datafile == NULL)
		return HPCS_E_CANT_OPEN;

	pret = read_string_at_offset(datafile, DATA_OFFSET_FILE_DESC, &mdata->file_description);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read file description");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_string_at_offset(datafile, DATA_OFFSET_SAMPLE_INFO, &mdata->sample_info);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read sample info");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_string_at_offset(datafile, DATA_OFFSET_OPERATOR_NAME, &mdata->operator_name);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read operator name");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_string_at_offset(datafile, DATA_OFFSET_METHOD_NAME, &mdata->method_name);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read method name");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_date(datafile, &mdata->date);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read date of measurement");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_string_at_offset(datafile, DATA_OFFSET_CS_VER, &mdata->cs_ver);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read ChemStation software version");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_string_at_offset(datafile, DATA_OFFSET_Y_UNITS, &mdata->y_units);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read values of Y axis");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_string_at_offset(datafile, DATA_OFFSET_CS_REV, &mdata->cs_rev);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read ChemStation software revision");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_sampling_rate(datafile, &mdata->sampling_rate);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read sampling rate of the file");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = autodetect_file_type(datafile, &mdata->file_type);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot determine the type of file");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}

	if (mdata->file_type == HPCS_TYPE_CE_DAD) {
		pret = read_dad_wavelength(datafile, WAVELENGTH_MEASURED, &mdata->dad_wavelength_msr);
		if (pret != PARSE_OK && pret != PARSE_W_NO_DATA) {
			PR_DEBUG("Cannot read measured wavelength");
			ret = HPCS_E_PARSE_ERROR;
			goto out;
		}
		pret = read_dad_wavelength(datafile, WAVELENGTH_REFERENCE, &mdata->dad_wavelength_ref);
		if (pret != PARSE_OK && pret != PARSE_W_NO_DATA) {
			PR_DEBUG("Cannot read reference wavelength");
			ret = HPCS_E_PARSE_ERROR;
			goto out;
		}
	}

	switch (mdata->file_type) {
	case HPCS_TYPE_CE_CCD:
		pret = read_floating_signal(datafile, &mdata->data, &mdata->data_count, CE_CCD_STEP, mdata->sampling_rate);
		break;
	case HPCS_TYPE_CE_CURRENT:
		pret = read_fixed_signal(datafile, &mdata->data, &mdata->data_count, CE_CURRENT_STEP, mdata->sampling_rate);
		break;
	case HPCS_TYPE_CE_DAD:
		pret = read_fixed_signal(datafile, &mdata->data, &mdata->data_count, CE_DAD_STEP, mdata->sampling_rate);
		break;
	case HPCS_TYPE_CE_POWER:
	case HPCS_TYPE_CE_VOLTAGE:
		pret = read_fixed_signal(datafile, &mdata->data, &mdata->data_count, CE_PWR_VOLT_STEP, mdata->sampling_rate);
		break;
	case HPCS_TYPE_UNKNOWN:
		ret = HPCS_E_UNKNOWN_TYPE;
		goto out;
	}

	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot parse data in the file");
		ret = HPCS_E_PARSE_ERROR;
 	}
	else
		ret = HPCS_OK;
out:
	fclose(datafile);
	return ret;
}

static enum HPCS_ParseCode autodetect_file_type(FILE* datafile, enum HPCS_File_Type* file_type)
{
	char* type_str;
	char* type_id;
	char* delim;
	size_t len;
	enum HPCS_ParseCode pret;

	pret = read_string_at_offset(datafile, DATA_OFFSET_DEVSIG_INFO, &type_str);
	if (pret != PARSE_OK)
		return pret;

	delim = strchr(type_str, DATA_FILE_COMMA);
	if (delim == NULL) {
		free(type_str);
		return PARSE_E_NOT_FOUND;
	}

	len = delim - type_str;
	type_id = malloc(len + 1);
	memcpy(type_id, type_str, len);
	type_id[len] = 0;

	if (strcmp(FILE_TYPE_CE_CCD, type_id) == 0)
		*file_type = HPCS_TYPE_CE_CCD;
	else if (strcmp(FILE_TYPE_CE_CURRENT, type_id) == 0)
		*file_type = HPCS_TYPE_CE_CURRENT;
	else if (strstr(type_id, FILE_TYPE_CE_DAD) != NULL)
		*file_type = HPCS_TYPE_CE_DAD;
	else if (strcmp(FILE_TYPE_CE_POWER, type_id) == 0)
		*file_type = HPCS_TYPE_CE_POWER;
	else if (strcmp(FILE_TYPE_CE_VOLTAGE, type_id) == 0)
		*file_type = HPCS_TYPE_CE_VOLTAGE;
	else
		*file_type = HPCS_TYPE_UNKNOWN;

	free(type_str);
	free(type_id);
	return PARSE_OK;
}

static enum HPCS_DataCheckCode check_for_marker(const char* const segment, size_t* const next_marker_idx)
{
	if (segment[0] == BIN_MARKER_END && segment[1] == BIN_MARKER_END)
		return DCHECK_EOF;
	else if (segment[0] == BIN_MARKER_A && segment[1] != BIN_MARKER_END) {
		*next_marker_idx += (uint8_t)segment[1] + 1;
		return DCHECK_GOT_MARKER;
	} else
		return DCHECK_E_NO_MARKER;
}

static enum HPCS_ParseCode read_dad_wavelength(FILE* datafile, const enum HPCS_Wavelength_Type wl_type, uint16_t* const wavelength)
{
	char* start_idx, *end_idx, *temp, *type_str;
	size_t len;
	enum HPCS_ParseCode pret;

	*wavelength = 0;
	pret = read_string_at_offset(datafile, DATA_OFFSET_DEVSIG_INFO, &type_str);
	if (pret != PARSE_OK)
		return pret;

	switch (wl_type) {
	case WAVELENGTH_MEASURED:
		start_idx = strstr(type_str, WAVELENGTH_MEASURED_TEXT);
		break;
	case WAVELENGTH_REFERENCE:
		start_idx = strstr(type_str, WAVELENGTH_REFERENCE_TEXT);
		break;
	default:
		return PARSE_E_INV_PARAM;
	}

	if (start_idx == NULL)
		return PARSE_W_NO_DATA;

	end_idx = strchr(start_idx, WAVELENGTH_DELIMITER_TEXT);
	if (end_idx == NULL)
		return PARSE_E_NOT_FOUND;

	len = end_idx - (start_idx + 4);
	temp = malloc(len + 1);
	if (temp == NULL)
		return PARSE_E_NO_MEM;
	memcpy(temp, start_idx + 4, len);
	temp[len] = 0;

	*wavelength = strtoul(temp, NULL, 10);
	free(temp);
	return PARSE_OK;
}

static enum HPCS_ParseCode read_date(FILE* datafile, struct HPCS_Date* date)
{
	char* date_str;
	char* date_time_delim;
	char* dm_delim, *my_delim;
	char* hm_delim, *ms_delim;
	char temp[32];
	size_t len;
	enum HPCS_ParseCode pret;

	pret = read_string_at_offset(datafile, DATA_OFFSET_DATE, &date_str);
	if (pret != PARSE_OK)
		return pret;

	/* Find date / time delimiter */
	date_time_delim = strchr(date_str, DATA_FILE_COMMA);
	if (date_time_delim == NULL) {
		free(date_str);
		return PARSE_E_NOT_FOUND;
	}

	/* Get day */
	dm_delim = strchr(date_str, DATA_FILE_DASH);
	if (dm_delim == NULL) {
		free(date_str);
		return PARSE_E_NOT_FOUND;
	}
	len = dm_delim - date_str;
	memcpy(temp, date_str, len);
	temp[len] = 0;
	date->day = strtoul(temp, NULL, 10);

	/* Get month */
	my_delim = strchr(dm_delim + 1, DATA_FILE_DASH);
	if (dm_delim == NULL) {
		free(date_str);
		return PARSE_E_NOT_FOUND;
	}
	len = my_delim - (dm_delim + 1);
	memcpy(temp, dm_delim + 1, len);
	temp[len] = 0;
	date->month = month_to_number(temp);

	/* Get year */
	len = date_time_delim - (my_delim + 1);
	memcpy(temp, my_delim + 1, len);
	temp[len] = 0;
	date->year = strtoul(temp, NULL, 10);
	if (date->year < 90) /* Y2K workaround */
		date->year += 2000;

	/* Get hour */
	hm_delim = strchr(date_time_delim + 1, DATA_FILE_COLON);
	if (hm_delim == NULL) {
		free(date_str);
		return PARSE_E_NOT_FOUND;
	}
	len = hm_delim - (date_time_delim + 1);
	memcpy(temp, date_time_delim + 1, len);
	temp[len] = 0;
	date->hour = strtoul(temp, NULL, 10);

	/* Get minute */
	ms_delim = strchr(hm_delim + 1, DATA_FILE_COLON);
	if (ms_delim == NULL) {
		free(date_str);
		return PARSE_E_NOT_FOUND;
	}
	len = ms_delim - (hm_delim + 1);
	memcpy(temp, hm_delim + 1, len);
	temp[len] = 0;
	date->minute = strtoul(temp, NULL, 10);

	/* Get second */
	date->second = strtoul(ms_delim + 1, NULL, 10);

	free(date_str);
	return PARSE_OK;
}

static uint8_t month_to_number(const char* const month)
{
	if (strcmp(MON_JAN_STR, month) == 0)
		return 1;
	else if (strcmp(MON_FEB_STR, month) == 0)
		return 2;
	else if (strcmp(MON_MAR_STR, month) == 0)
		return 3;
	else if (strcmp(MON_APR_STR, month) == 0)
		return 4;
	else if (strcmp(MON_MAY_STR, month) == 0)
		return 5;
	else if (strcmp(MON_JUN_STR, month) == 0)
		return 6;
	else if (strcmp(MON_JUL_STR, month) == 0)
		return 7;
	else if (strcmp(MON_AUG_STR, month) == 0)
		return 8;
	else if (strcmp(MON_SEP_STR, month) == 0)
		return 9;
	else if (strcmp(MON_OCT_STR, month) == 0)
		return 10;
	else if (strcmp(MON_NOV_STR, month) == 0)
		return 11;
	else if (strcmp(MON_DEC_STR, month) == 0)
		return 12;
	else
		return 0;
}

static enum HPCS_ParseCode read_fixed_signal(FILE* datafile, struct HPCS_TVPair** pairs, size_t* pairs_count,
					   const HPCS_step step, const double sampling_rate)
{
	const double time_step = 1 / (60 * sampling_rate);
	size_t alloc_size = 60 * sampling_rate;
	bool read_file = true;
	double value = 0;
	double time = 0;
	size_t segments_read = 0;
	size_t data_segments_read = 0;
	size_t next_marker_idx = 0;
	char raw[2];
	size_t r;
	enum HPCS_DataCheckCode dret;

	fseek(datafile, DATA_OFFSET_DATA_START, SEEK_SET);
	if (feof(datafile))
		return PARSE_E_OUT_OF_RANGE;

	/* Data block must begin with a marker, read it */
	r = fread(raw, SEGMENT_SIZE, 1, datafile);
	if (r != 1)
		return PARSE_E_CANT_READ;
	segments_read++;

	dret = check_for_marker(raw, &next_marker_idx);
	switch (dret) {
	case DCHECK_EOF:
	case DCHECK_E_NO_MARKER:
		return PARSE_E_NOT_FOUND; /* First segment is not a marker */
	default:
		break;
	}

	*pairs = malloc(sizeof(struct HPCS_TVPair) * alloc_size);
	if (pairs == NULL)
		return PARSE_E_NO_MEM;

	while (read_file) {
		if (ferror(datafile)) {
			free(*pairs);
			*pairs = NULL;
			return PARSE_E_CANT_READ;
		}
		if (feof(datafile))
			break;

		r = fread(raw, SEGMENT_SIZE, 1, datafile);
		if (r != 1) {
			free(*pairs);
			*pairs = NULL;
			return PARSE_E_CANT_READ;
		}
		segments_read++;

		if (alloc_size == data_segments_read) {
			struct HPCS_TVPair* nptr;
			alloc_size += 60 * sampling_rate;
			nptr = realloc(*pairs, sizeof(struct HPCS_TVPair) * alloc_size);

			if (nptr == NULL) {
				free(*pairs);
				*pairs = NULL;
				return PARSE_E_NO_MEM;
			}

			*pairs = nptr;
		}

		if (segments_read - 1 == next_marker_idx) {
			dret = check_for_marker(raw, &next_marker_idx);
			switch (dret) {
			case DCHECK_GOT_MARKER:
				break;
			case DCHECK_EOF:
				read_file = false;
				break;
			default:
				free(*pairs);
				*pairs = NULL;
				return PARSE_E_NOT_FOUND;
			}
		} else {
			be_to_cpu(raw);
			value += (*(int16_t*)(raw)) * step;

			(*pairs)[data_segments_read].time = time;
			(*pairs)[data_segments_read].value = value;
			data_segments_read++;
			time += time_step;
		}
	}

	*pairs_count = data_segments_read;
	return PARSE_OK;
}

static enum HPCS_ParseCode read_floating_signal(FILE* datafile, struct HPCS_TVPair** pairs, size_t* pairs_count,
					      const HPCS_step step, const double sampling_rate)
{
	const double time_step = 1 / (60 * sampling_rate);
	size_t alloc_size = 60 * sampling_rate;
	bool read_file = true;
	double value = 0;
	double time = 0;
	size_t segments_read = 0;
	size_t data_segments_read = 0;
	size_t next_marker_idx = 0;
	char raw[2];
	size_t r;
	enum HPCS_DataCheckCode dret;

	fseek(datafile, DATA_OFFSET_DATA_START, SEEK_SET);
	if (feof(datafile))
		return PARSE_E_OUT_OF_RANGE;

	r = fread(raw, SEGMENT_SIZE, 1, datafile);
	if (r != 1)
		return PARSE_E_CANT_READ;
	segments_read++;

	dret = check_for_marker(raw, &next_marker_idx);
	switch (dret) {
	case DCHECK_EOF:
	case DCHECK_E_NO_MARKER:
		return PARSE_E_NOT_FOUND;
	default:
		break;
	}

	*pairs = malloc(sizeof(struct HPCS_TVPair) * alloc_size);
	if (*pairs == NULL)
		return PARSE_E_NO_MEM;

	while (read_file) {
		if (ferror(datafile)) {
			free(*pairs);
			*pairs = NULL;
			return PARSE_E_CANT_READ;
		}
		if (feof(datafile))
			break;

		r = fread(raw, SEGMENT_SIZE, 1, datafile);
		if (r != 1) {
			free(*pairs);
			*pairs = NULL;
			return PARSE_E_CANT_READ;
		}
		segments_read++;

		/* Expand storage if there is more data than we can store */
		if (alloc_size == data_segments_read) {
			struct HPCS_TVPair* nptr;
			alloc_size += 60 * sampling_rate;
			nptr = realloc(*pairs, sizeof(struct HPCS_TVPair) * alloc_size);

			if (nptr == NULL) {
				free(*pairs);
				*pairs = NULL;
				return PARSE_E_NO_MEM;
			}

			*pairs = nptr;
		}

		/* Check for markers */
		if (segments_read - 1 == next_marker_idx) {
			dret = check_for_marker(raw, &next_marker_idx);
			switch (dret) {
			case DCHECK_GOT_MARKER:
				break;
			case DCHECK_EOF:
				read_file = false;
				break;
			default:
				free(*pairs);
				*pairs = NULL;
				return PARSE_E_NOT_FOUND;
			}
		} else {
			/* Check for a sudden jump of value */
			if (raw[0] == BIN_MARKER_JUMP && raw[1] == BIN_MARKER_END) {
				char lraw[4];
				int32_t _v;

				fread(lraw, LARGE_SEGMENT_SIZE, 1, datafile);
				if (feof(datafile) || ferror(datafile)) {
					free(*pairs);
					*pairs = NULL;
					return PARSE_E_CANT_READ;
				}

				be_to_cpu(lraw);
				_v = *(int32_t*)lraw;
				value = _v * step;
			} else {
				int16_t _v;

				be_to_cpu(raw);
				_v = *(int16_t*)raw;
				value += _v * step;
			}

			(*pairs)[data_segments_read].time = time;
			(*pairs)[data_segments_read].value = value;
			data_segments_read++;
			time += time_step;
		}
	}

	*pairs_count = data_segments_read;
	return PARSE_OK;
}

static enum HPCS_ParseCode read_sampling_rate(FILE* datafile, double* sampling_rate)
{
	char raw[2];
	uint16_t number;
	size_t r;

	fseek(datafile, DATA_OFFSET_SAMPLING_RATE, SEEK_SET);
	if (feof(datafile))
		return PARSE_E_OUT_OF_RANGE;

	r = fread(raw, SEGMENT_SIZE, 1, datafile);
	if (r != 1)
		return PARSE_E_CANT_READ;

	be_to_cpu(raw);
	number = *(uint16_t*)(raw);
	*sampling_rate = number / 10.0;

	return PARSE_OK;
}

static enum HPCS_ParseCode read_string_at_offset(FILE* datafile, const HPCS_offset offset, char** const result)
{
	char* string;
	uint8_t str_length, idx;
	size_t r;

	fseek(datafile, offset, SEEK_SET);
	if (feof(datafile))
		return PARSE_E_OUT_OF_RANGE;

	r = fread(&str_length, SMALL_SEGMENT_SIZE, 1, datafile);
	if (r != 1)
		return PARSE_E_CANT_READ;

	string = malloc(str_length + 1);
	if (string == NULL)
		return PARSE_E_NO_MEM;

	for (idx = 0; idx < str_length; idx++) {
		fread(string+idx, SMALL_SEGMENT_SIZE, 1, datafile);
		fseek(datafile, SMALL_SEGMENT_SIZE, SEEK_CUR);
	}
	string[str_length] = 0;

	*result = string;
	return PARSE_OK;
}
