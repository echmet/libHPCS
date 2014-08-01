#include "libhpcs.h"
#include "libhpcs_p.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>

struct HPCS_MeasuredData* hpcs_alloc()
{
	struct HPCS_MeasuredData* mdata = malloc(sizeof(struct HPCS_MeasuredData));
	if (mdata == NULL)
		return NULL;

	mdata->file_description = NULL;
	mdata->sample_info = NULL;
	mdata->operator_name = NULL;
	mdata->method_name = NULL;
	mdata->cs_ver = NULL;
	mdata->cs_rev = NULL;
	mdata->y_units = NULL;
	mdata->data = NULL;

	return mdata;
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
		PR_DEBUG("Cannot read file description\n");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_string_at_offset(datafile, DATA_OFFSET_SAMPLE_INFO, &mdata->sample_info);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read sample info\n");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_string_at_offset(datafile, DATA_OFFSET_OPERATOR_NAME, &mdata->operator_name);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read operator name\n");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_string_at_offset(datafile, DATA_OFFSET_METHOD_NAME, &mdata->method_name);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read method name\n");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_date(datafile, &mdata->date);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read date of measurement\n");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_string_at_offset(datafile, DATA_OFFSET_CS_VER, &mdata->cs_ver);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read ChemStation software version\n");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_string_at_offset(datafile, DATA_OFFSET_Y_UNITS, &mdata->y_units);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read values of Y axis\n");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_string_at_offset(datafile, DATA_OFFSET_CS_REV, &mdata->cs_rev);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read ChemStation software revision\n");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = read_sampling_rate(datafile, &mdata->sampling_rate);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read sampling rate of the file\n");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	pret = autodetect_file_type(datafile, &mdata->file_type, guess_p_meaning(mdata));
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot determine the type of file\n");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}

	if (mdata->file_type == HPCS_TYPE_CE_DAD) {
		pret = read_dad_wavelength(datafile, &mdata->dad_wavelength_msr, &mdata->dad_wavelength_ref);
		if (pret != PARSE_OK && pret != PARSE_W_NO_DATA) {
			PR_DEBUG("Cannot read wavelength\n");
			ret = HPCS_E_PARSE_ERROR;
			goto out;
		}
	}

	guess_sampling_rate(mdata);

	switch (mdata->file_type) {
	case HPCS_TYPE_CE_CCD:
		pret = read_signal(datafile, &mdata->data, &mdata->data_count, CE_CCD_STEP, mdata->sampling_rate);
		break;
	case HPCS_TYPE_CE_CURRENT:
		pret = read_signal(datafile, &mdata->data, &mdata->data_count, guess_current_step(mdata), mdata->sampling_rate);
		break;
	case HPCS_TYPE_CE_DAD:
		pret = read_signal(datafile, &mdata->data, &mdata->data_count, CE_DAD_STEP, mdata->sampling_rate);
		break;
	case HPCS_TYPE_CE_POWER:
	case HPCS_TYPE_CE_VOLTAGE:
		pret = read_signal(datafile, &mdata->data, &mdata->data_count, guess_elec_sigstep(mdata), mdata->sampling_rate);
		break;
	case HPCS_TYPE_CE_PRESSURE:
	case HPCS_TYPE_CE_TEMPERATURE:
		pret = read_signal(datafile, &mdata->data, &mdata->data_count, CE_WORK_PARAM_STEP, mdata->sampling_rate);
		break;
	case HPCS_TYPE_UNKNOWN:
		ret = HPCS_E_UNKNOWN_TYPE;
		goto out;
	}

	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot parse data in the file\n");
		ret = HPCS_E_PARSE_ERROR;
 	}
	else
		ret = HPCS_OK;
out:
	fclose(datafile);
	return ret;
}

static enum HPCS_ParseCode autodetect_file_type(FILE* datafile, enum HPCS_FileType* file_type, const bool p_means_pressure)
{
	char* type_id;
	enum HPCS_ParseCode pret;

	pret = read_string_at_offset(datafile, DATA_OFFSET_DEVSIG_INFO, &type_id);
	if (pret != PARSE_OK)
		return pret;

