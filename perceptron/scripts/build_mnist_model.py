import numpy as np
from tensorflow import keras
from sklearn.metrics import classification_report
# -----------------------------
# 1. Load MNIST
# -----------------------------
(x_train, y_train), (x_test, y_test) = keras.datasets.mnist.load_data()

# Flatten and normalize to 0–1
x_train = x_train.reshape(-1, 784).astype("float32") / 255.0
x_test  = x_test.reshape(-1, 784).astype("float32") / 255.0

# -----------------------------
# 2. Build tiny perceptron model
# -----------------------------
model = keras.Sequential([
    keras.layers.Input(shape=(784,)),
    keras.layers.Dense(10, activation="linear")  # no softmax
])

model.compile(
    optimizer="adam",
    loss=keras.losses.SparseCategoricalCrossentropy(from_logits=True),
    metrics=["accuracy"]
)

print("Training model...")
model.fit(x_train, y_train, epochs=10, batch_size=128, verbose=2)

# Evaluate
loss, acc = model.evaluate(x_test, y_test, verbose=0)
print(f"Test accuracy: {acc*100:.2f}%")

# -----------------------------
# 3. Extract weights
# -----------------------------
W, b = model.layers[0].get_weights()  # W: (784,10), b: (10,)

# Transpose to row-major: (10,784)
W = W.T

# -----------------------------
# 4. Quantize to int8
# -----------------------------
scale = np.max(np.abs(W))
bias_scale = np.max(np.abs(b))
Wq = np.round((W / scale) * 127).astype(np.int8)
bq = np.round((b / scale) * 127).astype(np.int8)
print("butt", b, bq)

print("Quantization weight scale:", scale)
print("Quantization bias scale:", bias_scale)

# -----------------------------
# 5. Save binary files
# -----------------------------
Wq.tofile("../a8mnist/mnist_quant.bin")
bq.tofile("../a8mnist/mnist_bias.bin")

# -----------------------------
# 6. Optional debug header
# -----------------------------
with open("../a8mnist/mnist_weights.h", "w") as f:
    f.write("static const signed char W[10][784] = {\n")
    for row in Wq:
        f.write("  { " + ", ".join(str(int(x)) for x in row) + " },\n")
    f.write("};\n\n")

    f.write("static const signed char B[10] = { ")
    f.write(", ".join(str(int(x)) for x in bq))
    f.write(" };\n")

print("Export complete.")
