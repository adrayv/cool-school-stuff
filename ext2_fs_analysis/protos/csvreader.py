import sys
import csv
def openFile():
	if len(sys.argv) != 2:
		error_message("incorrect num arguments", 1)
	else:
		try:
			file = open(sys.argv[1], 'r') 
		except IOError:
			error_message("file does not exist", 1)
		return file

def main():
	# get file and error check
	file = openFile()
	csvfile = csv.reader(file, delimiter=',')

	# parse through info and build up data structures
	for row in csvfile:
		print row
	
	file.seek(0)
	for row in csvfile:
		print row

if __name__ == "__main__":
	main()
	
