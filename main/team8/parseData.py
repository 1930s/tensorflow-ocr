import sys
import math

def file_len(fname):
  with open(fname) as f:
    for i, l in enumerate(f):
      pass
    return i+1

def parseFile(filename):
  file = open(filename, "r")
  fileLen = file_len(filename)

  tupleArray = [[0]*28]*fileLen
  lineCounter = 0
  for line in file:
    #print line + "\n"
    start = 0 #reset start of line range
    for digit in range(len(line)):
      if(line[digit] == " "): #look for a divider
        tupleArray[lineCounter][int(floor(digit/5))] = line[start:digit] #insert new value
        start = digit #set previous line range to previous divider
    lineCounter += 1 #we are on the next line for reading
    #tupleArray = line
    #print(tupleArray)

def main():
  #print(sys.argv[1])
  if (len(sys.argv) < 2 or len(sys.argv) > 2):
    print("python parseData.py <file_destination>\n")
    return

  parseFile(sys.argv[1])


main()
