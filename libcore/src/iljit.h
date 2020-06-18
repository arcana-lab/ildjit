/*
 * Copyright (C) 2006 - 2012  Campanoni Simone
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/**
 * @file iljit.h
 */
#ifndef ILJIT_H
#define ILJIT_H

/**
 * @mainpage ILDJIT project
 *
 * \image html ildjit_logo.jpeg "" width=5cm
 *
 * \author Simone Campanoni       http://www.eecs.harvard.edu/~xan
 * \version 2.0.0
 *
 * <b> Documentation </b>
 *
 * To install ILDJIT, please follow the instructions at \ref Installation
 *
 * To learn how to use ILDJIT, please read the  <a href="modules.html">documentation</a>.
 *
 * Additionally, there is documentation used during ILDJIT tutorials, which is distributed at the web-page of the project.
 *
 * <b> Release name: </b> Sloth-merge
 *
 * <b> Project web page: </b> http://ildjit.sourceforge.net
 * \image html ILDJIT_qr_code.png ""
 */

/**
 * \defgroup Installation Installation
 *
 * The installation of ILDJIT is composed by 
 * <ol>
 * <li> Instal software required by ILDJIT
 * <li> Download the ILDJIT source manager
 * <li> Install ILDJIT
 * </ol>
 *
 * Before starting installing ILDJIT, let's create our working directory
 * @code 
 * mkdir ~/ILDJIT_Installation
 * cd ~/ILDJIT_Installation
 * @endcode
 *
 * <H3> Install software required by ILDJIT </H3>
 * The following software must be installed in the system before starting the installation of ILDJIT:
 * <ul>
 * <li> gcc
 * <li> g++
 * <li> gcc-multilib
 * <li> g++-multilib
 * <li> LLVM 3.0
 * <li> bison
 * <li> flex
 * <li> gperf
 * <li> automake
 * <li> autoconf
 * <li> libtool
 * <li> makeinfo
 * <li> pkg-config
 * <li> bzip2
 * </ul>
 *
 * We tested ILDJIT on Ubuntu 14.04. 
 * In this system, the following packages provide the software mentioned above beside LLVM 3.0:
 * <ul>
 * <li> gcc
 * <li> gcc-multilib
 * <li> g++
 * <li> g++-multilib
 * <li> bison
 * <li> flex
 * <li> gperf
 * <li> automake
 * <li> autoconf
 * <li> libtool
 * <li> texinfo
 * <li> pkg-config
 * <li> bzip2
 * </ul>
 * To install it, run:
 * @code
 * sudo apt-get install gcc gcc-multilib g++ g++-multilib bison flex gperf automake autoconf libtool texinfo pkg-config bzip2
 * @endcode
 *
 * The only software left to be installed is LLVM 3.0.
 * To install it, you can download the pre-built binaries from the LLVM website:
 * @code
 * mkdir ~/LLVM_Installation
 * cd ~/LLVM_Installation
 * wget http://llvm.org/releases/3.0/clang+llvm-3.0-i386-linux-Ubuntu-11_10.tar.gz
 * tar xfz clang+llvm-3.0-i386-linux-Ubuntu-11_10.tar.gz
 * rm clang+llvm-3.0-i386-linux-Ubuntu-11_10.tar.gz
 * @endcode
 * 
 * We need to make LLVM 3.0 binaries available to the environment.
 * @code
 * export PATH=~/LLVM_Installation/clang+llvm-3.0-i386-linux-Ubuntu-11_10/bin:$PATH
 * @endcode
 * To check you have the right LLVM binary available, run:
 * @code
 * llvm-config --version
 * @endcode 
 * You need to see the following output
 * @code
 * 3.0
 * @endcode 
 *
 * Congratulations! you are now able to install ILDJIT.
 *
 * <H3> Download ILDJIT </H3>
 * ILDJIT is downloaded and installed by the script @code ILDJIT_Installer @endcode
 * 
 * The script can be downloaded from 
 * @code
 * http://sourceforge.net/projects/ildjit/files/aika/ILDJIT_Installer
 * @endcode
 * 
 * You can copy and paste the above address in your browser or download it directly by running
 * @code
 * cd ~/ILDJIT_Installation
 * wget http://sourceforge.net/projects/ildjit/files/aika/ILDJIT_Installer
 * @endcode
 *
 * <H3> Install ILDJIT </H3>
 * First, we need to give execution permission to the ILDJIT_Installer script
 * @code
 * chmod 744 ILDJIT_Installer
 * @endcode
 * 
 * Now, we can run the script to download and install ILDJIT
 * @code
 * ./ILDJIT_Installer ./src ./bin
 * @endcode
 * The above command downloads all ILDJIT sources to the subdirectory src .
 *
 * After having downloaded all sources, the above command line installs ILDJIT in the subdirectory ./bin
 *
 * IMPORTANT: In case of error during downloading the sources, you need just to re-run the above command line and the download procedure will restart from where it stopped.
 * 
 * <H3> Check ILDJIT </H3>
 * Both the ILDJIT binary and the CIL libraries need to be accessible to use ILDJIT.
 * 
 * To check the ILDJIT binary, run
 * @code
 * ildjit
 * @endcode
 * if you can use ILDJIT, then you need to see the following output:
 * @code
 * Usage: ildjit [options] file [bytecode_arguments]
 * Options:
 * -h    --help                                           Display this usage information
 * ...
 * @endcode
 *
 * If the output is 
 * @code
 * ildjit: command not found
 * @endcode
 * then the ILDJIT binary is not accessible from your PATH environment variable.
 * To add ILDJIT to your PATH, you need to run:
 * @code
 * export PATH=~/ILDJIT_Installation/bin/bin:$PATH
 * export LD_LIBRARY_PATH=~/ILDJIT_Installation/bin/lib:$LD_LIBRARY_PATH
 * @endcode
 *
 * To check the availability of CIL libraries, we need to run a program.
 * First we need to go to the regression test directory
 * @code
 * cd ~/ILDJIT_Installation/src/ILDJITRegressionTest-1.0.0/Running/TestC
 * @endcode
 * and then we can run a CIL program
 * @code
 * ildjit erat.exe
 * @endcode
 * if you can use ILDJIT properly, then you need to see the following output:
 * @code
 * Largest prime smaller than 1000: 997
 * @endcode
 *
 * If you see the following output
 * @code
 * LOADER: ERROR = Cannot find the binary "mscorlib". Error:
 * ENOENT=No such file or directory
 * Please check the environment variable ILDJIT_PATH which contains all the paths in which the CIL libraries are stored. 
 * Aborted (core dumped)
 * @endcode
 * then the CIL libraries are not accessible.
 * To add CIL libraries, you need to run:
 * @code
 * export ILDJIT_PATH=~/ILDJIT_Installation/bin/lib/cil_libraries/cscc/lib:~/ILDJIT_Installation/bin/lib/cil_libraries/gcc4cli
 * @endcode
 *
 * <H3> Use ILDJIT </H3>
 * Congratulations! You are now able to use ILDJIT to run your CIL programs.
 * 
 * ILDJIT can be used as a static compiler as well as a dynamic one.
 * 
 * For dynamic compilation schemes, please refer to the JIT one described in \ref CompilationSchemes .
 *
 * For static compilation scheme, please refer to \ref PartialCompilationScheme .
 */

/**
 * \defgroup Overview Overview
 *
 * ILDJIT is a compilation framework designed both to support parallel compilation and to be easy to extend and customize.
 *
 * ILDJIT can be used either to run programs or to create your own tool chain.
 *
 * How the code is generated in ILDJIT is described in \ref CodeGeneration.
 *
 * In order to generate the machine code, ILDJIT provides several compilation schemes that you can rely on to build your own tool chain.
 * These compilation schemes are described in \ref CompilationSchemes.
 *
 * The main components of ILDJIT are described in \ref Components.
 *
 * The current status (e.g. platforms currently supported) is described in \ref Status.
 *
 * Options to the command \verb ildjit  are described in \ref CommandLineOptions.
 *
 * Different type of plugins can be installed and used in the ILDJIT compilation framework.
 * Each of these types is described in \ref Plugins.
 *
 * All plugins can use the public API provided by ILDJIT.
 * This API is described in \ref APIs.
 */

/**
 * \defgroup CodeGeneration Code generation
 * \ingroup Overview
 *
 * In order to generate machine code, we need to
 * <ul>
 * <li> Generate the CIL bytecode from a source language (see \ref CILCodeGeneration)
 * <li> Generate the machine code by using ILDJIT (see \ref IRCodeGeneration)
 * </ul>
 */

/**
 * \defgroup CILCodeGeneration CIL Generation
 * \ingroup CodeGeneration
 *
 *  In order to generate the CIL bytecode from a source program like C or C#, we need to use an external compiler like <a href="http://www.irisa.fr/alf/index.php?option=com_content&view=article&id=38%3Agcc4cli-a-cli-backend-for-gcc&catid=3%3Aprojects&Itemid=3&lang=en">GCC</a> or <a href="http://www.mono-project.com/Main_Page">Mono</a>.
 *
 * <B> C </B>
 * <BR>
 * To generate CIL from the C language, an extension of GCC called <a href="http://www.irisa.fr/alf/index.php?option=com_content&view=article&id=38%3Agcc4cli-a-cli-backend-for-gcc&catid=3%3Aprojects&Itemid=3&lang=en">GCC4CLI</a> is used.
 * For example, let @c myProgram.c be a C program; its CIL version can be generated by running:
 * @code
 * $ cil32-gcc -o myProgram.cil myProgram.c
 * @endcode
 *
 * <B> C# </B>
 * <BR>
 * To generate CIL from the C# language, we use <a href="http://www.mono-project.com/Main_Page">Mono</a>.
 * For example, let @c myProgram.cs be a C# program; its CIL version can be generated by running:
 * @code
 * $ mcs -out:myProgram.cil myProgram.cs
 * @endcode
 *
 * <B> Java </B>
 * <BR>
 * Todo
 *
 * <B> Python </B>
 * <BR>
 * Todo
 *
 */

