What is NNCompression ?
-----------------------
NNCompression is a simple gui interface that allow loading an image, and then try to train a neural network
in the shape of an auto encoder on small square cut from this image.
Then, it display the output of this auto-encoder when run on the slices from the original image.
The idea is that by conserving the second half of the auto-encoder network, and the values of the original image through the first
half of the auto-encoder, you obtain a compressed representation of the original image. Though it doesn't seams very efficient, 
both in computation time, quality and space gained, it is an interesting exercise for implementing neural network training by hand.

Instalation
-----------

To compile the project : Install QT4 qmake (package `qt4-qmake` on ubuntu)
and run `qmake && make`.
