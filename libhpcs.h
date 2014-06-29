#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

enum HPCS_File_Type {
	TYPE_CE_CURRENT,
	TYPE_CE_CCD,
	TYPE_CE_DAD,
	TYPE_CE_POWER,
	TYPE_CE_VOLTAGE,
	TYPE_UNKNOWN
};

enum HPCS_RetCode {
	HPCS_OK,
	HPCS_E_NULLPTR,
	HPCS_E_CANT_OPEN,
	HPCS_E_PARSE_ERROR,
	HPCS_E_UNKNOWN_TYPE
};

struct HPCS_Date {
	uint32_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

struct HPCS_TVPair {
	double time;
	double value;
};

struct HPCS_MeasuredData {
	char* file_description;
	char* sample_info;
	char* operator_name;
	struct HPCS_Date date;
	char* method_name;
	char* cs_ver;
	char* cs_rev;
	char* y_units;
	double sampling_rate;
	uint16_t dad_wavelength_msr;
	uint16_t dad_wavelength_ref;
	enum HPCS_File_Type file_type;
	struct HPCS_TVPair* data;
	size_t data_count;
};

#ifdef __WIN32__
__declspec(dllexport) enum HPCS_RetCode __cdecl hpcs_read_file(const char* const filename, struct HPCS_MeasuredData* mdata);
__declspec(dllexport) char* __cdecl hpcs_error_to_string(const enum HPCS_RetCode);
#else
enum HPCS_RetCode hpcs_read_file(const char* const filename, struct HPCS_MeasuredData* mdata);
char* hpcs_error_to_string(const enum HPCS_RetCode);
#endif

#ifdef __cplusplus
}
#endif
