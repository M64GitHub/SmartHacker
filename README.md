# SmartHacker
Tool to automatically decode unknown SmartMeter data formats via "cypher known plaintext 'attack'" / intelligent brute forcing. A QT/GUI driven inspector for working with SmartMeter data. Blazingly fast! Compact C++ routines ready for embedded use!
 - GUI, decrypting, decoding works on all major platforms: GNU/linux, Windows, Mac OSX  
 - Decrypting/decoding routines work and tested on STM32, ESP32, multiple versions each

## Quick info

 - decode blocks of SmartMeter data
 - can read data via serial port
 - works and tested under windows, gnu/linux, mac OSX
 - automatically finds position of fields "SYSTEM TITLE", "FRAME COUNTER", begin of encrypted data
 - visually displays / highlights the DLMS / COSEM entries in hex data buffers
 - allows to test / find implementations of DLMS / COSEM decoding routines bypassing all layers
 - decoding done using compact / portable C++
 - hence: allows for blazingly fast and embedded decoders, using small portable code

## Please note
 - Very personal work tool, GUI would need a bit more serious error handling, written in "write and use mode" mostly
 - Please take as is for now, maybe not going to be updated a lot though
 - Please see 'LICENSE.txt' on how to use the routines in your own projects.

![SmartHacker](https://github.com/M64GitHub/SmartHacker/assets/84202356/7b7e9b37-7d94-4fef-a81c-bf5bbbd0e065)
