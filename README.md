# Food Spoilage Detection System

This project is a **Food Spoilage Detection System** using **ESP32-CAM**. The ESP32-CAM captures images of food, detects spoilage, and sends alerts for spoiled food via **Blynk**.  

---

## Project Structure
FoodSpoilageServer/
│
├─ .venv/            # Python virtual environment
├─ model/            # Contains your ML model
│   ├─ model.tflite  # TensorFlow Lite model
│   └─ labels        # Labels file for the model
├─ env/              # Another environment folder
├─ README.md         # Project README
├─ requirements.txt  # Python dependencies
└─ server.py         # Python server script


---

## Arduino IDE Setup

1. Open the `.ino` file in **Arduino IDE**.  
2. Install **ESP32** by Espressif:  
   - Go to **Sketch → Include Library → Manage Libraries**, search **ESP32**, and install it.  
3. Select the board: **Tools → Board → AI Thinker ESP32-CAM**.  
4. Test the camera:  
   - Go to **File → Examples → ESP32 → Camera → CameraWebServer**.  

---

## Visual Studio Code Setup

1. Open the project folder `FoodSpoilageServer/` in **VS Code**.  
2. Set up a Python virtual environment:  
   ```bash
   python -m venv .venv
   source .venv/bin/activate   # Linux/Mac
   .venv\Scripts\activate      # Windows
pip install -r requirements.txt
python server.py
