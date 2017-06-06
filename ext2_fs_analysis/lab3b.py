import sys
import csv
import math 

# global variables
num_blocks = 0
num_inodes = 0
block_size = 0


def openFile():
	if len(sys.argv) != 2:
		error_message("incorrect num arguments", 1)
	else:
		try:
			file = open(sys.argv[1], 'r') 
		except IOError:
			error_message("file does not exist", 1)
		return file

def processSuper(line):
	global num_blocks
	global num_inodes
        global block_size
	num_blocks = int(line[1])
	num_inodes = int(line[2])
        block_size = int(line[3])

def processInode(line):
	for i in range(12, 24):
		block_no = int(line[i])
		inode_no = int(line[1])
		checkBlock(block_no, inode_no, i, 0)

def checkBlock(block_no, inode_no, logical_offset, indir_level):
	error_type = "FIXME"
	error_exists = False
	if indir_level == 1: 
		indir_str = " INDIRECT "
	elif indir_level == 2: 
		indir_str = " DOUBLE INDIRECT "
	elif indir_level == 3: 
		indir_str = " TRIPLE INDIRECT "
	else:
		indir_str = " "
		
	if block_no < 0 or block_no >= num_blocks:
		error_type = "INVALID"
		error_exists = True
        elif block_no >=0 and block_no <= reserved_limit():
                error_type = "RESERVED"
                error_exists = True
	if error_exists:
		output = error_type + indir_str + "BLOCK " + `block_no` + " IN INODE " + `inode_no` + " AT OFFSET " + `(logical_offset - 12)`
		print output

def reserved_limit():
        return ((1 if block_size == 1024 else 0) + 3 + math.ceil((num_inodes*128.0)/block_size))

def error_message(message, rc):
	print message
	sys.exit(rc)

def main():
	# get file and error check
	file = openFile()
	csvfile = csv.reader(file, delimiter=',')

	# parse through info
	for row in csvfile:
		if row[0] == "SUPERBLOCK":
			processSuper(row)
		if row[0] == "INODE":
			processInode(row)

if __name__ == "__main__":
	main()
	
