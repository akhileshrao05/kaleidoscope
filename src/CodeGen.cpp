


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

#ifndef AST_DEFINED
#include "AST.h"
#define AST_DEFINED
#endif

#ifndef CODEGEN_DEFINED
#include "CodeGen.h"
#define CODEGEN_DEFINED
#endif



namespace myCompiler{

	Code_Gen::Code_Gen(bool _printLog)
	{
		TheModule = make_unique<Module>("my cool jit", TheContext);
		printLog = _printLog;
	}
	
	AllocaInst* Code_Gen::CreateEntryblockAlloca(Function *TheFunction, std::string varName)
	{
		llvm::IRBuilder<> TmpBuilder(&(TheFunction->getEntryBlock()), TheFunction->getEntryBlock().begin());
		return TmpBuilder.CreateAlloca(Type::getDoubleTy(TheContext), 0, varName.c_str());
	}
	
	Value* Code_Gen::codegen(NumberExprAST* numExprAST)
	{
		return llvm::ConstantFP::get(TheContext, APFloat(numExprAST->Val));
	}
	
	Value*  Code_Gen::codegen(VariableExprAST* variableExprAST)
	{
		Value* V = NamedValues[variableExprAST->Name];
		std::string msg = "VariableExprAST::codegen() ";
		msg += variableExprAST->Name;
		CodegenLog(msg);
		if (!V) {
			CodegenLog("Unknown variable name " + variableExprAST->Name);
		}
		
		return Builder.CreateLoad(V, (variableExprAST->Name).c_str());
	}
	
	Value*  Code_Gen::codegen(BinaryExprAST* binaryExprAST)
	{
		if (binaryExprAST->op == '=')
		{
			
			VariableExprAST *LHSE = dynamic_cast<VariableExprAST*>((binaryExprAST->LHS).get());
			
			if (!LHSE) {
				CodegenLog("ERROR: destination of '=' must be a variable");
				return nullptr;
			}
			
			Value *R = codegen(binaryExprAST->RHS);
			if (!R)
				return nullptr;
			
			Value *L = NamedValues[LHSE->getName()];
			if (!L) {
				CodegenLog("ERROR: Unknown variable name");
			}
			
			Builder.CreateStore(R, L);
			
			return R; //Returning a value allows for chained assignments like “X = (Y = Z)”
		}
		
		Value* L = codegen(binaryExprAST->LHS);
		Value* R = codegen(binaryExprAST->RHS);
		
		if (!L || !R)
			return nullptr;
		
		switch (binaryExprAST->op)
		{
			
			case '+':
			{
				return Builder.CreateFAdd(L, R, "addtmp");
			}
			case '-':
			{
				return Builder.CreateFSub(L, R, "subtmp");
			}
			case '*':
			{
				return Builder.CreateFMul(L, R, "multmp");
			}
			case '<':
			{
				Value* result = Builder.CreateFCmpULT(L, R, "cmplttmp");
				return Builder.CreateUIToFP(result, Type::getDoubleTy(TheContext));
			}
			case '>':
			{
				Value* result = Builder.CreateFCmpUGE(L, R, "cmpgrtmp");
				return Builder.CreateUIToFP(result, Type::getDoubleTy(TheContext));
			}
			case ':':
			{
				return R; //return this just so as to not return nullptr
			}
			default:
			{
				CodegenLog("ERROR: Invalid binary operator");
				return nullptr;
			}
		}

	}
	
	Value*  Code_Gen::codegen(CallExprAST* callExprAST) 
	{
	  // Look up the name in the global module table.
	  Function *CalleeF = TheModule->getFunction(callExprAST->Callee);
	  if (!CalleeF) {
		CodegenLog("ERROR: Unknown function referenced");
		return nullptr;
	  }

	  // If argument mismatch error.
	  auto Args = callExprAST->Args;
	  if (CalleeF->arg_size() != Args.size()) {
		CodegenLog("ERROR: Incorrect # arguments passed");
		return nullptr;
	  }

	  std::vector<Value *> ArgsV;
	  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
		ArgsV.push_back(codegen(callExprAST->Args[i]));
		if (!ArgsV.back())
		  return nullptr;
	  }

