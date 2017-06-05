import sys
import csv

# global variables
num_blocks = 0
num_inodes = 0

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
	num_blocks = int(line[1])
	num_inodes = int(line[2])

def processInode(line):
	for i in range(12, 24):
		block_no = int(line[i])
		inode_no = int(line[1])
		if block_no < 0 or block_no >= num_blocks:
			output = "INVALID BLOCK " + `block_no` + " IN INODE " + `inode_no` + " AT OFFSET " + `(i - 12)`
			print output

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
	
