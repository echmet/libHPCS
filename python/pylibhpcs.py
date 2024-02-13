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
    lib_name = './libHPCS.dll'
else:
    lib_name = './libHPCS.so'

libhpcs = CDLL(lib_name)


"""
`HPCS_FileType` describes the expected kind of data contained in the file.
Note that detection of `Power` and `Pressure` data might be unreliable.

 - 'HPCS_TYPE_CE_ANALOG': Analog signal of unspeficied type (0),
 - 'HPCS_TYPE_CE_CCD': Analog signal, usually from conductivity detector (1),
 - 'HPCS_TYPE_CE_CURRENT': Electric current in the CE system (2),
 - 'HPCS_TYPE_CE_DAD': UV/VIS detector signal trace (3),
 - 'HPCS_TYPE_CE_POWER': Electric power in the CE system (4),
 - 'HPCS_TYPE_CE_PRESSURE': Air pressure applied onto the CE system (5),
 - 'HPCS_TYPE_CE_TEMPERATURE': Temperature of the cassette (6),
 - 'HPCS_TYPE_CE_VOLTAGE': Electric voltage in the CE system (7),
 - 'HPCS_TYPE_UNKNOWN': Unknown type of data (8)
"""
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

"""
`HPCS_RetCode` represents the possible status codes returned from libHPCS

 - 'HPCS_OK': Operation completed successfully (0),
 - 'HPCS_E_NULLPTR': Null pointer was passed to a function (1),
 - 'HPCS_E_CANT_OPEN': File cannot be opened (2),
 - 'HPCS_E_PARSE_ERROR': Fail cannot be processed (3),
 - 'HPCS_E_UNKNOWN_TYPE': File contains unknown type of measurement (4),
 - 'HPCS_E_INCOMPATIBLE_FILE': File type is not compatible with the requested operation (5),
 - 'HPCS_E_NOTIMPL': Function is not implemented (6)
"""
HPCS_RetCode = {
    'HPCS_OK': 0,
    'HPCS_E_NULLPTR': 1,
    'HPCS_E_CANT_OPEN': 2,
    'HPCS_E_PARSE_ERROR': 3,
    'HPCS_E_UNKNOWN_TYPE': 4,
    'HPCS_E_INCOMPATIBLE_FILE': 5,
    'HPCS_E_NOTIMPL': 6
}

"""
`HPCS_Date` represents a timestamp returned by libHPCS

 - 'year': Year, as YYYY
 - 'month': Month, 1 = January, 12 = December, 0 = Unknown
 - 'day': Day, in 1 - 31 range, 0 = Unknown
 - 'hour': Hour, in 24h format
 - 'minute': Minute
 - 'second': Second
"""
class HPCS_Date(Structure):
    _fields_ = [("year", c_uint32),
                ("month", c_uint8),
                ("day", c_uint8),
                ("hour", c_uint8),
                ("minute", c_uint8),
                ("second", c_uint8)]

"""
`HPCS_Wavelength` describes a wavelength used by the DAD detector

 - 'wavelength': Detection wavelength, in nm
 - 'interval': Spectral interval, in nm

Both values set to zero indicate invalid or empty data
"""
class HPCS_Wavelength(Structure):
    _fields_ = [("wavelength", c_uint16),
                ("interval", c_uint16)]

"""
`HPCS_TVPair` is the basic datatype of signal traces

 - 'time': Time of sampling, in minutes
 - 'value': Recorded value, unit depends on the kind of the signal trace
"""
class HPCS_TVPair(Structure):
    _fields_ = [("time", c_double),
                ("value", c_double)]

"""
`HPCS_MeasuredData` contains information about a measurement,
containing both the signal trace and the associated metadata.

 - 'file_description': Internal file type description. For most LC or CE data this will be "LC DATA FILE"
 - 'sample_info': Information about the sample, as entered by the operator
 - 'operator_name': Name of the operator who conducted the measurement
 - 'date': Date and time when the measurement was conducted
 - 'method_name': Identifies the method file used for the measurement
 - 'cs_ver': Version of the used ChemStation software
 - 'cs_rev': Revision of the used ChemStation software
 - 'sampling_rate': Sampling rate of the detector, in Hz. Value is -1 if the sampling rate is unknown
 - 'y_units': Unit of the signal trace
 - 'dad_wavelength_msr': Detection wavelength of the DAD detector. Applicable only for DAD file types. See `HPCS_Wavelength`
 - 'dad_wavelength_ref': Reference wavelength, usually used for noise compensation. Applicable only for DAD file types. See `HPCS_Wavelength`
 - 'file_type': Type of the file. See `HPCS_FileType` for possible values
 - 'data': Array of `HPCS_TVPair`s that represent the signal trace
 - 'data_count': Number of `HPCS_TVPair`s in `data`
"""
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

"""
`HPCS_MethodInfoBlock` is a key-value pair of information
about a measurement method. Method information can be obtained
from .MTH files.

 - 'name': Name of the parameter
 - 'value': Value of the parameter
"""
class HPCS_MethodInfoBlock(Structure):
    _fields_ = [("name", c_char_p),
                ("value", c_char_p)]

"""
`HPCS_MethodInfo` is contains array of `HPCS_MethodInfoBlock`s
and the length of that array.

 - 'blocks': Array of `HPCS_MethodInfoBlock`s
 - 'count': Length of the array
"""
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
