# uJava-VM
A Java VM for microcontrollers (uses code from the uJ project)

uJ - a Java VM for microcontrollers
What is uJ and why do I care?

uJ is a Java VM for microcontrollers. It implements the entirety of the java bytecode and is written entirely in C, so that it may run on any processor. It has been tested and found functional on 8-bit AVR devices (ATmega64, Atmega128), some Microchip 16-bit PIC devices (dsPIC33FJ128GP802, dsPIC33FJ64GP802, PIC24FJ32GA002), and a few other architectures. It is quite efficient, fast, and usable for actual real-life situations. Minimum requirements? A few dozen K of code space and a few hundred bytes of RAM is all you need to get started. More is better, as always.

Why?

Lots of people want to program microcontrollers, few understand memory management, peripherals, and C in general. Several crutches exist for this (PICAXE, Arduino, etc), none of which really address the problem that many people still have no idea how to use pointers and other C-level code, but still want performance and ease of programming. Java is very well known, and most people studying in universities today learn just it (sad but true). So I set out to make a fully working JVM for a microcontroller. The goals were simple: complete support of the java bytecode, including exceptions, inheritance, interfaces, synchronization, threading, garbage collection, arrays (including multi-dimensional ones), etc...

How?

There were a few interesting challenges in the project, such as fitting into the code and data memory limits, reasonableness of speed, and in general the design. The VM is very modular, wherein various pieces can be disabled or enabled to tune the VM features and size. One of the major design guidelines was not copying class files to RAM. This would allow devices with very little RAM to still run the JVM. Java class files, however, are very unpleasant to parse, since for all effective purposes they are sequential-access only. All data structures are variable length, so to get to the N-th method, you have to read lots of file pieces before then (skiping all constants, previous methods, etc...). Some ways of speeding this up exist and are used by the VM, but a better method would be a better container format. uJ supports this better container format: UJC.

Details
Code Size

The VM size depends on the options selected and the CPU architecture at hand. For example a VM with all options enabled takes up about 80K of codespace on an AVR device, and only 60K on a PIC24 device. To fit into an Atmega64, disabling the "DOUBLE" is all it takes. Of course, if you need double, you can disable something else like "FLOAT" and "LONG" to free up the needed code space. You can also play with optimization options. For example Microchip's "Procedural Abstraction" optimization can reduce the code size by 30%, but at the cost of a 10% decrease in speed.

Speed

The VM speed is a difficult thing to measure - there are a lot of factors to think about and variables to consider. For purely arithmetic operations (addition, subtraction) the measurements are simple and depend pretty much on the CPU you use (16-bit CPUs are faster than 8, more MHz is better than less). More complex operations (floating point, multiplication, division) get murkier, since they start depending on the quality of the library you use. AVR's GCC's floating point and long-long code is very slow (which is why uJ has replacement 64-bit code for AVR hand-coded in assembly). AVR GCC also has no support for 64-bit double, so uJ has its own implementation, but keep in mind that it's wrong to expect stellar performance out of 64-bit floating point calculations (written in C) on a measly 8-bit CPU. Microchip's compiler for PIC24/dsPIC produces amazing floating point and double performance, however. And when you start talking about interface method invocations and exception handling, it gets even messier, depending on how well the compiler optimized the code. But no matter how well it does, method invocations are a lot of work in java. Now, with the scary technical details out of the way, onwards! On an ATmega64 (20 MHz), running a worst-case workload (10 threads, lots of function calls) the VM manages an average of 18,900 java ops per second. On a more realistic scenario (mostly numeric computation in a single thread, with some IO to an HD44780 display) it managed 50,000. On PIC24/dsPIC (at equal clockspeed) performance is between 3-4x better than the AVR (6x if you are doing a lot of math on double-precision floating-point numbers).

RAM

