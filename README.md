# DigitalNAAT
Source code for smartphone-enabled digital CRISPR device:
1. Source code of the smartphone app (developed by MIT App Inventor 2)
2. Source code of the TinyPICO ESP32 microcontroller (developed by Arduino IDE)
# How to reproduce
1. Import the smartphone app source code (file .aia) to your MIT App Inventor 2 (https://ai2.appinventor.mit.edu/) projects and from MIT App Inventor 2, build an .apk, download, and install the app onto your android smartphone. There could be security warnings but just install anyway. After app installation, go to Phone's Settings -> select the app DigitalNAAT -> add Bluetooth, Camera, and maybe Location...to the app's Permissions.
2. Download the TinyPICO ESP32 microcontroller soucre code onto your TinyPICO ESP32 microcontroller using Arduino IDE.
3. Open the app and connect with the ESP32 using Bluetooth (you may need to adjust Bluetooth settings of your phone for Bluetooth to work).
4. Set up parameters for the assay and press "Start".
Note: may need to turn on an background app (e.g., play a Youtube video in the background) to prevent the phone from going to sleep.

For more info about the device and program flowcharts, see https://www.medrxiv.org/content/10.1101/2023.05.12.23289911v1
