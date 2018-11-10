import sys
import os

#simply goes through the .data file and pushes each glyph into tuples
def parseFileArray(filename):
  
  file_to_read = open(filename, "r")
  tupleArray = []

  lineCounter = 0
  for line in file_to_read:
    tupleArray.append([])
    start = 0 #reset start of line range
    
    for digit in range(0, len(line)):
      if(line[digit] == " " or line[digit] == "\n" and start <= digit): #look for a divider
        tupleArray[lineCounter].append(line[start:digit]) #adds value into the tuple
        #tupleArray[lineCounter][int(math.floor(digit/valDivisor))] = line[start:digit] #insert new value to tupleArray[0][0] for example
        start = digit+1 #set previous line range to previous divider, +1 so its not on the " "
    
    lineCounter += 1 #we are on the next line for reading

    #file_to_read.close() #close file for safety

  return tupleArray

#converts a .data file into a .csv file
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

#simply pulls the end of each tupleArray in the 2D list, ignoring the data beforehand
def pullLabels(tupleArray):
  
  tuplesLen = len(tupleArray)
  tupleArrayLen = len(tupleArray[0]) #0 because the len of each array in the 2D set is the same

  labels = [] #create a list of labels the same length as the 2D tupleArray
  for array in tupleArray:
    labels.append(array[tupleArrayLen-1])

  return labels


def main():
  #print(sys.argv[1])

  if (len(sys.argv) < 2 or len(sys.argv) > 3):
    print("python parseData.py <file_destination> <new .csv file name>\n")
    return

  val = parseFileCSV(sys.argv[1], sys.argv[2])

  labels = pullLabels(parseFileArray(sys.argv[1]))

  print(labels)
  print(len(labels))

  if(val != 0):
    print("An error occurred.")

  return


main()
