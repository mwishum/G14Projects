#!/usr/bin/python

file = open("test", 'w')
i = 0
for num in range(0, 299):
  line = "%d" % (i)
  for j in range(len(line), 252):
    line = line + "="
  file.write(line)
  file.write("\n")
  i = i + 1
file.close()
