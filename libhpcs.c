#ifdef __cplusplus
extern "C" {
#endif

#include "include/libhpcs.h"
#include "libhpcs_p.h"

#ifdef _WIN32
#include <sdkddkver.h>
#ifndef _WIN32_WINNT_WIN8
#define _WIN32_WINNT_WIN8 0x0602
#endif
#if _WIN32_WINNT > _WIN32_WINNT_WIN8
#include <Stringapiset.h>
#else
#include <WinNls.h>
#endif
#include <Shlwapi.h>
#else
#include <unicode/ustdio.h>
#endif

#include <stdlib.h>
#include <string.h>

struct HPCS_MeasuredData* hpcs_alloc_mdata()
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

	mdata->data_count = 0;

	return mdata;
}

struct HPCS_MethodInfo* hpcs_alloc_minfo()
{
	struct HPCS_MethodInfo* minfo = malloc(sizeof(struct HPCS_MeasuredData));
	if (minfo == NULL)
		return NULL;

	minfo->blocks = NULL;
	minfo->count = 0;

	return minfo;
}

const char* hpcs_error_to_string(const enum HPCS_RetCode err)
{
	switch (err) {
	case HPCS_OK:
		return HPCS_OK_STR;
	case HPCS_E_NULLPTR:
		return HPCS_E_NULLPTR_STR;
	case HPCS_E_CANT_OPEN:
		return HPCS_E_CANT_OPEN_STR;
	case HPCS_E_PARSE_ERROR:
		return HPCS_E_PARSE_ERROR_STR;
	case HPCS_E_UNKNOWN_TYPE:
		return HPCS_E_UNKNOWN_TYPE_STR;
	case HPCS_E_INCOMPATIBLE_FILE:
		return HPCS_E_INCOMPATIBLE_FILE_STR;
	default:
		return HPCS_E__UNKNOWN_EC_STR;
	}
}

void hpcs_free_mdata(struct HPCS_MeasuredData* const mdata)
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

void hpcs_free_minfo(struct HPCS_MethodInfo* const minfo)
{
	size_t idx;

	if (minfo == NULL)
		return;

	for (idx = 0; idx < minfo->count; idx++) {
		free(minfo->blocks[idx].name);
		free(minfo->blocks[idx].value);
	}

	free(minfo->blocks);
	free(minfo);
}

enum HPCS_RetCode hpcs_read_mdata(const char* filename, struct HPCS_MeasuredData* mdata)
{
	FILE* datafile;
	enum HPCS_ParseCode pret;
	enum HPCS_RetCode ret;
	enum HPCS_GenType gentype;
	enum HPCS_ChemStationVer cs_ver;

	if (mdata == NULL)
		return HPCS_E_NULLPTR;

	datafile = open_measurement_file(filename);
	if (datafile == NULL)
		return HPCS_E_CANT_OPEN;

	pret = read_generic_type(datafile, &gentype);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read generic file type\n");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}

	if (!gentype_is_readable(gentype)) {
		PR_DEBUGF("%s: %d\n", "Incompatible file type", gentype);
		ret = HPCS_E_INCOMPATIBLE_FILE;
		goto out;
	}

	pret = read_file_type_description(datafile, &mdata->file_description, gentype);
	if (pret != PARSE_OK) {
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}

	if (!file_type_description_is_readable(mdata->file_description)) {
		PR_DEBUGF("Incompatible file description: %s\n", mdata->file_description);
		ret = HPCS_E_INCOMPATIBLE_FILE;
		goto out;
	}

	pret = read_file_header(datafile, &cs_ver, mdata, gentype);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read the header\n");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}
	
	/* Old data formats do not containg sampling rate information, set it manually */
	if (OLD_FORMAT(gentype)) {
		switch (mdata->file_type) {
		case HPCS_TYPE_CE_DAD:
			mdata->sampling_rate = 20.0;
			break;
		case HPCS_TYPE_CE_ANALOG:
			mdata->sampling_rate = 10.0;
			break;
		default:
			mdata->sampling_rate = CE_WORK_PARAM_SAMPRATE;
		}
	}

