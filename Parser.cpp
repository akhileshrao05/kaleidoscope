#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
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

#include "Lexer.h"


using namespace std;
using namespace llvm;



/*typedef llvm::Value Value;
typedef llvm::Function Function;
typedef llvm::Type Type;
typedef llvm::FunctionType FunctionType;
typedef llvm::BasicBlock BasicBlock;*/


static llvm::LLVMContext TheContext;
static llvm::IRBuilder<> Builder(TheContext);
static std::unique_ptr<llvm::Module> TheModule;
static std::map<string, Value*> NamedValues;

void LogError(const char *str) {
	fprintf(stderr, "LogError: %s\n", str);
}

void ParserLog(std::string str) {
	std::cout << str;
}

class ExprAST {
public:
    virtual ~ExprAST() {}
	virtual Value* codegen() {}
};


class NumberExprAST : public ExprAST {
    double Val;
public:
    NumberExprAST (double _val) : Val(_val) {}
    double getVal() {return Val;}
	Value* codegen()
	{
		return llvm::ConstantFP::get(TheContext, APFloat(Val));
	}
};


class VariableExprAST : public ExprAST {
    std::string Name;
public:
    VariableExprAST (std::string &_Name) : Name (_Name) {}
	Value* codegen()
	{
		Value* V = NamedValues[Name];
		std::cout << "VariableExprAST::codegen() " << Name;
		if (!V)
			LogError("Unknown variable name");
		return V;
	}
};

class BinaryExprAST : public ExprAST {
    char op;
    std::unique_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST (char _op, std::unique_ptr<ExprAST> _lhs, std::unique_ptr<ExprAST> _rhs) : op(_op), LHS(std::move(_lhs)), RHS(std::move(_rhs)) {}
    BinaryExprAST (BinaryExprAST&& other) {
        this->op = other.op;
        this->LHS = std::move(other.LHS);
        this->RHS = std::move(other.RHS);

        other.LHS = nullptr;
        other.RHS = nullptr;
    }
    BinaryExprAST& operator=(BinaryExprAST&& other) {
        this->op = other.op;
        this->LHS = std::move(other.LHS);
        this->RHS = std::move(other.RHS);

        other.LHS = nullptr;
        other.RHS = nullptr;

        return *this;
    }
	
	Value* codegen()
	{
		Value* L = LHS->codegen();
		Value* R = RHS->codegen();
		if (!L || !R)
			return nullptr;
		
		switch (op)
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
				Value* result = Builder.CreateFCmpULT(L, R, "subtmp");
				return Builder.CreateUIToFP(result, Type::getDoubleTy(TheContext));
			}
			default:
			{
				LogError("Invalid binary operator");
				return nullptr;
			}
		}
	}
};

class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

public:
    CallExprAST (std::string &_callee, std::vector<std::unique_ptr<ExprAST>> _args) : Callee(_callee), Args(std::move(_args)) {}
    CallExprAST (CallExprAST&& other) {
        this->Callee = other.Callee;
        this->Args = std::move(other.Args);
    }

    CallExprAST& operator=(CallExprAST&& other)
    {
        this->Callee = other.Callee;
        this->Args = std::move(other.Args);
        return *this;
    }
	
	Value* codegen() 
	{
	  // Look up the name in the global module table.
	  Function *CalleeF = TheModule->getFunction(Callee);
	  if (!CalleeF) {
		LogError("Unknown function referenced");
		return nullptr;
	  }

	  // If argument mismatch error.
	  if (CalleeF->arg_size() != Args.size()) {
		LogError("Incorrect # arguments passed");
		return nullptr;
	  }

	  std::vector<Value *> ArgsV;
	  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
		ArgsV.push_back(Args[i]->codegen());
		if (!ArgsV.back())
		  return nullptr;
	  }

	  return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
	}
};

class PrototypeAST : public ExprAST {
    std::string Name;
    std::vector<std::string> Args;

public:
    PrototypeAST (std::string _name, std::vector<std::string> _args) : Name(_name), Args(std::move (_args)) {}
    /*PrototypeAST (PrototypeAST&& other){
        this->Name = other.Name;
        this->Args = other.Args;
    }*/

    const std::string &getName() const { return Name; }
	
