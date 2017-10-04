# filegen: predictable file generation

## Synopsis
**filegen** [**-rvVS**] [**-f** *maxfilesiz*] [**-i** *interval*] [**-p** *prefix*] [**-s** *seed*] [**-t** *totalbytes*] *directory*

## Description
**filegen** is a program to write files of random sizes and random content in a predictably way. The is to generate files for post-mortem file system analysis. **filegen** can also be used to verify the integrity of files it generated.

Every file written by **filegen** is numbered and may receive an optional prefix. By default, file number 1 will be named *f0000* and composed entirely of 0-valued words, file number 2 will be named *f0001* and consist of 1-valued words, and so on. A word contains **sizeof**(*size_t*) bytes. This behaviour allows misallocated file system blocks to be diagnosed quickly, and can be switched off by using option **-r**.

Files are written by **filegen** in chunks of random sizes and placed inside *directory*. The size of each individual chunk cannot exceed 64KB.

The options are as follows:

* **-r**
  * Randomise the contents of each file.
* **-v**
  * Enable verbose mode.
* **-V**
  * Verify the contents of files in *directory*. If this option is specified, **filegen** does not write data.
* **-S**
  * Call fsync(2) after writing each file.
* **-f** *maxfilesiz*
  * The maximum size of each file in bytes. The default is 16MB.