The VM memory footprint is of utmost importance on a microcontroller, of course. To provide the kind of heap a java VM requires, it is pointless to try and use a normal C-style heap, so uJ implements its own heap manager. This means that your microcontroller need not have an implementation of malloc/free for uJ to work. All uJ memory is allocated in uJ's heap. This makes it very easy to limit uJ to a certain size, or to use the maximum memory that your microcontroller will give you efficiently. All chunks in uJ's heap can be relocated and are referenced by handles. This allows the garbage collector to move memory chunks that are not locked, which is essential when you only have a few kilobytes of RAM. Each heap chunk's header is 2-4 bytes depending on heap size and heap alignment requirements. Each loaded class takes up about 28 bytes (plus any static variables it may have). All objetcs are about 2 bytes more than the sum of the sizes of their instance variables. Each thread takes up about 20 bytes plus stack size (192 bytes by default) plus 3%. Thus even on a small memory footprint it is possible to run reasonably-sized programs.

Garbage Collector

The uJ garbage collector is optimized for code size and small heaps. If you give it many megabytes of memory, it will get quite slow - it is not made for such a case. The GC itself is a simple design. Each chunk has a 3-state "mark" field. First we set all of then to zero. Then we walk all the threads' stacks and mark all objects we discover to "1", same for global static variables. Then we loop until no more chunks are left with a "1" mark: find first chunk with a "1" mark, mark it "2", mark all of its children as 1. At the end of this the heap will have objetcs marked "2" and marked "0". All those marked "0" can be freed. Then all unlocked chunks are moved towards the end, so as to make more contigious heap space. Classes are all loaded at the start of the VM, and are non-movable chunks in the end of the heap. Being in the end is how they manage to not fragment the heap space.

Native Interface/Drivers

A programmable device is pointless without a way to interact with the environment. uJ has multiple ways for the java code to interact with the world. First and foremost, there is ability to add classes in C that are accessible by java, so you can have an I2C, SPI, and UART classes, as well as anything else you can think of. These classes can be called by java, just like normal java classes, and then in your C code you can pop your parameters off the stack, do your work, and push results back onto the stack. API exist to access primitives and arrays. Technically you can call java code from these C functions too, but I am not currently encouraging such deplorable things. (I am not trying to reinvent JNI here). Native classes are allowed to have static and instance variables, as well. Alternatively, and to allow more control over the host microcontroller, you can export native functions like readByte and writeByte to allow direct memory writes and left. This can be used to write UART, SPI, I2C, and ADC drivers in java directly, and thus save codespace in the host microcontroller. How do you compile a java class that needs to use a uJ native class, since javac knows nothing about uJ? You create a stub class in Java, which has the same interface as your native class, but no implementation of any methods (empty method bodies). This allows javac to compile against those, and you need not include those class files in the files you give to uJ for execution. The basic uJ classes already have such stub classes, and a simple build environment exists to use them. See the BUILDENV folder and the Makefile in it for that.

Strings

As pointed out above, any class can be either native or Java. uJ does not currently support classes that are both, so that leaves us in a pretty unpleasant place regarding the String class. String is the only class in Java that the JVM needs to construct by itself. However, String has a lot of methods that would waste a lot of code space to implement. How do we solve this problem? There is a native class uj.lang.MiniString that String inherits from. MiniString implements the most basic things a string does - storing a set of bytes and accessing them. It implements, by default, just three methods - a constructor, XbyteAt_, and Xlen_, which do exactly what you'd expect them to. The rest of the string class is implemented in Java, including UTF-8 parsing from bytes and all. Yes, indeed this can be slow, so the VM has an option to instead implement length() and charAt in C, significantly speeding those up.

Threads

uJ implementes threading, and you can run as many threads as you can fit into RAM. I did not [yet?] implement the entire Thread class, so right now a thread starts immediately when you create a new instance of Thread. This seems fine for me, for now, and I'll work on expanding the thread class soon. Threading introduces synchronization issues, of course, and uJ is ready! The synchronized keyword is fully supported in all possible cases, and functions as expected.

Runtime Exceptions

Java has a set of exceptions that the runtime can throw, like NullPointer and DivideByZero. uJ could throw those, but that would require you to have a class file for each, wasting 28 bytes of RAM, so instead uJ terminates the runtime and returns the error. This can easily be changed (all the code is there) but I choose not to do this by default. If you want to enable this, it is very easy.

UJC

