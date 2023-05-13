Instructions:
Copy the folder in Transforms folder in llvm-project preferably.
All the commands are given for test files that are placed next to the pass file i.e. loop.c is placed in FunctionInfo folder next to FunctionInfo.cpp. Thus, the path for running the pass only includes ./ as the path.
Same is done for algebraic.c, strength.c and constfold.c. They are placed next to LocalOpts.cpp in LocalOpts folder for convenience.
Path to the .so file needs to be changed if any other location files are used. 


//To run FunctionInfo pass:

//To compile and build loop.c: 
cd FunctionInfo
clang -O -emit-llvm -c loop.c

//To compile and build FunctionInfo.cpp:
cd FunctionInfo
make

//To apply FunctionInfo pass on loop.c:
cd FunctionInfo
opt -enable-new-pm=0 -load ./FunctionInfo.so -function-info loop.bc -o out


//To run LocalOpts pass:

//To compile and build algebraic, consfold and strength tests:
cd LocalOpts

clang -Xclang -disable-O0-optnone -O0 -emit-llvm -c algebraic.c -o algebraic.bc
opt -mem2reg algebraic.bc -o algebraic-m2r.bc

clang -Xclang -disable-O0-optnone -O0 -emit-llvm -c strength.c -o strength.bc
opt -mem2reg strength.bc -o strength-m2r.bc

clang -Xclang -disable-O0-optnone -O0 -emit-llvm -c constfold.c -o constfold.bc
opt -mem2reg constfold.bc -o constfold-m2r.bc

//To compile and build LocalOpts.cpp:
cd LocalOpts
make      

//To apply LocalOpts on the tests:
cd LocalOpts

opt -enable-new-pm=0 -load ./LocalOpts.so -local-opts algebraic-m2r.bc -o out

opt -enable-new-pm=0 -load ./LocalOpts.so -local-opts strength-m2r.bc -o out

opt -enable-new-pm=0 -load ./LocalOpts.so -local-opts constfold-m2r.bc -o out

