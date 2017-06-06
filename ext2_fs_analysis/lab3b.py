import sys
import csv
import math 

class attribute:
	def __init__(self):
		self.inodes = []
		self.offsets = []
		self.level = []
		self.ref_count = 0
	
	def append_attr(self, inode, offset, level):
		self.inodes.append(inode)
		self.offsets.append(offset)
		self.level.append(level)
		self.ref_count += 1

# global variables
num_blocks = 0
num_inodes = 0
block_size = 0
free_list = [1]
block_attributes = []

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
	global free_list
	global block_attributes
	num_blocks = int(line[1])
	num_inodes = int(line[2])
	block_size = int(line[3])
	free_list *= num_blocks
	for i in range(0, num_blocks):
		temp = attribute()
		block_attributes.append(temp)

def processInode(line):
	for i in range(12, 24):
		block_no = int(line[i])
		inode_no = int(line[1])
		if block_no != 0 and checkBlock(block_no, inode_no, i - 12, 0):	
			block_attributes[block_no].append_attr(inode_no, i - 12, 0)

def processIndirect(line):
	block_no = int(line[5])
	inode_no = int(line[1])
	offset = int(line[3])
	level = int(line[2])
	if block_no != 0 and checkBlock(block_no, inode_no, offset, level):
		block_attributes[block_no].append_attr(inode_no, offset, level)
		# ADD BLOCK NO FOR INDRECT REFERENCE

def processDuplicate(block_no):
	num_refs = block_attributes[block_no].ref_count

	for i in range(0, num_refs):
		indir_level = block_attributes[block_no].level[i]
		inode_num = block_attributes[block_no].inodes[i]
		offset = block_attributes[block_no].offsets[i]

		if indir_level == 1: 
			indir_str = " INDIRECT "
		elif indir_level == 2: 
			indir_str = " DOUBLE INDIRECT "
		elif indir_level == 3: 
			indir_str = " TRIPPLE INDIRECT "
		else:
			indir_str = " "

		output = "DUPLICATE" + indir_str + "BLOCK " + `block_no` + \
		" IN INODE " + `inode_num` + " AT OFFSET " + `offset`
		print output

def processAllocBlock(block_no):
	output = "ALLOCATED BLOCK " + `block_no` + " ON FREELIST"
	print output

def processUnrefBlock(block_no):
	output = "UNREFERENCED BLOCK " + `block_no`
	print output
		

def checkBlock(block_no, inode_no, logical_offset, indir_level):
	error_type = "FIXME"
	error_exists = False
	if indir_level == 1: 
		indir_str = " INDIRECT "
	elif indir_level == 2: 
		indir_str = " DOUBLE INDIRECT "
	elif indir_level == 3: 
		indir_str = " TRIPPLE INDIRECT "
	else:
		indir_str = " "
		
	if block_no < 0 or block_no >= num_blocks:
		error_type = "INVALID"
		error_exists = True

	elif block_no >= 0 and block_no <= reserved_limit(): # FIX: verify the limit
		error_type = "RESERVED"
		error_exists = True

	if error_exists:
		output = error_type + indir_str + "BLOCK " + `block_no` + " IN INODE " + `inode_no` + " AT OFFSET " + `logical_offset`
		print output
	
	return True if error_exists == False else False

def reserved_limit():
    return int(((1 if block_size == 1024 else 0) + 3 + math.ceil((num_inodes*128.0)/block_size)))


def processBFREE(line):
	free_list[int(line[1])] = 0

def error_message(message, rc):
	print message
	sys.exit(rc)

def main():
	# get file and error check
	file = openFile()
	csvfile = csv.reader(file, delimiter=',')

	# parse through info and build up data structures
	for row in csvfile:
		if row[0] == "SUPERBLOCK":
			processSuper(row)
		if row[0] == "INODE":
			processInode(row)
		if row[0] == "BFREE":
			processBFREE(row)
		if row[0] == "INDIRECT":
			processIndirect(row)
	
	# process block duplicates, unreferenced, allocated
	for i in range(reserved_limit(), num_blocks):
		if block_attributes[i].ref_count > 1: # more than one reference implies duplicate
			processDuplicate(i)
		if block_attributes[i].ref_count >= 1 and free_list[i] == 0:
			processAllocBlock(i)
		if block_attributes[i].ref_count == 0 and free_list[i] == 1:
			processUnrefBlock(i)
	
	for i in range(0,100):
		output = "BLOCK: " + `i`
		print output
		print block_attributes[i].inodes
		print block_attributes[i].offsets
		print block_attributes[i].level
		print block_attributes[i].ref_count

		output = "FREELIST AT: " + `i` + " " + `free_list[i]` + "\n"
		print output


if __name__ == "__main__":
	main()
	
