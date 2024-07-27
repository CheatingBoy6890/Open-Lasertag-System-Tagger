# Hardware
## General Information 

Warning: This project uses lasers. Be sure to **always** do your own testing by meassuring voltage and current and by looking in datasheets. I am **not** responsible for any injuries you encounter from using this project. Also make sure to wear proper eye-protection.

----

**The Tagger runs using 4 1,2V NiMh batteries! Don't use voltages greater than 5V in order to power the Tagger.**

----
Double check that you've soldered the electrolytic capacitors in the right orientation.

----


## 3D Printing

All 3D source files are in the FCstd format. They can be opened and edited using <a href ="www.freecad.org/index.php">Freecad</a>.
For printing you can just use the given .stl files.

-----

## Perf matrix board

We suggest the perf matrix board version only for people who know what they're doing. If you just want to build your own tagger without changing the layout later we suggest using a PCB.
The schematic for using a breadboard is in the .diy format. These files can be viewed and edited using <a href ="https://bancika.github.io/diy-layout-creator/">DIY Layout creator</a>.

-----

## PCB

We created the pcb layout with <a href="https://www.kicad.org/">KiCad</a>. Some Online PCB services like Aisler allow just uploading the .kicad_pcb file for other you will have to export the pcb as gerber file to do this you can follow this tutorial: <a>https://jlcpcb.com/help/article/362-how-to-generate-gerber-and-drill-files-in-kicad-7</a> 

-----

## The Displays

When buying Oleds from not-so-trustworthy sellers on Amazon or Aliexpress, they might claim to send you an SSD_1306 Oled, but in fact it's one with an SH_1106 controller. You notice that problem, when the pixles on the display are shifted a bit, the you can just change the Display settings to SH_1106 in the icons.h file.

-----

## The code

To flash the program to your ESP8266 you need to have <a href="https://code.visualstudio.com/">Visual Studio Code</a> and the platformio extension installed. If you haven't you can follow this tutorial:
<a>https://randomnerdtutorials.com/vs-code-platformio-ide-esp32-esp8266-arduino/</a> To flash an ESP8266 from Windows you might need additional drivers, you'll find a tutorial on this here https://randomnerdtutorials.com/install-esp32-esp8266-usb-drivers-cp210x-windows/

## Building the Tagger
1. Make sure you have got all the required parts
1. Sclice the stl files and print them with a 3d-printer
1. Remove the supports from the 3D printed parts
1. Flash your ESP8266 with the programm and the sound files
   - Open Visual Studio Code
   - Click Open Folder and choose the Folder you downloaded: ![Open_Folder](https://github.com/user-attachments/assets/cd5b0aad-ef70-473c-af14-2572854eef7a)
   - When it askes you, click "trust the authors:  ![trust_the_authors](https://github.com/user-attachments/assets/ed0244dd-f67b-48c4-ac9c-b3127e19d472)
   - Connect the ESP8266 to your pc and click "upload" to upload the program and then press "build filesystem image" and after that "upload file system image: ![Upload_1](https://github.com/user-attachments/assets/24741195-057c-4338-a4b5-8ada0f964bb4)
1. Solder the pins to your ESP
1. Solder all components and pin headers to the PCB or matrix board
1. Solder cables to the buttons,the TSOP and the speaker and crimp sockets at the ends if needed
1. Take one of the red laser pointer heads and tear out the laser
1. Grind down about 2mm of the laser pointer head housing 
1. Glue the infrared laser  in the housing
1. Solder cables to the LED strip and crimp sockets
1. Solder the red cable of your battery clip to the switch and connect the power pin headers on the PCB
1. Glue the lasers ,the Tsop ,the LED strip ,the speaker and the switch in place
1. Connect all your components to the PCB in the gun housing
1. Cut a 3mm diamter x 35mm piece of metal rod or pipe
1. Put in the trigger using the piece of rod
1. Tense the rubber band between the trigger and the fixture
1. Put in the battery pack with 4 AA 1,2V NiMh batteries
1. Close the Tagger and screw the two parts together
1. Slightly pull the trigger and then turn on power
1. Have fun



