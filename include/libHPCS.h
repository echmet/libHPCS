#ifndef LIBHPCS_H
#define LIBHPCS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
	#ifdef _HPCS_BUILD_DLL
		#define LIBHPCS_API __declspec(dllexport)
	#else
		#define LIBHPCS_API __declspec(dllimport)
	#endif /* _HPCS_BUILD_DLL */
#define LIBHPCS_CC __cdecl
#else
	#ifdef _HPCS_BUILD_DLL
		#define LIBHPCS_API __attribute__ ((visibility ("default")))
	#else
		#define LIBHPCS_API
	#endif /* _HPCS_BUILD_DLL */
	#define LIBHPCS_CC
#endif /* _WIN32 */

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

/**
 * Allocates \ref HPCS_MeasuredData object.
 *
 * The allocated object must be freed by calling \ref hpcs_free_mdata().
 *
 * \return Pointer to the allocated \ref HPCS_MeasuredData object.
 */
LIBHPCS_API struct HPCS_MeasuredData* LIBHPCS_CC hpcs_alloc_mdata();

/**
 * Allocates \ref HPCS_MethodInfo object.
 *
 * The allocated object must be freed by calling \ref hpcs_free_minfo().
 *
 * \return Pointer to the allocated \ref HPCS_MeasuredData object.
 */
LIBHPCS_API struct HPCS_MethodInfo* LIBHPCS_CC hpcs_alloc_minfo();

/**
 * Frees \ref HPCS_MeasuredData object.
 *
 * \param mdata Pointer to object to free.
 */
LIBHPCS_API void LIBHPCS_CC hpcs_free_mdata(struct HPCS_MeasuredData* const mdata);

/**
 * Frees \rec HPCS_MethodInfo object.
 *
 * \param minfo Pointer to object to free.
 */
LIBHPCS_API void LIBHPCS_CC hpcs_free_minfo(struct HPCS_MethodInfo* const minfo);

/**
 * Translates \ref HPCS_RetCode to a string with human-readable error message.
 *
 * \param err \ref HPCS_RetCode to translate.
 * \return String with human-readable error message.
 */
LIBHPCS_API const char* LIBHPCS_CC hpcs_error_to_string(const enum HPCS_RetCode err);

/**
 * Reads content of a HP/Agilent ChemStation data file.
 *
 * \param filename Path to the file to read.
 * \param mdata Pointer to \ref HPCS_MeasuredData object to be filled out by this function.
 * \return \ref HPCS_RetCode to indicate if the operation succeeded.
 */
LIBHPCS_API enum HPCS_RetCode LIBHPCS_CC hpcs_read_mdata(const char* filename, struct HPCS_MeasuredData* mdata);

/**
 * Reads content of a HP/Agilent ChemStation data file.
 * Unlike \ref hpcs_read_mdata() this function reads only the header (metadata)
 * associated with the measurement but not the signal trace.
 *
 * \param filename Path to the file to read.
 * \param mdata Pointer to \ref HPCS_MeasuredData object to be filled out by this function.
 * \return \ref HPCS_RetCode to indicate if the operation succeeded.
 */
LIBHPCS_API enum HPCS_RetCode LIBHPCS_CC hpcs_read_mheader(const char* filename, struct HPCS_MeasuredData* mdata);

/**
 * Reads the method information block of a HP/Agilent ChemStation data file.
 *
 * \param filename Path to the file to read.
 * \param mdata Pointer to \ref HPCS_MethodInfo object to be filled out by this function.
 * \return \ref HPCS_RetCode to indicate if the operation succeeded.
 */
LIBHPCS_API enum HPCS_RetCode LIBHPCS_CC hpcs_read_minfo(const char* filename, struct HPCS_MethodInfo* minfo);

#ifdef __cplusplus
}
#endif

#endif /* LIBHPCS_H */