/**
 * \defgroup IRCodeGeneration Code generation in ILDJIT
 * \ingroup CodeGeneration
 *
 * ILDJIT relies on the IR language (described in \ref IRLanguage) as its intermediate representation in order to decouple the source language (i.e., the CIL bytecode) and the machine code.
 *
 * You can easily manage the IR code (e.g. for code optimization, code profiling, etc...) by exploiting the plugins technology used within the system (described in the rest of this document).
 *
 * \image html ILDJIT_overview.jpeg "ILDJIT Overview" width=5cm
 *
 * The framework targets the increasingly popular ECMA-335 specification (i.e., CLI architecture, described in http://www.ecma-international.org/publications/standards/Ecma-335.htm).
 * The goal of this project is twofold:
 * <ul>
 * <li> it aims at exploiting the parallelism exposed by multi-core architectures to hide dynamic compilation latencies by pipelining compilation and execution tasks;
 * <li> it provides a flexible, modular and adaptive framework for both static and dynamic code optimizations.
 * </ul>
 */

/**
 * \defgroup RuntimeChecking Run time checking
 * \ingroup IRCodeGeneration
 *
 * ILDJIT supports the generation of code with or without performing checking at run time.
 *
 * For example, loading a value from a pointer can be performed by checking that the pointer is valid first and throws an exception if it is not valid.
 * This semantic can be found in modern languages like Java or C#.
 *
 * On the other hand, languages like C uses a different semantic where these checking are not performed and if the memory address is not valid, the program abort with an access violation (i.e., segmentation fault).
 *
 * ILDJIT includes these run time checks by default.
 * However, they can be turned off by using the <code> -R <code> option.
 * @code
 * $ ildjit -R myProgram.cil
 * @endcode
 *
 * These checks are performed by IR instructions explicitly.
 * Therefore, the <code> -R </code> option works at the front-end where the IR is generated.
 * Hence, if the IR has been already generated, this option has no effect.
 */

/**
 * \defgroup CompilationSchemes Compilation schemes
 * \ingroup Overview
 *
 * ILDJIT includes two static and two dynamic compilation schemes.
 *
 * <I> Suggestion: </I> Use \ref PartialCompilationScheme (with \ref IterativeCompilation if you want to recompile programs) for ahead-of-time compilation and JIT for the dynamic one.
 *
 * <hr>
 * <B> Static compilation schemes </B>
 * <hr>
 * Static compilation schemes are:
 * <ul>
 * <li> Static: the binary is generated and dumped into a binary output file. This compilation scheme can starts from either the input language (i.e., CIL) or the IR code.
 * @code
 * ildjit --static
 * @endcode
 * <li> AOT: first the binary is generated and installed in memory; then, the execution starts relying only on the previously generated code.
 * @code
 * ildjit --aot
 * @endcode
 * To shutdown the compiler after the generation of the binary, the option @c -x can be used. In this case, ILDJIT does not keep the compiler in memory during the execution and therefore the code cannot be recompiled at run time.
 * This option can be useful to keep the memory overhead low.
 * @code
 * ildjit --aot -x
 * @endcode
 * </ul>
 *
 * For static compilation schemes, the IR code can be stored and loaded from the file system by using the IR code cache.
 * Examples about how to use the code cache can be found here: \ref HowToUseCodeCache .
 *
 * Notice that static compilations (both <code> --static </code> and <code> --aot </code>) compile all possible methods in the program and in every linked library as well.
 * Compared to compilations of C programs (e.g. by using gcc or llvm), the static compilation in ILDJIT compiles more code because linked libraries are compiled as well.
 * Instead, when C programs are compiled, linked libraries are already available in machine code (e.g. you don't compile the standard C library every time you compile your C program).
 *
 * To avoid this significant effort of compiling everything is linked to your program, partial static compilations are possible in ILDJIT.
 * IR code can be generated at compile time just for a part of the program.
 * Section \ref PartialCompilationScheme describes how to do it.
 *
 * <hr>
 * <B> Dynamic compilation schemes </B>
 * <hr>
 * Dynamic compilation schemes are:
 * <ul>
 * <li> jit: the just-in-time scheme generates pieces of binaries as soon as the execution needs them (i.e., just in time).
 * @code
 * ildjit --jit
 * @endcode
 * <li> dla: the dynamic-look-ahead scheme generates pieces of binaries before the execution needs them but after the program starts its execution. This scheme exploits the parallelism that multicore architectures provide.
 * @code
 * ildjit --dla
 * @endcode
 * </ul>
 */

/**
 * \defgroup HowToUseCodeCache IR code cache
 * \ingroup CompilationSchemes
 *
 * ILDJIT can use the IR code cache.
 * The IR code cache is a directory in the file system where the IR code is stored.
 * Inside this directory, ILDJIT loads old IR code previously generated and/or stores new IR code newly generated.
 *
 * <BR>
 *
 * <hr>
 * <B> Set the ILDJIT IR code cache </B>
 * <hr>
 * The IR code cache is defined by the environment variable @c ILDJIT_CACHE.
 *
 * The default root directory of the IR code cache is the home of the user (e.g. /home/MYUSERNAME).
 * In order to change the directory, the environment variable @c ILDJIT_CACHE can be used:
 * @code
 * $ export ILDJIT_CACHE=/home/xan/MyNewCodeCache
 * @endcode
 *
 * <BR>
 *
 * <hr>
 * <B> Use of the ILDJIT IR code cache in static compilation schemes </B>
 * <hr>
 * When a static compilation scheme is used (e.g. --static, --aot), ILDJIT first checks if the IR code of the program has been already generated and stored into the IR code cache (e.g. /home/xan/MyNewCodeCache).
 * If the IR code is available, then ILDJIT loads that code and it follows the next steps declared by the compilation scheme in use (e.g., generate the machine code and run it).
 * Otherwise, if the IR code is not available in the IR code cache, ILDJIT generates and optimizes the IR, it stores the optimized IR to the code cache and then it follows the next steps (e.g. generate the machine code and run it).
 *
 * <BR>
 *
 * <hr>
 * <B> Example </B>
 * <hr>
 * Consider the following case where we setup a custom code cache.
 *
 * Let's assume that we have a CIL program called @c myProgram.cil.
 * This file can be generated by using <a href="http://www.irisa.fr/alf/index.php?option=com_content&view=article&id=38%3Agcc4cli-a-cli-backend-for-gcc&catid=3%3Aprojects&Itemid=3&lang=en">GCC</a> or <a href="http://www.mono-project.com/Main_Page">Mono</a>.
 * For example:
 * @code
 * $ cil32-gcc -o myProgram.cil myProgram.c
 * @endcode
 *
 * First, we create the code cache directory
 * @code
 * $ export ILDJIT_CACHE=/home/xan/MyNewCodeCache
 * $ mkdir -p $ILDJIT_CACHE
 * @endcode
 * The code cache at this point is empty
 * @code
 * $ ls -a $ILDJIT_CACHE
 * $
 * @endcode
 *
 * We generate the IR code of the program @c myProgram.cil without executing it:
 * @code
 * $ ildjit --static -O1 -s myProgram.cil
 * @endcode
 * At this point, there is an entry into the IR code cache where the IR code of @c myProgram.cil has been stored:
 * @code
 * $ ls -a $ILDJIT_CACHE
 *   .ildjit
 * $ ls -a $ILDJIT_CACHE/.ildjit
 *   manfred
 * $ ls -a $ILDJIT_CACHE/.ildjit/manfred
 *   myProgram.cil
 * @endcode
 * The IR code has been stored in the directory @c myProgram.cil.
 *
 * Now, we can generate the machine code starting from the IR that has been previously generated.
 * @code
 * $ ildjit --static myProgram.cil
 * @endcode
 * This run did not change the IR code cache.
 *
 * These last two passes can be collapsed by executing the following command:
 * @code
 * $ ildjit --static -O1 myProgram.cil
 * @endcode
 */

