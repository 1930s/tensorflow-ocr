import sys

#simply goes through the .data file and pushes each glyph into tuples
def parseFile(filename):
  file = open(filename, "r")
  tupleArray = []

  lineCounter = 0
  for line in file:
    tupleArray.append([])
    start = 0 #reset start of line range
    
    for digit in range(0, len(line)):
      if(line[digit] == " " and start <= digit or line[digit] == "\n"): #look for a divider
        tupleArray[lineCounter].append(line[start:digit]) #adds value into the tuple
        #tupleArray[lineCounter][int(math.floor(digit/valDivisor))] = line[start:digit] #insert new value to tupleArray[0][0] for example
        start = digit+1 #set previous line range to previous divider, +1 so its not on the " "
    
    lineCounter += 1 #we are on the next line for reading

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
