#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

enum HPCS_File_Type {
	HPCS_TYPE_CE_CCD,
	HPCS_TYPE_CE_CURRENT,
	HPCS_TYPE_CE_DAD,
	HPCS_TYPE_CE_POWER,
	HPCS_TYPE_CE_VOLTAGE,
	HPCS_TYPE_UNKNOWN
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

struct HPCS_Wavelength {
	uint16_t wavelength;
	uint16_t interval;
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
	struct HPCS_Wavelength dad_wavelength_msr;
	struct HPCS_Wavelength dad_wavelength_ref;
	enum HPCS_File_Type file_type;
	struct HPCS_TVPair* data;
	size_t data_count;
};

#ifdef __WIN32__
__declspec(dllexport) struct HPCS_MeasuredData* __cdecl hpcs_alloc();
__declspec(dllexport) void __cdecl hpcs_free(struct HPCS_MeasuredData* const mdata);
__declspec(dllexport) char* __cdecl hpcs_error_to_string(const enum HPCS_RetCode);
__declspec(dllexport) enum HPCS_RetCode __cdecl hpcs_read_file(const char* const filename, struct HPCS_MeasuredData* mdata);
#else
struct HPCS_MeasuredData* hpcs_alloc();
void hpcs_free(struct HPCS_MeasuredData* const mdata);
char* hpcs_error_to_string(const enum HPCS_RetCode);
enum HPCS_RetCode hpcs_read_file(const char* const filename, struct HPCS_MeasuredData* mdata);
#endif

#ifdef __cplusplus
}
#endif
