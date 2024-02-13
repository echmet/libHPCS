from dataclasses import dataclass
import platform
from ctypes import (
    c_uint8, c_uint16, c_uint32, c_int,
    c_char_p, c_size_t,
    c_double,
    c_void_p,
    sizeof,
    Structure, POINTER,
    CDLL
)
from enum import IntEnum
from typing import Dict, List, Optional, Tuple

# Load the library based on the operating system
if platform.system() == 'Windows':
    if sizeof(c_void_p) == 8:
        lib_name = './win64/libHPCS.dll'
    else:
        lib_name = './win32/libHPCS.dll'
else:
    lib_name = './libHPCS.so'

libhpcs = CDLL(lib_name)


class HPCSError(Exception):
    def __init__(self, error_code):
        super(Exception, self).__init__(self)
        self.error_code = error_code
        self.error_description = error_to_string(error_code)

    def __str__(self):
        return f'HPCSError: {self.error_description} ({self.error_code})'


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
class HPCS_FileType(IntEnum):
    HPCS_TYPE_CE_ANALOG = 0
    HPCS_TYPE_CE_CCD = 1
    HPCS_TYPE_CE_CURRENT = 2
    HPCS_TYPE_CE_DAD = 3
    HPCS_TYPE_CE_POWER = 4
    HPCS_TYPE_CE_PRESSURE = 5
    HPCS_TYPE_CE_TEMPERATURE = 6
    HPCS_TYPE_CE_VOLTAGE = 7
    HPCS_TYPE_UNKNOWN = 8

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
class HPCS_RetCode(IntEnum):
    HPCS_OK = 0
    HPCS_E_NULLPTR = 1
    HPCS_E_CANT_OPEN = 2
    HPCS_E_PARSE_ERROR = 3
    HPCS_E_UNKNOWN_TYPE = 4
    HPCS_E_INCOMPATIBLE_FILE = 5
    HPCS_E_NOTIMPL = 6

"""
`HPCS_Date` represents a timestamp returned by libHPCS

 - 'year': Year, as YYYY
 - 'month': Month, 1 = January, 12 = December, 0 = Unknown
 - 'day': Day, in 1 - 31 range, 0 = Unknown
 - 'hour': Hour, in 24h format
 - 'minute': Minute
 - 'second': Second
"""
class _HPCS_Date(Structure):
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
class _HPCS_Wavelength(Structure):
    _fields_ = [("wavelength", c_uint16),
                ("interval", c_uint16)]

"""
`HPCS_TVPair` is the basic datatype of signal traces

 - 'time': Time of sampling, in minutes
 - 'value': Recorded value, unit depends on the kind of the signal trace
"""
class _HPCS_TVPair(Structure):
    _fields_ = [("time", c_double),
                ("value", c_double)]

"""
`_HPCS_MeasuredData` contains information about a measurement,
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
 - 'dad_wavelength_msr': Detection wavelength of the DAD detector. Applicable only for DAD file types. See `_HPCS_Wavelength`
 - 'dad_wavelength_ref': Reference wavelength, usually used for noise compensation. Applicable only for DAD file types. See `_HPCS_Wavelength`
 - 'file_type': Type of the file. See `HPCS_FileType` for possible values
 - 'data': Array of `_HPCS_TVPair`s that represent the signal trace
 - 'data_count': Number of `_HPCS_TVPair`s in `data`
"""
class _HPCS_MeasuredData(Structure):
    _fields_ = [("file_description", c_char_p),
                ("sample_info", c_char_p),
                ("operator_name", c_char_p),
                ("date", _HPCS_Date),
                ("method_name", c_char_p),
                ("cs_ver", c_char_p),
                ("cs_rev", c_char_p),
                ("y_units", c_char_p),
                ("sampling_rate", c_double),
                ("dad_wavelength_msr", _HPCS_Wavelength),
                ("dad_wavelength_ref", _HPCS_Wavelength),
                ("file_type", c_int),  # Use c_int for enum
                ("data", POINTER(_HPCS_TVPair)),
                ("data_count", c_size_t)]

"""
`_HPCS_MethodInfoBlock` is a key-value pair of information
about a measurement method. Method information can be obtained
from .MTH files.

 - 'name': Name of the parameter
 - 'value': Value of the parameter
"""
class _HPCS_MethodInfoBlock(Structure):
    _fields_ = [("name", c_char_p),
                ("value", c_char_p)]

"""
`HPCS_MethodInfo` is contains array of `HPCS_MethodInfoBlock`s
and the length of that array.

 - 'blocks': Array of `HPCS_MethodInfoBlock`s
 - 'count': Length of the array
"""
class _HPCS_MethodInfo(Structure):
    _fields_ = [("blocks", POINTER(_HPCS_MethodInfoBlock)),
                ("count", c_size_t)]


@dataclass(frozen=True)
class HPCS_Date:
    year: int
    month: int
    day: int
    hour: int
    minute: int
    second: int


@dataclass(frozen=True)
class HPCS_Wavelength:
    wavelength: int
    interval: Optional[int]


