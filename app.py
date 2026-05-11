from flask import Flask, request, jsonify, render_template
from flask_cors import CORS
import requests
import base64
import time
import os

app = Flask(__name__)
CORS(app)

ROBOFLOW_API_KEY = "Xi9q0U8qd4pLO3EzyzaP"
ROBOFLOW_MODEL = "human-surveillance-detection-kesab/7"
ROBOFLOW_URL = f"https://detect.roboflow.com/{ROBOFLOW_MODEL}?api_key={ROBOFLOW_API_KEY}"

detection_log = []
latest_image_b64 = None
latest_result = None


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/upload", methods=["POST"])
def upload():
    global latest_image_b64, latest_result

    try:
        if "image" not in request.files:
            return jsonify({"error": "No image received"}), 400

        image_file = request.files["image"]
        image_bytes = image_file.read()

        # Simpan base64 untuk ditampilkan di web
        latest_image_b64 = base64.b64encode(image_bytes).decode("utf-8")

        # Kirim ke Roboflow
        response = requests.post(
            ROBOFLOW_URL,
            files={"file": ("image.jpg", image_bytes, "image/jpeg")},
            timeout=15
        )

        if response.status_code != 200:
            raise Exception(f"Roboflow error: {response.status_code}")

        result = response.json()
        predictions = result.get("predictions", [])
        human_count = len(predictions)

        # Simpan ke log
        log_entry = {
            "timestamp": time.strftime("%H:%M:%S"),
            "count": human_count,
            "status": "ok"
        }
        detection_log.insert(0, log_entry)
        if len(detection_log) > 50:
            detection_log.pop()

        latest_result = log_entry

        # Balikan ke ESP32
        return jsonify({
            "count": human_count,
            "message": f"{human_count} manusia"
        })

    except requests.exceptions.Timeout:
        err = {"timestamp": time.strftime("%H:%M:%S"), "count": 0, "status": "error: roboflow timeout"}
        detection_log.insert(0, err)
        return jsonify({"error": "Roboflow timeout"}), 504

    except Exception as e:
        err = {"timestamp": time.strftime("%H:%M:%S"), "count": 0, "status": f"error: {str(e)}"}
        detection_log.insert(0, err)
        return jsonify({"error": str(e)}), 500


@app.route("/status", methods=["GET"])
def status():
    return jsonify({
        "log": detection_log[:20],
        "latest_image": latest_image_b64,
        "latest_result": latest_result
    })


if __name__ == "__main__":
    port = int(os.environ.get("PORT", 5000))
    app.run(host="0.0.0.0", port=port)