Java class file format is incredibly inefficient to use as is. It was never really meant to be used directly, from what I can tell. It is meant to be loaded into a complex structure in RAM. Needless to say, I haven't got any desire to waste RAM like this - it does not grow on trees. This leaves us with a problem - very inefficient ways of accessing class data (methods, fields, interfaces, etc...). uJ still supports java class files, but it is slow. To solve the problem, at least partially, a different format is used: UJC. UJC stands for "uJ class". Not too creative, I know. What is so special about it?

    Some file-level optimizations
        Unused constants from the original class file are discarded, to avoid empty slots in the constant table, constants are renumbered and the code is adjusted accordingly.
        A table of contents is created for constants, methods, and fields so that the N-th constant can be located in the file without reading the previous N-1.
        Some constants at times refer to other constants, for example nametypeinfo refers to name and type by index. This introduces extra lookups. Such references are replaced with direct offsets, removing the unneeded constant lookups. This places us in a position where we need the constant data for some constants, but they are never referenced by index. We can do that, thanks to the abovementioned constant renumbering - they are stored in the file, but have no index pointing to them.
    Some instruction-level optimizations
        Some constants are moved directly into the code: ldc and ldc2 that load floats or integers are replaced with a custon instruction 0xFE, folowed by the 4 bytes of the constant itself. This significantly speeds up usage of integers > 32767 in java code.
        Static and private method calls to the same class's methods are optimized by finding the index of the method in the class, so that we no longer need to search for it by name. This is impossible for virtual and interface functions, of course. This variant of the invokestatic and invokespecial instructions is encoded using a wide prefix instruction before them.
        Field accesses in the same class (static and instance both) are optimized, replacing the constant index describing the variable with just a type and offset, since it is known already. This variant of the getfield, putfield, getstatic and putstatic instructions is encoded using a wide prefix instruction before them.

So what does this give us? The resulting files are smaller (20% typical) and much faster (depending on the workload, up to 3x). How do we do all this? The tool I created is called classCvt and it is a rather cool piece of code. It reads the java class file into RAM, with complete understanding of what each chunk of it is. Then it goes over all the code pieces, and breaks them into code blocks. A code block is a piece of code which has a single entry point; that is to say there exist no jumps anywhere else that land you in the middle of this chunk of code. Once we have these blocks, we know for sure that we can freely change their lengths, without disturbing any jumps. We then go over them and modify the instructions as we see fit. We also note which constants from the pool get used, so that we can throw away the ones that aren't and rearrange the rest. After we're done with all the changs, we can reassemble the code back into a single stream. This is not very complex, since, except the first one, the rest of the chunks can be arranged in any order. One possible caveat here: java jumps use 16-bit offsets, so if somehow our changes grow the code size, we can end up in a situation where we cannot fit some jumps into the offset. In this case classCvt will print an error, as expected.

Class Loading

Classes, somehow, need to end up being available to the VM. Making this happen is your job. To the VM you provide a function ujReadClassByte, which the VM will call to read class bytes. Whenever you attempt to load a class file, you'll pass a "descriptor" to ujClassLoad function. uJ does nothing with the descriptor, but it does get passed back to your callback function, so that you may use it. A good thing to store here would be a FILE* or perhaps an EEPROM offset, flash offset, or maybe a pointer to a complex structure describing where you stored the class file. The relevant part is that you need to make this function available. Classes have dependencies. When you try to load a class, the return value has a lot of meaning. It may be UJ_ERR_NONE, meaning that all is well, or it may be UJ_ERR_DEPENDENCY_MISSING, meaning that you have not yet loaded a dependency. Keep loading your other classes and then try this one again later. You have failed when all the "remaining-to-load" classes fail in this way. That means you're missing a dependency. As you can see the VM itself does not care where you store the classes and how you read them. The class files are never written to by the VM.

Future work

    Proper Thread support. This is not hard to do - just some boring code to write.
    A board support file for ArduinoMega. This is a work in progress, since this would be an easy board to run uJ on to try it.
    Support for larger heaps. Current heap manager will lose efficiency somewhere about the 2-4MB mark. Luckily most microcontrollers do not have nearly that much RAM.
    More work on base classes?
    Support for mixed native/java classes

Feature options

uJ has many build options that determine what VM will and will not support. If you want float support, your compiler must support IEEE-compliant "float" type, as uJ does not provide a software implementation of that. uJ does provide a software implementation of 64-bit double though. Also uJ has a few architecture-based options which are self-explanatory:

    Does compiler support 64-bit long long?
    Does compiler support 64-bit double?
    Preferred alignment

