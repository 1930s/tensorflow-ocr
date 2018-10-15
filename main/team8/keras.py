import tensorflow as tf
from tensorflow import keras #library for creating datasets and training programs
import numpy as np
import matplotlib.pyplot as plt #can be used to show a visual representation of our data as a plot
import string

#open file here to begin implementing data
ocr_file = 'temp.data' 
#we would pull the file based on which type of command they used in the makefile
#this means the file for bashevis text would be labeled bashevis.data and we can set ocr_file to be that
#this file will be given from kd.c
with open(ocr_file, 'r') as f: #open with read privileges
  #insert code to use ocr_file

class model_tools:
  # Defined functions for all the basic tensorflow components that we needed for building a model.

  #Parameter: shape
  def add_weights(self, shape):
    # a common method to create all sorts of weight connections
    # takes in shapes of previous and new layer as a list e.g.
    #  [2,10]
    # starts with random values of that shape.
    return tf.Variable(tf.truncated_normal(shape=shape,stddev=0.05))

  #Parameter: shape
  def add_biases(self, shape):
    # a common method to add create biases with default=0.05
    # takes in shape of the current layer e.g. x=10
    return tf.Variable(tf.constant(0.05, shape=shape))

  #Parameters: layer, kernel, input_shape, output_shape, stride_size
  #layer - takes in last layer.
  #kernel - kernel size for convoluting on the image.
  #input_shape - size of the input image.
  #ouput_shape - size of the convoluted image.
  #stride_size - determines kernel jump size
  def conv_layer(self,layer, kernel, input_shape, output_shape, stride_size):
    weights = self.add_weights([kernel, kernel, input_shape, output_shape])
    biases = self.add_biases([output_shape])
    #stride=[image_jump,row_jump,column_jump,color_jump]=[1,1,1,1]
    stride = [1, stride_size, stride_size, 1]
    #does a convolution scan on the given image
    layer = tf.nn.conv2d(layer, weights, strides=stride, padding='SAME') + biases               
    return layer  

  #Parameters: previous_layer, kernel, stride
  def pooling_layer(self,layer, kernel_size, stride_size):
    # basically it reduces the complexity involved by only    taking the important features alone
    # many types of pooling is there.. average pooling, max pooling..
    # max pooling takes the maximum of the given kernel
    #kernel=[image_jump,rows,columns,depth]
    kernel = [1, kernel_size, kernel_size, 1]
    #stride=[image_jump,row_jump,column_jump,color_jump]=[1,2,2,1] mostly
    stride = [1, stride_size, stride_size, 1]
    return tf.nn.max_pool(layer, ksize=kernel, strides=stride, padding='SAME')

  #Parameters: layer
  #converts all multidimensional matrices into a single dimension
  def flattening_layer(self, layer):
    #make it single dimensional
    input_size = layer.get_shape().as_list()
    new_size = input_size[-1] * input_size[-2] * input_size[-3]
    return tf.reshape(layer, [-1, new_size]),new_size

  #Parameters: previous layer, shape of previous layer, shape of output layer
  #This is a vanilla layer. It connects the previous layer with the output layer. Here is where the mx+b operation occurs.
  def fully_connected_layer(self,layer, input_shape, output_shape):
    #create weights and biases for the given layer shape
    weights = self.add_weights([input_shape, output_shape])
    biases = self.add_biases([output_shape])
    #most important operation
    layer = tf.matmul(layer,weights) + biases  # mX+b
    return layer

  #Parameters: layer
  #we use Rectified linear unit Relu. it's the standard activation layer used.
  #There are also other layer like sigmoid,tanh..etc. but relu is more efficent.
  #function: 0 if x<0 else x.
  def activation_layer(self,layer):
    return tf.nn.relu(layer)
