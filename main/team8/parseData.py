import sys

def parseFile(filename):
  file = open(filename, "r")
  
  for line in file:
    print line + "\n"

def main():
  #print(sys.argv[1])

  print("Input: python parseData.py <file>\n")
  
  parseFile(sys.argv[1])


main()