	Function* codegen()
	{
		
		std::vector<Type*> Doubles(Args.size(), Type::getDoubleTy(TheContext));
		FunctionType* FT = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
		
		Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());
		
		//Set name of each argument, not necessary but make IR more readable
		int id = 0;
		for (auto &Arg : F->args())
			Arg.setName(Args[id++]);
		
		return F;
	}
};

class FunctionAST : public ExprAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

public:
    FunctionAST (FunctionAST&& other) {
        this->Proto = std::move(other.Proto);
        this->Body = std::move(other.Body);
    }
	
	FunctionAST (std::unique_ptr<PrototypeAST> _proto, std::unique_ptr<ExprAST> _body) : Proto(std::move(_proto)), Body(std::move(_body)) {}
	
	Function* codegen()
	{
		
		Function* TheFunction = TheModule->getFunction(Proto->getName());
		
		if (!TheFunction)
			TheFunction = Proto->codegen();
		
		if (!TheFunction)
			return nullptr;
		
		if (!TheFunction->empty())
			LogError("Function cannot be redifined");
		
		BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
		Builder.SetInsertPoint(BB);
		
		NamedValues.clear();
		for (auto &Arg : TheFunction->args()) {
			fprintf(stderr, "Adding auryment to NamedValues %s",std::string(Arg.getName()).c_str());
			NamedValues[Arg.getName()] = &Arg;
		}
		
		if (Value* retVal = Body->codegen())
		{
			Builder.CreateRet(retVal);
			
			verifyFunction(*TheFunction);
			
			return TheFunction;
		}
		
		TheFunction->eraseFromParent();
		return nullptr;
	}
};

typedef lexer::tokStruct tokStruct;

class Parser {

    private:
	
	template <typename T, typename ...Args> std::unique_ptr<T> make_unique(Args&& ...args)
	{
		return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}
    
    lexer lex;
    tokStruct m_curToken;

    std::map<char, int> BinopPrecedence;
    
    std::unique_ptr<ExprAST> LogError(const char *str) {
        fprintf(stderr, "LogError: %s\n", str);
        return nullptr;
    }

    int GetPrecedence(char op)
    {
        if (!isascii(op))
            return -1;

        int tokPrec = BinopPrecedence[op];
        if (tokPrec <= 0)
            return -1;
        return tokPrec;
    }

    //sets member 'nextToken' so the next token is visible to the main loop when getNextToken is called by one of the 'ParseIdentifierExpr' type functions
    tokStruct getNextToken() {
		ParserLog("Parser getNextToken: ");
        m_curToken = lex.getTok();
        ParserLog("Parser getNextToken: ");
		ParserLog("Parser getNextToken.unknownChar ");
        return m_curToken;
    }

    std::unique_ptr<ExprAST> ParseNumberExpr()
    {
		ParserLog("ParseNumberExpr numVal:");
        auto newASTNode = make_unique<NumberExprAST>(m_curToken.numVal);
		getNextToken();
        return std::move(newASTNode);
    }

    //called when current token is '('
    std::unique_ptr<ExprAST> ParseParenExpr() {
        //tokStruct curTok = getNextToken();
        getNextToken();
        auto parenExpr = ParseExpression();

        if (!parenExpr)
        {
            return nullptr;
        }

        getNextToken();
        if (m_curToken.unknownChar != ')')
        {
            LogError("expected ')'");
        }

        return parenExpr;

    }

