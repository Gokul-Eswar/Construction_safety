# 🏗️ Construction Safety System: User Manual
**Version 2.1 (2026 Update)**

Welcome to the Construction Safety System. This manual will guide you through setting up and operating the AI-powered safety monitoring system. No technical background is required.

---

## 1. System Overview
This system uses Artificial Intelligence (AI) to monitor construction sites in real-time. It detects workers and sends alerts if they enter restricted "Danger Zones."

**Key Features:**
*   **Wired & Wireless Support:** Works with standard USB cameras and professional Network (RTSP) cameras.
*   **Smart Detection:** Only tracks people. Birds, vehicles, and moving machinery are automatically ignored.
*   **Danger Zones:** Customizable areas that trigger alerts when stepped into.
*   **Automatic Recording:** Logs every safety violation with a timestamp for later review.

---

## 2. Quick Start Guide (The "One-Click" Start)

To start the system, follow these three steps:

1.  **Plug in your cameras:** Ensure your USB cameras or network cameras are connected.
2.  **Run the System:** Double-click the file named `start_system.bat` in the main folder.
3.  **Open the Dashboard:** Open your web browser (Chrome or Edge) and type:
    `http://localhost:3000`

---

## 3. Managing Your Cameras

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

## 4. Setting Up Danger Zones

A "Danger Zone" is a virtual fence. If a worker's feet cross this line, the system sounds an alarm.

1.  Go to the **"Zone Editor"** page.
2.  Select the camera feed you want to draw on.
3.  Click on the video image to place "dots." Connect the dots to create a shape around the restricted area.
4.  Give the zone a name (e.g., "Deep Pit Area").
5.  Click **"Save Zone."**

---

## 5. Understanding the AI (Edge Case Handling)

The system is designed to be highly "smart" to avoid annoying false alarms:

*   **The "Bird" Problem:** If a bird flies across the camera, the AI identifies it as a bird and **does not** trigger an alert. It only looks for the "Person" shape.
*   **The "Leaning" Problem:** If a worker stands *outside* a zone but leans their head *into* it, the system **will not** alert. It only checks the "Footprint"—where the person is actually standing on the ground.
*   **Night Mode:** The system works in low light, but for best results, ensure your cameras have "Infrared (IR) Night Vision" enabled.

---

## 6. Reviewing Safety Logs

To see past violations:
1.  Click on **"Safety Logs"** in the Dashboard.
2.  You will see a list of every time someone entered a danger zone.
3.  Each entry shows:
    *   Which camera caught it.
    *   Which zone was violated.
    *   The exact time and date.

---

## 7. Troubleshooting

| Issue | Solution |
| :--- | :--- |
| **Black Screen** | Ensure the camera is plugged in. For USB cameras, try changing the "Device Index" from `0` to `1`. |
| **Too many alerts** | Increase the "Alert Cooldown" in the **Settings** page so the system doesn't beep every second for the same person. |
| **System is slow** | Close other heavy programs on the computer. The AI requires significant "brainpower" (CPU/GPU) to run. |
| **Can't open Dashboard** | Ensure you ran `start_system.bat` first and that it didn't show any red error messages. |

---

## 8. Safety Disclaimer
*This system is a safety **assistant** and should not be the only method of ensuring worker safety. Always follow standard site safety protocols and use human spotters where necessary.*
