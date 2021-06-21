
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"


#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <utility>
#include <iostream>

using namespace std;
using namespace llvm;

static llvm::LLVMContext TheContext;
static llvm::IRBuilder<> Builder(TheContext);
static std::unique_ptr<llvm::Module> TheModule;
static std::map<string, AllocaInst*> NamedValues;


namespace myCompiler{
	

class Code_Gen {
	public:
	bool printLog;
	
	Code_Gen(bool _printLog);

	
	template <typename T, typename ...Args> std::unique_ptr<T> make_unique(Args&& ...args)
	{
		return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}
	
	AllocaInst *CreateEntryblockAlloca(Function *TheFunction, std::string varName);
	
	
	Value* codegen(NumberExprAST* numExprAST);
	
	
	Value*  codegen(VariableExprAST* variableExprAST);
	
	
	Value*  codegen(BinaryExprAST* binaryExprAST);
	
	
	Value*  codegen(CallExprAST* callExprAST);
	
	
	Function*  codegen(PrototypeAST* protoExprAST);

	
	Function*  codegen(FunctionAST* fnExprAST);
	

	Value*  codegen(IfExprAST* ifExprAST);

	
	
	Value*  codegen(ForExprAST* forExprAST);

	
	Value*  codegen(VarExprAST* varExprAST);

	
	Value* codegen(std::shared_ptr<ExprAST> exprAST);

	
	void CodegenLog(std::string str);

	
	void WriteIRFile(std::string irFile);
	
	int WriteObjectFile();
	
};


}