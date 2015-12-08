#ifndef LIBHPCS_P_H
#define LIBHCPS_P_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
typedef int bool;
#define true 1
#define false 0
#else
#include <stdbool.h>
#endif
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#define HPCS_NChar WCHAR
#define HPCS_UFH FILE*
#else
#include <unicode/ustdio.h>
#include <unicode/ustring.h>
#define HPCS_NChar UChar
#define HPCS_UFH UFILE*
#endif

enum HPCS_DataCheckCode {
	DCHECK_GOT_MARKER,
	DCHECK_EOF,
	DCHECK_NO_MARKER
};

enum HPCS_ParseCode {
	PARSE_OK,
	PARSE_E_OUT_OF_RANGE,
	PARSE_E_NO_MEM,
	PARSE_E_CANT_READ,
	PARSE_E_NOT_FOUND,
	PARSE_E_INV_PARAM,
	PARSE_E_INTERNAL,
	PARSE_W_NO_DATA
};

typedef size_t HPCS_offset;
typedef double HPCS_step;
typedef size_t HPCS_segsize;


/* Identifiers of file types found at offset 0x1075 onward */
const char FILE_TYPE_ID_DAD[] = "DAD";
const char FILE_TYPE_ID_HPCE[] = "HPCE";
const char FILE_TYPE_HPCE_CCD = 'L';
const char FILE_TYPE_HPCE_CURRENT = 'C';
const char FILE_TYPE_HPCE_POWER_PRESSURE = 'P';
const char FILE_TYPE_HPCE_POWER = 'E';
const char FILE_TYPE_HPCE_TEMPERATURE = 'T';
const char FILE_TYPE_HPCE_VOLTAGE = 'V';

/* Char and text values */
const char* WAVELENGTH_MEASURED_TEXT = "Sig=";
const char* WAVELENGTH_REFERENCE_TEXT = "Ref=";
const char* WAVELENGTH_REFERENCE_OFF_TEXT = "off";
const char WAVELENGTH_DELIMITER_TEXT = (const char)(0x2C);
const char WAVELENGTH_END_TEXT = (const char)(0x20);
const char DATA_FILE_COMMA = (const char)(0x2C);
const char DATA_FILE_DASH = (const char)(0x2D);
const char DATA_FILE_COLON = (const char)(0x3A);
const char MON_JAN_STR[] = "Jan";
const char MON_FEB_STR[] = "Feb";
const char MON_MAR_STR[] = "Mar";
const char MON_APR_STR[] = "Apr";
const char MON_MAY_STR[] = "May";
const char MON_JUN_STR[] = "Jun";
const char MON_JUL_STR[] = "Jul";
const char MON_AUG_STR[] = "Aug";
const char MON_SEP_STR[] = "Sep";
const char MON_OCT_STR[] = "Oct";
const char MON_NOV_STR[] = "Nov";
const char MON_DEC_STR[] = "Dec";

/* Precision of measured values. */
const HPCS_step CE_CURRENT_STEP = 0.01;
const HPCS_step CE_CCD_STEP = 0.0000596046450027643;
const HPCS_step CE_DAD_STEP = 0.000476837158203;
const HPCS_step CE_ENERGY_STEP = 0.00000459365687208207;
const HPCS_step CE_WORK_PARAM_STEP = 0.001;
const HPCS_step CE_WORK_PARAM_OLD_STEP = 0.000083333333;

/* Hardcoded sampling rates */
const double CE_WORK_PARAM_SAMPRATE = 1.67;

/* Offsets containing data of interest in .ch files */
const HPCS_offset DATA_OFFSET_GENTYPE = 0x000;
const HPCS_offset DATA_OFFSET_FILE_DESC = 0x15B;
const HPCS_offset DATA_OFFSET_SAMPLE_INFO = 0x35A;
const HPCS_offset DATA_OFFSET_OPERATOR_NAME = 0x758;
const HPCS_offset DATA_OFFSET_DATE = 0x957;
const HPCS_offset DATA_OFFSET_METHOD_NAME = 0xA0E;
const HPCS_offset DATA_OFFSET_CS_VER = 0xE11;
const HPCS_offset DATA_OFFSET_CS_REV = 0xEDA;
const HPCS_offset DATA_OFFSET_SAMPLING_RATE = 0x101C;
const HPCS_offset DATA_OFFSET_Y_UNITS = 0x104C;
const HPCS_offset DATA_OFFSET_DEVSIG_INFO = 0x1075;
const HPCS_offset DATA_OFFSET_DATA_START = 0x1800;

