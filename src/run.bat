@echo off
REM Upload an SREC file to the C256 Foenix

python ..\..\FoenixMCP\C256Mgr\c256mgr.py --port COM13 --binary fte.bin --address 20000