/**
 * \defgroup PartialCompilationScheme Partial compilation scheme
 * \ingroup CompilationSchemes
 *
 * You can generate the IR code just for a part of the program.
 * This section describes how.
 *
 * <BR>
 * 
 * <hr>
 * <B> Clean the IR code cache </B>
 * <hr>
 * First we need to set the IR code cache directory. To see how, read this \ref HowToUseCodeCache .
 *
 * Now we clean the code cache by erasing it entirely.
 * This operation is going to remove all IR code cache stored inside the IR code cache, even the ones about other programs.
 * @code
 * $ ildjit --clean-code-cache
 * @endcode
 *
 * <BR>
 *
 * <hr>
 * <B> Choose what to compile in IR </B>
 * <hr>
 * To choose the part of the program to generate the IR for, we tag the methods to compile.
 * For this purpose, we run the program to identify this set of methods.
 * @code
 * $ ildjit -P1 myProgram.cil
 * @endcode
 * This command has executed @c myProgram.cil and recorded the set of methods executed in the code cache.
 *
 * <BR>
 * 
 * <hr>
 * <B> Generate the IR </B>
 * <hr>
 * We now generates the IR just for the methods previously tagged:
 * @code
 * $ ildjit -P1 -O3 --aot -x -s -m --optimizationleveltool=default myProgram.cil
 * @endcode
 * This command has generated, optimized and stored the IR to the code cache.
 *
 * <BR>
 * 
 * <hr>
 * <B> Execute the IR code previously generated </B>
 * <hr>
 * Now we can execute the IR previously generated as many times as we want.
 * To do so, we execute the machine code generated starting from the IR previously stored:
 * @code
 * $ ildjit --aot -x myProgram.cil
 * @endcode
 *
 * These last two passes (i.e., IR code generation and code execution) can be collapsed by executing the following command:
 * @code
 * $ ildjit -P1 -O1 --aot -x myProgram.cil
 * @endcode
 */

/**
 * \defgroup Components Components
 * \ingroup Overview
 *
 * ILDJIT is composed by a framework and by external plugins that you can customize or extend for your specific purposes.
 *
 * The framework provides a set of APIs the plugins can rely on, which are the following:
 * <ul>
 * <li> \ref SystemAPI : ILDJIT system interface
 * <li> \ref InputLanguage : abstraction of supported input languages
 * <li> \ref IRLanguage : how to understand and manipulate the IR language
 * <li> \ref IROptimizerInterface : how to interact with codetools
 * <li> \ref PLATFORM_API : abstraction of the underlying platform
 * <li> \ref CompilerMemory : how to manage the memory
 * <li> \ref XanLibDoc : data structures
 * </ul>
 *
 * Different type of plugins can be built within the ILDJIT framework, which are the following:
 * <ul>
 * <li> \ref IRCodeOptimization : plugins that analyze and/or transform IR code.
 * <li> \ref Optimizationleveltools : these plugins define the optimization levels. They decide which optimizations to run for each optimization level.
 * <li> \ref GCtools : these plugins implement the garbage collection for memory used by the IR code.
 * <li> \ref CLRtools : these plugins provide methods in their native form (e.g. native methods of the base class library -- BCL).
 * <li> \ref Backendtools : these plugins provide the translation from IR to the machine code.
 * </ul>
 *
 * Each type of plugin can access different set of APIs
 *
 */

/**
 * \defgroup Status Current status
 * \ingroup Overview
 *
 * Information about the status of the project is provided in the following sections:
 * <ul>
 * <li> \ref PlatformsStatus : information about platforms currently supported by ILDJIT
 * <li> \ref DependencesStatus : we provide information about the software you need to install in your system in order to use ILDJIT
 * <li> <a href="http://sourceforge.net/apps/mediawiki/ildjit/index.php?title=ILDJIT_Documentation">Online documentation</a> : on the website of the project you can find more information.
 * </ul>
 *
 * ILDJIT project has been started by <a href="http://www.eecs.harvard.edu/~xan">Simone Campanoni</a> at the end of 2005 and it is still mainly maintained end extended by him.
 * The full team behind the project can be found here: \ref Team.
 *
 * Since we think the framework is robust enough, currently, we are working on ILDJIT in order to add new features by adding new external plugins.
 *
 * The rest of the document describes how to extend and/or customize ILDJIT by using the provided APIs.
 *
 * Next: \ref Plugins
 */

/**
 * \defgroup CommandLineOptions Command line options
 * \ingroup Overview
 *
 * @code
 * $ ildjit

Usage: ildjit [options] file [bytecode_arguments]
Options:
   -h    --help                                           Display this usage information
   -l    --optimizations                                  Dump the optimizations (codetools) available inside the paths specified by ILDJIT_PLGUINS
   -C    --dump-config                                    Dump configuration information for the ILDJIT engine
   -c    --clean-code-cache                               Clean the code cache completely
   -O    --optimization-level=level                       Set the optimization level(e.g. -O0, -O1); -O0 means no optimizations
   -t    --trace                                          To enable the tracer of the method executed and to print their name
   -t    --trace-options=num                              Options of the method tracer.
                                                                 0: trace all methods
                                                                 1: trace only methods that do not belong to any library
                                                                 2: trace only methods that belong to libraries
                                                                 3: trace only methods provided by CLR
   -p    --profiler=num                                   To enable the profiler and to print to stderr the profiling informations after the execution with the deep information level equal to num
   -v    --verbose                                        To ability a verbose output
   -V    --version                                        To print out the version and exit.
   -j    --jit                                            Enable JIT technique
   -d    --dla                                            Enable DLA technique
   -A    --aot                                            Enable AOT technique
   -a    --static                                         Enable the static compilation scheme
   -s    --disable-execution                              Disable the execution of the produced code
   -T    --do-not-overlap-compiler-and-executor-threads   Do not use compiler threads to execute code and vice versa
   -i    --disable-static-constructors                    Disable both code generation and execution of static constructors
   -L    --lab=num                                        Set the initial Look-Ahead Probability of the DLA technique
   -P    --pgc=num                                        Active Profile Guided Compilation
   -D    --dump-all                                       Dump all the CIL methods translated in the CIL, IR and assembly languages
   -G    --gc=name                                        Use the garbage collector name
   -X    --debug-execution                                Translate and execute each CIL method with extra checkers
   -H    --heap-size=num                                  Set the size of the heap to num kilobytes
   -F    --oprefix=name                                   Set to name the prefix for each files generated by the compiler
   -z    --optimizationleveltool=name                     Use the optimizationleveltool called "name"
   -R    --disable-runtime-checks                         Disable checks performed at runtime. Notice that the code produced is not CLI-compliant any more.
   -I    --enable-ir-checks                               Enable checks for IR code produced by the compiler
   -N    --no-explicit-gc                                 Not collect the memory garbage explicitly
   -M    --enable-machine-dependent-optimizations         Enable code optimizations that are specific of the underlying machine
   -e    --enable-optimizations=LIST                      Enable the specified optimizations.
                                                          LIST is a comma separated list of optimizations.
   -b    --disable-optimizations=LIST                     Disable the specified optimizations.
                                                          LIST is a comma separated list of optimizations.
 * @endcode
 *
 */

/**
 * \defgroup PlatformsStatus Supported platforms
 * \ingroup Status
 *
 * Currently ILDJIT can run on top of Linux OS.
 *
 * The current version of ILDJIT has been succesfully tested in the following systems:
 * <ul>
 * <li> Ubuntu 12.10
 * <li> CentOS 5
 * </ul>
 *
 * In the past, ILDJIT has been succesfully tested and used in the following systems:
 * <ul>
 * <li> Ubuntu 10.04, 10.10, 11.04, 11.10 and 12.04
 * <li> CentOS 4
 * </ul>
 *
 * ILDJIT can be used in 32 and 64 bit Intel platforms.
 * However, currently it needs to be compiled for 32 bit x86 by using the "-m32" option.
 * More information about how to install ILDJIT can be found <a href="http://sourceforge.net/apps/mediawiki/ildjit/index.php?title=InstallILDJIT">here</a>.
 *
 * In the past, ILDJIT has been succesfully tested on an ARM 32 bit platform.
 *
 * \image html ARM_platform1.jpeg "ARM platform where ILDJIT has been tested" width=5cm
 *
 * \image html ARM_platform2.jpeg "ARM platform where ILDJIT has been tested" width=5cm
 *
 * The default configuration of ILDJIT uses <a href="http://llvm.org">LLVM 3.0</a> as backend.
 * Therefore, ILDJIT supports as many platforms as LLVM supports.
 */

/**
 * \defgroup DependencesStatus Software dependences
 * \ingroup Status
 *
 * The compiler has two sets of dependences: the former set is to run ILDJIT, the latter is to compile it.
 *
 * The dependences to run ILDJIT are the following:
 * <ul>
 * <li> POSIX thread library
 * <li> C library
 * <li> Math library
 * </ul>
 *
 * \image html ILDJIT_dependencies.jpeg "" width=5cm
 *
 * The dependences to compile ILDJIT include the previous set, plus the following ones:
 * <ul>
 * <li> Bison
 * <li> Flex
 * <li> Gperf
 * </ul>
 */

/**
 * \defgroup Plugins Plugins
 *
 * This section describes how to write plugins that rely on the ILDJIT framework.
 */

