# CacheTuna
A terminal based L3 Cache monitoring and tuning tool, integrating Intel's intel-cmt-cat and FTXUI C++ open-source library.

## Functionalities
* Give information about the CPU and its L3 Cache configuration 
* Give information regarding cores and cache ways associated to a policy (Class of Service a.k.a COS)
* Show running processes in each policy (COS)
* Monitor policy’s performances (LLC, Cache Misses)
* Support manual configurating of tag (user specific), cores, and cache ways associated to a policy. Then allows user to save the new configuration or roll back to old configurations
* Support automatic configurating of cache ways to achieve optimum performance of latency-critical policies. Then allows user to save the new configuration or keep original configuration

## How to Install and Run
> Make sure `intel-cmt-cat`, `cmake3`, and `gcc-c++` are installed.
1. Make a local copy of this repository:
(with git clone, for example!)
2. Go into the cloned project’s directory: 
>```cd cachetuna_opensource```
3. Create your own 'build' directory:
>```mkdir build```
4. Go into the 'build' directory you just created:
>```cd build```
5. From inside the ‘build’ directory, generate Makefile by running: 
>cmake3 .. -DCMAKE_CXX_COMPILER=\`which g++\` -DCMAKE_C_COMPILER=\`which gcc\`
6. After Makefile has been generated, build the code: 
>```make```
7. The code is now ready. Run the program by running: 
>```./cachetuna```