The build options are summarized in the following table:
FEATURE	USAGE	IMPACT
ENABLED	DISABLED
DOUBLE 	Using double-precision floating point values. 	Opcodes become available: dconst_0 dconst_1 dadd dsub dmul ddiv drem dneg i2d l2d f2d d2i d2l d2f dcmpl dcmpg 	Code size smaller (in some cases, significantly)
FLOAT 	Using single-precision floating point values. 	Opcodes become available: fconst_0 fconst_1 fconst_2 fadd fsub fmul fdiv frem fneg i2f l2f d2f f2i f2l f2d fcmpl fcmpg 	Code size smaller
LONG 	Using 64-bit long values. 	Opcodes become available: lconst_0 lconst_1 ladd lsub lmul ldiv lrem lneg land lor lxor lshr lshl lushr i2l f2l d2l l2i l2f l2d lcmp 	Code size smaller (not alaways by significantly much)
LONG & DOUBLE 	(If either is enabled) 	Opcodes become available: ldc2_w lload dload lload_0 lload_1 lload_2 lload_3 dload_0 dload_1 dload_2 dload_3 laload daload lstore dstore lstore_0 lstore_1 lstore_2 lstore_3 dstore_0 dstore_1 dstore_2 dstore_3 lastore dastore lreturn dreturn 	Code size smaller
EXCEPTIONS 	Using exceptions 	Opcodes become available: athrow 	Code size smaller
SYNCHRONIZATION 	Enables the VM to honor synchronization-related keywords in all of their uses 	synchronized keyword is honored instead of being ignored. Code size is larger. Objects and classes on heap are 4 bytes larger 	Code size slightly smaller, objects on heap are smaller.
RAM STRINGS 	VM copies constant strings to RAM 	Code size smaller, this puts a bit more pressure on the garbage collector, but speeds up all string-related tasks and then each string in RAM uses 4 bytes less. 	Code size slightly larger, each string uses 4 bytes more of RAM, but you may use constant strings larger than VM's available RAM size easily
CLASS SEARCH HASH 	Speeding up class (and method in some cases) lookup 	Code size slightly larger, but faster class/method calls 	Code size smaller
FILE FORMAT: class 	Using unmodified class files produced by java compilers like javac or ecj 	.class files supported 	Code size smaller
FILE FORMAT: ujc 	Using UJC class files produced from .class filed by the classCvt tool 	.ujc files supported 	Code size smaller
STRING FEATURES 	Implement charAt() and length() in C 	charAt() and length() need not be implemented in Java - faster operation 	Smaller code size in C, java String() must implement charAt() and length()
Building/Hardware

uJ will build on PC, as well as any other platform that has a C compiler. I've tested this on AVR8, PIC24, dsPIC33, and Cortex-M3 platforms. They all worked as expected. I've prototyped two development boards for uJ. There is an AVR ATmega1284P/644A-based Diana board and a PIC24/dsPIC33-based Helsinki board. Both boards have an SD card, and store java code in the microcontroller's flash. They feature a bootloader that copies the java files from the SD card to flash. They code then runs from there, and the SD card is free for it to use. Since uJ does not load code into RAM, you can have as much code as you wish (I have implemented a full-featured FAT filesystem and a few games easily on the AVR-based board). uc_main.c has the main() for microcontrollers, and is the starting point for making your own board. It, among other things, exports a native class called UC which gives the java code access to EEPROM, GPIO, clocks, displays, i2c, etc...
Diana board

This is meant to be the simpler, lower-cost board to try uJ, and you can put one together relatively easily. The MCU is clocked at 20MHz, executing approximately 20 million instructions per second. The UART is available, 8 red LEDs, one dimmable white LED, a few buttons, and an HD44780 20x2 display. The SD card is connected to PORTB thusly: MISO - B0, MOSI - B1, CLK - B2. HD44780 is connected to PORTA thusly: EN - A4, D4 - A2, D5 - A1, D6 - A0, D7 - A3, RS - A5. The eight red LEDs are connected to PORTD. There are two PWM channels exported to java - one for the LCD backlight and one for a white LED on the board. The LCD backlight is on pin B3 and the white LED on B4. Three buttons are connected to C2, C3, and C4. This board also exposes i2c, EEPROM, and raw RAM access to Java.

