# SmartHacker
Tool to automatically decode unknown SmartMeter data formats via "cypher known plaintext 'attack'" / intelligent brute forcing. A QT/GUI driven inspector for working with SmartMeter data. Blazingly fast! Works on all major platforms: win, lin, OSX

## Quick info

This readme has to be updated. The tool allows for
 - decode blocks of SmartMeter data
 - can read data via serial port
 - works and tested under windows, gnu/linux, mac OSX
 - automatically finds position of fields "SYSTEM TITLE", "FRAME COUNTER", begin of encrypted data
 - visually displays / highlights the DLMS / COSEM entries in hex data buffers
 - allows to test / find implementations of DLMS / COSEM decoding routines bypassing all layers
 - decoding done using compact / portable C++
 - hence: allows for blazingly fast and embedded decoders, using small portable code

## Please note
 - Very personal project, needs a bit more error handling for making good OSS code
 - Please take as is for now, maybe not going to be updated a lot though
 - (I know how to segfault it in multiple ways, it was just a personal work tool)
 - definately a short usage guide will be provided, including screenshots

![image](https://github.com/M64GitHub/SmartHacker/assets/84202356/1c82d41e-4023-4e92-ab65-3ec2108bea21)