/**
 * \defgroup IRCodeOptimization Codetools
 * \ingroup Plugins
 *
 * ILDJIT is distributed with a set of plugins we have developed and tested.
 * This set includes the most common code analysis and optimizations like available expressions analysis and copy propagation code transformation.
 * Every package called
 * @code
 * optimizer-ANYTHING
 * @endcode
 * is an external plugin, loaded automatically by ILDJIT, in charge to analyze or transform IR methods.
 *
 * This section describes how to execute ILDJIT IR code optimization plugins and how to debug the IR code they produce.
 *
 * <hr>
 * <b> Add a new plugin </b>
 * <hr>
 *
 * If you want to start a new plugin, you need to download the plugin called
 * @code
 * optimizer-dummy
 * @endcode
 * from the official ILDJIT website (i.e., http://ildjit.sourceforge.net).
 * From the <code> optimizer-dummy </code> plugin, you need to implement the stub functions you can find within it in order to implement and evaluate your specific customization of ILDJIT.
 *
 * In order to use a plugin, it has to be invoked by ILDJIT.
 * This is done inside the <code> libiljitiroptimizer/src/optimization_levels.c </code> file.
 *
 * This file contains a bunch of functions, named <code> optimizeMethod_O0 </code> , <code> optimizeMethod_O1 </code> , <code> optimizeMethod_O2 </code> , and so on, each corresponding to a different optimization level of the compiler.
 * Each optimization level includes all of the preceding ones.
 *
 * Level 0 means no optimizations at all. \n
 * Level 1 activates the most basic, fully functional and well tested optimizations available.
 * Currently this set is composed by (and they are called in this strict order)
 * <ul>
 * <li> constant propagation (i.e., <code> \ref CONSTANT_PROPAGATOR </code>)
 * <li> copy propagation (i.e., <code> \ref COPY_PROPAGATOR </code>)
 * <li> common sub-expressions elimination (i.e., <code> \ref COMMON_SUBEXPRESSIONS_ELIMINATOR </code>)
 * <li> dead-code elimination (i.e., <code> \ref DEADCODE_ELIMINATOR </code>)
 * <li> null checks elimination (i.e., <code> \ref NULL_CHECK_REMOVER </code>)
 * <li> instruction scheduler (i.e., <code> \ref INSTRUCTIONS_SCHEDULER </code>)
 * <li> algebraic simplification (i.e., <code> \ref ALGEBRAIC_SIMPLIFICATION </code>)
 * </ul>
 * Level 2 repeatedly invokes the optimizations of level 1, until a fixed point is reached (i.e., no more modification are made to the code by further invocations of level 1 optimizations).\n
 * Moreover it calls the following additional code transformations:
 * <ul>
 * <li> Inline native methods (i.e., <code> \ref NATIVE_METHODS_ELIMINATION </code>)
 * <li> Inline native methods (i.e., <code> \ref CONVERSION_MERGING </code>)
 * <li> Variables renaming (i.e., <code> \ref VARIABLES_RENAMING </code>);
 * </ul>
 * Level 3 is equal to Level 2 plus calling the CUSTOM plugin (the default job type of the plugin <code> optimizer-dummy </code>).
 * It means that if you do not change the job type of your plugin, it will be called within the level 3.\n
 * Level 4 and following levels activate further, more advanced, optimizations.
 *
 * <hr>
 * <b> Plugin Dependencies </b>
 * <hr>
 *
 * Every code transformation has its own dependencies with other plugins (code analysis).
 * For example a dead-code eliminator plugin (i.e., <code> \ref DEADCODE_ELIMINATOR </code>) has the variable liveness and basic block analysis (i.e., <code> \ref LIVENESS_ANALYZER </code>, <code> \ref BASIC_BLOCK_IDENTIFIER </code>) as dependences.
 * The dead-code eliminator plugin has to declare this dependence setting to 1 the bit of <code> \ref LIVENESS_ANALYZER </code> of the variable returned by its own function <code> get_dependences </code>.
 * @code
 * static inline JITUINT64 get_dependences (void){
 *	return LIVENESS_ANALYZER | BASIC_BLOCK_IDENTIFIER;
 * }
 * @endcode
 * When ILDJIT boosts, it will check these dependencies and it ensures they are satisfied (they are called) before calling a given plugin.
 *
 * <hr>
 * <b> Compile, Install and Execute a new plugin </b>
 * <hr>
 *
 * In order to execute a plugin, you need to add a line like the following one:
 * @code
 * internal_callMethodOptimization (lib, method, irLib, DEADCODE_ELIMINATOR);
 * @endcode
 * This line has to be added within the file <code> libiljitiroptimizer/src/optimization_levels.c </code>, in the exact point of the execution where you want your plugin to run.
 * Notice that <code> \ref DEADCODE_ELIMINATOR </code> has to be substituted with the ID of the plugin to run.
 * The ID is the same that appears in the <code> get_ID_job </code> function of the plugin you want to run.
 * Even if you could call your plugin from every point, we suggest to insert this line within the function optimizeMethod_O3 in order to have a clean and easy manageable input code of your plugin.
 *
 * After modifying the file <code> optimization_levels.c </code>, you need to recompile and install the <code> libiljitiroptimizer </code> module by running:
 * @code
 * make && make install
 * @endcode
 *
 * Now you are ready to execute ILDJIT with your new optimization plugin.
 * For this purpose, ILDJIT has to be started activating the right optimization level.
 * For example, if you added your plugin in optimization level 3 and you want to run it on testprogram.exe, then you will have to run:
 * @code
 * iljit -O3 testprogram.exe
 * @endcode
 * This is sufficient to have your plugin working.
 *
 * <hr>
 * <b> Testing a new plugin </b>
 * <hr>
 *
 * When testing new plugins, the best thing to do is usually to insert the following call
 * @code
 * IROPTIMIZER_callMethodOptimization(lib, method, METHOD_PRINTER);
 * @endcode
 * as a first and last instruction within the function <code> do_job </code> of the plugin that is in testing.
 * @code
 * static inline void do_job (ir_method_t *method){
 *      IROPTIMIZER_callMethodOptimization(lib, method, METHOD_PRINTER);
 *      ...
 *      IROPTIMIZER_callMethodOptimization(lib, method, METHOD_PRINTER);
 * }
 * @endcode
 * In this way, you can check the input and output of the plugin in testing.
 *
 * Moreover, it is a good practice to call a new plugin after the level O2, for example by running ildjit with the -O3 parameter, because the code will be simplified by the optimizations in O2, and then just the plugin to be tested will be executed, showing whether it works correctly or not.
 *
 * Next: \ref IRPluginExecutionModel
 */

/**
 * \defgroup IRPluginExecutionModel Execution model
 * \ingroup IRCodeOptimization
 *
 * This section provides information on how every plugin are called every time you run ILDJIT.
 * The interface between the IR code optimization plugin and ILDJIT is defined within the file <code> libiljitiroptimizer/src/ir_optimization_interface.h </code>.
 *
 * The IR code optimization plugins have to implement the following functions:
 * @code
 * JITUINT64   (*get_job_kind)      (void);
 * JITUINT64   (*get_dependences)   (void);
 * void        (*init)              (ir_lib_t *irLib, ir_optimizer_t *optimizer, char *outputPrefix);
 * void        (*shutdown)          (void);
 * void        (*do_job)            (ir_method_t *method);
 * char *      (*get_version)       (void);
 * char *      (*get_information)   (void);
 * char *      (*get_author)        (void);
 * JITUINT64   (*get_invalidations) (void);
 * @endcode
 *
 * The function <code> init </code> is called when ILDJIT boosts and hence only once.
 * On the other hand, the function <code> shutdown </code> is called just before ILDJIT shuts down the system (hence only once).
 *
 * The <code> get_job_kind </code>, <code> get_invalidations </code> and <code> get_dependences </code> functions are called every time ILDJIT needs for its purposes.
 *
 * Finally, the function <code> do_job </code> is called every time a IR method needs to be optimized.
 * Hence, this last function may be called multiple times.
 *
 * Every plugin can call every function provided by ILDJIT and defined within the \ref IRLanguage module described later in order to manage the IR code.
 * On the other hand, in order to call other plugins, ILDJIT provides the \ref IROptimizerInterface.
 *
 * <hr>
 * <b> Enabling and Disabling plugins </b>
 * <hr>
 *
 * ILDJIT has a list of optimizations to use for every optimization level.
 * If you want to change this list, you can enable (i.e., --enable-optimizations=...) or disable (i.e., --disable-optimizations=...) optimizations by command line.
 *
 * If you use the "--enable-optimizations=" option, then ILDJIT will run only the optimizations you specify in the command line.
 *
 * On the other hand, if you use the "--disable-optimizations=" option, then ILDJIT consider every plugin activated by default minus the ones you specify in the command line.
 *
 * When ILDJIT boosts, it will load every plugin is able to find on the default installation directory (i.e., <code> INSTALLATION_PREFIX/lib/iljit/optimizers </code>).
 * If you would like to customize this behavior, you can set the environment variable <code> ILDJIT_PLUGINS </code> to point to a list of directories where you have placed your own plugins.
 *
 * For example, consider the following case:
 * @code
 * export ILDJIT_PLUGIN=/home/xan/quite_stable_plugins:/home/xan/experimental_plugins:/home/xan/completely_crazy_plugins
 * @endcode
 * In this example, ILDJIT will loads every plugins installed in the following directories:
 * <ul>
 * <li> <code> INSTALLATION_PREFIX/lib/iljit/optimizers </code>
 * <li> <code> /home/xan/quite_stable_plugins </code>
 * <li> <code> /home/xan/experimental_plugins </code>
 * <li> <code> /home/xan/completely_crazy_plugins </code>
 * </ul>
 * Next: \ref IROptimizerInterface
 */

