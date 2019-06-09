import sys
import os
import subprocess
import tempfile

PROGAM_PATH = os.path.realpath(__file__)
CASE_DIR_PATH = os.path.join(os.path.dirname(PROGAM_PATH), 'cases')
ARRAY_HEADER_PATH = os.path.join(os.path.dirname(PROGAM_PATH), '..', 'array.h')

SOURCE_TEMPLATE = '''
#include "{array_h_path}"

using namespace safearray;

int main() {{
    {case_source}
    return 0;
}}
'''

def get_case_source(case_name):
    path = os.path.join(CASE_DIR_PATH, case_name + '.cpp')
    with open(path) as f:
        return f.read()

def get_cases():
    for _, _ , filenames in os.walk(CASE_DIR_PATH):
        break
    return [fn.split('.')[0] for fn in filenames]

def run_test(case_name, cc_cmd, tempdir_path):
    # make program
    source = SOURCE_TEMPLATE.format(array_h_path=ARRAY_HEADER_PATH, \
        case_source=get_case_source(case_name))
    source_path = os.path.join(tempdir_path, case_name + '.cpp')
    with open(source_path, 'w') as f:
        f.write(source)

    # run compiler
    cmd = cc_cmd + ['-o', case_name, source_path]
    proc = subprocess.Popen(cmd, stderr=subprocess.PIPE)
    _, stderr = proc.communicate()
    if proc.returncode == 0:
        print("FAIL: {}: compiled sucessfully".format(case_name))
        return False
    if "static assertion failed" not in stderr:
        print("FAIL: {}: no static assertion failure".format(case_name))
        print(stderr)
        return False
    return True

def main():
    tempdir = tempfile.mkdtemp()
    cc_cmd = ['avr-g++', '-std=gnu++11']
    num_success = 0
    num_fail = 0
    for case_name in get_cases():
        if run_test(case_name, cc_cmd, tempdir):
            num_success += 1
        else:
            num_fail += 1
    
    print("\ntests: {}\tsuccesses: {}\tfailures: {}".\
        format(num_success + num_fail, num_success, num_fail))

if __name__ == '__main__':
    main()