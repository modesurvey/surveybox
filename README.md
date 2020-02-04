### SurveyBox

SurveyBox is the on-the-ground data collection device used by (ModeSurvey)[https://modesurvey.org]. This repo contains the information, source, and instructions for SurveyBox.

#### Setup

After plugging the charged battery into the ESP32, and closing the box, follow the steps to continue setup:

1. Open up Wifi connections on phone. If the device is able to connect automatically to a network nearby, it will flash the status change signal which is WALK, WHEELS, TRANSIT, CAR for two seconds each. Otherwise the network, wait about 30 seconds for the network prefixed with esp32 to show up.
2. Connect to this network using the password "12345678".
3. After your phone is connected you may see an autologin browser session open to a url like "[ESP32 Ip Address]/_ac". Otherwise, open up your browser and connect to gstatic.com. gstatic.com will autoresolve to the login session.
4. Once on the AutoConnect start page, click the menu bar in the top right and select "Configure New AP".
5. Once here, you will be able to connect to the wifi network of your choice by clicking on the network, and typing in the password.
6. If the connection is successful, then the status change signal will display, and the device should be able to send data to the "events-test" table in firebase. If you do not have access to this table, then a flashing light on button press should be enough to validate that the device is sending data.
7. Once you have verified that the device is connected and the buttons are still wired properly after closing the box, you can press the following sequence to enter production mode and send events to the real events table. The sequence is WALK, TRANSIT, WHEELS, CAR, WALK. The status change signal will flash, and you can know setup the device in the desired location in the store.

This functionality is primary given by (AutoConnect)[https://github.com/Hieromon/AutoConnect], and more information can be found at the listed repo.

Enjoy!

- ModeSurvey Team
