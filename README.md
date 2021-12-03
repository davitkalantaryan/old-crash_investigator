# Crash investigations

## Introduction  
  
See details [here](docs/crash_investigations_nov_29.pdf)  
This is the newer version of the [crash-investigations](https://github.com/davitkalantaryan/crash-investigations)
The goal of this project to document and to implement some helper files in order to investigate crashes.  
Usually best way to investigate crashes is to attach debugger (if crash is reproducible) and see what is ongoing.  
From time to time this is not possible for several reasons:  
 - No proper debugger is available, because host is server host with minimal installations  
 - Crash happens in very early stage of application  
 - Crash happens inside third party library (missing the sources)  
 - Crash happens not in the time of faulty code execution. For example memory is cleaned twice by some erroneous code, then crash due to this can happen later  
Not in all cases mentioned above there is guarantied method that will solve everything, but at least here you will find some hints those should work for many cases.  
With this developer will have [valgrind](http://valgrind.org/docs/manual/quick-start.html) on hand.   
 
## Methods  
  currently `C++` part is multiplatform. While `C` part (`malloc` and friends) is implemented only for Linux.    
  -- `operator new` and `operator delete`   all variants are overloaded (works for all platforms and hopefully for all compilers)  
  -- `malloc` and friends are currently overloaded only for Linux  
  
For now system will find possible memory corruption and print log on it. Further details will be implemented
  

## Prepare repository for compilation

### Debian based
``` bash
sudo apt-get update -y
sudo apt-get install -y libdwarf-dev
sudo apt-get install -y libelf-dev
```


## Usefull links
 1. [DWARF 4 (Version 4 Debugging Format Standard)](https://dwarfstd.org/Dwarf4Std.php)  

