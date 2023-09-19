# SmartHacker
Tool to automatically decode unknown SmartMeter data formats via "cypher known plaintext 'attack'" / intelligent brute forcing. A QT/GUI driven inspector for working with SmartMeter data. Blazingly fast!

## Quick info

This readme has to be updated. The tool allows for
 - decode blocks of SmartMeter data
 - automatically finds position of fields "SYSTEM TITLE", "FRAME COUNTER", begin of encrypted data
 - visually displays / highlights the DLMS / COSEM entries in hex data buffers
 - allows to test / find implementations of DLMS / COSEM decoding routines bypassing all layers
 - decoding done using compact / portable C++
 - hence: allows for blazingly fast and embedded decoders, using small portable code
