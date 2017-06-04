import sys

def openFile():
	if len(sys.argv) != 2:
		error_message("incorrect num arguments", 1)
	else:
		try:
			file = open(sys.argv[1], 'r')
		except IOError:
			error_message("file does not exist", 1)
		return file

def error_message(message, rc):
	print message
	sys.exit(rc)

if __name__ == "__main__":
	# get file and error check
	file = openFile()