	switch (mdata->file_type) {
	case HPCS_TYPE_CE_ANALOG:
	    pret = read_signal(datafile, &mdata->data, &mdata->data_count, CE_WORK_PARAM_OLD_STEP, mdata->sampling_rate, gentype);
	    break;
	case HPCS_TYPE_CE_CCD:
	    pret = read_signal(datafile, &mdata->data, &mdata->data_count, CE_CCD_STEP, mdata->sampling_rate, gentype);
	    break;
	case HPCS_TYPE_CE_CURRENT:
	    pret = read_signal(datafile, &mdata->data, &mdata->data_count, guess_current_step(cs_ver, gentype), mdata->sampling_rate, gentype);
	    break;
	case HPCS_TYPE_CE_DAD:
	    pret = read_signal(datafile, &mdata->data, &mdata->data_count, CE_DAD_STEP, mdata->sampling_rate, gentype);
	    break;
	case HPCS_TYPE_CE_POWER:
	case HPCS_TYPE_CE_VOLTAGE:
	    pret = read_signal(datafile, &mdata->data, &mdata->data_count, guess_elec_sigstep(cs_ver, mdata->file_type), mdata->sampling_rate, gentype);
	    break;
	case HPCS_TYPE_CE_PRESSURE:
	    pret = read_signal(datafile, &mdata->data, &mdata->data_count, CE_WORK_PARAM_STEP, mdata->sampling_rate, gentype);
	    break;
	case HPCS_TYPE_CE_TEMPERATURE:
	    pret = read_signal(datafile, &mdata->data, &mdata->data_count, CE_WORK_PARAM_OLD_STEP * 10.0, mdata->sampling_rate, gentype);
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

enum HPCS_RetCode hpcs_read_mheader(const char* filename, struct HPCS_MeasuredData* mdata)
{
	FILE* datafile;
	enum HPCS_ParseCode pret;
	enum HPCS_RetCode ret;
	enum HPCS_GenType gentype;
	enum HPCS_ChemStationVer cs_ver;

	if (mdata == NULL)
		return HPCS_E_NULLPTR;

	datafile = open_measurement_file(filename);
	if (datafile == NULL)
		return HPCS_E_CANT_OPEN;

	pret = read_generic_type(datafile, &gentype);
	if (pret != PARSE_OK) {
		PR_DEBUG("Cannot read generic file type\n");
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}

	if (!gentype_is_readable(gentype)) {
		PR_DEBUGF("%s: %d\n", "Incompatible file type", gentype);
		ret = HPCS_E_INCOMPATIBLE_FILE;
		goto out;
	}

	pret = read_file_type_description(datafile, &mdata->file_description, gentype);
	if (pret != PARSE_OK) {
		ret = HPCS_E_PARSE_ERROR;
		goto out;
	}

	if (!file_type_description_is_readable(mdata->file_description)) {
		PR_DEBUGF("Incompatible file description: %s\n", mdata->file_description);
		ret = HPCS_E_INCOMPATIBLE_FILE;
		goto out;
	}

	pret = read_file_header(datafile, &cs_ver, mdata, gentype);
	if (pret != PARSE_OK)
		ret = HPCS_E_PARSE_ERROR;
	else
		ret = HPCS_OK;

out:
	fclose(datafile);
	return ret;
}

enum HPCS_RetCode hpcs_read_minfo(const char* filename, struct HPCS_MethodInfo* minfo)
{
	enum HPCS_ParseCode pret;
	HPCS_UFH fh;

	if (minfo == NULL)
		return HPCS_E_NULLPTR;

	fh = open_data_file(filename);
	if (fh == NULL)
		return HPCS_E_CANT_OPEN;

	pret = read_method_info_file(fh, minfo);
	if (pret != PARSE_OK)
		return HPCS_E_PARSE_ERROR;

	return HPCS_OK;
}

static enum HPCS_ParseCode autodetect_file_type(FILE* datafile, enum HPCS_FileType* file_type, const bool p_means_pressure, const enum HPCS_GenType gentype)
{
	char* type_id;
	enum HPCS_ParseCode pret;
	const HPCS_offset devsig_info_offset = OLD_FORMAT(gentype) ? DATA_OFFSET_DEVSIG_INFO_OLD : DATA_OFFSET_DEVSIG_INFO;

	pret = read_string_at_offset(datafile, devsig_info_offset, &type_id, OLD_FORMAT(gentype));
	if (pret != PARSE_OK)
		return pret;

	if (!strcmp(type_id, FILE_TYPE_ID_ADC_A)) {
		*file_type = HPCS_TYPE_CE_ANALOG;
		goto out;
	}
	if (!strcmp(type_id, FILE_TYPE_ID_ADC_B)) {
		*file_type = HPCS_TYPE_CE_ANALOG;
		goto out;
	}

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

out:
	free(type_id);

	return PARSE_OK;
}

static enum HPCS_DataCheckCode check_for_marker(const char* segment, size_t* const next_marker_idx)
{
	if (segment[0] == BIN_MARKER_A && segment[1] != BIN_MARKER_END) {
		*next_marker_idx += (uint8_t)segment[1] + 1;
		return DCHECK_GOT_MARKER;
	} else
		return DCHECK_NO_MARKER;
}

static enum HPCS_ChemStationVer detect_chemstation_version(const char*const version_string)
{
	PR_DEBUGF("ChemStation version string: %s\n", version_string);

	if (!strcmp(version_string, CHEMSTAT_B0625_STR)) {
		PR_DEBUG("ChemStation B.06.25\n");
		return CHEMSTAT_B0625;
	}
	else if (!strcmp(version_string, CHEMSTAT_B0626_STR)) {
		PR_DEBUG("ChemStation B.06.26\n");
		return CHEMSTAT_B0626;
	}
	else if (!strcmp(version_string, CHEMSTAT_B0643_STR)) {
		PR_DEBUG("ChemStation B.06.43\n");
		return CHEMSTAT_B0643;
	}
	else if (strlen(version_string) == 0) {
		PR_DEBUG("ChemStation Untagged\n");
		return CHEMSTAT_UNTAGGED;
	}

	PR_DEBUG("Unknown ChemStation version\n");
	return CHEMSTAT_UNKNOWN;
}


static bool file_type_description_is_readable(const char*const description)
{
	if (!strcmp(FILE_DESC_LC_DATA_FILE, description))
		return true;
	else
		return false;
}

static bool gentype_is_readable(const enum HPCS_GenType gentype)
{
	switch (gentype) {
	case GENTYPE_ADC_LC:
	case GENTYPE_ADC_LC2:
		return true;
	default:
		return false;
	}
}

static HPCS_step guess_current_step(const enum HPCS_ChemStationVer version, const enum HPCS_GenType gentype)
{
	if (version == CHEMSTAT_B0625 || OLD_FORMAT(gentype))
		return CE_WORK_PARAM_OLD_STEP * 10.0;

	return CE_CURRENT_STEP;
}

static HPCS_step guess_elec_sigstep(const enum HPCS_ChemStationVer version, const enum HPCS_FileType file_type)
{
	if (version != CHEMSTAT_B0625) {
		switch (file_type) {
		case HPCS_TYPE_CE_POWER:
			return CE_ENERGY_STEP;
		default:
			return CE_WORK_PARAM_OLD_STEP;
		}
	}

	return CE_WORK_PARAM_STEP;
}

static void guess_sampling_rate(const enum HPCS_ChemStationVer version, struct HPCS_MeasuredData *mdata)
{
	switch (version) {
	case CHEMSTAT_UNTAGGED:
		switch (mdata->file_type) {
		case HPCS_TYPE_CE_DAD:
			mdata->sampling_rate *= 10;
			break;
		default:
			mdata->sampling_rate = CE_WORK_PARAM_SAMPRATE;
		}
		break;
	case CHEMSTAT_B0626:
	case CHEMSTAT_B0643:
		switch (mdata->file_type) {
		case HPCS_TYPE_CE_DAD:
		case HPCS_TYPE_CE_CCD:
			mdata->sampling_rate /= 100;
			break;
		default:
			mdata->sampling_rate = CE_WORK_PARAM_SAMPRATE;
			break;
		}
		break;
	default:
		break;
	}
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

static enum HPCS_ParseCode next_native_line(HPCS_UFH fh, HPCS_NChar* line, int32_t length)
{
#ifdef _WIN32
	return __win32_next_native_line(fh, line, length);
#else
	return __unix_next_native_line(fh, line, length);
#endif
}

static HPCS_UFH open_data_file(const char* filename)
{
#ifdef _WIN32
	return __win32_open_data_file(filename);
#else
	return __unix_open_data_file(filename);
#endif
}

static FILE* open_measurement_file(const char* filename)
{
#ifdef _WIN32
	FILE* f;
	wchar_t *win_filename;

	if (!__win32_utf8_to_wchar(&win_filename, filename))
		return NULL;

	f = _wfopen(win_filename, L"rb");
	free(win_filename);
	return f;
#else
	return fopen(filename, "rb");
#endif
}

static bool p_means_pressure(const enum HPCS_ChemStationVer version)
{
	if (version == CHEMSTAT_B0625)
		return false;

	return true;
}

static enum HPCS_ParseCode parse_native_method_info_line(char** name, char** value, HPCS_NChar* line)
{
#ifdef _WIN32
	return __win32_parse_native_method_info_line(name, value, line);
#else
	return __unix_parse_native_method_info_line(name, value, line);
#endif
}

static enum HPCS_ParseCode read_dad_wavelength(FILE* datafile, struct HPCS_Wavelength* const measured, struct HPCS_Wavelength* const reference, const enum HPCS_GenType gentype)
{
	char* start_idx, *interv_idx, *end_idx, *temp, *str;
	size_t len, tmp_len;
	enum HPCS_ParseCode pret, ret;
	const HPCS_offset devsig_info_offset = OLD_FORMAT(gentype) ? DATA_OFFSET_DEVSIG_INFO_OLD : DATA_OFFSET_DEVSIG_INFO;

	measured->wavelength = 0;
	measured->interval = 0;
	reference->wavelength = 0;
	reference->interval = 0;
	pret = read_string_at_offset(datafile, devsig_info_offset, &str, OLD_FORMAT(gentype));
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

	if (start_idx >= interv_idx) {
		PR_DEBUG("start_idx >= interv_idx\n");
		ret = PARSE_E_CANT_READ;
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
	measured->wavelength = (uint16_t)strtoul(temp, NULL, 10);

	/* Read MEASURED spectral interval */
	if (interv_idx >= end_idx) {
		PR_DEBUG("interv_idx >= end_idx\n");
		ret = PARSE_E_CANT_READ;
		goto out2;
	}

	tmp_len = end_idx - interv_idx;
	if (tmp_len < 1) {
		PR_DEBUG("end_idx - interv_idx < 1\n");
		ret = PARSE_E_CANT_READ;
		goto out2;
	}
	if (tmp_len - 1 > len) {
		free(temp);
		temp = malloc(tmp_len - 1);
		if (temp == NULL) {
			PR_DEBUG("No memory for temporary string\n");
			ret = PARSE_E_NO_MEM;
			goto out;
		}
	}
	len = tmp_len - 2;
	memcpy(temp, interv_idx + 1, len);
	temp[len] = 0;
	measured->interval = (uint16_t)strtoul(temp, NULL, 10);

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

	if (start_idx >= interv_idx) {
		PR_DEBUG("start_idx >= interv_idx\n");
		ret = PARSE_E_CANT_READ;
		goto out2;
	}

	tmp_len = interv_idx - start_idx;
	if (tmp_len > len + 1) {
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
	reference->wavelength = (uint16_t)strtoul(temp, NULL, 10);
	reference->interval = (uint16_t)strtoul(interv_idx + 1, NULL, 10);
	ret = PARSE_OK;

out2:
	free(temp);
out:
	free(str);
	return ret;
}

/* This function assumes that the date information are composed only of the
   first 127 characters from ISO-8859-1 charset. Under such assumption it is
   possible to treat UTF-8 strings as single-byte strings with ISO-8859-1
   encoding */
static enum HPCS_ParseCode read_date(FILE* datafile, struct HPCS_Date* date, const enum HPCS_GenType gentype)
{
	char* date_str;
	char* date_time_delim;
	char* dm_delim, *my_delim;
	char* hm_delim, *ms_delim;
	char temp[32];
	size_t len;
	enum HPCS_ParseCode pret;
	const HPCS_offset date_offset = OLD_FORMAT(gentype) ? DATA_OFFSET_DATE_OLD : DATA_OFFSET_DATE;

	pret = read_string_at_offset(datafile, date_offset, &date_str, OLD_FORMAT(gentype));
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
	date->day = (uint8_t)strtoul(temp, NULL, 10);

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
	date->hour = (uint8_t)strtoul(temp, NULL, 10);

	/* Get minute */
	ms_delim = strchr(hm_delim + 1, DATA_FILE_COLON);
	if (ms_delim == NULL) {
		free(date_str);
		return PARSE_E_NOT_FOUND;
	}
	len = ms_delim - (hm_delim + 1);
	memcpy(temp, hm_delim + 1, len);
	temp[len] = 0;
	date->minute = (uint8_t)strtoul(temp, NULL, 10);

	/* Get second */
	date->second = (uint8_t)strtoul(ms_delim + 1, NULL, 10);

	free(date_str);
	return PARSE_OK;
}

static enum HPCS_ParseCode read_file_header(FILE* datafile, enum HPCS_ChemStationVer* cs_ver, struct HPCS_MeasuredData* mdata, const enum HPCS_GenType gentype)
{
	enum HPCS_ParseCode pret;
	const bool old_format = OLD_FORMAT(gentype);
	const HPCS_offset sample_info_offset = old_format ? DATA_OFFSET_SAMPLE_INFO_OLD : DATA_OFFSET_SAMPLE_INFO;
	const HPCS_offset operator_name_offset = old_format ? DATA_OFFSET_OPERATOR_NAME_OLD : DATA_OFFSET_OPERATOR_NAME;
	const HPCS_offset method_name_offset = old_format ? DATA_OFFSET_METHOD_NAME_OLD : DATA_OFFSET_METHOD_NAME;
	const HPCS_offset y_units_offset = old_format ? DATA_OFFSET_Y_UNITS_OLD : DATA_OFFSET_Y_UNITS;

	pret = read_string_at_offset(datafile, sample_info_offset, &mdata->sample_info, old_format);
	if (pret != PARSE_OK) {
	    PR_DEBUGF("%s%d\n", "Cannot read sample info, errno: ", pret);
	    return pret;
	}
	pret = read_string_at_offset(datafile, operator_name_offset, &mdata->operator_name, old_format);
	if (pret != PARSE_OK) {
	    PR_DEBUGF("%s%d\n", "Cannot read operator name, errno: ", pret);
	    return pret;
	}
	pret = read_string_at_offset(datafile, method_name_offset, &mdata->method_name, old_format);
	if (pret != PARSE_OK) {
	    PR_DEBUGF("%s%d\n", "Cannot read method name, errno: ", pret);
	    return pret;
	}
	pret = read_date(datafile, &mdata->date, gentype);
	if (pret != PARSE_OK) {
	    PR_DEBUGF("%s%d\n", "Cannot read date of measurement, errno: ", pret);
	    return pret;
	}

	if (!old_format) {
		pret = read_string_at_offset(datafile, DATA_OFFSET_CS_VER, &mdata->cs_ver, old_format);
		if (pret != PARSE_OK) {
			PR_DEBUGF("%s%d\n", "Cannot read ChemStation software version, errno: ", pret);
			return pret;
		}
		pret = read_string_at_offset(datafile, DATA_OFFSET_CS_REV, &mdata->cs_rev, old_format);
			if (pret != PARSE_OK) {
			PR_DEBUGF("%s%d\n", "Cannot read ChemStation software revision, errno: ", pret);
			return pret;
		}
	} else {
		mdata->cs_ver = DEFAULT_CS_VER;
		mdata->cs_rev = DEFAULT_CS_REV;
	}

	pret = read_string_at_offset(datafile, y_units_offset, &mdata->y_units, old_format);
	if (pret != PARSE_OK) {
		PR_DEBUGF("%s%d\n", "Cannot read values of Y axis, errno: ", pret);
		return pret;
	}

	pret = read_sampling_rate(datafile, &mdata->sampling_rate, old_format);
	if (pret != PARSE_OK) {
	    PR_DEBUGF("%s%d\n", "Cannot read sampling rate of the file, errno: ", pret);
	    return pret;
	}

	*cs_ver = detect_chemstation_version(mdata->cs_ver);
	if (pret != PARSE_OK) {
		PR_DEBUGF("%s%d\n", "Cannot detect ChemStation version, errno: ", pret);
		return pret;
	}

	pret = autodetect_file_type(datafile, &mdata->file_type, p_means_pressure(*cs_ver), gentype);
	if (pret != PARSE_OK) {
	    PR_DEBUGF("%s%d\n", "Cannot determine the type of file, errno: ", pret);
	    return pret;
	}

	if (mdata->file_type == HPCS_TYPE_CE_DAD) {
	    pret = read_dad_wavelength(datafile, &mdata->dad_wavelength_msr, &mdata->dad_wavelength_ref, gentype);
	    if (pret != PARSE_OK && pret != PARSE_W_NO_DATA) {
			PR_DEBUGF("%s%d\n", "Cannot read wavelength, errno: ", pret);
			return pret;
	    }
	}

	guess_sampling_rate(*cs_ver, mdata);
	return PARSE_OK;
}

static enum HPCS_ParseCode read_file_type_description(FILE* datafile, char** const description, const enum HPCS_GenType gentype)
{
	enum HPCS_ParseCode pret;
	const HPCS_offset offset = OLD_FORMAT(gentype) ? DATA_OFFSET_FILE_DESC_OLD : DATA_OFFSET_FILE_DESC;

	pret = read_string_at_offset(datafile, offset, description, OLD_FORMAT(gentype));
	if (pret != PARSE_OK)
		PR_DEBUGF("%s%d\n", "Cannot read file description, errno: ", pret);

	return pret;
}

static enum HPCS_ParseCode read_method_info_file(HPCS_UFH fh, struct HPCS_MethodInfo* minfo)
{
	HPCS_NChar line[64];
	size_t allocated = 0;

	while (next_native_line(fh, line, 64) == PARSE_OK) {
		enum HPCS_ParseCode pret;
		char* name = NULL;
		char* value = NULL;

		pret = parse_native_method_info_line(&name, &value, line);
		if (pret != PARSE_OK) {
			free(name);
			free(value);
			return pret;
		}

		if (minfo->count+1 > allocated) {
			size_t to_allocate;
			if (allocated == 0)
				to_allocate = 256;
			else
			    to_allocate = allocated * 2;

			struct HPCS_MethodInfoBlock* new_blocks = realloc(minfo->blocks, to_allocate * sizeof(struct HPCS_MethodInfoBlock));
			if (new_blocks == NULL)
				return PARSE_E_NO_MEM;
			else {
				minfo->blocks = new_blocks;
				allocated = to_allocate;
			}
		}

		minfo->blocks[minfo->count].name = name;
		minfo->blocks[minfo->count].value = value;
		minfo->count++;
	}

	return PARSE_OK;
}

static enum HPCS_ParseCode read_generic_type(FILE* datafile, enum HPCS_GenType* gentype)
{
	enum HPCS_ParseCode ret;
	uint8_t len;
	char* gentype_str;

	fseek(datafile, DATA_OFFSET_GENTYPE, SEEK_SET);
	if (feof(datafile))
		return PARSE_E_OUT_OF_RANGE;
	if (ferror(datafile))
		return PARSE_E_CANT_READ;

	if (fread(&len, SMALL_SEGMENT_SIZE, 1, datafile) < SMALL_SEGMENT_SIZE)
		return PARSE_E_CANT_READ;

	gentype_str = malloc((sizeof(char) * len) + 1);
	if (gentype_str == NULL)
		return PARSE_E_NO_MEM;

	if (fread(gentype_str, SMALL_SEGMENT_SIZE, len, datafile) < SMALL_SEGMENT_SIZE * len) {
		ret = PARSE_E_CANT_READ;
		goto out;
	}

	if (feof(datafile)) {
		ret = PARSE_E_OUT_OF_RANGE;
		goto out;
	}
	if (ferror(datafile)) {
		ret = PARSE_E_CANT_READ;
		goto out;
	}

	gentype_str[len] = '\0';

	PR_DEBUGF("Generic type: %s\n", gentype_str);

	*gentype = strtol(gentype_str, NULL, 10);
	ret = PARSE_OK;

out:
	free(gentype_str);

	return ret;
}

static enum HPCS_ParseCode read_signal(FILE* datafile, struct HPCS_TVPair** pairs, size_t* pairs_count,
				       const HPCS_step step, const double sampling_rate, const enum HPCS_GenType gentype)
{
	const double time_step = 1 / (60 * sampling_rate);
        size_t alloc_size = (size_t)((60 * sampling_rate) + 0.5);
	bool read_file = true;
	double value = 0;
	double time = 0;
	size_t segments_read = 0;
	size_t data_segments_read = 0;
	size_t next_marker_idx = 0;
	const HPCS_offset data_start_offset = OLD_FORMAT(gentype) ? DATA_OFFSET_DATA_START_OLD : DATA_OFFSET_DATA_START;
	char raw[2];
	size_t r;
	enum HPCS_DataCheckCode dret;

	fseek(datafile, data_start_offset, SEEK_SET);
	if (feof(datafile))
		return PARSE_E_OUT_OF_RANGE;
	if (ferror(datafile))
		return PARSE_E_CANT_READ;

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

static enum HPCS_ParseCode read_sampling_rate(FILE* datafile, double* sampling_rate, const bool old_format)
{
	char raw[2];
	uint16_t number;
	size_t r;

	if (old_format) {
		*sampling_rate = 0.0; /* This information cannot be read from the datafile */
		return PARSE_OK;
	}

	fseek(datafile, DATA_OFFSET_SAMPLING_RATE, SEEK_SET);
	if (feof(datafile))
		return PARSE_E_OUT_OF_RANGE;
	if (ferror(datafile))
		return PARSE_E_CANT_READ;

	r = fread(raw, SEGMENT_SIZE, 1, datafile);
	if (r != 1)
		return PARSE_E_CANT_READ;

	be_to_cpu(raw);
	number = *(uint16_t*)(raw);
	*sampling_rate = number / 10.0;

	return PARSE_OK;
}

static enum HPCS_ParseCode read_string_at_offset(FILE* datafile, const HPCS_offset offset, char** const result, const bool old_format)
{
	if (old_format)
		return __read_string_at_offset_v1(datafile, offset, result);
	return __read_string_at_offset_v2(datafile, offset, result);
}

static enum HPCS_ParseCode __read_string_at_offset_v1(FILE* datafile, const HPCS_offset offset, char** const result)
{
	size_t r;
	char ch;
	char* string;
	enum HPCS_ParseCode ret;
	size_t str_length = 0;

	PR_DEBUG("Using v1 string read\n");

	fseek(datafile, offset, SEEK_SET);
	if (feof(datafile))
		return PARSE_E_OUT_OF_RANGE;
	if (ferror(datafile))
		return PARSE_E_CANT_READ;

	/* Read the length of the string */
	while (true) {
		r = fread(&ch, SMALL_SEGMENT_SIZE, 1, datafile);
		if (r != 1)
			return PARSE_E_CANT_READ;

		if (ch != '\0')
			str_length++;
		else
			break;
	}

	PR_DEBUGF("String length to read: %lu\n", str_length);

	if (str_length == 0) {
		*result = malloc(sizeof(char));

		if (*result == NULL)
			return PARSE_E_NO_MEM;

		(*result)[0] = '\0';
		return PARSE_OK;
	}

	/* Allocate read buffer */
	string = calloc(str_length + 1, SMALL_SEGMENT_SIZE);
	if (string == NULL)
		return PARSE_E_NO_MEM;

	memset(string, 0, (str_length + 1));

	/* Rewind the file and read the string */
	fseek(datafile, offset, SEEK_SET);
	r = fread(string, SMALL_SEGMENT_SIZE, str_length, datafile);
	if (r < str_length) {
		free(string);
		return PARSE_E_CANT_READ;
	}

#ifdef _WIN32
	ret = __win32_latin1_to_utf8(result, string);
#else
	ret = __unix_data_to_utf8(result, string, "ISO-8859-1", str_length);
#endif
	free(string);

	return ret;
}

static enum HPCS_ParseCode __read_string_at_offset_v2(FILE* datafile, const HPCS_offset offset, char** const result)
{
	char* string;
	uint8_t str_length;
	size_t r;
	enum HPCS_ParseCode ret;

	PR_DEBUG("Using v2 string read\n");

	fseek(datafile, offset, SEEK_SET);
	if (feof(datafile))
		return PARSE_E_OUT_OF_RANGE;
	if (ferror(datafile))
		return PARSE_E_CANT_READ;

	r = fread(&str_length, SMALL_SEGMENT_SIZE, 1, datafile);
	if (r != 1)
		return PARSE_E_CANT_READ;

	PR_DEBUGF("String length to read: %u\n", str_length);

	if (str_length == 0) {
		*result = malloc(sizeof(char));

		if (*result == NULL)
			return PARSE_E_NO_MEM;

		(*result)[0] = '\0';
		return PARSE_OK;
	}

	string = calloc(str_length + 1, SEGMENT_SIZE);
	if (string == NULL)
		return PARSE_E_NO_MEM;
	memset(string, 0, (str_length + 1) * SEGMENT_SIZE);

	r = fread(string, SEGMENT_SIZE, str_length, datafile);
	if (r < str_length) {
		free(string);
		return PARSE_E_CANT_READ;
	}

#ifdef _WIN32
	/* String is stored as native Windows WCHAR */
	ret = __win32_wchar_to_utf8(result, (WCHAR*)string);
#else
	/* Explicitly convert from UTF-16LE (internal WCHAR representation) */
	ret = __unix_data_to_utf8(result, string, "UTF-16LE", str_length * SEGMENT_SIZE);
#endif

	free(string);
	return ret;
}

static void remove_trailing_newline(HPCS_NChar* s)
{
	HPCS_NChar* newline;
#ifdef _WIN32
	newline = StrStrW(s, CR_LF);
	if (newline != NULL)
		*newline = (WCHAR)0;
#else	
	newline = u_strrstr(s, CR_LF);
	if (newline != NULL)
		*newline = (UChar)0;
#endif
}

/** Platform-specific functions */

#ifdef _WIN32
static enum HPCS_ParseCode __win32_next_native_line(FILE* fh, WCHAR* line, int32_t length)
{
	if (fgetws(line, length, fh) == NULL)
		return PARSE_E_CANT_READ;

	return PARSE_OK;
}

static FILE* __win32_open_data_file(const char* filename)
{
	FILE* fh;
	WCHAR* w_filename;
	int w_size;

	/* Get the required size */
	w_size = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, filename, -1, NULL, 0);
	if (w_size == 0) {
		PR_DEBUGF("Count MultiByteToWideChar() error: %x\n", GetLastError());
		return NULL;
	}
	w_filename = calloc(w_size, sizeof(WCHAR));
	if (w_filename == NULL)
		return NULL;

	if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, filename, -1, w_filename, w_size) == 0) {
		PR_DEBUGF("Convert MultiByteToWideChar() error: %x\n", GetLastError());
		free(w_filename);
		return NULL;
	}
	fh = _wfopen(w_filename, L"r, ccs=UNICODE");

	free(w_filename);
	return fh;
}

static enum HPCS_ParseCode __win32_parse_native_method_info_line(char** name, char** value, WCHAR* line)
{
	WCHAR* w_name;
	WCHAR* w_value;
	enum HPCS_ParseCode ret;

#if _MSC_VER >= 1900
	w_name = wcstok(line, EQUALITY_SIGN, NULL);
#else
	w_name = wcstok(line, EQUALITY_SIGN);
#endif

	if (w_name == NULL)
		return PARSE_E_NOT_FOUND;

	remove_trailing_newline(w_name);
	ret = __win32_wchar_to_utf8(name, w_name);
	if (ret != PARSE_OK)
		return ret;

#if _MSC_VER >= 1900
	w_value = wcstok(line, EQUALITY_SIGN, NULL);
#else
	w_value = wcstok(NULL, EQUALITY_SIGN);
#endif
	if (w_value == NULL) {
		/* Add an empty string if there is no value */
		*value = malloc(1);
		if (*value == NULL)
			return PARSE_E_NO_MEM;
		*value[0] = 0;
		return PARSE_OK;
	}

	remove_trailing_newline(w_value);
	ret = __win32_wchar_to_utf8(value, w_value);
	if (ret != PARSE_OK)
		return ret;

	return PARSE_OK;
}

static enum HPCS_ParseCode __win32_latin1_to_utf8(char** target, const char *s)
{
	wchar_t* intermediate;
	size_t mb_size;

	size_t w_size = MultiByteToWideChar(28591, 0, s, -1, NULL, 0);
	if (w_size == 0) {
		PR_DEBUGF("Count MultiByteToWideChar() error 0x%x\n", GetLastError());
		return PARSE_E_INTERNAL;
	}
	PR_DEBUGF("w_size: %d\n", w_size);

	intermediate = malloc(sizeof(wchar_t) * w_size);
	if (intermediate == NULL)
		return PARSE_E_NO_MEM;

	if (MultiByteToWideChar(28591, 0, s, -1, intermediate, w_size) == 0) {
		PR_DEBUGF("Convert MultiByteToWideChar() error 0x%x\n", GetLastError());
		return PARSE_E_INTERNAL;
	}

	mb_size = WideCharToMultiByte(CP_UTF8, 0, intermediate, -1, NULL, 0, NULL, NULL);
	if (mb_size == 0) {
		PR_DEBUGF("Count WideCharToMultiByte() error: 0x%x\n", GetLastError());
		return PARSE_E_INTERNAL;
	}

	*target = malloc(mb_size);
	if (*target == NULL) {
		free(intermediate);
		return PARSE_E_NO_MEM;
	}

	if (WideCharToMultiByte(CP_UTF8, 0, intermediate, -1, *target, mb_size, NULL, NULL) == 0) {
		free(*target);
		PR_DEBUGF("Convert WideCharToMultiByte() error: 0x%x\n", GetLastError());
		return PARSE_E_INTERNAL;
	}

	free(intermediate);

	return PARSE_OK;
}

static bool __win32_utf8_to_wchar(wchar_t** target, const char *s)
{
	size_t w_size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0);
	if (w_size == 0) {
		PR_DEBUGF("Count MultiByteToWideChar() error 0x%x\n", GetLastError());
		return false;
	}
	PR_DEBUGF("w_size: %d\n", w_size);
	*target = malloc(sizeof(wchar_t) * w_size);
	if (*target == NULL)
		return false;

	if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, *target, w_size) == 0) {
		free(*target);
		PR_DEBUGF("Convert MultiByteToWideChar() error 0x%x\n", GetLastError());
		return false;
	}

