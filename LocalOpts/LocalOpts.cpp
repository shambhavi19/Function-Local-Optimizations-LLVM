#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Instrumentation.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace {
class LocalOpts : public FunctionPass {
protected:
	//Declare variables for counting each transformations
    int algebraicCount;
    int constantCount;
    int strengthCount;

    /**
    Algebraic Identities
    */
    class Algebraic: public InstVisitor<Algebraic>{
        protected:
            LocalOpts &localOpts_obj;
        
        public:
            Algebraic(LocalOpts &parent) : localOpts_obj(parent){};
            
            //Case operand zero
            bool eliminateZero(Instruction &I, unsigned int op)
            {
                bool ret = false;
                switch (I.getOpcode()){

                //Case: y = x + 0; & y = 0 + x; -> y = x; Both integers and floating point
                case Instruction::Add:
                case Instruction::FAdd:
                I.replaceAllUsesWith(I.getOperand(1-op));
                ++localOpts_obj.algebraicCount;
                ret = true;
                break;

 		//Case: y = x - 0; -> y = x; Both integers and floating point
                case Instruction::Sub:
                case Instruction::FSub:
                if(op == 1)
                {
                    I.replaceAllUsesWith(I.getOperand(1-op));
                    ++localOpts_obj.algebraicCount;
                    ret = true;
                }
                break;

 		//Case: y = x * 0; & y = 0 * x; -> y = 0; Both integers and floating point
                case Instruction::Mul:
                case Instruction::FMul:
                I.replaceAllUsesWith(I.getOperand(op));
                ++localOpts_obj.algebraicCount;
                ret = true;
                break;

                }
                return ret;
            }

	    //Case operand one
            bool eliminateOne(Instruction &I, unsigned int op)
            {
                bool ret = false;
                switch (I.getOpcode()){

		//Case: y = x * 1; & y = 1 * x; -> y = x; For both integers and floating point
                case Instruction::Mul:
                case Instruction::FMul:
                I.replaceAllUsesWith(I.getOperand(1-op));
                ++localOpts_obj.algebraicCount;
                ret = true;
                break;
		
		//Case: y = x / 1; -> y = x; For both integers and floating point
		case Instruction::SDiv:
		case Instruction::UDiv:
		case Instruction::FDiv:
		if(op == 1)    //Check if divisor is 1
		{
			I.replaceAllUsesWith(I.getOperand(1-op));
			++localOpts_obj.algebraicCount;
			ret = true;
		}
		break;

                }
                return ret;
            }

	    //Case operands are same
	    bool eliminateSameOperands(Instruction &I)
	    {
	    	bool ret = false;
		switch (I.getOpcode()){
		
		//Case: y = x - x; -> y = 0; for integers
		case Instruction::Sub:
                I.replaceAllUsesWith(ConstantInt::get(I.getType(), 0));
                ++localOpts_obj.algebraicCount;
                ret = true;
                break;
		
		//Case: y = x - x; -> y = 0.0; for floating point
		case Instruction::FSub:
                I.replaceAllUsesWith(ConstantFP::get(I.getType(), 0.0));
                ++localOpts_obj.algebraicCount;
                ret = true;
                break;

		//Case: y = x / x; -> y = 1; for integers
		case Instruction::SDiv:
		case Instruction::UDiv:
		I.replaceAllUsesWith(ConstantInt::get(I.getType(), 1));
		++localOpts_obj.algebraicCount;
		ret = true;
		break;
		
		//Case: y = x / x; -> y = 1.0; for floating point
		case Instruction::FDiv:
		I.replaceAllUsesWith(ConstantFP::get(I.getType(), 1.0));
		++localOpts_obj.algebraicCount;
		ret = true;
		break;

                }
                return ret;

	    }



            void visitBinaryOperator(BinaryOperator &I){
              	bool eliminate = false;
//                errs() << I << "\n";
                if(I.getOperand(0) != I.getOperand(1))	//Checks if the operands are not equal
                {
                    for (User::op_iterator op = I.op_begin(), end = I.op_end(); op != end; ++op)
                    {
                        if (Constant *CI = dyn_cast<Constant>(*op))	//Converts operand to constant value
                        {
                            if(CI->isZeroValue())	//Checks if constant value is 0
                            {
                               eliminate = eliminateZero(I, op->getOperandNo());	
                            }
			    else if(CI->isOneValue())	//Checks if constant value is 1
			    {
			       eliminate = eliminateOne(I, op->getOperandNo());
			    }
                        }
                    }
                }
		else{
			eliminate = eliminateSameOperands(I);	//if operands are equal, calls eliminateSameOperands()
		}
                if (eliminate == true)
                {
                    I.eraseFromParent();	//Erases the original instruction
                }
            }

    } algebraic_obj;

class ConstantFolding : public InstVisitor<ConstantFolding>{
    protected:
        LocalOpts &localOpts_obj;
    public:
        ConstantFolding(LocalOpts &parent) : localOpts_obj(parent) {};

