import os
import subprocess
import shutil
from datetime import datetime

from flask import Flask, render_template, request, send_file
import tensorflow as tf


HOST = os.getenv("HOST", "localhost")
PORT = os.getenv("PORT", "5000")


CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
print("Current directory:", CURRENT_DIR)

model_dir = os.path.join(CURRENT_DIR, "model")
if not os.path.exists(model_dir):
    os.makedirs(model_dir)

def tfjs2keras_saved_model(model_name: str = "model.json"):
    """
    Convert a TensorFlow.js saved model to a Keras saved model.

    Args:
        model_name (str): Path to the TensorFlow.js saved model.
    """

    output_path = os.path.join(CURRENT_DIR, "convertered_model")
    command = [
        "tensorflowjs_converter",
        "--input_format=tfjs_layers_model",
        "--output_format=keras_saved_model",
        os.path.join(CURRENT_DIR, "model", model_name),
        output_path,
    ]

    subprocess.run(command, check=True)

    # Convert the model

    converter = tf.lite.TFLiteConverter.from_saved_model(
        os.path.join(CURRENT_DIR, output_path)
    )
    tflite_model = converter.convert()

    # Save the model.
    with open(os.path.join(CURRENT_DIR, "model", "model.tflite"), "wb") as f:
        f.write(tflite_model)


app = Flask(__name__)


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/upload", methods=["POST"])
def upload():
    folder_path = os.path.join(CURRENT_DIR, "model")

    for filename in os.listdir(folder_path):
        file_path = os.path.join(folder_path, filename)
        if os.path.isfile(file_path) or os.path.islink(file_path):
            os.remove(file_path)
        elif os.path.isdir(file_path):
            shutil.rmtree(file_path)

    now = datetime.now()
    minute = now.minute
    if minute < 10:
        minute = "0" + str(minute)
    key = request.form.get("key")
    if key != str(minute) + "0721":
        return "Invalid key", 403

    json_file = request.files.get("json_file")
    bin_file = request.files.get("bin_file")

    if not json_file or not bin_file:
        return "Missing file(s)", 400

    json_path = os.path.join(CURRENT_DIR, "model", json_file.filename)
    bin_path = os.path.join(CURRENT_DIR, "model", bin_file.filename)

    json_file.save(json_path)
    bin_file.save(bin_path)

    print("Files saved:", json_path, bin_path)
    download_name = os.path.splitext(json_file.filename)[0] + ".tflite"
    print("Download name:", download_name)

    try:
        tfjs2keras_saved_model(json_file.filename)
        return send_file(
            os.path.join(CURRENT_DIR, "model", "model.tflite"),
            mimetype="application/octet-stream",
            as_attachment=True,
            download_name=download_name,
        )
    except subprocess.CalledProcessError as e:
        print("Error during conversion:", e)
        return str(e), 500


if __name__ == "__main__":
    app.run(debug=True, host=HOST, port=int(PORT))