    std::unique_ptr<ExprAST> ParseIdentifierExpr()
    {
		ParserLog("Parse identifierExpr");
        tokStruct curTok = m_curToken;
        getNextToken();
        tokStruct nextTok = m_curToken;

        if (nextTok.unknownChar != '(') {
			ParserLog("Parse identifierExpr '(' not found return VariableExprAST ");
            return make_unique<VariableExprAST> (curTok.identifierStr);
		}
		ParserLog("Parse identifierExpr found '(' parsing callExpr ");
        
		getNextToken();
        nextTok = m_curToken;
        std::vector<std::unique_ptr<ExprAST>> Args;
        
        if (nextTok.unknownChar != ')')
        {
			ParserLog("Parse identifierExpr parsing callExpr Arguments ");
            while(1)
            {
                if (auto Arg = ParseExpression()) {
					std::cout << "ParseIdentifierExpr Parsed one argument " << std::endl;
                    Args.push_back(std::move(Arg));
				}
                else
                    return nullptr;
				
				if (m_curToken.unknownChar == ')') {
					ParserLog("Parse identifierExpr found ')' stopping parsing of call arguments ");
					break;
				}
				
				getNextToken();
            
                
				/*if (m_curToken.unknownChar == ')') {
					std::cout << "Parse identifierExpr found ')' stopping parsing of call arguments " << std::endl;
					break;
				}*/
				
                if ((m_curToken.tok == lexer::Token::tok_unknown) && ((m_curToken.unknownChar != ')') && (m_curToken.unknownChar != ','))) {
                    LogError("Expected ')' or ',' in argument list");
                    return nullptr;
                }

                getNextToken();
             }


        }
        //getNextToken();
		ParserLog("ParseIdentifierExpr: returning CAllExprAST");
        return make_unique<CallExprAST>(curTok.identifierStr, std::move(Args));
    }

    std::unique_ptr<ExprAST> ParsePrimaryExpr()
    {
		ParserLog("ParsePrimaryExpr");
        if (m_curToken.tok == lexer::tok_identifier)
            return ParseIdentifierExpr();
        else if (m_curToken.tok == lexer::tok_number)
            return ParseNumberExpr();
        else if ((m_curToken.tok == lexer::tok_unknown) && (m_curToken.unknownChar == '('))
            return ParseParenExpr();
        else {
            LogError("Unknown type of expression");
            return nullptr;
        }
    }
    
    bool isBinaryOperator (tokStruct curToken)
    {
		std::string msg = "curToken.unknownChar ";
		msg += curToken.unknownChar;
		ParserLog(msg);
		if ((curToken.unknownChar == '&') || (curToken.unknownChar == '|') || (curToken.unknownChar == '>') || (curToken.unknownChar == '<') || (curToken.unknownChar == '+') || (curToken.unknownChar == '-') || (curToken.unknownChar == '*') || (curToken.unknownChar == '/'))
            return true;
        return false;
    }

    int getTokPrecedence(tokStruct curToken)
    {
        //check if it is a binary operator
        if (!isBinaryOperator(curToken))
		{
			ParserLog("Not a binary operator");
            return -1;
		}

        int TokPrec = BinopPrecedence[curToken.unknownChar];

        if (TokPrec <= 0)
            return -1;

        return TokPrec;
    }

    std::unique_ptr<ExprAST> ParseExpression()
    {
		ParserLog("ParseExpression");
		//getNextToken();
        auto LHS = ParsePrimaryExpr();
        if (!LHS)
            return nullptr;

        return ParseBinOpRHS(0, std::move(LHS));
    }

