import sys

#simply goes through the .data file and pushes each glyph into tuples
def parseLabels(filename):
  file = open(filename, "r")
  labels_arr = []

  lineCounter = 0
  for line in file:
    #labels_arr.append([])
    start = 0 #reset start of line range
    
    for symbol in range(0, len(line)-1):
        #print(digit)
        #start = digit+1 #set previous line range to previous divider, +1 so its not on the " "
        if(symbol == 161):
            #for letter in range(162, len(line)):
            letter = line[162:len(line)-1]
                #print(line[162:len(line)-1])
            labels_arr.append(letter)    
        
    
    lineCounter += 1 #we are on the next line for reading
  return labels_arr

def main():
  #print(sys.argv[1])
  if (len(sys.argv) < 2 or len(sys.argv) > 2):
    print("python parseData.py <file_destination>\n")
    return

  labels_arr = parseLabels(sys.argv[1])
  print(labels_arr, " ")

  remove_dups_labels_arr = set(labels_arr)
  print(remove_dups_labels_arr)

  return


main()
