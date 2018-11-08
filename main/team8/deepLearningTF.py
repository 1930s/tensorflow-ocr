import tensorflow as tf
from tensorflow import keras
#import compress_labels as labels
#import os
import array
import subprocess

import numpy as np
import matplotlib.pyplot as plt

#print(tf.__version__)

lables = []

labels = subprocess.run('compress_labels.py ../fontData/kafka.data')