    std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS)
    {
		ParserLog("ParseBinOpRHS");
        while(1)
        {
            
            //getNextToken(); //get operator
            //tokStruct nextToken = m_curToken;
			std::string msg = "ParseBinOpRHS nextToken.unknownChar";
			msg += m_curToken.unknownChar;
			ParserLog(msg);
            int tokPrec = getTokPrecedence(m_curToken);
			msg = "Precedence";
			msg += tokPrec;
			ParserLog("Precedence: ");
            if (tokPrec < ExprPrec)
                return LHS;

            char binOp = m_curToken.unknownChar;

            getNextToken(); //get expression after operator
            //nextToken = m_curToken;

            auto RHS = ParsePrimaryExpr();
            if (!RHS)
                return nullptr;

            
            //nextToken = m_curToken;
			
            int nextPrec = getTokPrecedence(m_curToken); //get the operator after the next expression
			msg = "nextToken precedence";
			msg += nextPrec;
			ParserLog(msg);
            if (nextPrec >= tokPrec)
            {
                RHS = ParseBinOpRHS(tokPrec+1, std::move(RHS));

                if (!RHS)
                    return nullptr;
            }
			
            LHS = make_unique<BinaryExprAST>(binOp, std::move(LHS), std::move(RHS));

        }
    }

    std::unique_ptr<PrototypeAST> ParsePrototype()
    {
		std::cout << "ParsePrototype " << std::endl << std::flush;
        if (m_curToken.tok != lexer::tok_identifier) {
            LogError("Expected function name in prototype");
            return nullptr;
        }

        std::string fnName = m_curToken.identifierStr;
        getNextToken();
        if (m_curToken.unknownChar != '(') {
            LogError("Expected '(' in prototype");
			std::cout << m_curToken.unknownChar << std::endl;
            return nullptr;
        }

        std::vector<std::string> ArgNames;
        getNextToken();
        while ((m_curToken.tok == lexer::tok_identifier) || (m_curToken.unknownChar == ','))
        {
			if (m_curToken.tok == lexer::tok_identifier) {
				ArgNames.push_back(m_curToken.identifierStr);
			}
            getNextToken();
        }

        if (m_curToken.unknownChar != ')')
            LogError("Expected ')' in prototype");
		
		getNextToken();

        return make_unique<PrototypeAST>(fnName, ArgNames);
    }

    std::unique_ptr<FunctionAST> ParseDefinition()
    {
        ParserLog("ParseDefinition");
        getNextToken(); //consume 'def'
        auto proto = ParsePrototype();
        if (!proto)
            return nullptr;
		
        if (auto E = ParseExpression())
            return make_unique<FunctionAST>(std::move(proto), std::move(E));

        return nullptr;
    }

    std::unique_ptr<PrototypeAST> ParseExtern()
    {
        getNextToken(); //consume 'extern'
        return ParsePrototype();
    }

    std::unique_ptr<FunctionAST> ParseTopLevelExpr()
    {
		ParserLog("ParseTopLevelExpr");
		//getNextToken();
        if (auto E = ParseExpression())
        {
            //make anonymous prototype
            auto proto = make_unique<PrototypeAST>("", std::vector<std::string>());;
            return make_unique<FunctionAST>(std::move(proto), std::move(E));
        }
        return nullptr;
    }

    void HandleDefinition () {
        ParserLog("HandleDefinition");
        if (auto FnAST = ParseDefinition())
        {
			if (auto *FnIR = FnAST->codegen())
			{
				fprintf(stderr, "Parsed a function definition\n");
				FnIR->print(errs());
				fprintf(stderr, "\n");
			}
        }
        else
        {
            getNextToken();
        }
    }

    void HandleExtern () {
        if (auto ProtoAST = ParseExtern())
        {
			
			if (auto *FnIR = ProtoAST->codegen())
			{
				fprintf(stderr, "Parsed an extern\n");
				FnIR->print(errs());
				fprintf(stderr, "\n");
			}
        }
        else
        {
            getNextToken();
        }
    }

    void HandleTopLevelExpression () {
		ParserLog("HandleTopLevelExpression");
        if (auto FnAST = ParseTopLevelExpr())
        {
			fprintf(stderr, "Parsed a top-level expression\n");
			if (auto *FnIR = FnAST->codegen())
			{
				FnIR->print(errs());
				fprintf(stderr, "\n");
			}
        }
        else
        {
            getNextToken();
        }
    }

    public:
    void MainLoop()
    {
        getNextToken();
        while(1)
        {
			std::string msg = "MainLoop m_curToken.unknownChar:";
			msg += m_curToken.unknownChar;
			ParserLog(msg);
            if (m_curToken.tok == lexer::tok_eof)
                return;

            else if (m_curToken.unknownChar == ';') {
                getNextToken();
               
            }
            else if (m_curToken.tok == lexer::tok_def) {
                HandleDefinition();
				ParserLog("done with HandleDefinition");
				getNextToken();
                
            }
            else if (m_curToken.tok == lexer::tok_extern) {
                HandleExtern();
				ParserLog("done with HandleExtern" );
				getNextToken();
            }
            else {
                HandleTopLevelExpression();
                ParserLog("done with HandleTopLevelExpression");
				getNextToken();
            }
        }
    }

    Parser(char* fileName):lex(lexer(fileName)),m_curToken(tokStruct(lexer::tok_unknown)) {
        BinopPrecedence = 
        {
            {'<' , 10},
            {'+' , 20},
            {'-' , 30},
            {'*' , 40},
        };
    }

};


// ################################# Test Code ############################################
int main()
{
    char fileName[] = "./kaleidoscope.kl.txt";
    Parser ps(fileName);
	TheModule = make_unique<Module>("my cool jit", TheContext);
    ps.MainLoop();
}