        void visitBinaryOperator(BinaryOperator &I){
          // errs() << I << "\n";
	    int64_t intValue;
	    double floatValue;
	    int isFloat = 0;
	    APInt first;
       	    APInt second;

            if(isa<Constant>(I.getOperand(0)) && isa<Constant>(I.getOperand(1)))	//Checks if both operands are constants
            {

		switch(I.getOpcode())
                {
		//Case x = 2 + 3;
                case Instruction::Add:
		    first = dyn_cast<ConstantInt>(I.getOperand(0))->getValue();	//Gets the constant value of operand 0
            	    second = dyn_cast<ConstantInt>(I.getOperand(1))->getValue(); //Gets the constant value of operand 1
		    intValue = first.getZExtValue() + second.getZExtValue();	//Performs addition of the constant values
                    isFloat = 0;
		    break;
		
		///Case x = 3 - 2;
		case Instruction::Sub:
		    first = dyn_cast<ConstantInt>(I.getOperand(0))->getValue();
            	    second = dyn_cast<ConstantInt>(I.getOperand(1))->getValue();
		    intValue = first.getZExtValue() - second.getZExtValue();	//Performs subtraction of the constant values
		    isFloat = 0;
		    break;
		
		//Case x = 3 * 2;
		case Instruction::Mul:
		    first = dyn_cast<ConstantInt>(I.getOperand(0))->getValue();
            	    second = dyn_cast<ConstantInt>(I.getOperand(1))->getValue();
		    intValue = first.getZExtValue() * second.getZExtValue();	//Performs multiplication of the constant values
                    isFloat = 0;
		    break;
			
		///Case x = 6 / 2;
		case Instruction::UDiv:
		case Instruction::SDiv:
		    first = dyn_cast<ConstantInt>(I.getOperand(0))->getValue();
            	    second = dyn_cast<ConstantInt>(I.getOperand(1))->getValue();
		    intValue = first.getZExtValue() / second.getZExtValue();	//Performs division of the constant values
		    isFloat = 0;
		    break;
		
		default: 
		    break;
                }

		if(isFloat == 0)
		{

			ConstantInt *constant = ConstantInt::get(dyn_cast<IntegerType>(I.getOperand(0)->getType()), intValue);	//Extracts and stores the integer value as a constant
			I.replaceAllUsesWith(constant);	//Replaces all the instructions with a constant value
	
		}

            	I.eraseFromParent();	//Erases original instruction
		++localOpts_obj.constantCount;

            }
            
	}
} constantFolding_obj;

//Class for Strength reduction transformation
class StrengthReduction : public InstVisitor<StrengthReduction>{
    protected:
        LocalOpts &localOpts_obj;
    public:
        StrengthReduction(LocalOpts &parent) : localOpts_obj(parent) {};
	
	//For every instruction
	void visitBinaryOperator(BinaryOperator &I)
	{
		switch(I.getOpcode())
		{
			//Case: a = x * 2; -> a = x << 1; 
			//Case: a = 2 * a; -> a = x << 1; 
			//Considers all multiples of 2 and replaces multiplication with shift left operator
			case Instruction::Mul:
				if(ConstantInt *CI = dyn_cast<ConstantInt>(I.getOperand(1))){

					if(CI->getValue().isPowerOf2()){ //Checks if the operand 1 is a power of 2
	
						ReplaceInstWithInst(&I, BinaryOperator::CreateShl(I.getOperand(0), ConstantInt::get(CI->getType(), CI->getValue().logBase2())));  //Calculates log of base 2 of the operand 1 and replaces Mul with Shl(shift left).
						++localOpts_obj.strengthCount;
					}

				}
				else if(ConstantInt *CI = dyn_cast<ConstantInt>(I.getOperand(0)))
				{
					if(CI->getValue().isPowerOf2()){ //Checks if the operand 0 is a power of 2
						ReplaceInstWithInst(&I, BinaryOperator::CreateShl(I.getOperand(1), ConstantInt::get(CI->getType(), CI->getValue().logBase2())));  //Calculates log of base 2 of operand 0 and replaces Mul with Shl(Shift left).
						++localOpts_obj.strengthCount;
					}
				}
				break;
			
			//Case: a = x / 2; -> a = x >> 1;
			//Considers all multiples of 2
			case Instruction::UDiv:
			case Instruction::SDiv:	
				if(ConstantInt *CI = dyn_cast<ConstantInt>(I.getOperand(1)))
				{
					if(CI->getValue().isPowerOf2()) //Checks if operand 1 is a power of 2
					{
						ReplaceInstWithInst(&I, BinaryOperator::CreateAShr(I.getOperand(0), ConstantInt::get(CI->getType(), CI->getValue().logBase2())));  //Calculates log of base 2 of the operand 0 and replaces Div with AShr(shift right)
						++localOpts_obj.strengthCount;
					}
				}
				break;


		}
	}

} strengthReduction_obj;  


public:
    static char ID;

    //Initializes the pass LocalOpts. 
    //Initializes the transformation count variables to 0 at the beginning of the implementation. 
    //Initializes the objects of the transformation classes
    LocalOpts() : FunctionPass(ID), algebraicCount(0), constantCount(0), strengthCount(0), algebraic_obj(*this), constantFolding_obj(*this), strengthReduction_obj(*this){}
    ~LocalOpts() {}

//    void visitInstruction(Instruction &I) { errs() << I << "\n"; }

     // Do some initialization
    bool doInitialization(Module &M) override { return false; }

    //Prints the number of transformations applied after full implementation
    bool doFinalization(Module &M) override {
        outs() << "Transformations:"<< "\n";
        outs() << "Algebraic transformations count: " << algebraicCount << "\n";
	outs() << "Constant folding transformations count: " << constantCount << "\n";
	outs() << "Strength reduction transformations count " << strengthCount << "\n";
        return false;
    }

    bool runOnFunction(Function &F) override {
	//Every transformation below visits the instructions caught by the visitBinaryOperator() function in their respective classes.
        algebraic_obj.visit(F);
	constantFolding_obj.visit(F);
	strengthReduction_obj.visit(F);
        return true;
    }

}; // class
} // namespace

// LLVM uses the address of this static member to identify the pass, so the
// initialization value is unimportant.`
char LocalOpts::ID = 0;
static RegisterPass<LocalOpts> X("local-opts", "5544: Local Optimizations",
                                 false, false);
