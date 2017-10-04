# filegen: predictable file generation

## Synopsis
**filegen** [**-rvVS**] [**-f** __maxfilesiz__] [**-i** __interval__] [**-p** __prefix__] [**-s** __seed__] [**-t** totalbytes] __directory__

## Description
**filegen** is a program to write files of random sizes and random content in a predictably way. The is to generate files for post-mortem file system analysis. **filegen** can also be used to verify the integrity of files it generated.

Every file written by **filegen** is numbered and may receive an optional prefix. By default, file number 1 will be named __f0000__ and composed entirely of 0-valued words, file number 2 will be named __f0001__ and consist of 1-valued words, and so on. A word contains **sizeof**(__size_t__) bytes. This behaviour allows misallocated file system blocks to be diagnosed quickly, and can be switched off by using option **-r**.