/**
 * \defgroup Optimizationleveltools Optimization level tools
 * \ingroup Plugins
 *
 * Information about how to create your own optimization level tool is supposed to be in this section.
 * However, this section still needs to be written. Sorry about that.
 *
 * The installed plugin is called
 * @code
 * optimization_levels-default
 * @endcode
 * which defines optimization levels for every compilation schemes and it implements the \ref IterativeCompilation .
 */

/**
 * @defgroup IterativeCompilation Iterative compilation
 * \ingroup Optimizationleveltools
 *
 * The iterative compilation allows you to re-generate the IR code starting from an already generated one.
 *
 * Following we describe how to use the iterative compilation.
 *
 * <BR>
 *
 * <hr>
 * <B> Generating the IR code </B>
 * <hr>
 * The first step is to generate the IR code by using a static compilation scheme; for example \ref PartialCompilationScheme .
 *
 * At this point, the program is available in its IR representation.
 *
 * <BR>
 *
 * <hr>
 * <B> Executing the code without iterative compilation </B>
 * <hr>
 * Without using the iterative compilation, if you try to run ILDJIT with a static compilation scheme, the previously generated IR will be loaded from the disk, the machine code will be generated starting from that IR and its execution will start.
 * @code
 * $ ildjit --aot -x myProgram.cil
 * @endcode
 * Therefore, by running the above command, no change will be performed to the generated code compared to the previous run.
 *
 * Even by specifying different optimization level, the result will be the same as the default semantic of the aot compilation scheme is: if the IR code is available, use it without changing it.
 * @code
 * $ ildjit --aot -x -O3 myProgram.cil
 * @endcode
 *
 * In order to enable machine-dependent optimizations, you can use the <code> -M </code> option. In this case, the translation from the loaded IR to the machine code will use these optimizations because they are performed in the backend rather than at the IR level.
 * @code
 * $ ildjit --aot -x -O3 -M myProgram.cil
 * @endcode
 *
 * If you want to know the time spent running the program, you can use the internal profiler of ILDJIT, by executing the following command:
 * @code
 * $ ildjit --aot -x -O3 -M -p1 myProgram.cil
 * #########################################################################
 *		PROFILING
 * #########################################################################
Profiling clockID = CLOCK_PROCESS_CPUTIME_ID
Garbage Collector used  = Allocator
Methods translation
        Total methods considered			= 254
        Total internal methods                          = 11
        Total methods translated			= 33
        Methods translated using high priority		= 0
        Methods moved to high priority			= 0
        Total reprioritization				= 0
        Total checkpointing				= 0
        Trampolines taken
        	Before starting the execution of the entry point = 0
        	After starting the execution of the entry point  = 28
        Static methods executed				= 4

Types
	Types loaded				= 96

Total time:
	CPU Time	= 0.077625 seconds
	Wall Time	= 0.077511 seconds

Compilation overhead:
		CPU Time	= 0.109084 seconds
		CPU Time percent: 99.526370
		Wall Time	= 0.109323 seconds
		Wall Time percent: 99.941510
Time spent executing the machine code generated:
		CPU Time	=  0.031504 seconds
		CPU Time percent:  0.00001
		Wall Time	=  0.031837 seconds
		Wall Time percent: 0.00001
 * @endcode
 *
 * <BR>
 *
 * <hr>
 * <B> Re-generate the IR code </B>
 * <hr>
 * In order to modify the IR code, we can use the iterative compilation implemented by the plugin <code> optimization_levels-default </code>.
 *
 * To enable the iterative compilation, you need to set the environment variable @c ILDJIT_ITERATIVE_OPTIMIZATIONS .
 * @code
 * $ export ILDJIT_ITERATIVE_OPTIMIZATIONS=1
 * @endcode
 *
 * Now you can re-generate the IR by running:
 * @code
 * $ export ILDJIT_ITERATIVE_OPTIMIZATIONS=1
 * $ ildjit --aot -x -O3 -M myProgram.cil
 * @endcode
 * The optimizations defined in the <code> PostAOT </code> level of the plugin <code> optimization_levels-default </code> are invoked.
 *
 * The plugin defines different possible set of optimizations for the PostAOT level, which can be chosen by using environment variables.
 * Without setting any additional variable, the level specified as input is used (i.e., O3 in the above example).
 *
 * In order to enable the optimization <code> CUSTOM </code> implemented by the codetool <code> optimizer-dummy </code> designed to be the starting point for new code analyses and transformations, you can set the following environment variable:
 * @code
 * $ export ILDJIT_ITERATIVE_OPTIMIZATIONS=1
 * $ export ILDJIT_CUSTOM_OPTIMIZATION=1
 * $ ildjit --aot -x -O3 -M myProgram.cil
 * @endcode
 * In this case, first the IR is loaded from the disk, then the level O3 is applied to it.
 * The new optimized IR is the input of the CUSTOM optimization.
 * At this point the IR is generated and optimized. Then, due to the <code> -M </code> option, the machine-dependent optimizations are applied at a lower level by using the installed backend (e.g. LLVM).
 * The machine code is generated and installed in memory.
 * Finally, the code is executed.
 *
 * If the following environment variable is used, the method inliner and the function pointer elimination are applied to the code as well:
 * @code
 * $ export ILDJIT_ITERATIVE_OPTIMIZATIONS=1
 * $ export ILDJIT_AGGRESSIVE_OPTIMIZATIONS=1
 * $ ildjit --aot -x -O3 -M myProgram.cil
 * @endcode
 * Notice that these two optimizations have not been released yet.
 * However, you can change the set of <I> aggressive optimizations </I>  by customizing the plugin <code> optimization_levels-default </code>.
 *
 * <hr>
 * <B> Save the new IR code </B>
 * <hr>
 * Up to this point, nothing we did lead to substitute the IR stored in the disk.
 * The IR was loaded and modified before generating the machine code.
 *
 * However, you can save the new IR by setting the following environment variables:
 * @code
 * $ export ILDJIT_ITERATIVE_OPTIMIZATIONS=1
 * $ export ILDJIT_ITERATIVE_OPTIMIZATIONS_SAVE_PROGRAM=1
 * $ ildjit --aot -x -O3 -M myProgram.cil
 * @endcode
 * You can use all possible combination of environment variables to generate the new IR before storing it to the disk.
 * For example:
 * @code
 * $ export ILDJIT_ITERATIVE_OPTIMIZATIONS=1
 * $ export ILDJIT_CUSTOM_OPTIMIZATION=1
 * $ export ILDJIT_ITERATIVE_OPTIMIZATIONS_SAVE_PROGRAM=1
 * $ ildjit --aot -x -O3 -M myProgram.cil
 * @endcode
 * In this case, the following actions are performed in the following order:
 * <ul>
 * <li> the IR is loaded from the disk (let called this IR, IR0)
 * <li> IR0 is transformed by using the optimizations defined in the level <code> O3 </code> generating IR1
 * <li> IR1 is transformed by the codetool that provides the <code> CUSTOM </code> optimization (e.g. <code> optimizer-dummy </code>) generating IR2
 * <li> IR2 is stored in the disk substituting the previously stored IR0
 * <li> IR2 is given as input to the backend (e.g. LLVM)
 * <li> Machine-dependent optimizations are used to generate the machine code
 * <li> The machine code is installed in memory
 * <li> The execution of the installed code starts
 * </ul>
 *
 * After saving the new IR into the IR code cache, you can now measure the time spent running the new code by following the conventional way.
 * @code
 * $ unset ILDJIT_ITERATIVE_OPTIMIZATIONS ;
 * $ unset ILDJIT_ITERATIVE_OPTIMIZATIONS_SAVE_PROGRAM=1 ;
 * $ unset ILDJIT_CUSTOM_OPTIMIZATION=1 ;
 * $ ildjit --aot -x -O3 -M -p1 myProgram.cil
 * @endcode
 */

/**
 * \defgroup GCtools Garbage collection tools
 * \ingroup Plugins
 *
 * This section still needs to be written. Sorry about that.
 */

/**
 * \defgroup CLRtools CLR tools
 * \ingroup Plugins
 *
 * This section still needs to be written. Sorry about that.
 */

/**
 * \defgroup Backendtools Backend tools
 * \ingroup Plugins
 *
 * This section still needs to be written. Sorry about that.
 */

/**
 * \defgroup APIs APIs
 *
 * APIs that plugin can rely on are described in this section.
 */

/**
 * \defgroup SystemAPI System API
 * \ingroup APIs
 * @brief ILDJIT system interface
 *
 * The system API can be used to retrieve information about the system installed.
 *
 * To use this API, the following header must be included.
 * @code
 * #include <ildjit.h>
 * @endcode
 */

/**
 *\defgroup InvokeCodetools Invoke codetools
 * \ingroup IROptimizerInterface
 *
 * How to invoke a codetool is described in this section.
 */

/**
 *\defgroup Codetools Codetool types
 * \ingroup IROptimizerInterface
 *
 * Here there is a list of type of codetools that can be used within the ILDJIT framework.
 *
 * Notice that the only limit of what you can do as a codetool is your immagination (the codetool type CUSTOM means <em> everything you want </em>).
 *
 * Next: \ref IRDebugging
 */

