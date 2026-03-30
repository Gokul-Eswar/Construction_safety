# 🏗️ Construction Safety System: User Manual
**Version 2.2 (2026 Production Update)**

Welcome to the Construction Safety System. This manual will guide you through setting up and operating the AI-powered safety monitoring system. No technical background is required.

---

## 1. System Overview
This system uses Artificial Intelligence (AI) to monitor construction sites in real-time. It detects workers and sends alerts if they enter restricted "Danger Zones."

**Key Features:**
*   **Wired & Wireless Support:** Works with standard USB cameras and professional Network (RTSP) cameras.
*   **Native TensorRT Acceleration:** Powered by NVIDIA GPUs for ultra-fast, real-time person detection.
*   **Smart Detection:** Only tracks people. Birds, vehicles, and moving machinery are automatically ignored.
*   **Danger Zones:** Customizable areas that trigger alerts when stepped into.
*   **Automatic Recording:** Logs every safety violation with a timestamp for later review.

---

## 2. Quick Start Guide (The "One-Click" Start)

To start the system, follow these three steps:

1.  **Plug in your cameras:** Ensure your USB cameras or network cameras are connected.
2.  **Run the System:** Double-click the file named `start_system.bat` in the main folder.
3.  **Open the Dashboard:** Open your web browser (Chrome or Edge) and type:
    `http://localhost:3001`

---

## 3. Advanced AI Optimization (TensorRT)

The system now includes **Native TensorRT** optimization. This allows the AI to run significantly faster on NVIDIA hardware.

*   **First Run:** The very first time you start the system with a new AI model, it may take 2-5 minutes to "optimize" the model for your specific computer. You will see a message saying "Building TensorRT engine."
*   **Subsequent Runs:** After the first run, the system will start almost instantly by loading the pre-optimized "Engine" file.
*   **Manual Optimization:** If you want to optimize a new model without starting the full camera system, you can run:
    `SentinelEngine.exe --build-engine-only`

---

## 4. Managing Your Cameras

You can now add both professional network cameras and simple wired USB cameras.

### How to add a camera:
1.  On the Dashboard, click on **"Camera Management"** in the sidebar.
2.  Click the **"Add Camera"** button.
3.  **Fill in the details:**
    *   **Friendly Name:** Give it a name (e.g., "Main Entrance" or "Crane Area").
    *   **Camera Type:** 
        *   Choose **RTSP** for professional network cameras.
        *   Choose **USB/Wired** for cameras plugged directly into the computer.
    *   **URI / Device:**
        *   For **RTSP**, enter the address (e.g., `rtsp://admin:password@192.168.1.50`).
        *   For **USB**, simply enter `0` for the first camera, `1` for the second, etc.
4.  Click **Save**. The system will automatically start the new feed.

---

## 5. Setting Up Danger Zones

A "Danger Zone" is a virtual fence. If a worker's feet cross this line, the system sounds an alarm.

1.  Go to the **"Zone Editor"** page.
2.  Select the camera feed you want to draw on.
3.  Click on the video image to place "dots." Connect the dots to create a shape around the restricted area.
4.  Give the zone a name (e.g., "Deep Pit Area").
5.  Click **"Save Zone."**

---

## 6. System Control & Remote Restart

If you change major settings or if a camera becomes unresponsive, you can restart the AI engine remotely:

1.  Go to the **"Settings"** page on the Dashboard.
2.  Click the **"Restart Inference Engine"** button.
3.  The system will send a secure signal to the AI engine to reboot. This takes about 5-10 seconds.

---

## 7. Understanding the AI (Edge Case Handling)

The system is designed to be highly "smart" to avoid annoying false alarms:

*   **The "Bird" Problem:** If a bird flies across the camera, the AI identifies it as a bird and **does not** trigger an alert. It only looks for the "Person" shape.
*   **The "Leaning" Problem:** If a worker stands *outside* a zone but leans their head *into* it, the system **will not** alert. It only checks the "Footprint"—where the person is actually standing on the ground.
*   **Night Mode:** The system works in low light, but for best results, ensure your cameras have "Infrared (IR) Night Vision" enabled.

---

## 8. Troubleshooting

| Issue | Solution |
| :--- | :--- |
| **Black Screen** | Ensure the camera is plugged in. For USB cameras, try changing the "Device Index" from `0` to `1`. |
| **Too many alerts** | Increase the "Alert Cooldown" in the **Settings** page so the system doesn't beep every second for the same person. |
| **System is slow** | Ensure you have an NVIDIA GPU with the latest drivers. The system relies on "TensorRT" for speed. |
| **Can't open Dashboard** | Ensure you ran `start_system.bat` first and that it didn't show any red error messages. |
| **"System Offline" Alert** | The AI engine may be starting up or optimizing. Wait 1-2 minutes. If it persists, check your MQTT broker connection. |

---

## 9. Safety Disclaimer
*This system is a safety **assistant** and should not be the only method of ensuring worker safety. Always follow standard site safety protocols and use human spotters where necessary.*