	return true;
}

static enum HPCS_ParseCode __win32_wchar_to_utf8(char** target, const WCHAR* s)
{
	int mb_size;

	mb_size = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
	if (mb_size == 0) {
		PR_DEBUGF("Count WideCharToMultiByte() error: 0x%x\n", GetLastError());
		return PARSE_E_INTERNAL;
	}
	PR_DEBUGF("mb_size: %d\n", mb_size);
	*target = malloc(mb_size);
	if (*target == NULL)
		return PARSE_E_NO_MEM;

	if (WideCharToMultiByte(CP_UTF8, 0, s, -1, *target, mb_size, NULL, NULL) == 0) {
		free(*target);
		PR_DEBUGF("Convert WideCharToMultiByte() error: 0x%x\n", GetLastError());
		return PARSE_E_INTERNAL;
	}

	return PARSE_OK;
}

#else
static void __unix_hpcs_initialize()
{
	/* Initialize all Unicode strings */
	__ICU_INIT_STRING(EQUALITY_SIGN, "\\x3d");
	__ICU_INIT_STRING(CR_LF, "\\x0d\\x0a");
}

static void __unix_hpcs_destroy()
{
	free(EQUALITY_SIGN);
	free(CR_LF);
}

static enum HPCS_ParseCode __unix_icu_to_utf8(char** target, const UChar* s)
{
	int32_t utf8_size;
	UConverter* cnv;
	UErrorCode uec = U_ZERO_ERROR;

