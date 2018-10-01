import tensorflow as tf
from tensorflow import keras #library for creating datasets and training programs
import numpy as np
import matplotlib.pyplot as plt #can be used to show a visual representation of our data as a plot
import string

#open file here to begin implementing data
ocr_file = 'temp.data' 
#we would pull the file based on which type of command they used in the makefile
#this means the file for bashevis text would be labeled bashevis.data and we can set ocr_file to be that
with open(ocr_file, 'r') as f: #open with read privileges
  #insert code to use ocr_file

