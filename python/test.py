import sys
from pathlib import Path
import pylibhpcs  # Make sure this is correctly pointing to your wrapper module

def main():
    if len(sys.argv) != 3:
        print('Usage: test.py MODE FILE')
        sys.exit(1)

    mode = sys.argv[1]
    file_path = Path(sys.argv[2])

    if mode == 'd':
        measurement = pylibhpcs.read_mdata(file_path)
        print(measurement)
    elif mode == 'i':
        info = pylibhpcs.read_minfo(file_path)
        print(info)
    elif mode == 'h':
        header = pylibhpcs.read_mheader(file_path)
        print(header)
    else:
        print('Invalid mode')

if __name__ == "__main__":
    main()