/* General data file types */
enum HPCS_GenType {
	GENTYPE_GC_MS = 2,
	GENTYPE_ADC_LC = 30,
	GENTYPE_UV_SPECT = 31,
	GENTYPE_GC_A = 8,
	GENTYPE_GC_A2 = 81,
	GENTYPE_GC_B = 179,
	GENTYPE_GC_B2 = 180,
	GENTYPE_GC_B3 = 181,
	GENTYPE_ADC_LC2 = 130,
	GENTYPE_ADC_UV2 = 131
};

/* Known file descriptions */
const char FILE_DESC_LC_DATA_FILE[] = "LC DATA FILE";

enum HPCS_ChemStationVer {
	CHEMSTAT_UNTAGGED,
	CHEMSTAT_B0625,
	CHEMSTAT_B0626,
	CHEMSTAT_B0643,
	CHEMSTAT_UNKNOWN
};

/* Known ChemStation format versions */
const char CHEMSTAT_B0625_STR[] = "B.06.25 [0003]";
const char CHEMSTAT_B0626_STR[] = "B.06.26 [0010]";
const char CHEMSTAT_B0643_STR[] = "B.06.43 [0001]";

/* Values of markers found in .ch files */
const char BIN_MARKER_A = 0x10;
const char BIN_MARKER_END = 0x00;
const char BIN_MARKER_JUMP = (const char)(0x80);

const HPCS_segsize SMALL_SEGMENT_SIZE = 1;
const HPCS_segsize SEGMENT_SIZE = 2;
const HPCS_segsize LARGE_SEGMENT_SIZE = 4;

const char HPCS_OK_STR[] = "OK.";
const char HPCS_E_NULLPTR_STR[] = "Null pointer to measured data struct.";
const char HPCS_E_CANT_OPEN_STR[] = "Cannot open the specified file.";
const char HPCS_E_PARSE_ERROR_STR[] = "Cannot parse the specified file, it might be corrupted or of unknown type.";
const char HPCS_E_UNKNOWN_TYPE_STR[] = "The specified file contains an unknown type of measurement.";
const char HPCS_E_INCOMPATIBLE_FILE_STR[] = "The specified file is of type that is unreadable by libHPCS.";
const char HPCS_E__UNKNOWN_EC_STR[] = "Unknown error code.";

#ifdef _WIN32
WCHAR EQUALITY_SIGN[] = { 0x003D, 0x0000 };
WCHAR CR_LF[] = { 0x000A, 0x0000 }; /* Windows hides the actual end-of-line which is {0x000D, 0x000A} from us */
#else
UChar* EQUALITY_SIGN;
UChar* CR_LF;
#endif

