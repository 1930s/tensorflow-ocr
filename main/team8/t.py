from __future__ import print_function
from tensorflow import keras

import tensorflow as tf
import numpy as np
import numbers
import decimal

#HelloWorld
hello = tf.constant('Hello, TensorFlow!')
sess2 = tf.Session()
print(sess2.run(hello))

###############################################

#Leah's code here
#file = open("../fontData/kafka.data", "r")
filename = './kafka2.data'
file = open(filename, "r")
labels_arr = []
lineCounter = 0


for line in file:
  start = 0 #reset start of line range
  for symbol in range(0, len(line)-1):
    if(symbol == 161):
      letter = line[162:len(line)-1]
      labels_arr.append(letter)
  lineCounter += 1 #we are on the next line for reading

#print(labels_arr)
remove_dups_labels_arr = set(labels_arr)
#print(remove_dups_labels_arr)
sorted_labels_arr = sorted(remove_dups_labels_arr)
#print(sorted_labels_arr)

numeric_labels_arr = []  #label input into tensyflow

for value in labels_arr:
    for label in sorted_labels_arr:
        if value == label:
            index = sorted_labels_arr.index(label)
            numeric_labels_arr.append(index)

flowy_labels = np.array(numeric_labels_arr)
#print(flowy_labels)
#print("Shape: ")
#print(flowy_labels.shape)

###############################################

#filenames = tf. train.string_input_producer(["./k3.csv"]);
#reader = tf.TextLineReader()
#key, value = reader.read(filenames)

###############################################

#Blake's code
#simply goes through the .data file and pushes each glyph into tuples
#filename = '../fontData/kafka.data'
filename = './kafka2.data'
file = open(filename, "r")

tupleArray = []

lineCounter = 0
for line in file:
    tupleArray.append([]) #tupleArray is the 27-float array
    start = 0 #reset start of line range

    for digit in range(0, len(line)-1):
      if(line[digit] == " "  and start <= digit or line[digit] == "\n"): #look for a divider
        tupleArray[lineCounter].append(line[start:digit]) #adds value into the tuple
        start = digit+1 #set previous line range to previous divider, +1 so its not on the " "

    lineCounter += 1 #we are on the next line for reading

floaty_floats = np.array(tupleArray)
#print("len and other len:")
#print(tupleArray)
#print(len(tupleArray))
#print(len(tupleArray[0]))
#print("random sample")
#print(tupleArray[5][5])
#print([isinstance(tupleArray[5][5], numbers.Number) for x in (0, 0.0, 0j, decimal.Decimal(0))])
#print(floaty_floats)
#print("Shape: ")
#print(floaty_floats.shape)

###############################################

filename_test = '../tmp.out'
file_test = open(filename_test, "r")

linecount = len(open(filename_test).readlines( ))

testmatrix = np.zeros(shape=(linecount, 27))

rowcount = 0
for line in file_test:
  colcount = 0
  for place in range(0, len(line)):
    if(line[place]==","):
      testmatrix[rowcount][colcount]=line[(place-8):place]
      #testmatrix[rowcount][colcount]=colcount
      colcount+=1
  rowcount+=1

print("Test Matrix gets tested here")
print(len(testmatrix))
print(len(testmatrix[0]))
print("random sample")
print(testmatrix[5][5])
print([isinstance(testmatrix[5][5], numbers.Number) for x in (0, 0.0, 0j, decimal.Decimal(0))])

###############################################
# Default values, in case of empty columns. Also specifies the type of the
# decoded result.


with tf.Session() as sess:
  # Start populating the filename queue.

    model = keras.Sequential([
      #keras.layers.Dense(131072, input_shape=(27,), activation=tf.nn.relu), 
      keras.layers.Dense(256, input_shape=(27,), activation=tf.nn.relu), 
      keras.layers.Dense(len(sorted_labels_arr), activation=tf.nn.softmax), #first parameter is hardcoded as # of unique labels
    ])

    model.compile(optimizer=tf.train.AdamOptimizer(), 
    #model.compile(optimizer='rmsprop', 
              loss='sparse_categorical_crossentropy',
              metrics=['accuracy'])

    model.fit(tupleArray, flowy_labels, epochs=100)

    print("Flowy labels:")
    print(flowy_labels)

    test_loss, test_acc = model.evaluate(tupleArray, flowy_labels)

    print('Test accuracy:', test_acc)

    meow = model.predict(testmatrix)

    #print(flowy_labels)
    #print(testmatrix[0])
    #print(testmatrix[1])
    #print(testmatrix[2])
    #print(testmatrix[3])
    #print(testmatrix[4])

    print(sorted_labels_arr)

    for i in range(0,100):    
      #print(meow[i])
      #print(i)
      #print("Argmax:", np.argmax(meow[i]))
      #print("corresponding label: ")
      print(sorted_labels_arr[np.argmax(meow[i])])


#    print(meow[777])
 #   print("Meow 4:", np.argmax(meow[777]))

    print(meow.shape)    