	cnv = ucnv_open("UTF-8", &uec);
	if (U_FAILURE(uec)) {
		PR_DEBUGF("Unable to create converter, error: %s\n", u_errorName(uec));
		return PARSE_E_INTERNAL;
	}

	utf8_size = ucnv_fromUChars(cnv, NULL, 0, s, -1, &uec);
	if (U_FAILURE(uec) && uec != U_BUFFER_OVERFLOW_ERROR) {
		ucnv_close(cnv);
		PR_DEBUGF("Count ucnv_fromUChars(), error: %s\n", u_errorName(uec));
		return PARSE_E_INTERNAL;
	}
	uec = U_ZERO_ERROR;

	if (utf8_size == 0) {
		ucnv_close(cnv);
		return PARSE_E_CANT_READ;
	}

	*target = malloc(utf8_size + 1);
	if (*target == NULL) {
		ucnv_close(cnv);
		return PARSE_E_NO_MEM;
	}
	memset(*target, 0, utf8_size + 1);

	ucnv_fromUChars(cnv, *target, utf8_size, s, -1, &uec);
	ucnv_close(cnv);
	if (U_FAILURE(uec)) {
		free(*target);
		PR_DEBUGF("Convert ucnv_fromUChars(), error: %s\n", u_errorName(uec));
		return PARSE_E_INTERNAL;
	}