	if (strstr(type_id, FILE_TYPE_ID_DAD) == type_id)
		*file_type = HPCS_TYPE_CE_DAD;
	else if (strstr(type_id, FILE_TYPE_ID_HPCE) == type_id) {
		const char hpce_id = type_id[strlen(FILE_TYPE_ID_HPCE) + 1];

		if (hpce_id == FILE_TYPE_HPCE_CCD)
			*file_type = HPCS_TYPE_CE_CCD;
		else if (hpce_id == FILE_TYPE_HPCE_CURRENT)
			*file_type = HPCS_TYPE_CE_CURRENT;
		else if (hpce_id == FILE_TYPE_HPCE_POWER)
			*file_type = HPCS_TYPE_CE_POWER;
		else if (hpce_id == FILE_TYPE_HPCE_POWER_PRESSURE)
			*file_type = p_means_pressure ? HPCS_TYPE_CE_PRESSURE : HPCS_TYPE_CE_POWER;
		else if (hpce_id == FILE_TYPE_HPCE_TEMPERATURE)
			*file_type = HPCS_TYPE_CE_TEMPERATURE;
		else if (hpce_id == FILE_TYPE_HPCE_VOLTAGE)
			*file_type = HPCS_TYPE_CE_VOLTAGE;
		else
			*file_type = HPCS_TYPE_UNKNOWN;
	} else
		*file_type = HPCS_TYPE_UNKNOWN;	

	return PARSE_OK;
}

static enum HPCS_DataCheckCode check_for_marker(const char* const segment, size_t* const next_marker_idx)
{
	if (segment[0] == BIN_MARKER_A && segment[1] != BIN_MARKER_END) {
		*next_marker_idx += (uint8_t)segment[1] + 1;
		return DCHECK_GOT_MARKER;
	} else
		return DCHECK_NO_MARKER;
}

static HPCS_step guess_current_step(const struct HPCS_MeasuredData* mdata)
{
	if (strcmp(mdata->cs_ver, CHEMSTAT_VER_B0625) == 0)
		return CE_CURRENT_STEP;

	return CE_WORK_PARAM_STEP;
}

static HPCS_step guess_elec_sigstep(const struct HPCS_MeasuredData* mdata)
{
	if (strcmp(mdata->cs_ver, CHEMSTAT_VER_B0625)) {
		switch (mdata->file_type) {
		case HPCS_TYPE_CE_POWER:
			return CE_ENERGY_STEP;
		default:
			return CE_WORK_PARAM_OLD_STEP;
		}
	}



	return CE_WORK_PARAM_STEP;
}

static bool guess_p_meaning(const struct HPCS_MeasuredData* mdata)
{
	if (strcmp(mdata->cs_ver, CHEMSTAT_VER_B0625) == 0)
		return false;

	return true;
}

static void guess_sampling_rate(struct HPCS_MeasuredData* mdata)
{
	if (strcmp(mdata->cs_ver, CHEMSTAT_VER_B0625)) {
		switch (mdata->file_type) {
		case HPCS_TYPE_CE_DAD:
			mdata->sampling_rate *= 10;
			break;
		default:
			mdata->sampling_rate = CE_WORK_PARAM_SAMPRATE;
		}
	}
}

static enum HPCS_ParseCode read_dad_wavelength(FILE* datafile, struct HPCS_Wavelength* const measured, struct HPCS_Wavelength* const reference)
{
	char* start_idx, *interv_idx, *end_idx, *temp, *str;
	size_t len;
	enum HPCS_ParseCode pret, ret;

	measured->wavelength = 0;
	measured->interval = 0;
	reference->wavelength = 0;
	reference->interval = 0;
	pret = read_string_at_offset(datafile, DATA_OFFSET_DEVSIG_INFO, &str);
	if (pret != PARSE_OK)
		return pret;

	/* Read MEASURED wavelength */
	start_idx = strstr(str, WAVELENGTH_MEASURED_TEXT);
	if (start_idx == NULL) {
		ret = PARSE_W_NO_DATA;
		goto out;
	}
	start_idx += strlen(WAVELENGTH_MEASURED_TEXT);
	interv_idx = strchr(start_idx, WAVELENGTH_DELIMITER_TEXT);
	if (interv_idx == NULL) {
		PR_DEBUG("No spectral interval value\n");
		ret = PARSE_E_NOT_FOUND;
		goto out;
	}
	end_idx = strchr(interv_idx, WAVELENGTH_END_TEXT);
	if (end_idx == NULL) {
		PR_DEBUG("No measured/reference wavelength delimiter found\n");
		ret = PARSE_E_NOT_FOUND;
		goto out;
	}
	len = interv_idx - start_idx;
	temp = malloc(len + 1);
	if (temp == NULL) {
		PR_DEBUG("No memory for temporary string\n");
		ret = PARSE_E_NO_MEM;
		goto out;
	}
	memcpy(temp, start_idx, len);
	temp[len] = 0;
	measured->wavelength = strtoul(temp, NULL, 10);

