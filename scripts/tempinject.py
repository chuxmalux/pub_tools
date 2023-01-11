#!/bin/python3 
"""
TEMPLATE INJECT --- inject template to a file (e.g. notes)
"""

import os
import sys

# take template file
def inject(template, target):

    c = int(input('How many injects?: '))
    if (c <= 0):
        print ('# must be > 0')
        quit()

    content = template.read()
    for i in range(c):
        target.write(content)
        target.write('\n')

# ask # of injects
def argv_check():
    if len(sys.argv) != 3:
        print('/// ERROR - missing args///')
        print('e.g. --- $python3 tempinject.py [template_file] [target_file]')
        quit()

def files_exist():
    if (os.path.exists(sys.argv[1]) == False):
        print("%s is not a valid file or path"%sys.argv[1])
        quit()
    elif (os.path.exists(sys.argv[2]) == False):
        print("%s is not a valid file or path"%sys.argv[2])
        quit()
    else:
        pass

def main():
    argv_check()
    files_exist()

    try:
        with open(sys.argv[1], 'r') as f01, open(sys.argv[2], 'a') as f02:
            inject(f01,f02)
    except IOError as e:
        print('Operation failed: %s '%e.strerror)

if __name__ == "__main__":
    main()