	return PARSE_OK;
}

static enum HPCS_ParseCode __unix_next_native_line(UFILE* fh, UChar* line, int32_t length)
{
	if (u_fgets(line, length, fh) == NULL)
		return PARSE_E_CANT_READ;

	return PARSE_OK;
}

static UFILE* __unix_open_data_file(const char* filename)
{
	return u_fopen(filename, "r", "en_US", "UTF-16");
}

static enum HPCS_ParseCode __unix_parse_native_method_info_line(char** name, char** value, UChar* line)
{
	UChar* u_name;
	UChar* u_value;
	UChar* saveptr;
	enum HPCS_ParseCode ret;

	u_name = u_strtok_r(line, EQUALITY_SIGN, &saveptr);
	if (u_name == NULL)
		return PARSE_E_NOT_FOUND;

	remove_trailing_newline(u_name);
	ret = __unix_icu_to_utf8(name, u_name);
	if (ret != PARSE_OK)
		return ret;

	u_value = u_strtok_r(NULL, EQUALITY_SIGN, &saveptr);
	if (u_value == NULL) {
		/* Add an empty string if there is no value */
		*value = malloc(1);
		if (*value == NULL)
			return PARSE_E_NO_MEM;
		*value[0] = 0;
		return PARSE_OK;
	}

