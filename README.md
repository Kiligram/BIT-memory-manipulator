# BIT-memory-manipulator

Developed and tested in Visual Studio 2019 by Andrii Rybak

Use C++20 standard to compile the project. The project consists of 2 files: `main.cpp` and `memory_search.cpp`.

https://github.com/Kiligram/BIT-memory-manipulator


To modify the some value in the process, you must find its address. To do so, follow the following steps:

1.	Scan the process memory for the current value (`command 2: first scan for a value`)
2.	Make something in the game or process, so that the value is changed, for example make a shot or reload.
3.	Search for the new value within the addresses found in the first step (`command 3: next scan for a value`)
4.	Repeat from step 1 until the least possible number of addresses is found (ideally only one address is left)
5.  Execute `command 4: modify value on address`, enter the address found in previous step and value you want.

For more info refer to the documentation uploaded in AIS.
