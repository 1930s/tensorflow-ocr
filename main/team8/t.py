from __future__ import print_function
from tensorflow import keras

import tensorflow as tf
import numpy as np

#HelloWorld
hello = tf.constant('Hello, TensorFlow!')
sess2 = tf.Session()
print(sess2.run(hello))

###############################################

#Leah's code here
file = open("../fontData/kafka.data", "r")
labels_arr = []
lineCounter = 0


for line in file:
  start = 0 #reset start of line range
  for symbol in range(0, len(line)-1):
    if(symbol == 161):
      letter = line[162:len(line)-1]
      labels_arr.append(letter)
  lineCounter += 1 #we are on the next line for reading

print(labels_arr)
remove_dups_labels_arr = set(labels_arr)
#print(remove_dups_labels_arr)
sorted_labels_arr = sorted(remove_dups_labels_arr)
print(sorted_labels_arr)

numeric_labels_arr = []  #label input into tensyflow

for value in labels_arr:
    for label in sorted_labels_arr:
        if value == label:
            index = sorted_labels_arr.index(label)
            numeric_labels_arr.append(index)

flowy_labels = np.array(numeric_labels_arr)
print(flowy_labels)
print("Shape: ")
print(flowy_labels.shape)
###############################################

#filenames = tf. train.string_input_producer(["./k3.csv"]);
#reader = tf.TextLineReader()
#key, value = reader.read(filenames)

###############################################

#Blake's code
#simply goes through the .data file and pushes each glyph into tuples
filename = '../fontData/kafka.data'
file = open(filename, "r")

tupleArray = []

lineCounter = 0
for line in file:
    tupleArray.append([]) #tupleArray is the 27-float array
    start = 0 #reset start of line range

    for digit in range(0, len(line)-1):
      if(line[digit] == " "  and start <= digit or line[digit] == "\n"): #look for a divider
        tupleArray[lineCounter].append(line[start:digit]) #adds value into the tuple
        #tupleArray[lineCounter][int(math.floor(digit/valDivisor))] = line[start:digit] #insert new value to tupleArray[0][0] for example
        start = digit+1 #set previous line range to previous divider, +1 so its not on the " "

    lineCounter += 1 #we are on the next line for reading

floaty_floats = np.array(tupleArray)
print(floaty_floats)
print("Shape: ")
print(floaty_floats.shape)

###############################################

# Default values, in case of empty columns. Also specifies the type of the
# decoded result.


#r = [[1.0]]*27;
#col1, col2, col3, col4, col5, col6, col7, col8, col9, col10, col11, col12, col13, col14, col15, col16, col17, col18, col19, col20, col21, col22, col23, col24, col25,  col26, col27 = tf.decode_csv(value, record_defaults=r, field_delim=' ')
#features = tf.stack([col1, col2, col3, col4, col5, col6, col7, col8, col9, col10, col11, col12, col13, col14, col15, col16, col17, col18, col19, col20, col21, col22, col23, col24, col25,  col26, col27])

with tf.Session() as sess:
  # Start populating the filename queue.

    model = keras.Sequential([
      keras.layers.Dense(131072, input_shape=(27,), activation=tf.nn.relu), 
      keras.layers.Dense(58, activation=tf.nn.softmax), #first parameter is hardcoded as # of unique labels
    ])

    model.compile(optimizer=tf.train.AdamOptimizer(), 
              loss='sparse_categorical_crossentropy',
              metrics=['accuracy'])

    model.fit(floaty_floats, flowy_labels, epochs=20)

    test_loss, test_acc = model.evaluate(floaty_floats, flowy_labels)

    print('Test accuracy:', test_acc)



#  coord = tf.train.Coordinator()
#  threads = tf.train.start_queue_runners(coord=coord)

#  for i in range(50):
    # Retrieve a single instance:
#    example, label = sess.run([features, col27]) #change col27 with a different list of the labels

#  coord.request_stop()
#  coord.join(threads)

#  print("rly go fuck yourself")






