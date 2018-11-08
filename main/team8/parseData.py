import sys
import os

#simply goes through the .data file and pushes each glyph into tuples
def parseFileArray(filename):
  
  file_to_read = open(filename, "r")
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

    file_to_read.close() #close file for safety

  return tupleArray

def parseFileCSV(filename, newfile):
  
  directory = "CSV_Data"

  if not os.path.exists(directory): #create file directory it it doesnt exist already
    os.makedirs(directory)

  file_to_read = open(filename, "r")
  file_to_write = open(os.path.join(directory, newfile + ".csv"), "w")

  for line in file_to_read:
    line = line.replace(" ", ",") #replace spaces with ','
    file_to_write.write(line)

  file_to_read.close() #close file for safety
  file_to_write.close() #close file for safety

  return 0


def main():
  #print(sys.argv[1])
  if (len(sys.argv) < 2 or len(sys.argv) > 3):
    print("python parseData.py <file_destination> <new .csv file name>\n")
    return

  val = parseFileCSV(sys.argv[1], sys.argv[2])
  if(val != 0)
    print("An error occurred.")

  return


main()
