# Hardware
## General Information 

Warning: This project uses lasers. Be sure to **always** do your own testing by meassuring voltage and current and by looking in datasheets. I am **not** responsible for any injuries you encounter from using this project. Also make sure to wear proper eye-protection.

----

**The Tagger runs using 4 1,2V NiMh batteries! Don't use voltages greater than 5V in order to power the Tagger.**

----
Double check that you've soldered the electrolytic capacitors in the right orientation.

----


## 3D Printing

All 3D files are in the FCstd format. They can be opened and edited or converted into stl using FreeCAD.

-----

## Perf matrix board
The schematic for using a breadboard is in the .diy format. These files can be viewed and edited using DIY Layout creator which you can find here:
https://bancika.github.io/diy-layout-creator/
##

-----

## The Displays

When buying Oleds from not-so-trustworthy sellers on Amazon or Aliexpress, they might claim to send you an SSD_1306 Oled, but in fact it's one with an SH_1106 controller. You notice that problem, when the pixles on the display are shifted a bit, the you can just change the Display settings to SH_1106 in the icons.h file.

## Building the Tagger
1. Make sure you have everything you need
2. Print out the two Tagger parts and the trigger
3. Remove the supports from the 3D printed parts
4. Flash your ESP8266 with the programm and the sound files
5. Solder the pins to your ESP
6. Solder all components and pin headers to the PCB or matrix board
7. Solder cables to the buttons,the TSOP and the speaker and crimp sockets at the ends if needed
8. Take one of the red laser pointer heads and tear out the laser
9. Grind down about 2mm of the laser pointer head housing 
10. Glue the infrared laser  in the housing
11. Solder cables to the LED strip and crimp sockets
12. Solder the red cable of your battery clip to the switch and connect the power pin headers on the PCB
13. Glue the lasers ,the Tsop ,the LED strip ,the speaker and the switch in place
14. Connect all your components to the PCB in the gun housing
15. Cut a 3mm diamter x 35mm piece of metal rod or pipe
16. Put in the trigger using the piece of rod
17. Tense the rubber band between the trigger and the fixture
18. Put in the battery pack with 4 AA 1,2V NiMh batteries
19. Close the Tagger and screw the two parts together
20. Slightly pull the trigger and then turn on power
21. Have fun