	/* Read MEASURED spectral interval */
	if (end_idx - interv_idx - 1 > len) {
		free(temp);
		temp = malloc(end_idx - interv_idx - 1);
		if (temp == NULL) {
			PR_DEBUG("No memory for temporary string\n");
			ret = PARSE_E_NO_MEM;
			goto out;
		}
	}
	len = end_idx - interv_idx - 2;
	memcpy(temp, interv_idx + 1, len);
	temp[len] = 0;
	measured->interval = strtoul(temp, NULL, 10);


	/* Read REFERENCE wavelength */
	start_idx = strstr(end_idx, WAVELENGTH_REFERENCE_TEXT);
	if (start_idx == NULL) {
		PR_DEBUG("No reference wavelength data\n");
		ret = PARSE_W_NO_DATA;
		goto out2;
	}
	start_idx += strlen(WAVELENGTH_REFERENCE_TEXT);
	interv_idx = strchr(start_idx, WAVELENGTH_DELIMITER_TEXT);
	if (interv_idx == NULL) {
		/* Is the reference wavelength disabled? */
		if (strcmp(start_idx, WAVELENGTH_REFERENCE_OFF_TEXT) == 0) {
			ret = PARSE_OK;
			goto out2;
		}
		PR_DEBUG("No reference spectral interval but reference wavelength is not 'off'\n");
		ret = PARSE_E_NOT_FOUND;
		goto out2;
	}
	if (interv_idx - start_idx > len + 1) {
		free(temp);
		temp = malloc(interv_idx - start_idx + 1);
		if (temp == NULL) {
			PR_DEBUG("No memory for temporary string\n");
			ret = PARSE_E_NO_MEM;
			goto out;
		}
	}
	len = interv_idx - start_idx;
	memcpy(temp, start_idx, len);
	temp[len] = 0;
	reference->wavelength = strtoul(temp, NULL, 10);
	reference->interval = strtoul(interv_idx + 1, NULL, 10);
	ret = PARSE_OK;

out2:
	printf("%u %u %u %u\n", measured->wavelength, measured->interval, reference->wavelength, reference->interval);
	free(temp);
out:
	free(str);
	return ret;
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

static uint8_t month_to_number(const char* month)
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

static enum HPCS_ParseCode read_signal(FILE* datafile, struct HPCS_TVPair** pairs, size_t* pairs_count,
				       const HPCS_step step, const double sampling_rate)
{
	const double time_step = 1 / (60 * sampling_rate);
        size_t alloc_size = (size_t)((60 * sampling_rate) + 0.5);
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
	case DCHECK_NO_MARKER:
		return PARSE_E_NOT_FOUND;
	default:
		break;
	}

	*pairs = malloc(sizeof(struct HPCS_TVPair) * alloc_size);
	if (*pairs == NULL)
		return PARSE_E_NO_MEM;

	while (read_file) {
		r = fread(raw, SEGMENT_SIZE, 1, datafile);

		if (ferror(datafile)) {
			free(*pairs);
			*pairs = NULL;
			PR_DEBUG("Error reading stream - ferror\n");
			return PARSE_E_CANT_READ;
		}
		if (feof(datafile))
			break;

		if (r != 1) {
			free(*pairs);
			*pairs = NULL;
			PR_DEBUGF("Error reading stream, r=%lu\n", r);
			return PARSE_E_CANT_READ;
		}
		segments_read++;

		/* Expand storage if there is more data than we can store */
		if (alloc_size == data_segments_read) {
			struct HPCS_TVPair* nptr;
                        alloc_size += (size_t)((60 * sampling_rate) + 0.5);
			nptr = realloc(*pairs, sizeof(struct HPCS_TVPair) * alloc_size);

			if (nptr == NULL) {
				free(*pairs);
				*pairs = NULL;
				return PARSE_E_NO_MEM;
			}

			*pairs = nptr;
		}

		/* Check for markers */
		dret = check_for_marker(raw, &next_marker_idx);
		switch (dret) {
		case DCHECK_GOT_MARKER:
			PR_DEBUGF("Got marker at 0x%lx\n", segments_read - 1);
			continue;
		case DCHECK_NO_MARKER:
			/* Check for a sudden jump of value */
			if (raw[0] == BIN_MARKER_JUMP && raw[1] == BIN_MARKER_END) {
				char lraw[4];
				int32_t _v;

				PR_DEBUGF("Value has jumped at 0x%lx\n", segments_read);
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
			break;
		default:
			PR_DEBUG("Invalid value from check_for_marker()\n");
			free(*pairs);
			*pairs = NULL;
			return PARSE_E_CANT_READ;
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

#ifdef __cplusplus
}
#endif