/**
 * \defgroup IRDebugging Debugging
 * \ingroup IRCodeOptimization
 *
 * Now, suppose your plugin does not work as expected.
 *
 * In order to simplify the debugging procedure, printing the IR produced by the plugin is a good way to go.
 * This can be done by downloading, compiling and installing the optimization-dotprinter plugin (if you have used the default ILDJIT installer, you have already it within your system).
 * As every other plugin, dotprinter can be called from your plugin.
 * In particular, you can add the following line within your code (hence also within your do_job procedure):
 * @code
 * IROPTIMIZER_callMethodOptimization(lib, method, METHOD_PRINTER);
 * @endcode
 * This line has to be added every time you want to print the code of the method being optimized.
 *
 * A good practice is to print the IR code immediately before and after the execution of the plugin to debug in order to detect the modifications that have been done.
 * So, for example, if <code> do_job </code> is the function body provided by your plugin, you need to modify it in the following way:
 * @code
 * void do_job (ir_method_t *method){
 *     IROPTIMIZER_callMethodOptimization(lib, method, METHOD_PRINTER);
 *     ... // your code
 *     IROPTIMIZER_callMethodOptimization(lib, method, METHOD_PRINTER);
 *     return ;
 * }
 * @endcode
 * When you run ildjit with:
 * @code
 * iljit -On test.exe
 * @endcode
 * (with 'n' set to the appropriate optimization level, as explained before) a series of ILDJIT*.cfg files will be produced.
 * Each of them will contain the control flow graphs for the method whose name identifies the file.
 *
 * In order to view them, you must have the dot program installed (you can find it in your linux distribution repositories, usually in the graphviz package).
 * Typing
 * @code
 * dot -Tsvg -O filename.cfg
 * @endcode
 * will produce one SVG file for each graph. These files can then be opened with firefox, or with inkscape
 * @code
 * firefox *.svg
 * @endcode
 * Looking at the Control Flow Graphs contained in these files, it is much easier to understand what modifications have been done by the plugin, and whether they are correct or not.
 *
 * In oder to understand IR methods, Section \ref IRLanguage describes the IR language by means of data types and the instructions set.
 * The same section provides information on the API that can be used by plugins in order to manage IR methods.
 *
 * Next: \ref SystemAPI
 */

/**
 * \defgroup InputLanguage Input language API
 * \ingroup APIs
 * @brief Abstraction of supported input languages
 *
 * The input language API can be used to retrieve information about the input language used for the running program.
 *
 * Next: \ref IRLanguage
 */

/**
 * \defgroup IRLanguage IR API
 * \ingroup APIs
 * @brief Understand and manipulate the IR language
 *
 * The IR language is composed by data types and instructions.
 * Values (always typed) can be constants or stored within IR variables.
 *
 * In \ref IRLanguageInstructions you can find information about IR instructions.\n
 * In \ref IRLanguageDataTypes you can find information about IR data types (variables and constants).\n
 *
 * The IR code can be managed by using the IR API described in \ref IRMETHOD .
 *
 * Eventually IR code is translated to machine code of the underlying platform by relying on the IR backend described in \ref IRBACKEND .
 *
 * To use this API, the following header must be included.
 * @code
 * #include <ir_method.h>
 * @endcode
 */

/**
 * \defgroup IROptimizerInterface Codetool API
 * \ingroup APIs
 * @brief How to interact with codetools
 *
 * Every plugin can call the IR optimizer interface in order to exploit other ones.
 * Through this interface you can call a single plugin or a set of defined by the optimization levels of ILDJIT.
 *
 * An example of use of this interface is the following: after you have performed your code transformation within your new plugin, you may ask to ILDJIT to print the CFG by calling:
 * @code
 * IROPTIMIZER_callMethodOptimization(lib, method, METHOD_PRINTER);
 * @endcode
 * As you can see, this interface hide the details about which plugin is able to perform the <code> METHOD_PRINTER </code> job and how to call it.
 * ILDJIT will do it for you automatically.
 *
 * To use this API, the following header must be included.
 * @code
 * #include <ir_optimizer.h>
 * @endcode
 */

/**
 * \defgroup PLATFORM_API Platform API
 * \ingroup APIs
 * @brief Abstraction of the underlying platform
 *
 * In order to write platform independent code, ILDJIT provides the API described in this section.
 *
 * To use this API, the following header must be included.
 * @code
 * #include <platform_API.h>
 * @endcode
 */

/**
 * \defgroup CompilerMemory Memory API
 * \ingroup APIs
 * @brief How to manage the memory
 *
 * The memory needed by every module of ILDJIT is allocated by the methods defined in this module.
 *
 * Every time you need to allocate a structure, call \ref allocFunction (sizeof(myStructure)).\n
 * Every time you need to free memory, call \ref freeFunction (pointer)\n
 * Every time you need to change the size of a piece of memory previously allocated, call \ref dynamicReallocFunction (pointer, newSize).
 *
 * To use this API, the following header must be included.
 * @code
 * #include <compiler_memory_manager.h>
 * @endcode
 */

/**
 * \defgroup XanLibDoc Data structure API
 * \ingroup APIs
 * @brief Data structures
 *
 * In this module, you can find information about data structures used inside the ILDJIT project.
 *
 * To use this API, the following header must be included.
 * @code
 * #include <xanlib.h>
 * @endcode
 */

