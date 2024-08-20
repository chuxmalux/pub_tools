
#!/bin/python3
import re

def check(encoded):
    # len check
    if len(encoded) != 24:
        print ('Invalid length (!24 bytes)')
        exit()
    # char check
    if not re.match("^[a-fA-F0-9]+$", encoded):
        print("String contains invalid characters")
        exit()

print("### SIDecoder (⌐■_■) ###\nexample encoded string: 2E43AC40C085385D07E53B2B\n")
SID = input('Enter your encoded machine SID: ')
check(SID)

# divide SID into chunks of 2 characters
chunks = [SID[i:i+2] for i in range(0, len(SID), 2)]

# divide chunks into 3 sections 
sections = [chunks[:4], chunks[4:8], chunks[8:12]]

# reverse order of each section 
for section in sections:
    section.reverse()

# convert each section to decimal
for i in range(len(sections)):
    sections[i] = int(''.join(sections[i]), 16)

# print machine SID
print(f'Machine SID: S-1-5-21-{sections[0]}-{sections[1]}-{sections[2]}')



