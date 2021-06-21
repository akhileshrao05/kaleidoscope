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

#ifndef AST_DEFINED
#include "AST.h"
#define AST_DEFINED
#endif

#ifndef CODEGEN_DEFINED
#include "CodeGen.h"
#define CODEGEN_DEFINED
#endif

using namespace std;


namespace myCompiler {

typedef lexer::tokStruct tokStruct;

class Parser {

    private:
	
	lexer lex;
	
    tokStruct m_curToken;
	bool printLog;

    std::map<char, int> BinopPrecedence;
	
    
    //sets member 'nextToken' so the next token is visible to the main loop when getNextToken is called by one of the 'ParseIdentifierExpr' type functions
    tokStruct getNextToken();
	

    std::shared_ptr<ExprAST> ParseNumberExpr();

    //called when current token is '('
    std::shared_ptr<ExprAST> ParseParenExpr();

    std::shared_ptr<ExprAST> ParseIdentifierExpr();

    std::shared_ptr<ExprAST> ParsePrimaryExpr();
    
    bool isBinaryOperator (tokStruct curToken);

    int getTokPrecedence(tokStruct curToken);

    std::shared_ptr<ExprAST> ParseExpression();

    std::shared_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::shared_ptr<ExprAST> LHS);
	
    std::shared_ptr<PrototypeAST> ParsePrototype();

    std::shared_ptr<FunctionAST> ParseDefinition();
	
	std::shared_ptr<IfExprAST> ParseIfExpr();
	
	std::shared_ptr<ForExprAST> ParseForExprAST ();
	
	std::shared_ptr<ExprAST> ParseVarExprAST();

    std::shared_ptr<PrototypeAST> ParseExtern();

    std::shared_ptr<FunctionAST> ParseTopLevelExpr();

    void HandleDefinition ();

    void HandleExtern ();

    void HandleTopLevelExpression ();

    public:
	
	Code_Gen code_Gen;
	
    void MainLoop();

    Parser(char* fileName, bool _printLog);
	
	void ParserLog(std::string str) ;
};

}