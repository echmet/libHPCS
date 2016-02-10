#ifndef LIBHPCS_H
#define LIBHPCS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#define LIBHPCS_API __declspec(dllexport)
#define LIBHPCS_CC __cdecl
#else
#define LIBHPCS_API
#define LIBHPCS_CC
#endif

enum HPCS_FileType {
	HPCS_TYPE_CE_ANALOG,
	HPCS_TYPE_CE_CCD,
	HPCS_TYPE_CE_CURRENT,
	HPCS_TYPE_CE_DAD,
	HPCS_TYPE_CE_POWER,
	HPCS_TYPE_CE_PRESSURE,
	HPCS_TYPE_CE_TEMPERATURE,
	HPCS_TYPE_CE_VOLTAGE,
	HPCS_TYPE_UNKNOWN
};

enum HPCS_RetCode {
	HPCS_OK,
	HPCS_E_NULLPTR,
	HPCS_E_CANT_OPEN,
	HPCS_E_PARSE_ERROR,
	HPCS_E_UNKNOWN_TYPE,
	HPCS_E_INCOMPATIBLE_FILE,
	HPCS_E_NOTIMPL
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
	enum HPCS_FileType file_type;
	struct HPCS_TVPair* data;
	size_t data_count;
};

struct HPCS_MethodInfoBlock {
	char* name;
	char* value;
};

struct HPCS_MethodInfo {
	struct HPCS_MethodInfoBlock* blocks;
	size_t count;
};

LIBHPCS_API struct HPCS_MeasuredData* LIBHPCS_CC hpcs_alloc_mdata();
LIBHPCS_API struct HPCS_MethodInfo* LIBHPCS_CC hpcs_alloc_minfo();
LIBHPCS_API void LIBHPCS_CC hpcs_free_mdata(struct HPCS_MeasuredData* const mdata);
LIBHPCS_API void LIBHPCS_CC hpcs_free_minfo(struct HPCS_MethodInfo* const minfo);
LIBHPCS_API const char* LIBHPCS_CC hpcs_error_to_string(const enum HPCS_RetCode);
LIBHPCS_API enum HPCS_RetCode LIBHPCS_CC hpcs_read_mdata(const char* filename, struct HPCS_MeasuredData* mdata);
LIBHPCS_API enum HPCS_RetCode LIBHPCS_CC hpcs_read_mheader(const char* filename, struct HPCS_MeasuredData* mdata);
LIBHPCS_API enum HPCS_RetCode LIBHPCS_CC hpcs_read_minfo(const char* filename, struct HPCS_MethodInfo* minfo);

#ifdef __cplusplus
}
#endif

#endif /* LIBHPCS_H */
