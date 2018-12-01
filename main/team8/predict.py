from __future__ import print_function
from tensorflow import keras

import tensorflow as tf
import numpy as np
import numbers
import decimal
import sys
from train import *

print("labels")
print(sorted_labels_arr[0])
print(sorted_labels_arr[1])


#And this is the function that predicts the new test data
predictions = model.predict(testmatrix)

#Printing the first 100 characters in the document: actual vs expected
for i in range(0,100):    
    print("Actual: ", sorted_labels_arr[np.argmax(predictions[i])])
    print("Expected: ",testlabels_arr[i])
    print()

#Figuring out the percentage of correct characters within the entire document
#total = len(testlabels_arr)
#adjusted_total = total
#correct = 0
#for i in range(0,len(testlabels_arr)):
#    if(testlabels_arr[i]=='XX'):
#        adjusted_total-=1
#    elif(sorted_labels_arr[np.argmax(predictions[i])]==testlabels_arr[i]):
#        correct+=1
#all_trials_total_total = adjusted_total*5
#all_trials_total_correct += correct
#print("Percentage correct: ")
#print(100.0*correct/adjusted_total)
#print(correct, "Correct")
#print(adjusted_total, "Adjusted Total")
#print(total, "Total")

#print(100.0*all_trials_total_correct/all_trials_total_total, "% accuracy over 5 trials")
#print(meow.shape)    
