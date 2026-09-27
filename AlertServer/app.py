from flask import Flask, request, jsonify
import time

app = Flask(__name__)

# Global state to store the most recent alert
latest_alert = {}

@app.route('/data', methods=['POST'])
def receive_data():
    """
    ESP32 posts data here. 
    Example JSON: {"lat": 40.7128, "lon": -74.0060}
    """
    global latest_alert
    data = request.json
    if data:
        # Generate a unique alert ID based on current timestamp
        data['alert_id'] = int(time.time() * 1000)
        latest_alert = data
        print(f"New Alert Received: {latest_alert}")
        return jsonify({"status": "success", "alert_id": data['alert_id']}), 200
    return jsonify({"status": "error", "message": "No data received"}), 400

@app.route('/get', methods=['GET'])
def get_data():
    """
    Android App polls this route every 3 seconds.
    """
    return jsonify(latest_alert), 200

@app.route('/clear', methods=['POST'])
def clear_data():
    """
    Optional: Clear the current alert from the server
    """
    global latest_alert
    latest_alert = {}
    return jsonify({"status": "cleared"}), 200

if __name__ == '__main__':
    # host='0.0.0.0' allows external devices (like your phone) to connect
    app.run(host='0.0.0.0', port=5000, debug=True)