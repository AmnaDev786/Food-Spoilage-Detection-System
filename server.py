from flask import Flask, request, jsonify
import tensorflow as tf
import numpy as np
from PIL import Image
import io
import requests
import os

# -------------------------
# Blynk CONFIG
# -------------------------
BLYNK_TOKEN = "RFptSStvtFdcC_Hszer3fVbUenismNej"
V0 = "V0"
V1 = "V1"
V2 = "V2"
EVENT = "spoilage_alert"

# -------------------------
# LOAD LABELS
# -------------------------
labels_path = os.path.join("model", "labels.txt")
with open(labels_path, "r") as f:
    LABELS = [line.strip().split(" ", 1)[1] for line in f.readlines()]

# -------------------------
# LOAD TFLITE MODEL
# -------------------------
model_path = os.path.join("model", "model.tflite")
interpreter = tf.lite.Interpreter(model_path=model_path)
interpreter.allocate_tensors()
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

# -------------------------
# INIT FLASK
# -------------------------
app = Flask(__name__)

# -------------------------
# BLYNK HELPERS
# -------------------------
def send_to_blynk(vpin, value):
    url = f"https://blynk.cloud/external/api/update?token={BLYNK_TOKEN}&{vpin}={value}"
    try:
        requests.get(url)
        print(f"Blynk updated: {vpin} = {value}")
    except Exception as e:
        print("Failed to send to Blynk:", e)

def trigger_blynk_event(message):
    url = f"https://blynk.cloud/external/api/logEvent?token={BLYNK_TOKEN}&event={EVENT}&value={message}"
    try:
        requests.get(url)
        print(f"Blynk event triggered: {message}")
    except Exception as e:
        print("Failed to trigger Blynk event:", e)

# -------------------------
# PREDICT ROUTE
# -------------------------
@app.route("/upload", methods=["POST"])
def predict():
    try:
        # Read image from POST request
        image = Image.open(io.BytesIO(request.data)).resize((224, 224))
        
        # Model expects UINT8 input (not normalized to float)
        img_array = np.array(image).astype(np.uint8)
        img_array = np.expand_dims(img_array, axis=0)

        # Run TFLite inference
        interpreter.set_tensor(input_details[0]['index'], img_array)
        interpreter.invoke()
        output = interpreter.get_tensor(output_details[0]['index'])
        class_id = int(np.argmax(output))
        label = LABELS[class_id]

        # Map label to Blynk values
        if label == "Spoiled_Apples":
            v0Text, v1Text, severity = "Spoiled Apple", "", 2
        elif label == "Slightly_Spoiled_Apples":
            v0Text, v1Text, severity = "Slightly Spoiled Apple", "Make jam / smoothie / pie", 1
        elif label == "Fresh_Apples":
            v0Text, v1Text, severity = "Fresh Apple", "", 0
        elif label == "Spoiled_Banana":
            v0Text, v1Text, severity = "Spoiled Banana", "", 2
        elif label == "Slightly_Spoiled_Banana":
            v0Text, v1Text, severity = "Slightly Spoiled Banana", "Make smoothie / bread", 1
        elif label == "Fresh_Banana":
            v0Text, v1Text, severity = "Fresh Banana", "", 0
        else:
            v0Text, v1Text, severity = "Unknown", "", 0

        # Send updates to Blynk
        send_to_blynk(V0, v0Text)
        send_to_blynk(V1, v1Text)
        send_to_blynk(V2, str(severity))

        # Trigger notification if spoiled
        if severity == 2:
            trigger_blynk_event(v0Text + " detected!")

        # Return prediction to ESP32
        return jsonify({"predicted_class": label})

    except Exception as e:
        print("Error:", e)
        return jsonify({"error": str(e)}), 500

# -------------------------
# RUN SERVER
# -------------------------
if __name__ == "__main__":
    # Run on all network interfaces
    app.run(host="0.0.0.0", port=5000)