import sys
import math

def file_len(fname):
  with open(fname) as f:
    for i, l in enumerate(f):
      pass
    return i+1

def parseFile(filename):
  file = open(filename, "r")
  fileLen = file_len(filename)  #get length of the file
  lineLen = len(file.readline()) #get length of each line
  valDivisor = 6
  glyphLen = (lineLen/valDivisor)+1 #get glyph value length +1 for all 28 values
  #tupleArray = [[0]*glyphLen]*fileLen #set dimensions of array to hold values
  tupleArray = []
  tupleArray.append([])

  lineCounter = 0
  for line in file:
    #print line + "\n"
    start = 0 #reset start of line range
    for digit in range(0, len(line)):
      if(line[digit] == " " and start <= digit and line[digit] != "\n"): #look for a divider
        tupleArray[lineCounter].append(line[start:digit])
        #tupleArray[lineCounter][int(math.floor(digit/valDivisor))] = line[start:digit] #insert new value to tupleArray[0][0] for example
        start = digit+1 #set previous line range to previous divider, +1 so its not on the " "
    lineCounter += 1 #we are on the next line for reading
    tupleArray.append([])

  return tupleArray

def main():
  #print(sys.argv[1])
  if (len(sys.argv) < 2 or len(sys.argv) > 2):
    print("python parseData.py <file_destination>\n")
    return

  tupleArray = parseFile(sys.argv[1])
  print(tupleArray)

  return


main()
