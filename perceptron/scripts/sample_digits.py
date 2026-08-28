import numpy as np
from sklearn.datasets import fetch_openml
from PIL import Image
import pickle
import os
import random

# Fetch one image of a digit
#mnist = fetch_openml('mnist_784', version=1, as_frame=False)
with open("mnist.pkl", "rb") as fobj:
    x_train, y_train, x_test, y_test = pickle.load(fobj)

for i, a in enumerate(random.sample(list(range(len(x_test))), k=250)):
#for a in (10000, 20000, 30000, 40000, 50000, 60000):
    sample_digit = x_test[a].astype(np.uint8)

    fn = f"DIGIT{i:03d}.dat"
    # Save to 784-byte binary file
    with open(fn, "wb") as f:
        f.write(sample_digit.tobytes())

    print(f"{fn} saved with size: {len(sample_digit.tobytes())} bytes")

    image_array = sample_digit.reshape(28, 28)

    # Convert the numpy array to an image using Pillow (PIL)
    # The data is typically uint8 (0-255 range)
    # For some libraries (like matplotlib display), it might need scaling if it's a float, but for PIL it works as is
    image = Image.fromarray(image_array.astype('uint8'), 'L') # 'L' mode is for grayscale images

    # Define the filename
    filename = os.path.join("img", f"DIGIT{i:03d}.png")

    # Save the image
    image.save(filename)
    print(f"Saved as image to {filename}.")

