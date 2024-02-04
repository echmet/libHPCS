import platform
from ctypes import (
    c_uint8, c_uint16, c_uint32, c_int,
    c_char_p, c_size_t,
    c_double,
    Structure, POINTER,
    CDLL
)

# Load the library based on the operating system
if platform.system() == 'Windows':
    lib_name = 'HPCS.dll'
else:
    lib_name = './libHPCS.so'

libhpcs = CDLL(lib_name)

# Enum definitions
HPCS_FileType = {
    'HPCS_TYPE_CE_ANALOG': 0,
    'HPCS_TYPE_CE_CCD': 1,
    'HPCS_TYPE_CE_CURRENT': 2,
    'HPCS_TYPE_CE_DAD': 3,
    'HPCS_TYPE_CE_POWER': 4,
    'HPCS_TYPE_CE_PRESSURE': 5,
    'HPCS_TYPE_CE_TEMPERATURE': 6,
    'HPCS_TYPE_CE_VOLTAGE': 7,
    'HPCS_TYPE_UNKNOWN': 8
}

HPCS_RetCode = {
    'HPCS_OK': 0,
    'HPCS_E_NULLPTR': 1,
    'HPCS_E_CANT_OPEN': 2,
    'HPCS_E_PARSE_ERROR': 3,
    'HPCS_E_UNKNOWN_TYPE': 4,
    'HPCS_E_INCOMPATIBLE_FILE': 5,
    'HPCS_E_NOTIMPL': 6
}

# Structure definitions
class HPCS_Date(Structure):
    _fields_ = [("year", c_uint32),
                ("month", c_uint8),
                ("day", c_uint8),
                ("hour", c_uint8),
                ("minute", c_uint8),
                ("second", c_uint8)]

class HPCS_Wavelength(Structure):
    _fields_ = [("wavelength", c_uint16),
                ("interval", c_uint16)]

class HPCS_TVPair(Structure):
    _fields_ = [("time", c_double),
                ("value", c_double)]

class HPCS_MeasuredData(Structure):
    _fields_ = [("file_description", c_char_p),
                ("sample_info", c_char_p),
                ("operator_name", c_char_p),
                ("date", HPCS_Date),
                ("method_name", c_char_p),
                ("cs_ver", c_char_p),
                ("cs_rev", c_char_p),
                ("y_units", c_char_p),
                ("sampling_rate", c_double),
                ("dad_wavelength_msr", HPCS_Wavelength),
                ("dad_wavelength_ref", HPCS_Wavelength),
                ("file_type", c_int),  # Use c_int for enum
                ("data", POINTER(HPCS_TVPair)),
                ("data_count", c_size_t)]

class HPCS_MethodInfoBlock(Structure):
    _fields_ = [("name", c_char_p),
                ("value", c_char_p)]

class HPCS_MethodInfo(Structure):
    _fields_ = [("blocks", POINTER(HPCS_MethodInfoBlock)),
                ("count", c_size_t)]

# Function wrappers
def wrap_function(lib, funcname, restype, argtypes):
    func = getattr(lib, funcname)
    func.restype = restype
    func.argtypes = argtypes
    return func

# Allocate and free measured data
alloc_mdata = wrap_function(libhpcs, "hpcs_alloc_mdata", POINTER(HPCS_MeasuredData), [])
free_mdata = wrap_function(libhpcs, "hpcs_free_mdata", None, [POINTER(HPCS_MeasuredData)])

# Allocate and free method information
alloc_minfo = wrap_function(libhpcs, "hpcs_alloc_minfo", POINTER(HPCS_MethodInfo), [])
free_minfo = wrap_function(libhpcs, "hpcs_free_minfo", None, [POINTER(HPCS_MethodInfo)])

# Error to string translation
error_to_string = wrap_function(libhpcs, "hpcs_error_to_string", c_char_p, [c_int])

# Reading measured data and method information
read_mdata = wrap_function(libhpcs, "hpcs_read_mdata", c_int, [c_char_p, POINTER(HPCS_MeasuredData)])
read_mheader = wrap_function(libhpcs, "hpcs_read_mheader", c_int, [c_char_p, POINTER(HPCS_MeasuredData)])
read_minfo = wrap_function(libhpcs, "hpcs_read_minfo", c_int, [c_char_p, POINTER(HPCS_MethodInfo)])