So what can you do with one of these? Well, as a test I had a simple app that ran a few threads - one went through the eight LEDs and blinked them, another calculated prime numbers, and main did an ADC sample and calculated the battery voltage, and then ran forever printing the current time. Three java threads at once is not bad at all. You can find this under DianaTest.java. Another test I ran on this board was a set of ten threads running at once, each with a prime period in ticks, toggling an LED, causing it to flicker in a cool way. Another thread uses uJ's profiling API (UC.instrsGet()) to measure the number of instructions executed per second on average and logs it. This test you can find under CoolTest.java. A third test was a 20-questions style learning game that tries to guess the animal you're thinking of, by asking you yes-or-no questions. If it fails to guess, it asks you to input the name and a question distinguishing your animal from its last guess. This is then memorized, and the game starts over. Over time the game can learn quite a lot. The data is stored in the EEPROM, and thus persists across power cycles. You can find this example under Animals.java.
Helsinki board

This is meant to be a faster, more complex board for uJ development. It supports either a PIC24 or a dsPIC33 CPU at 32 or 100MHz respectively. Unlike the AVR, though, these CPUs execute only half an instruction per cycle, so their effective speeds are 16 and 50 million instructions per second. However, these are 16-bit CPUs, so 32-bit operations are much faster here. Also Microchip's compiler implements double math properly, so it is a lot faster on this board than it is on the Diana board. Hardware-wise, this board features a 128x64 OLED LCD on the i2c bus, a few LEDs, and, of course, an SD card. The wiring for the SD card is: MOSI - B13, MISO - B14, SCK - B14. i2c wiring is: SDA - B8 SCL - B8. PWMed LED is on B5. We also implement here a simple 8x6 font, so that java can treat this graphic LCD as a text LCD as well, if it so pleases.

What can you do with one of these? Well, I connected a Wii nunchuck to the i2c port and used it to create a simple drawing application, wherein you can move a blinking pixel onscreen using the analog stick, press the C button to draw while you move, Z to erase. It runs very fast and looks very cool. Another cool thing this example shows is the convenience of having a garbage collector - no freeing of array is needed - cool. You can find this code in Wii.java.
Make your own

To use uJ in your own project, you'll need to provide a few pieces of code. First, you'll want to figure out where to store the class files, and how to get them there. That part is completely up to you, though you can use uc_loader.c as an exaple of storing classes in the flash of the microcontroller and putting them there from the SD card. You are responsible for, at boot, calling ujInit() to initialize uJ. At this point you can inject your own native classes, if you so wish. This is where you provide access to your hardware to java. Feel free to use uc_main.c as an example of doing that. After this, you should start loading java classes. Call ujLoadClass() on each, and remember the ones that fail with UJ_ERR_DEPENDENCY_MISSING. Re-try them later - you may have just not yet loaded their dependencies. After you're all done with that, and all classes loaded without errors, it is time to initialize all classes. Use ujInitAllClasses() for this. Now you can start the VM. Create the main thread via a call to ujThreadCreate() and have it start running main, or whatever your entry point is via a call to ujThreadGoto(). As previously, uc_main.c can be used as an example of doing this. At this point everything is ready, and you can let the VM go. Code like this is what you need now: [while(ujCanRun() && EJ_ERR_NONE == (err = ujInstr()));]. If that loop ever exits, it means your code finished or an error occured. Examine the value of err for details. Of course you'll also need to implement ujReadClassByte() that actually reads bytes from whereever you store your class files.

License

This project was made in hopes of being useful and educational, and as such is released under the following license. You may use this code for any use, provided the following conditions are met:

    uJ's source and all your modifications are published together with your project, or made available for free to anyone who requests them in a form complete enough to build and run as your device/product does.
    uJ cannot be used in a commercial product/service without first obtaining permission from me (dmitrygr@gmail.com).
    If you use this project in any physical device, you must, in the written materials accompanying it (box inset, manual, or some such thing) include a notice that your device uses code from the uJ project, and a link to http://dmitry.gr. If you use uJ in a purely software project, you must include such a notice in your readme file or manual.
    These license terms, unmodified in any way, always accompany the code and apply to it.

If you would like to license this code under different terms, contact me, of course. http://dmitry.gr
