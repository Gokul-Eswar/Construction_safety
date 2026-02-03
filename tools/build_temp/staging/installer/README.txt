Sentinel Safety System - Installer Package

This folder contains the setup script for the Sentinel Safety System.

To create a distributable installer:
1. Ensure the "installer" folder is inside the project root.
2. The user should run "Setup.bat" located in the project root.
3. This will launch a GUI Wizard to install the software to "C:\SentinelSafety" (default) and create shortcuts.

Note:
This installer copies the source code and configuration. The actual system requires Docker Desktop to be installed on the target machine.
The first time the installed application is run, it will pull/build the necessary Docker containers.
