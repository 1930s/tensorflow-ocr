from __future__ import print_function
from tensorflow import keras

import tensorflow as tf
import numpy as np
import numbers
import decimal
import sys

from parseData import *

#HelloWorld
hello = tf.constant('Loading Tensorflow...')
sess2 = tf.Session()
print(sess2.run(hello))

#grab file from user
if (len(sys.argv) < 2 or len(sys.argv) > 2):
  print("python train.py <file_destination>\n")
  exit(0)
else:
  filename = sys.argv[1]

fp = open(filename, "r")


###############################################
#Thank you Leah for this magical code. 
#And here we witness how the training data gets parsed and each label gets mapped to an appropriate integer.
# sorted_labels_arr: the finalized array of all the labels in the correct order
# flowy_labels: a NumPy of the integers associated with those values fed into TensorFlow

labels_arr = charToInt27(filename)

remove_dups_labels_arr = set(labels_arr)
  
sorted_labels_arr = sorted(remove_dups_labels_arr)
  

numeric_labels_arr = []  #label input into tensyflow

for value in labels_arr:
  for label in sorted_labels_arr:
    if value == label:
      index = sorted_labels_arr.index(label)
      numeric_labels_arr.append(index)

flowy_labels = np.array(numeric_labels_arr)

###############################################
# Creating the training data
# tupleArray: the array of training glyphs

#Blake's code
#simply goes through the .data file and pushes each glyph into tuples
tupleArray = parseFileArray(filename)

###############################################
# Creating the Test Data matrices
# testmatrix: the list of test glyphs
# testlabels_arr: the list of correct characters that are supposed to be associated with those glyphs

filename_test = '../tmp.out'
file_test = open(filename_test, "r", encoding='utf-8')

linecount = len(open(filename_test).readlines( ))
print("Linecount: ", linecount)

testmatrix = np.zeros(shape=(linecount, 27))
testlabels_arr = []

#Fun parsy times inside this loop
rowcount = 0
for line in file_test:
  colcount = 0
  addystring = ""
  for place in range(0, len(line)):
    if(line[place]=="," and colcount<27):
      testmatrix[rowcount][colcount]=line[(place-8):place]
      colcount+=1
    elif(colcount==27):
      if(line[place]!="\n" and line[place]!=" "):
        addystring+=line[place]
  rowcount+=1
  testlabels_arr.append(addystring)

###############################################
# Default values, in case of empty columns. Also specifies the type of the
# decoded result.


with tf.Session() as sess:
  # Start populating the filename queue.

  #Used for averaging multiple test trials
  all_trials_total_correct = 0
  all_trials_total_total = 0

  for iter in range(0,5):
    #This is basically the function that creates the neural network and all its magical juicy insides
    model = keras.Sequential([
      keras.layers.Dense(512, input_shape=(27,), activation=tf.nn.relu), 
      keras.layers.Dense(len(sorted_labels_arr), activation=tf.nn.softmax), #first parameter is hardcoded as # of unique labels
    ])

    #Compiling the neural network
    model.compile(optimizer=tf.train.AdamOptimizer(), #so these are the different optimizers. word on google is adam is the best
    #model.compile(optimizer='rmsprop', 
              loss='sparse_categorical_crossentropy',
              metrics=['accuracy'])

    #Training here. Epochs are the number of runs. Batch size isn't necessary but another parameter I've been experimenting with that seems to improve performance
    model.fit(np.array(tupleArray), flowy_labels, epochs=20)

    #Testing the training data here
    test_loss, test_acc = model.evaluate(np.array(tupleArray), flowy_labels)

    #And this is the function that predicts the new test data
    predictions = model.predict(testmatrix)

    #Figuring out the percentage of correct characters within the entire document
    total = len(testlabels_arr)
    adjusted_total = total
    correct = 0
    for i in range(0,len(testlabels_arr)):
      if(testlabels_arr[i]=='XX'):
        adjusted_total-=1
      elif(sorted_labels_arr[np.argmax(predictions[i])]==testlabels_arr[i]):
        correct+=1
    all_trials_total_total = adjusted_total*5
    all_trials_total_correct += correct
    print("Percentage correct: ")
    print(100.0*correct/adjusted_total)
    print(correct, "Correct")
    print(adjusted_total, "Adjusted Total")
    print(total, "Total")

  print(100.0*all_trials_total_correct/all_trials_total_total, "% accuracy over 5 trials")   
