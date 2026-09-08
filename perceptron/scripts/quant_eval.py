import numpy as np
import os
import pickle
from tqdm import tqdm
from tensorflow import keras
from sklearn.metrics import classification_report

Wq = np.fromfile("../a8mnist/mnist_quant.bin", dtype=np.int8).reshape((10, -1))
bq = np.fromfile("../a8mnist/mnist_bias.bin", dtype=np.int8)

(x_train, y_train), (x_test, y_test) = keras.datasets.mnist.load_data()
with open("mnist.pkl", "wb") as fobj:
    pickle.dump((x_train, y_train, x_test, y_test), fobj)

# with open("mnist.pkl", "rb") as fobj:
#     x_train, y_train, x_test, y_test = pickle.load(fobj)

# Flatten and normalize to 0–1
# x_train = x_train.reshape(-1, 784).astype("float32") / 255.0
x_test  = x_test.reshape(-1, 784).astype("uint8")
print(f"{len(x_test)} samples in x_test.")

#scale = 433
#bias_scale = 325
scale = 1
bias_scale = 1

def quant_infer(ws, biases, inp):
    outputs = []
    for k, r in enumerate(ws):
        acc = 0
        for j, w in enumerate(r):
            acc += ((inp[j] * w * scale) >> 8)
            # print(f"{j} inp {inp[j]} w {w} acc {acc} inc {((inp[j] * w * scale) >> 8)}")

        #acc_scaled = acc * scale / 256
        #acc_scaled = (acc * scale) >> 8

        #bias_scaled = (biases[k] * bias_scale) >> 8
        bias_scaled = 0

        outputs.append(acc + bias_scaled)

    if max(outputs) > (1 << 16):
        print(f"warning: {outputs}")
    return np.argmax(outputs)

quinfs = []
for X in tqdm(x_test):
#for f in ("DIGIT0.DAT", "DIGIT1.DAT", "DIGIT2.DAT", "DIGIT3.DAT", "DIGIT4.DAT", "DIGIT5.DAT"):
#    X = np.fromfile(os.path.join(f), dtype=np.uint8)
   quinfs.append(quant_infer(Wq, bq, r))
   Xprime = np.array([255 if a > 0 else 0 for a in X], dtype=np.uint8)
   quinfs.append(quant_infer(Wq, bq, Xprime))
 
print("Quantized inference classification report:")
print(classification_report(y_test, quinfs))
