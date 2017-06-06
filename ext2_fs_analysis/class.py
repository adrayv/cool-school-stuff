class attributes:
	def __init__(self):
		self.status = "none"

array = []
for i in range(0,10):
	temp = attributes()
	array.append(temp)
array[4].status = "changed"
for i in range(0,10):
	print array[i].status