	remove_trailing_newline(u_value);
	ret = __unix_icu_to_utf8(value, u_value);
	if (ret != PARSE_OK)
		return ret;

	return PARSE_OK;
}

static enum HPCS_ParseCode __unix_data_to_utf8(char** target, const char* bytes, const char* encoding, const size_t bytes_count)
{
	int32_t u_size;
	UChar* u_str;
	UConverter* cnv;
	enum HPCS_ParseCode ret;
	UErrorCode uec = U_ZERO_ERROR;

	cnv = ucnv_open(encoding, &uec);
	if (U_FAILURE(uec)) {
		PR_DEBUGF("Unable to create converter, error: %s\n", u_errorName(uec));
		return PARSE_E_INTERNAL;
	}

	u_size = ucnv_toUChars(cnv, NULL, 0, bytes, bytes_count, &uec);
	if (U_FAILURE(uec) && uec != U_BUFFER_OVERFLOW_ERROR) {
		ucnv_close(cnv);
		PR_DEBUGF("Count ucnv_toUchars(), error: %s\n", u_errorName(uec));
		return PARSE_E_INTERNAL;
	}
	uec = U_ZERO_ERROR;

	if (u_size == 0) {
		ucnv_close(cnv);
		return PARSE_E_CANT_READ;
	}
	u_str = calloc(u_size + 1, sizeof(UChar));
	if (u_str == NULL) {
		ucnv_close(cnv);
		return PARSE_E_NO_MEM;
	}
	memset(u_str, 0, (u_size + 1) * sizeof(UChar));

	ucnv_toUChars(cnv, u_str, u_size, bytes, bytes_count, &uec);
	ucnv_close(cnv);
	if (U_FAILURE(uec)) {
		free(u_str);
		PR_DEBUGF("Convert ucnv_toUchars(), error: %s\n", u_errorName(uec));
		return PARSE_E_INTERNAL;
	}

	ret = __unix_icu_to_utf8(target, u_str);
	free(u_str);
	return ret;
}
#endif


static char* __DEFAULT_CS_REV()
{
	static const char* s = "UNKNOWN_REVISION";
	char* ns = malloc(strlen(s) + 1);
	memset(ns, 0, strlen(s) + 1);
	strcpy(ns, s);
	return ns;
}

static char* __DEFAULT_CS_VER()
{
	static const char* s = "UNKNOWN_VERSION";
	char* ns = malloc(strlen(s) + 1);
	memset(ns, 0, strlen(s) + 1);
	strcpy(ns, s);
	return ns;
}

#ifdef __cplusplus
}
#endif