/**
 *\defgroup InternalMethods Internal Methods
 *
 * Here we provide a Howto for the implementation of the internal methods inside the ILDJIT project.

   \n \n \n \n <b>POINT 1 - WHAT ARE INTERNAL CALLS</b>
   \n \n For performance reasons C# standard states that some functionalities have to be implemented at a native level,
   which is in C code inside the compiler. Internal calls are functions inside the CIL compiler implementing CIL
   class methods that are marked with the <code>[MethodImpl(MethodImplOptions.InternalCall)]</code> metadata tag.
   \n \n \n \n <b>POINT 2 - IMPLEMENTATION OF AN INTERNAL CALL</b>
   \n \n <b>Where insert your internal call</b>
   \n You can find the files where internal calls are implemented in the iljit module.
   Here, there are many files <code>called internal_calls_X.c</code> and <code>internal_calls_X.h</code> where X is the name of
   class that you want to implement. Here you have to insert your code of internal call (e.g. If you are implementing
   an internal call about <code>String</code> class, you insert the function in <code>internal_calls_string.c</code> file).
   There isn’t a straight correspondence between .cs files (one for each C# class) and internal_calls files in iljit,
   in which functions are grouped by semantic. So, for instance, in <code>internal_calls_string.c</code> you also find other functions
   about string, like the implementation of the internal methods of <code>StringBuilder</code> and <code>DefaultEncoding</code> classes.
   \n \n <b>The signature of an internal call</b>
   \n The signature of a function that implements an internal call is composed by a return type, the name of the function
   and a list of formal parameters, the same that you find in the signature of the C# method in the pnetlib .cs file.
   (eg. <code>String.cs</code>).
   <ul>
   <li>Return type: it is the return type of the method of the class C# translated in ILDJIT type (see table below).
   <li>Name of function: it is composed by the package that contains the class and the name of the class: for example,
   for the method <code>InternalOrdinal</code> contained in  <code>String</code> class in  <code>System</code> package the name of the function will be
   <code>System_String_InternalOrdinal</code>.
   <li>Formal parameters: they are the same parameters that you find in C# method signature, written as return type
   (translate in ILDJIT type) and name of parameter.
   </ul>
   There is a difference between the implementation of an internal call about a method that belong to a class and a
   method that belong to a structure in the return type and in the formal parameters. To see this difference, look at the point 5.
   \n \n <b>Implementation</b>
   \n After you have declared your signature in the appropriate <code>internal_calls_X</code> files you can proceed to implement
   your internal call. You must use C code to write the internal call. Also ILDJIT uses support libraries that can help you to write internal calls. You can use a t_system structure to call other function and let your code be more efficient and readable. To see how to use t_system structure read the point 3.
   The correspondence between ILJIT and C# types is summarized in the table at the end of this document.
   \n \n \n \n <b>POINT 3 - USE THE SUPPORT LIBRARIES TO IMPLEMENT INTERNAL CALLS.</b>
   \n \n <b>The support libraries</b>
   \n Internal calls are written in C code. ILDJIT helps you with others functions that you can find in iljit module.
   Each file <code>internal_call_X</code> is linked to another file called <code>lib_X.c</code> where there are implemented these support function.
   (e.g <code>internal_calls_string.c</code> is linked to <code>lib_string.c</code>).To use these functionalities you have to declare
   a <code>t_system</code> variable.
   \n Lib files contains general purpose function for accessing object fields through fields offsets (eg. field length of a string) and utility
   functions (like <code>compareStrings</code> or <code>getUTF8copy</code>) that facilitates operations on the objects. Internal_calls files, instead,
   contains only the functions that implements the internal calls. In these implementation functions you can access a <code>lib_X</code>
   function through a <code>t_system</code> structure.
   \n \n <b>What is <code>t_system</code> structure</b>
   \n <code>t_system</code> is a structure that is defined in system.h file that you can find in iljit module.
   <code>t_system</code> holds pointers to others structures to manage the libraries function. Each field of t_system is a pointer to a class manager
   structure (<code>stringManager</code>, <code>arrayManager</code>, and so on), which contains function pointers to lib functions defined in lib files.
   To use this functionality in your internal call, you have to declare a pointer to t_system, initialize it with the instruction
   <code>getSystem(NULL)</code>.
   @code
   t_system *system;
   system=getSystem(NULL);
   @endcode
   Now to call a function defined in <code>lib_file</code> you call <code>(system->classManager).functionName</code>,
   where <code>classManager</code> is the name of the class you are implementing (or better the name of the semantic group of function to
   which your class belongs).
   @code
   (system->stringManager).getLength(str);
   where str is a void* that points to C# string object.
   @endcode
   You can also implement a library function ad hoc for your purpose, modifying <code>lib_file</code> that already exists.
   \n \n <b>How to add a new library function in <code>lib_X</code></b>
   \n If you think a function is useful or necessary to manipulate the objects of the class you are implementing also outside
   the <code>internal_calls_X</code> file or if you believe this functionality can make your code more readable because it can hide
   usefulness implementation details, you should move the implementation code in the <code>lib_X</code> file. To do so you have to:
   <ul>
   <li>declare the signature of the new function in the <code>lib_X.h</code> file
   <li>add the implementation code of the new function in the <code>lib_X.c</code> file
   <li>add a new pointer to the function in the Xmanager structure in <code>lib_X.h</code> file
   <li>add the function pointer assignment in the initialization function inside <code>lib_X.h</code>. If you don't remember to do this assignment
   the pointer to the function points to <code>NULL</code> and a call <code>(system->XManager).yourFunctionPointer</code> will produce
   a <code>Segmentation Fault</code> error during execution)
   </ul>
   \n \n \n <b>POINT 4 - ADD YOUR INTERNAL CALL AT THE TREE OF THE INTERNAL CALLS</b>
   \n \n <b>The file <code>lib_internalCall.gperf</code></b>
   \n When you have finished to implement your internal call in files (<code>internal_calls_file</code> and <code>lib_files</code>)
   you have to add your internal call at the table of internal calls. In iljit module you can find a file that manage this table. This is
   called <code>lib_internalCall.gperf/code> and it allows you to associate the call of a C# method with the correspondent internal calls
   written in C.
   \n To allow this association, methods are organized as a table structure
   \n \n<b>Add your internal call</b>
   \n When you add your internal call you have to provide a new entry. An Entry is composed by two things the identifier and the reference of function to call
   <ul>
   <li> The identifier is composed by full qualified name of method (Namespace.TypeName.MethodName) followed by TypeName (e.g. for System.String[] use only String[])
   of all in parameters separated by special character '_'.
   <li>The reference is a simple pointer to function associated to internal call. See the next point for further details about implementation of method.
   </ul>
   \n \n Example:
   @code
        extern public String Replace(char oldChar, char newChar);                               //C# method signature
                System.String.Replace_char_char, function_implementing_method				//internal call entry
   @endcode
   \n \n \n <b>POINT 5 - IMPLEMENTATION OF A METHOD THAT HAVE A STRUCTURE AS INPUT  PARAMETER (OR RETURN TYPE)</b>
   \n \n When you implement an internal call you can find two cases: static methods and virtual methods.
   \n If you are implementing a static method the list of parameters is the same of the C# signature of the method. Instead,
   for virtual methods, you have to add an additional parameter at the beginning of the list, which is the object that calls
   the method. This parameter is usually called <code>void *_this</code> and is a pointer to the calling object.
   \n Indifferently by the method is static or virtual, you can find or not a structure among its input parameters and it can
   return or not a structure as output parameter. So you have 3 different cases.
   <ol>

   <li>If the method has no input nor output structure parameters, the implementation is normally done with the list of the
   C# signature parameters.

   <li>If the method has input structure parameters, the structure is passed to the implementation function by reference
   through a void pointer <code>(void*)</code> . The implementation function must not modify the structure, because the structure is considered
   a read-only parameter.
   \n \n Example:
   @code
        extern public String Replace(char oldChar, char newChar);                               //C# method signature
        void * System_String_Replace(void *_this, JITINT16 oldChar, JITINT16 newChar);          //internal call
   @endcode
   \n In C# String is represented by a class. There is a return type <code>String</code> in the method signature that corresponded to
   a return type <code>void*</code>  in the internal call. In the formal parameters of internal call there is a <code>void*_this</code>  that is a pointer
   to the object <code>String</code>  which calls this method.

   <li>If the method has output structure parameter, put an extra void pointer at the end of the input parameter list.
   You can name it <code>void*_ret</code>  or <code>void* _result</code>  and it is a pointer to the output structure that is allocated on the stack of
   the caller method.
   \n \n Example:
   @code
        extern public ValueType GetNextArg(RuntimeTypeHandle type);        //C# method signature
        void System_ArgIterator_GetNextArg(void *_this,void *_result);		//internal call
   @endcode
   \n In C# <code>ValueType</code>  is represented by a structure and in this case is the return type of method <code>GetNextArg</code> of
   <code>ArgIterator</code>  structure. So in the internal call the return type is not a type that represent a <code>ValueType</code> but it is void.
        You have to add an additional parameter. In fact the internal call has two formal parameters: the first
        <code>(void* _this)</code> that refers to a <code>RuntimeTypeHandle</code> object and the second (<code>void* _result</code>) that
        is the return type <code>ValueType</code> . Than the <code>void* _result</code> will be used as a return value.
   </ol>
   \n \n \n <b>POINT 6 - A COMPLETE EXAMPLE: <code>System.String.Trim()</code> </b>
   <ol>

   <li>Compose the signature of the function <code>System.String.Trim</code> from the definition of the method <code>Trim</code> in
   <code>pnetlib-0.8.0/runtime/System/String.cs</code>:
   @code
   [MethodImpl(MethodImplOptions.InternalCall)]
   extern private String Trim(char[] trimChars, int trimFlags);				//C# method signature
   void System_String_Trim(void *_this, void *trimCharsArray, JITINT32 trimFlags);		//internal call
   @endcode
   and insert it in <code>internal_calls_string.h</code>


   <li>Insert in internal_calls_string.c the implementation of the function System.String.Trim, where you can use library functions defined in lib_string.h:
   @code
   void * System_String_Trim(void *_this, void *trimCharsArray, JITINT32 trimFlags)
   {
        t_system *system;
        void *obj;
        JITINT32 len;

        system=getSystem(NULL);
        obj = (system->stringManager).trim(_this,trimCharsArray,trimFlags);
        len = (system->stringManager).getLength(_this);

        return obj;
   }
   @endcode

   <li>Insert in lib_internalCall.gperf the line corrisponding to the internal call:
   @code
   // System.String.Trim
   System.String.Trim_Char[]_Int32, System_String_Trim
   @endcode
   </ol>

   \n \n The correspondence between C, C#, ILDJIT and IR types is summarized in the table below. \n \n
   <TABLE WIDTH=60% BORDERCOLOR="#000000" CELLPADDING=4 CELLSPACING=0>

        <TR >
                <TD >
                        <P><B>C# types</B></P>
                </TD>
                <TD >
                        <P><B>CLI types</B></P>
                </TD>
                <TD >
                        <P><B>C types</B></P>
                </TD>
                <TD >
                        <P><B>JIT types [1]</B></P>
                </TD>
                <TD >
                        <P><B>IR types [2]</B></P>
                </TD>
                <TD >
                        <P><B>IR types code</B></P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>bool</P>
                </TD>
                <TD >
                        <P>System.Boolean</P>
                </TD>
                <TD >
                        <P>unsigned char</P>
                </TD>
                <TD >
                        <P> JITBOOLEAN</P>
                </TD>
                <TD >
                        <P>IRINT8</P>
                </TD>
                <TD >
                        <P>5</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>char</P>
                </TD>
                <TD >
                        <P>System.Char</P>
                </TD>
                <TD >
                        <P>unsigned short int</P>
                </TD>
                <TD >
                        <P> JITUINT16
                        </P>
                </TD>
                <TD >
                        <P>IRUINT16</P>
                </TD>
                <TD >
                        <P>10</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>sbyte</P>
                </TD>
                <TD >
                        <P>System.SByte</P>
                </TD>
                <TD >
                        <P>signed char</P>
                </TD>
                <TD >
                        <P> JITINT8
                        </P>
                </TD>
                <TD >
                        <P>IRINT8</P>
                </TD>
                <TD >
                        <P>4</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>short</P>
                </TD>
                <TD >
                        <P>System.Int16</P>
                </TD>
                <TD >
                        <P>short int</P>
                </TD>
                <TD >
                        <P> JITINT16
                        </P>
                </TD>
                <TD >
                        <P>IRINT16</P>
                </TD>
                <TD >
                        <P>5</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>int</P>
                </TD>
                <TD >
                        <P>System.Int32</P>
                </TD>
                <TD >
                        <P>int</P>
                </TD>
                <TD >
                        <P> JITINT32
                        </P>
                </TD>
                <TD >
                        <P>IRINT32</P>
                </TD>
                <TD >
                        <P>6</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>long</P>
                </TD>
                <TD >
                        <P>System.Int64</P>
                </TD>
                <TD >
                        <P>long long int</P>
                </TD>
                <TD >
                        <P> JITINT64
                        </P>
                </TD>
                <TD >
                        <P>IRINT64</P>
                </TD>
                <TD >
                        <P>7</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>byte</P>
                </TD>
                <TD >
                        <P>System.Byte</P>
                </TD>
                <TD >
                        <P>unsigned char</P>
                </TD>
                <TD >
                        <P> JITUINT8
                        </P>
                </TD>
                <TD >
                        <P>IRUINT8</P>
                </TD>
                <TD >
                        <P>9</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>ushort</P>
                </TD>
                <TD >
                        <P>System.UInt16</P>
                </TD>
                <TD >
                        <P>unsigned short int</P>
                </TD>
                <TD >
                        <P> JITUINT16
                        </P>
                </TD>
                <TD >
                        <P>IRUINT16</P>
                </TD>
                <TD >
                        <P>10</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>uint</P>
                </TD>
                <TD >
                        <P>System.UInt32</P>
                </TD>
                <TD >
                        <P>unsigned int</P>
                </TD>
                <TD >
                        <P> JITUINT32
                        </P>
                </TD>
                <TD >
                        <P>IRUINT32</P>
                </TD>
                <TD >
                        <P>11</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>ulong</P>
                </TD>
                <TD >
                        <P>System.UInt64</P>
                </TD>
                <TD >
                        <P>unsigned long long int</P>
                </TD>
                <TD >
                        <P> JITUINT64
                        </P>
                </TD>
                <TD >
                        <P>IRUINT64</P>
                </TD>
                <TD >
                        <P>12</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>float</P>
                </TD>
                <TD >
                        <P>System.Single</P>
                </TD>
                <TD >
                        <P>float</P>
                </TD>
                <TD >
                        <P> JITFLOAT32</P>
                </TD>
                <TD >
                        <P>IRFLOAT32</P>
                </TD>
                <TD >
                        <P>14</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>double</P>
                </TD>
                <TD >
                        <P>System.Double</P>
                </TD>
                <TD >
                        <P>double</P>
                </TD>
                <TD >
                        <P> JITFLOAT64</P>
                </TD>
                <TD >
                        <P>IRFLOAT64</P>
                </TD>
                <TD >
                        <P>15</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>IntPtr</P>
                </TD>
                <TD >
                        <P>System.IntPtr</P>
                </TD>
                <TD >
                        <P>int</P>
                </TD>
                <TD >
                        <P> JITNINT
                        </P>
                </TD>
                <TD >
                        <P>IRNUINT</P>
                </TD>
                <TD >
                        <P>13</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>IntPtr</P>
                </TD>
                <TD >
                        <P>System.IntPtr</P>
                </TD>
                <TD >
                        <P>unsigned int</P>
                </TD>
                <TD >
                        <P> JITNINT
                        </P>
                </TD>
                <TD >
                        <P>IRNINT</P>
                </TD>
                <TD >
                        <P>8</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>float</P>
                </TD>
                <TD >
                        <P>System.Single</P>
                </TD>
                <TD >
                        <P>float</P>
                </TD>
                <TD >
                        <P> JITNFLOAT</P>
                </TD>
                <TD >
                        <P>IRNFLOAT</P>
                </TD>
                <TD >
                        <P>16</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>decimal</P>
                </TD>
                <TD >
                        <P>System.Decimal</P>
                </TD>
                <TD >
                        <P>void*</P>
                </TD>
                <TD >
                        <P>void*</P>
                </TD>
                <TD >
                        <P>IRVALUETYPE</P>
                </TD>
                <TD >
                        <P>24</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>string</P>
                </TD>
                <TD >
                        <P>System.String</P>
                </TD>
                <TD >
                        <P>void*</P>
                </TD>
                <TD >
                        <P>void*</P>
                </TD>
                <TD >
                        <P>IROBJECT</P>
                </TD>
                <TD >
                        <P>23</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>object</P>
                </TD>
                <TD >
                        <P>System.object</P>
                </TD>
                <TD >
                        <P>void*</P>
                </TD>
                <TD >
                        <P>void*</P>
                </TD>
                <TD >
                        <P>IROBJECT</P>
                </TD>
                <TD >
                        <P>23</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>struct</P>
                </TD>
                <TD >
                        <P><BR>
                        </P>
                </TD>
                <TD >
                        <P>void*</P>
                </TD>
                <TD >
                        <P>void*</P>
                </TD>
                <TD >
                        <P>IRMPOINTER/IRVALUETYPE</P>
                </TD>
                <TD >
                        <P>20</P>
                </TD>
        </TR>
        <TR >
                <TD >
                        <P>enum</P>
                </TD>
                <TD >
                        <P> System.Enum </P>
                </TD>
                <TD >
                        <P>int</P>
                </TD>
                <TD >
                        <P>JITINT32</P>
                </TD>
                <TD >
                        <P>IRINT32</P>
                </TD>
                <TD >
                        <P>20</P>
                </TD>
        </TR>

   </TABLE>

   \n Notes:
   \n [1] JIT types are C types defined in libiljitu/src/jitsystem.h
   \n [2] IR types are numeric constants defined in libiljitu/src/ir_language.h

 * Next: \ref CompilerMemory
 */

