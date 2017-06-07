#!/usr/bin/python
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

class inode_attr:
	def __init__(self):
		self.ref_count = 0
		self.inodePtrs = []
		self.mode = -1
	def searchInodes(target):
		for i in range(0, len(self.inodePtrs)):
			if self.inodePtrs[i] == target:
				return True
		return False
	def incr_ref(self):
		self.ref_count += 1

# global variables
num_blocks = 0
num_inodes = 0
block_size = 0
free_list = [1]
inode_free_list = [1]
block_attributes = []
inode_attributes = []
first_non_res = 0

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
	global first_non_res
	global inode_free_list
	num_blocks = int(line[1])
	num_inodes = int(line[2])
	block_size = int(line[3])
	free_list *= num_blocks
	inode_free_list *= num_inodes
	first_non_res = int(line[7])
	for i in range(0, num_blocks):
		temp = attribute()
		block_attributes.append(temp)
	for i in range(0, num_inodes):
		temp = inode_attr()
		inode_attributes.append(temp)

def processInode(line):
	inode_no = int(line[1])
	mode = int(line[3])
	link_count = int(line[6])
	for i in range(12, 27):
		block_no = int(line[i])
		if block_no != 0 and checkBlock(block_no, inode_no, i - 12, 0):	
			# fill in data block attribute
			block_attributes[block_no].append_attr(inode_no, i - 12, 0)

	if inode_no >= 1 and inode_no <= num_inodes:
		if (inode_free_list[inode_no - 1] == 0 and link_count > 0) or \
			(inode_free_list[inode_no -1] == 0 and file_exists(mode)):
			processAllocInode(inode_no)
		
		elif (inode_free_list[inode_no - 1] == 1 and link_count <= 0) or \
			(inode_free_list[inode_no -1] == 1 and file_exists(mode) == False):
			processUnallocInode(inode_no)

		else: # fill in inode attributes table
			inode_attributes[inode_no - 1].mode = mode
	else:
		output = "INVALID INODE " + `inode_no`
		print output

def processAllocInode(inode):
	output = "ALLOCATED INODE " + `inode` + " ON FREELIST"
	print output

def processUnallocInode(inode):
	output = "UNALLOCATED INODE " + `inode` + " NOT ON FREELIST"
	print output

def file_exists(mode):
	if mode != 0:
		return True
	else: 
		return False

def processIndirect(line):
	block_no = int(line[5])
	inode_no = int(line[1])
	offset = int(line[3])
	level = int(line[2])
	indir_block_no = int(line[4])
	if block_no != 0 and checkBlock(block_no, inode_no, offset, level):
		block_attributes[block_no].append_attr(inode_no, offset, level)
	#if indir_block_no != 0 and checkBlock(indir_block_no, inode_no, level):
		#block_attributes[indir_block_no].append_attr(inode_no, offset, level)

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

def processDirent(line):
	parent_inode = int(line[1])
	inode_no = int(line[3])
	name = line[6]
	if inodeIsValid(inode_no, parent_inode, name):
		inode_attributes[inode_no - 1].incr_ref()
		inode_attributes[parent_inode - 1].inodePtrs.append(inode_no)

def inodeIsValid(inode, parent, name):
	if (inode >= first_non_res and inode <= num_inodes) or (inode == 2):
		ret_value = True
	else:
		output = "DIRECTORY INODE " + `parent` + " NAME " + name + " INVALID INODE " + `inode` 
		print output
		ret_value = False
		return ret_value

	if inode_attributes[inode - 1].mode != 0:
		ret_value = True
	else:
		output = "DIRECTORY INODE " + `parent` + " NAME " + name + " UNALLOCATED INODE " + `inode` 
		print output
		ret_value = False
		return ret_value

	return ret_value

def processDot(line):
	parent_inode = int(line[1])
	inode = int(line[3])
	name = line[6]
	if parent_inode != inode:
		output = "DIRECTORY INODE " + `parent_inode` + " NAME " + name + " LINK TO INODE " + `inode` + \
		" SHOULD BE " + `parent_inode`
		print output

def processDotDot(line):
	parent_inode = int(line[1])
	inode = int(line[3])
	name = line[6]
	if inode_attributes[inode - 1].searchInodes(parent_inode) == False:
		output = "DIRECTORY INODE " + `parent_inode` + " NAME " + name + " LINK TO INODE " + `inode` + \
		" SHOULD BE " + `findActualParentOf(parent_inode)`
		print output

def findActualParentOf(inode):
	for i in range(0, len(inode_attributes)):
		for j in range(0, len(inode_attributes[i].inodePtrs)):
			if inode_attributes[i].inodePtrs[j] == inode:
				return inode_attributes[i].inodePtrs[j]
	return -1

def checkInodeLinks(line):
	given_link_count = int(line[6])
	inode_no = int(line[1])
	real_link_count = inode_attributes[inode_no - 1].ref_count
	if given_link_count != real_link_count:
		output = "INODE " + `inode_no` + " HAS " + `given_link_count` + \
		" LINKS BUT LINKCOUNT IS " + `real_link_count`
		print output

def reserved_limit():
    return int(((1 if block_size == 1024 else 0) + 3 + math.ceil((num_inodes*128.0)/block_size)))

def processBFREE(line):
	free_list[int(line[1])] = 0

def processIFREE(line):
	inode_free_list[int(line[1]) - 1] = 0

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
		if row[0] == "IFREE":
			processIFREE(row)
		if row[0] == "INDIRECT":
			processIndirect(row)
	
	# process block duplicates, unreferenced, allocated
	for i in range(reserved_limit() + 1, num_blocks):
		if block_attributes[i].ref_count > 1: # more than one reference implies duplicate
			processDuplicate(i)
		if block_attributes[i].ref_count >= 1 and free_list[i] == 0:
			processAllocBlock(i)
		if block_attributes[i].ref_count == 0 and free_list[i] == 1:
			processUnrefBlock(i)

	# process directory entries, and update the reference count per inode
	for row in csvfile:
		if row[0] == "DIRENT":
			processDirent(row)

	# directory consistency audit
	for row in csvfile:
		if row[0] == "INODE":
			checkInodeLinks(row)

	# checking . and .. consistency
	for row in csvfile:
		if row[0] == "DIRENT":
			if row[6] == "\'.\'":
				processDot(row)
			if row[6] == "\'..\'":
				processDotDot(row)
	
	"""
	for i in range(0,100):
		output = "BLOCK: " + `i`
		print output
		print block_attributes[i].inodes
		print block_attributes[i].offsets
		print block_attributes[i].level
		print block_attributes[i].ref_count

		output = "FREELIST AT: " + `i` + " " + `free_list[i]` + "\n"
		print output

	print inode_free_list
	"""


if __name__ == "__main__":
	main()
	
