# filegen: predictable file generation

## Synopsis
**filegen** [**-rvVS**] [**-f** *maxfilesiz*] [**-i** *interval*] [**-p** *prefix*] [**-s** *seed*] [**-t** *totalbytes*] *directory*

## Description
**filegen** is a program to write files of random sizes and random content in a predictably way. The is to generate files for post-mortem file system analysis. **filegen** can also be used to verify the integrity of files it generated.

Every file written by **filegen** is numbered and may receive an optional prefix. By default, file number 1 will be named *f0000* and composed entirely of 0-valued words, file number 2 will be named *f0001* and consist of 1-valued words, and so on. A word contains **sizeof**(*size_t*) bytes. This behaviour allows misallocated file system blocks to be diagnosed quickly, and can be switched off by using option **-r**.

Files are written by **filegen** in chunks of random sizes and placed inside *directory*. The size of each individual chunk cannot exceed 64KB.

The options are as follows:

* **-r**
  * Randomise the content of each file.
* **-v**
  * Enable verbose mode.
* **-V**
  * Verify the content of files in *directory*. If this option is specified, **filegen** does not write data.
* **-S**
  * Call fsync(2) after writing each file.
* **-f** *maxfilesiz*
  * The maximum size of each file in bytes. The default is 16MB.
* **-i** *interval*
  * The interval between two consecutive writes in nanoseconds. The default is zero.
* **-p** *prefix*
  * The prefix used when naming files. The default is an empty string.
* **-s** *seed*
  * The seed used to initialise the sequence of pseudo-random numbers used by **filegen**. By default, a time-derived seed is used.
* **-t** *totalbytes*
  * The total amount of bytes to be written. The default is 1GB.

# Examples

One could use the script below to write 128GB worth of data in */stage* using 16 concurrent processes, with random content and each file holding 128MB at most, an interval of 0.5ms between writes, and 6010 as the seed:

```
# cat << 'EOF' > run.sh
T=8589934592    # 8GB
F=134217728     # 128MB
S=6010          # seed
I=500000        # 0.5ms
jot -c 16 A | xargs -t -n 1 -J % -P 16 filegen $1 -r -i $I -t $T -f $F -s $S -p % /stage
EOF
```

Or, on Linux with bash or zsh:

```
# cat << 'EOF' > run.sh
T=8589934592    # 8GB
F=134217728     # 128MB
S=6010          # seed
I=500000        # 0.5ms
echo {A..P} | tr ' ' '\n' | xargs -t -n 1 -I {} -P 16 ./filegen $1 -r -i $I -t $T -f $F -s $S -p {} /stage
EOF
```

It would then be possible to dispatch the processes using:

```
# sh run.sh
```

After a simulated (or actual) outage, the files could be verified using:

```
# sh run.sh -V
```

