# Large DataWrite Comparison

* This program aims at comparing two methods of writing to a 1GB file, either through the `mmap` system call, or the `write` function to simply write to a file desciptor.

* Here, `mmap()` is observed to be slightly faster than `write()` to write the same data to the file. Thus, mapping memory from pages to the file is preferred if there are sufficient resources available.
