import sys

def parseFile(filename):
  file = open(filename, "r")
  
  for line in file:
    print line + "\n"

def main():
  #print(sys.argv[1])
  if (len(sys.argv) < 2 or len(sys.argv) > 2):
    print("python parseData.py <file_destination>\n")
    return
  
  parseFile(sys.argv[1])


main()