/**
 * \defgroup Team Team
 *
 * Simone would like to thank Prof. Stefano Crespi Reghizzi for his advise on ILDJIT project received during Simone's PhD.
 *
 * Moreover, we would like to thank all the students of Politecnico di Milano, which helped Simone during his PhD.
 *
 * <hr>
 * <b> <em> Leader </em> </b>
 * <hr>
 *
 * <b> Simone Campanoni </b>
 *
 * \image html Simone_Campanoni.jpeg "" width=5cm
 *
 * Simone Campanoni is currently working under Prof. David Brooks in conjunction with Prof. Gu-yeon Wei at Harvard University.
 * His work focuses on the boundary between hardware and software, relying on dynamic compilation, run-time optimizations and virtual execution environments for investigating opportunities on auto-parallelization.
 * He received his Ph.D. degree with honours from Politecnico di Milano in 2009 with Prof. Stefano Crespi Reghizzi as advisor. Simone is the author of ILDJIT, a parallel dynamic compiler demonstrating principles from his thesis work.
 * He is still an active developer of this project.
 *
 * <hr>
 * <b> <em> Current Developers </em> </b>
 * <hr>
 *
 * <b> Simone Campanoni </b>
 *
 * \image html Simone_Campanoni.jpeg "" width=5cm
 *
 * <b> Timothy M. Jones </b>
 *
 * \image html Timothy_Jones.jpeg "" width=5cm
 *
 * <a href="http://www.cl.cam.ac.uk/~tmj32">Timothy Jones</a> is a post-doctoral researcher at the University of Cambridge Computer Laboratory.
 * He works in the Computer Architecture Group and in collaboration with other researchers throughout the world.
 * From September 2008 - August 2013 he holds a Research Fellowship from the UK's Royal Academy of Engineering and EPSRC to investigate compiler-directed power saving in multicore processors.
 * As part of his Fellowship he worked with David Brooks and his research team at Harvard for the whole of 2010 and now works part-time with ARM.
 * He is technical leader of the adaptive compilation cluster in the HiPEAC2 network of excellence.
 *
 * <b> Glenn Holloway </b>
 *
 * \image html Glenn_Holloway.jpeg "" width=5cm
 *
 *
 * <hr>
 * <b> <em> Past Developers </em> </b>
 * <hr>
 *
 * <b> Luca Rocchini </b>
 *
 * \image html Luca_Rocchini.jpeg "" width=5cm
 *
 * Luca Rocchini is currently working as research assistant at Politecnico di Milano.
 * He received master in computer science degree from Politecnico di Milano in 2010 with Prof. Stefano Crespi Reghizzi as advisor.
 * His work focuses on dynamic compilation, Adaptive and Self-Aware operative system and dynamic reconfigurable system design.
 *
 * <b> Michele Tartara </b>
 *
 * \image html Michele_Tartara.jpeg "" width=5cm
 *
 * Michele Tartara obtained both his BS degree (September 2006) and MS degree (April 2009) in Computer Engineering at Politecnico di Milano.
 * His master thesis work "ARM code generation and optimization in a dynamic compiler" was set in the ambit of the OMP (Open Media Platform) project and focused on the production of the ARM backend for a dynamic compiler, ILDJIT.
 * He then worked on helping to increase the number of programming languages supported by ILDJIT (beside C#, the originally supported language), focusing more on C language support.
 * He is currently pursuing his PhD at "Dipartimento di Elettronica e Informazione" of Politecnico di Milano, working on algorithms and virtual machines to exploit parallel architectures.
 *
 * <b> Ettore Speziale </b>
 *
 * <b> Andrea Di Biagio </b>
 *
 * <hr>
 * <b> <em> Sponsors </em> </b>
 * <hr>
 *
 * <b> Microsoft Research </b>
 *
 * \image html MSR.jpeg "" width=5cm
 *
 * <b> HiPEAC </b>
 *
 * \image html Hipeac.jpeg "" width=5cm
 *
 * Next: \ref Copyright
 */

/**
 *\defgroup Copyright Copyright
 *
 * This package was written by Simone Campanoni <simo.xan@gmail.com> from 25 September 2005 up to 2010
 *
 * It can be downloaded from http://ildjit.sourceforge.net
 *
 * Copyright (C) 2005 - 2012 Simone Campanoni <simo.xan@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * On Debian GNU/Linux systems, the text of the GPL can be found in /usr/share/common-licenses/GPL.
 */

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args ...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args ...)
#endif

#ifdef PREFIX
#undef PREFIX
#endif
#define PREFIX get_prefix()

#define DIM_BUF                         1024
#define LIBRARY_PATH                    get_library_path()

#endif