static enum HPCS_ParseCode autodetect_file_type(FILE* datafile, enum HPCS_FileType* file_type, const bool p_means_pressure);
static enum HPCS_DataCheckCode check_for_marker(const char* segment, size_t* const next_marker_idx);
static enum HPCS_ChemStationVer detect_chemstation_version(const char*const version_string);
static bool gentype_is_readable(const enum HPCS_GenType gentype);
static HPCS_step guess_current_step(const enum HPCS_ChemStationVer version);
static HPCS_step guess_elec_sigstep(const enum HPCS_ChemStationVer version, const enum HPCS_FileType file_type);
static void guess_sampling_rate(const enum HPCS_ChemStationVer version, struct HPCS_MeasuredData* mdata);
static bool file_type_description_is_readable(const char*const description);
static enum HPCS_ParseCode next_native_line(HPCS_UFH fh, HPCS_NChar* line, int32_t length);
static HPCS_UFH open_data_file(const char* filename);
static FILE* open_measurement_file(const char* filename);
static enum HPCS_ParseCode parse_native_method_info_line(char** name, char** value, HPCS_NChar* line);
static enum HPCS_ParseCode read_dad_wavelength(FILE* datafile, struct HPCS_Wavelength* const measured, struct HPCS_Wavelength* const reference);
static uint8_t month_to_number(const char* month);
static bool p_means_pressure(const enum HPCS_ChemStationVer version);
static enum HPCS_ParseCode read_date(FILE* datafile, struct HPCS_Date* date);
static enum HPCS_ParseCode read_file_header(FILE* datafile, enum HPCS_ChemStationVer* cs_ver, struct HPCS_MeasuredData* mdata);
static enum HPCS_ParseCode read_file_type_description(FILE* datafile, char** const description);
static enum HPCS_ParseCode read_generic_type(FILE* datafile, enum HPCS_GenType* gentype);
static enum HPCS_ParseCode read_method_info_file(HPCS_UFH fh, struct HPCS_MethodInfo* minfo);
static enum HPCS_ParseCode read_signal(FILE* datafile, struct HPCS_TVPair** pairs, size_t* pairs_count,
				       const HPCS_step step, const double sampling_rate);
static enum HPCS_ParseCode read_sampling_rate(FILE* datafile, double* sampling_rate);
static enum HPCS_ParseCode read_string_at_offset(FILE* datafile, const HPCS_offset, char** const result);
static void remove_trailing_newline(HPCS_NChar* s);

/** Platform-specific functions */
#ifdef _WIN32
static enum HPCS_ParseCode __win32_next_native_line(FILE* fh, WCHAR* line, int32_t length);
static HPCS_UFH __win32_open_data_file(const char* filename);
static enum HPCS_ParseCode __win32_parse_native_method_info_line(char** name, char** value, WCHAR* line);
static bool __win32_utf8_to_wchar(wchar_t** target, const char* s);
static enum HPCS_ParseCode __win32_wchar_to_utf8(char** target, const WCHAR* s);
#else
static void __attribute((constructor)) __unix_hpcs_initialize();
static void __attribute((destructor)) __unix_hpcs_destroy();
static enum HPCS_ParseCode __unix_icu_to_utf8(char** target, const UChar* s);
static HPCS_UFH __unix_open_data_file(const char* filename);
static enum HPCS_ParseCode __unix_next_native_line(UFILE* fh, UChar* line, int32_t length);
static enum HPCS_ParseCode __unix_parse_native_method_info_line(char** name, char** value, UChar* line);
static enum HPCS_ParseCode __unix_wchar_to_utf8(char** target, const char* bytes, const size_t bytes_count);

#define __ICU_INIT_STRING(dst, s) do { \
	UChar temp[64]; \
	int32_t length = u_unescape(s, temp, sizeof(temp)); \
	dst = calloc(length + 1, sizeof(UChar)); \
	u_strcpy(dst, temp); \
} while(0)
#endif

#ifdef _HPCS_LITTLE_ENDIAN
#define be_to_cpu(bytes) reverse_endianness((char*)bytes, sizeof(bytes));

void reverse_endianness(char* bytes, size_t sz) {
	size_t i;
	for (i = 0; i < sz/2; i++) {
		char t = bytes[i];
		bytes[i] = bytes[sz - i - 1];
		bytes[sz - i - 1] = t;
	}

}

#elif defined _HPCS_BIG_ENDIAN
#define be_to_cpu(bytes)
#else
#error "Endiannes has not been determined."
#endif

#ifndef NDEBUG
 #ifdef _MSC_VER
 #define __func__ __FUNCTION__
 #endif
 #define PR_DEBUGF(fmt, ...) fprintf(stderr, "[%s()] "fmt, __func__, __VA_ARGS__)
 #define PR_DEBUG(msg) fprintf(stderr, "[%s()] "msg, __func__)
#else
 #define PR_DEBUGF(fmt, ...) ((void)0)
 #define PR_DEBUG(msg) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* LIBHCPS_P_H */