@dataclass(frozen=True)
class HPCS_MeasuredData:
    file_description: str
    sample_info: str
    operator_name: str
    date: HPCS_Date
    method_name: str
    cs_ver: str
    cs_rev: str
    y_units: str
    sampling_rate: Optional[float]
    dad_wavelength_msr: Optional[HPCS_Wavelength]
    dad_wavelength_ref: Optional[HPCS_Wavelength]
    file_type: HPCS_FileType
    data: List[Tuple[float, float]]


@dataclass(frozen=True)
class HPCS_MethodInfo:
    information: Dict[str, str]


def wrap_function(lib, funcname, restype, argtypes):
    func = getattr(lib, funcname)
    func.restype = restype
    func.argtypes = argtypes
    return func

# Internal C functions. It is inadvisable to call these directly
_error_to_string = wrap_function(libhpcs, "hpcs_error_to_string", c_char_p, [c_int])
_read_mdata = wrap_function(libhpcs, "hpcs_read_mdata", c_int, [c_char_p, POINTER(_HPCS_MeasuredData)])
_read_mheader = wrap_function(libhpcs, "hpcs_read_mheader", c_int, [c_char_p, POINTER(_HPCS_MeasuredData)])
_read_minfo = wrap_function(libhpcs, "hpcs_read_minfo", c_int, [c_char_p, POINTER(_HPCS_MethodInfo)])
_alloc_mdata = wrap_function(libhpcs, "hpcs_alloc_mdata", POINTER(_HPCS_MeasuredData), [])
_free_mdata = wrap_function(libhpcs, "hpcs_free_mdata", None, [POINTER(_HPCS_MeasuredData)])
_alloc_minfo = wrap_function(libhpcs, "hpcs_alloc_minfo", POINTER(_HPCS_MethodInfo), [])
_free_minfo = wrap_function(libhpcs, "hpcs_free_minfo", None, [POINTER(_HPCS_MethodInfo)])

def _make_date(raw_date):
    return HPCS_Date(
        raw_date.year,
        raw_date.month,
        raw_date.day,
        raw_date.hour,
        raw_date.minute,
        raw_date.second
    )


def _make_wavelength(raw_wavelength):
    if raw_wavelength.wavelength == 0:
        return None

    wavelength = raw_wavelength.wavelength
    interval = raw_wavelength.interval if raw_wavelength.interval != 0 else None

    return HPCS_Wavelength(wavelength, interval)


def _make_data(raw_data, raw_data_count):
    data = []
    for idx in range(0, raw_data_count):
        v = raw_data[idx]
        data.append((v.time, v.value))
    return data


def _make_hpcs_measured_data(ptr):
    detection_wavelength = None
    reference_wavelength = None
    if ptr.contents.file_type == HPCS_FileType.HPCS_TYPE_CE_DAD:
        detection_wavelength = _make_wavelength(ptr.contents.dad_wavelength_msr)
        reference_wavelength = _make_wavelength(ptr.contents.dad_wavelength_ref)

    data = _make_data(ptr.contents.data, ptr.contents.data_count)

    return HPCS_MeasuredData(
        ptr.contents.file_description.decode('utf-8'),
        ptr.contents.sample_info.decode('utf-8'),
        ptr.contents.operator_name.decode('utf-8'),
        _make_date(ptr.contents.date),
        ptr.contents.method_name.decode('utf-8'),
        ptr.contents.cs_ver.decode('utf-8'),
        ptr.contents.cs_rev.decode('utf-8'),
        ptr.contents.y_units.decode('utf-8'),
        None if ptr.contents.sampling_rate == -1.0 else ptr.contents.sampling_rate,
        detection_wavelength,
        reference_wavelength,
        ptr.contents.file_type,
        data
    )


def _make_hpcs_method_info(ptr):
    information = {}

    for idx in range(0, ptr.contents.count):
        kv = ptr.contents.blocks[idx]
        information[kv.name.decode('utf-8')] = kv.value.decode('utf-8')

    return HPCS_MethodInfo(information)


# Error to string translation
def error_to_string(err):
    return _error_to_string(err).decode('utf-8')


# Reading measured data and method information
def read_mdata(file_path):
    ptr = _alloc_mdata()
    ret =  _read_mdata(str(file_path).encode('utf-8'), ptr)
    if ret != HPCS_RetCode.HPCS_OK:
        _free_mdata(ptr)
        raise HPCSError(ret)
    else:
        data = _make_hpcs_measured_data(ptr)
        _free_mdata(ptr)
        return data


def read_mheader(file_path):
    ptr = _alloc_mdata()
    ret =  _read_mheader(str(file_path).encode('utf-8'), ptr)
    if ret != HPCS_RetCode.HPCS_OK:
        _free_mdata(ptr)
        raise HPCSError(ret)
    else:
        data = _make_hpcs_measured_data(ptr)
        _free_mdata(ptr)
        return data


def read_minfo(file_path):
    ptr = _alloc_minfo()
    ret =  _read_minfo(str(file_path).encode('utf-8'), ptr)
    if ret != HPCS_RetCode.HPCS_OK:
        _free_minfo(ptr)
        raise HPCSError(ret)
    else:
        data = _make_hpcs_method_info(ptr)
        _free_minfo(ptr)
        return data
