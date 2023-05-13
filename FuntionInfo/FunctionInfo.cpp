// ECE-5984 S21 Assignment 1: FunctionInfo.cpp
// PID:
////////////////////////////////////////////////////////////////////////////////


#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include <iostream>

using namespace llvm;

namespace {
  class FunctionInfo : public FunctionPass {
  public:
    static char ID;
    FunctionInfo() : FunctionPass(ID) { }
    ~FunctionInfo() { }

    // We don't modify the program, so we preserve all analyses
    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesAll();
    }

    // Do some initialization
    bool doInitialization(Module &M) override {
      errs() << "5544 Compiler Optimization Pass\n"; // TODO: remove this.
      // outs() << "Name,\tArgs,\tCalls,\tBlocks,\tInsns\n,\tAdd/Sub,\tMul/Div,\tBr(Cond),\tBr(UnCond)";

      return false;
    }

    // Print output for each function
    bool runOnFunction(Function &F) override {
      // outs() << "name" << ",\t" << "args" << ",\t" << "calls" << ",\t" << "bbs" << ",\t" << "insts ...." << "\n";
      // TODO: remove this
 //     outs() << "Inside Function: "<<F.getName()<<"\n";
 //     outs() << "Function # of args: "<<F.arg_size()<<"\n";
 
 //	Initializing all count variables to 0
      unsigned int BBCount = 0;
      unsigned int instCount = 0;
      unsigned int mulCount = 0;
      unsigned int addCount = 0;
      unsigned int divCount = 0;
      unsigned int subCount = 0;
      unsigned int cBranchCount = 0;
      unsigned int ucBranchCount = 0;
      unsigned int directCalls = 0;
      for (BasicBlock& BB : F) {
      	++BBCount;	//Counts every basic block

	for (Instruction &i : BB){
		++instCount;	//Counts every instruction

		switch(i.getOpcode()){

		//Check for addition instructions of integer and floating point
		case Instruction::Add:
		case Instruction::FAdd:
			++addCount;
			break;

		//Check for subtraction instruction
		case Instruction::Sub:
		case Instruction::FSub:
			++subCount;
			break;

		//Check for multiplication instruction
		case Instruction::Mul:
		case Instruction::FMul:
			++mulCount;
			break;

		//Check for division instruction
		case Instruction::FDiv:
		case Instruction::SDiv:
		case Instruction::UDiv:
			++divCount;
			break;

		//Check for direct and indirect branch instructions
		case Instruction::Br:
		case Instruction::IndirectBr:
			BranchInst *BI;
			if(BI = dyn_cast<BranchInst>(&i))
			{
				if(BI->isConditional()){	//Checks if branch is conditional
					++cBranchCount;
				}
				else{
					++ucBranchCount;
				}
			}
			break;

		}

		if(auto *CI = dyn_cast<CallBase>(&i))
		{
			if(!(CI->isIndirectCall()))	//Checks if it is a direct call
			{
				++directCalls;
			}
		}
      
	}
      }

      //Prints all the data
      outs() << "Name: " << "\t" << F.getName()<<"\t" << "Args: " << "\t" << F.arg_size() <<"\t" << "Calls: " << "\t" << directCalls << "\t" << "Blocks: " << BBCount <<"\t" << "Insts: " << instCount <<"\t" << "Add/Sub: " << addCount+subCount <<"\t"<< "Mul/Div: " << mulCount+divCount <<"\t"<< "Cond. Br: " << cBranchCount <<"\t" << "Uncond. Br: " << ucBranchCount << "\n"; 

      return false;

	
    }
  };
}

// LLVM uses the address of this static member to identify the pass, so the
// initialization value is unimportant.
char FunctionInfo::ID = 0;
static RegisterPass<FunctionInfo> X("function-info", "5984: Function Information", false, false);