	  return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
	}
	
	Function*  Code_Gen::codegen(PrototypeAST* protoExprAST)
	{
		
		std::vector<Type*> Doubles(protoExprAST->Args.size(), Type::getDoubleTy(TheContext));
		FunctionType* FT = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
		
		Function *F = Function::Create(FT, Function::ExternalLinkage, protoExprAST->Name, TheModule.get());
		
		//Set name of each argument, not necessary but make IR more readable
		int id = 0;
		for (auto &Arg : F->args())
			Arg.setName(protoExprAST->Args[id++]);
		
		return F;
	}
	
	Function*  Code_Gen::codegen(FunctionAST* fnExprAST)
	{
		CodegenLog("Function.codegen()");
		Function* TheFunction = TheModule->getFunction((fnExprAST->Proto)->getName());
		
		if (!TheFunction)
		{
			PrototypeAST *protoExprAST = dynamic_cast<PrototypeAST*>((fnExprAST->Proto).get());
			TheFunction = codegen(protoExprAST);
		}
		
		if (!TheFunction)
			return nullptr;
		
		if (!TheFunction->empty())
			CodegenLog("ERROR: Function cannot be redifined");
		
		BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
		Builder.SetInsertPoint(BB);
		
		NamedValues.clear();
		for (auto &Arg : TheFunction->args()) {
			CodegenLog("Adding argument to NamedValues " + std::string(Arg.getName()));
			AllocaInst *ArgAlloca = CreateEntryblockAlloca(TheFunction, Arg.getName());
			
			Builder.CreateStore(&Arg, ArgAlloca);
			
			NamedValues[Arg.getName()] = ArgAlloca;
		}
		
		if (Value* retVal = codegen(fnExprAST->Body))
		{
			Builder.CreateRet(retVal);
			
			verifyFunction(*TheFunction);
			
			return TheFunction;
		}
		
		TheFunction->eraseFromParent();
		return nullptr;
	}

	Value*  Code_Gen::codegen(IfExprAST* ifExprAST)
	{
		Value *CondV = codegen(ifExprAST->Cond); 
		if (!CondV)
			return nullptr;
		
		CondV = Builder.CreateFCmpONE(CondV, ConstantFP::get(TheContext, APFloat(0.0)), "ifcond");
		
		Function *TheFunction = Builder.GetInsertBlock()->getParent();
		
		BasicBlock *ThenBB = BasicBlock::Create(TheContext, "then", TheFunction);
		BasicBlock *ElseBB = BasicBlock::Create(TheContext, "else");
		BasicBlock *MergeBB = BasicBlock::Create(TheContext, "ifcont");
		
		Builder.CreateCondBr(CondV, ThenBB, ElseBB);
		
		//set insert point to ThenBB
		Builder.SetInsertPoint(ThenBB);
		
		//codegen ThenBB
		Value *ThenV = codegen(ifExprAST->Then);
		if (!ThenV)
			return nullptr;
		
		//add branch to MergeBB
		Builder.CreateBr(MergeBB);
		
		ThenBB = Builder.GetInsertBlock();
		
		TheFunction->getBasicBlockList().push_back(ElseBB);
		//set insert point to ElseBB
		Builder.SetInsertPoint(ElseBB);
		
		//codegen ElseBB
		Value *ElseV = codegen(ifExprAST->Else);
		
		if (!ElseV)
			return nullptr;
		
		//add branch to MergeBB
		Builder.CreateBr(MergeBB);
		
		ElseBB = Builder.GetInsertBlock();
		TheFunction->getBasicBlockList().push_back(MergeBB);
		
		//set insert point to MergeBB
		Builder.SetInsertPoint(MergeBB);
		
		//add phi to merge values from ThenBB and ElseBB
		PHINode *PN = Builder.CreatePHI(Type::getDoubleTy(TheContext), 2, "iftmp");
		
		PN->addIncoming(ThenV, ThenBB);
		PN->addIncoming(ElseV, ElseBB);
		
		return PN;
	}
	
	Value*  Code_Gen::codegen(ForExprAST* forExprAST)
	{
		Function *TheFunction = Builder.GetInsertBlock()->getParent();
		
		//code gen start value
		AllocaInst* InductionVar = CreateEntryblockAlloca(TheFunction, forExprAST->InductionVarName);
		
		Value* StartV = codegen(forExprAST->Start);
		if (!StartV)
			return nullptr;
		
		Builder.CreateStore(StartV, InductionVar);
		
		//get current function and basic block (it can get modified by Start->codegen() )
		
		BasicBlock *PreHeaderBB = Builder.GetInsertBlock();
		
		//add new block (loop) 
		BasicBlock *LoopBB = BasicBlock::Create(TheContext, "loop", TheFunction);
		
		//unconditional branch to loop block
		Builder.CreateBr(LoopBB);
		
		Builder.SetInsertPoint(LoopBB);
		
		//add phi instruction to update induction variable
		//PHINode *InductionVar = Builder.CreatePHI(Type::getDoubleTy(TheContext), 2, "loop");
		//InductionVar->addIncoming(StartV, PreHeaderBB);
		
		//Error out if induction variable has been defined earlier
		if (NamedValues.find(forExprAST->InductionVarName) != NamedValues.end()) {
			CodegenLog("ERROR: Redefinition of variable in for loop");
			return nullptr;
		}		
		NamedValues[forExprAST->InductionVarName] = InductionVar;
		
		
		//codegen body of loop
		if (!(codegen(forExprAST->Body)))
			return nullptr;
		
		//codegen increment
		Value *StepV = codegen(forExprAST->Step);
		
		if (!StepV)
			return nullptr;
		
		//increment induction variable by increment
		//Value *NextVar = Builder.CreateFAdd(InductionVar, StepV, "nextvar");
		
		//increment induction variable by increment
		Value *Tmp = Builder.CreateLoad(InductionVar);
		Value *NextVar = Builder.CreateFAdd(Tmp, StepV, "nextvar");
		Builder.CreateStore(NextVar, InductionVar);
		
		//codegen end condition
		Value *EndCondV = codegen(forExprAST->End);
		
		
		//convert EndV from (0.0 or 1.0) to bool 
		EndCondV = Builder.CreateFCmpONE(EndCondV, ConstantFP::get(TheContext, APFloat(0.0)), "loopcond");
		
		//Create BasicBlock after loop
		BasicBlock *AfterLoopBB = BasicBlock::Create(TheContext, "afterloop", TheFunction);
		
		//conditional branch back to loop or afterloop
		Builder.CreateCondBr(EndCondV, LoopBB, AfterLoopBB);
		
		Builder.SetInsertPoint(AfterLoopBB);
		
		
		//for loop always returns 0
		return Constant::getNullValue(Type::getDoubleTy(TheContext));
	}
	
	Value*  Code_Gen::codegen(VarExprAST* varExprAST) 
	{
		Function *TheFunction = Builder.GetInsertBlock()->getParent();
		for (int i = 0; i < varExprAST->VarNames.size(); i++) {
			std::string name = varExprAST->VarNames[i].first;
			
			//Error out if variable has been defined earlier
			if (NamedValues.find(name) != NamedValues.end()) {
				CodegenLog("ERROR: Redefinition of variable");
				return nullptr;
			}
			auto init = varExprAST->VarNames[i].second;

			Value* initVal;
			if (init) {
				initVal = codegen(init);
				if (!initVal)
					return nullptr;	
			}
			else {
				initVal = ConstantFP::get(TheContext, APFloat(0.0));
			}			
			
			AllocaInst* VarAlloca = CreateEntryblockAlloca(TheFunction, name);
			Builder.CreateStore(initVal, VarAlloca);
			NamedValues[name] = VarAlloca;
		}
		
		auto *BodyVal = codegen(varExprAST->Body);
		if (!BodyVal)
			return nullptr;
		
		return BodyVal;
	}
	
	Value* Code_Gen::codegen(std::shared_ptr<ExprAST> exprAST)
	{
		ExprType exprType = exprAST->getType();
		switch (exprType)
		{
			case exprTypeNumber : 
			{
				NumberExprAST *numberExprAST = dynamic_cast<NumberExprAST*>(exprAST.get());
				return codegen (numberExprAST);
			}
			
			
			case exprTypeVariable :
			{
				VariableExprAST *varExprAST = dynamic_cast<VariableExprAST*>(exprAST.get());
				return codegen (varExprAST);
			}
			
			case exprTypeBinaryExpr :
			{
				BinaryExprAST *binaryExprAST = dynamic_cast<BinaryExprAST*>(exprAST.get());
				return codegen (binaryExprAST);
			}
			
			case exprTypeIf :
			{
				IfExprAST *ifExprAST = dynamic_cast<IfExprAST*>(exprAST.get());
				return codegen (ifExprAST);
			}
			
			case exprTypeCall :
			{
				CallExprAST *callExprAST = dynamic_cast<CallExprAST*>(exprAST.get());
				return codegen (callExprAST);
			}
			
			case exprTypePrototype :
			{
				PrototypeAST *prototypeExprAST = dynamic_cast<PrototypeAST*>(exprAST.get());
				return codegen (prototypeExprAST);
			}
			
			case exprTypeFunc :
			{
				FunctionAST *funcExprAST = dynamic_cast<FunctionAST*>(exprAST.get());
				return codegen (funcExprAST);
			}
			
			case exprTypeFor :
			{
				ForExprAST *forExprAST = dynamic_cast<ForExprAST*>(exprAST.get());
				return codegen (forExprAST);
			}
			
			case exprTypeVarExpr :
			{
				VarExprAST *varExprAST = dynamic_cast<VarExprAST*>(exprAST.get());
				return codegen (varExprAST);
			}
			
			default:
				CodegenLog("ERROR: Uknown expression type");
		}
			
		
	}
	
	void Code_Gen::CodegenLog(std::string str) 
	{
		if (printLog) 
		{
			std::cout << "Codegen " << str << std::endl;
		}
	}
	
	int Code_Gen::WriteObjectFile()
	{
		// Initialize the target registry etc.
		InitializeAllTargetInfos();
		InitializeAllTargets();
		InitializeAllTargetMCs();
		InitializeAllAsmParsers();
		InitializeAllAsmPrinters();
		
		auto TargetTriple = sys::getDefaultTargetTriple();
		TheModule->setTargetTriple(TargetTriple);
		
		std::string Error;
		auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);
		
		// Print an error and exit if we couldn't find the requested target.
		// This generally occurs if we've forgotten to initialise the
		// TargetRegistry or we have a bogus target triple.
		if (!Target) {
			errs() << Error;
			return 1;
		}
		
		auto CPU = "generic";
		auto Features = "";
		
		TargetOptions opt;
		auto RM = Optional<Reloc::Model>();
		auto TheTargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);
		TheModule->setDataLayout(TheTargetMachine->createDataLayout());
		TheModule->setTargetTriple(TargetTriple);
		
		auto Filename = "output.o";
		std::error_code EC;
		raw_fd_ostream dest(Filename, EC);//, sys::fs::OF_None);

		if (EC) {
		  errs() << "Could not open file: " << EC.message();
		  return 1;
		}
		
		legacy::PassManager pass;
		auto FileType = llvm::LLVMTargetMachine::CGFT_ObjectFile;

		if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
		  errs() << "TargetMachine can't emit a file of this type";
		  return 1;
		}

		pass.run(*TheModule);
		dest.flush();
		
		return 0;
		
	}
	
	void Code_Gen::WriteIRFile(std::string irFile)
	{
		std::ofstream irDump(irFile);
		std::string irString;
		llvm::raw_string_ostream OS(irString);
		OS << *TheModule;
		OS.flush();
		
		irDump << irString;
		irDump.close();
	}

}