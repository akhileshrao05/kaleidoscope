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

#include "Parser.h"




using namespace std;


namespace myCompiler {

    //sets member 'nextToken' so the next token is visible to the main loop when getNextToken is called by one of the 'ParseIdentifierExpr' type functions
    tokStruct Parser::getNextToken() {
		ParserLog("Parser getNextToken: ");
        m_curToken = lex.getTok();
		std::string msg = "curToken.unknownChar ";
		msg += m_curToken.unknownChar;
        ParserLog(msg);
		
        return m_curToken;
    }

    std::shared_ptr<ExprAST> Parser::ParseNumberExpr()
    {
		ParserLog("ParseNumberExpr numVal:");
        auto newASTNode = make_shared<NumberExprAST>(m_curToken.numVal);
		getNextToken();
        return newASTNode;
    }

    //called when current token is '('
    std::shared_ptr<ExprAST> Parser::ParseParenExpr() {
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
            ParserLog("expected ')'");
        }

        return parenExpr;

    }

    std::shared_ptr<ExprAST> Parser::ParseIdentifierExpr()
    {
		ParserLog("Parse identifierExpr");
        tokStruct curTok = m_curToken;
        getNextToken();
        tokStruct nextTok = m_curToken;

        if (nextTok.unknownChar != '(') {
			ParserLog("Parse identifierExpr '(' not found return VariableExprAST ");
            return make_shared<VariableExprAST> (curTok.identifierStr);
		}
		ParserLog("Parse identifierExpr found '(' parsing callExpr ");
        
		getNextToken();
        nextTok = m_curToken;
        std::vector<std::shared_ptr<ExprAST>> Args;
        
        if (nextTok.unknownChar != ')')
        {
			ParserLog("Parse identifierExpr parsing callExpr Arguments ");
            while(1)
            {
                if (auto Arg = ParseExpression()) {
					std::cout << "ParseIdentifierExpr Parsed one argument " << std::endl;
                    Args.push_back(Arg);
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
                    ParserLog("Expected ')' or ',' in argument list");
                    return nullptr;
                }

                getNextToken();
             }


        }
        //getNextToken();
		ParserLog("ParseIdentifierExpr: returning CallExprAST");
        return make_shared<CallExprAST>(curTok.identifierStr, Args);
    }

    std::shared_ptr<ExprAST> Parser::ParsePrimaryExpr()
    {
		ParserLog("ParsePrimaryExpr");
        if (m_curToken.tok == lexer::tok_identifier)
            return ParseIdentifierExpr();
        else if (m_curToken.tok == lexer::tok_number)
            return ParseNumberExpr();
        else if ((m_curToken.tok == lexer::tok_unknown) && (m_curToken.unknownChar == '('))
            return ParseParenExpr();
		else if (m_curToken.tok == lexer::tok_if)
			return ParseIfExpr();
		else if (m_curToken.tok == lexer::tok_then)
			return ParseIfExpr();
		else if (m_curToken.tok == lexer::tok_else)
			return ParseIfExpr();
		else if (m_curToken.tok == lexer::tok_for)
			return ParseForExprAST();
		else if (m_curToken.tok == lexer::tok_var)
			return ParseVarExprAST();
        else {
            ParserLog("Unknown type of expression");
            return nullptr;
        }
    }
    
    bool Parser::isBinaryOperator(tokStruct curToken)
    {
		if (curToken.tok != lexer::tok_unknown)
			return false;
		std::string msg = "curToken.unknownChar ";
		msg += curToken.unknownChar;
		ParserLog(msg);
		if ((curToken.unknownChar == '&') || (curToken.unknownChar == '|') || (curToken.unknownChar == '>') || (curToken.unknownChar == '<') || (curToken.unknownChar == '+') || (curToken.unknownChar == '-') || (curToken.unknownChar == '*') || (curToken.unknownChar == '/') || (curToken.unknownChar == '=') || (curToken.unknownChar == ':'))
            return true;
        return false;
    }

    int Parser::getTokPrecedence(tokStruct curToken)
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

    std::shared_ptr<ExprAST> Parser::ParseExpression()
    {
		ParserLog("ParseExpression");
		//getNextToken();
        auto LHS = ParsePrimaryExpr();
        if (!LHS)
            return nullptr;

        return ParseBinOpRHS(0, LHS);
    }

    std::shared_ptr<ExprAST> Parser::ParseBinOpRHS(int ExprPrec, std::shared_ptr<ExprAST> LHS)
    {
		ParserLog("ParseBinOpRHS");
        while(1)
        {
                        
			std::string msg = "ParseBinOpRHS nextToken.unknownChar";
			msg += m_curToken.unknownChar;
			ParserLog(msg);
            int tokPrec = getTokPrecedence(m_curToken);
			msg = "Precedence ";
			msg += std::to_string(tokPrec);
			ParserLog(msg);
            if (tokPrec < ExprPrec) {
                ParserLog("returning LHS from ParseBinOpRHS");
				return LHS;
			}
            char binOp = m_curToken.unknownChar;

            getNextToken(); //get expression after operator            

            auto RHS = ParsePrimaryExpr();
            if (!RHS)
                return nullptr;

            int nextPrec = getTokPrecedence(m_curToken); //get the operator after the next expression
			msg = "nextToken precedence";
			msg += nextPrec;
			ParserLog(msg);
            if (nextPrec >= tokPrec)
            {
                RHS = ParseBinOpRHS(tokPrec+1, RHS);

                if (!RHS)
                    return nullptr;
            }
			
            LHS = make_shared<BinaryExprAST>(binOp, LHS, RHS);

        }
    }

    std::shared_ptr<PrototypeAST> Parser::ParsePrototype()
    {
		std::cout << "ParsePrototype " << std::endl << std::flush;
        if (m_curToken.tok != lexer::tok_identifier) {
            ParserLog("Expected function name in prototype");
            return nullptr;
        }

        std::string fnName = m_curToken.identifierStr;
        getNextToken();
        if (m_curToken.unknownChar != '(') {
            ParserLog("Expected '(' in prototype");
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
            ParserLog("Expected ')' in prototype");
		
		getNextToken();

        return make_shared<PrototypeAST>(fnName, ArgNames);
    }

    std::shared_ptr<FunctionAST> Parser::ParseDefinition()
    {
        ParserLog("ParseDefinition");
        getNextToken(); //consume 'def'
        auto proto = ParsePrototype();
        if (!proto)
            return nullptr;
		
        if (auto E = ParseExpression())
            return make_shared<FunctionAST>(proto, E);

        return nullptr;
    }
	
	std::shared_ptr<IfExprAST> Parser::ParseIfExpr()
	{
		ParserLog("ParseIfExpr");
		getNextToken();
		//if condition doesnt have brackets
		auto Cond = ParseExpression();
		
		ParserLog("ParseIfExpr Parsed Cond");
		if (!Cond)
			return nullptr;
		
		if (m_curToken.tok != lexer::tok_then) {
			ParserLog("Expected then after if statement");
			return nullptr;
		}
		
		getNextToken(); //consume 'then' expr
		
		auto Then = ParseExpression();
				
		if (!Then)
			return nullptr;
		
		ParserLog("ParseIfExpr Parsed Then");
		
		if (m_curToken.tok != lexer::tok_else)
			ParserLog("Expected else after then statement");
		
		getNextToken();  //consume 'else'
		
		
		auto Else = ParseExpression();
		
		if (!Else)
			return nullptr;
		
		ParserLog("ParseIfExpr Parsed Else");
		
		return make_shared<IfExprAST>(Cond, Then, Else);
		
		
	}
	
	std::shared_ptr<ForExprAST> Parser::ParseForExprAST ()
	{
		ParserLog("In ParseForExprAST");
		getNextToken();
		
		if (m_curToken.tok != lexer::tok_identifier)
			ParserLog("Expected identifier after 'for' ");
		
		std::string idName = m_curToken.identifierStr;
		
		getNextToken();
		
		if (m_curToken.unknownChar != '=')
			ParserLog("Expected '=' after for");
		
		getNextToken();
		
		auto Start = ParseExpression();
		if (!Start)
			return nullptr;
		
		if (m_curToken.unknownChar != ',')
			ParserLog("Expected ',' after start value in for statement");
		getNextToken();
		
		auto End = ParseExpression();
		
		if (!End)
			return nullptr;
		
		if (m_curToken.unknownChar != ',')
			ParserLog("Expected ',' after end value in for statement");
		getNextToken();
		
		
		auto Step = ParseExpression();
		
		if (!Step)
			return nullptr;
		
		if (m_curToken.tok != lexer::tok_in)
			ParserLog("Expected 'in' after Step in for statement");
		
		getNextToken();
		
		auto Body = ParseExpression();
		
		if (!Body)
			return nullptr;
		
		return make_shared<ForExprAST>(idName, Start, End, Step, Body);
		
	}
	
	std::shared_ptr<ExprAST> Parser::ParseVarExprAST()
	{
		ParserLog("In ParseVarExprAST");
		std::vector<std::pair<std::string, std::shared_ptr<ExprAST>>> VarNames;
		getNextToken();
		
		if (m_curToken.tok != lexer::tok_identifier) {
			ParserLog("Expected identifier in var expression");
			return nullptr;
		}
		
		while(1) {
			std::string Name = m_curToken.identifierStr;
			getNextToken();
			
			std::shared_ptr<ExprAST> Init;
			if (m_curToken.unknownChar == '=') {
				getNextToken();
				Init = ParseExpression();
				if (!Init)
					return nullptr;
			}
			VarNames.push_back(std::make_pair(Name, Init));
			
			if (m_curToken.unknownChar != ',')
				break;
			
			getNextToken();
			if (m_curToken.tok != lexer::tok_identifier) {
				ParserLog("Expected identifier in var expression");
				return nullptr;
			}
		}
		if (m_curToken.tok != lexer::tok_in) {
			ParserLog("Error: Expected 'in' in var expression");
			return nullptr;
		}
			
		getNextToken();
		
		auto Body = ParseExpression();
		
		if (!Body) 
			return nullptr;
		
		return make_shared<VarExprAST>(VarNames, Body);
	}

    std::shared_ptr<PrototypeAST> Parser::ParseExtern()
    {
        getNextToken(); //consume 'extern'
        return ParsePrototype();
    }

    std::shared_ptr<FunctionAST> Parser::ParseTopLevelExpr()
    {
		ParserLog("ParseTopLevelExpr");
		//getNextToken();
        if (auto E = ParseExpression())
        {
            //make anonymous prototype
            auto proto = make_shared<PrototypeAST>("", std::vector<std::string>());;
            return make_shared<FunctionAST>(proto, E);
        }
        return nullptr;
    }

    void Parser::HandleDefinition () {
        ParserLog("HandleDefinition");
        if (auto FnAST = ParseDefinition())
        {
			auto *FnIR = code_Gen.codegen(FnAST);
			if (FnIR)
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

    void Parser::HandleExtern () {
        if (auto ProtoAST = ParseExtern())
        {
			auto *FnIR = code_Gen.codegen(ProtoAST);
			if (FnIR)
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

    void Parser::HandleTopLevelExpression () {
		ParserLog("HandleTopLevelExpression");
        if (auto FnAST = ParseTopLevelExpr())
        {
			fprintf(stderr, "Parsed a top-level expression\n");
			
			auto *FnIR = code_Gen.codegen(FnAST);
		
			if (FnIR)
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

    void Parser::MainLoop()
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
	
	Parser::Parser(char* fileName, bool _printLog):lex(lexer(fileName, printLog)),m_curToken(tokStruct(lexer::tok_unknown)), printLog(_printLog) , code_Gen(Code_Gen(_printLog)){
	BinopPrecedence = 
	{
		{':' ,  1},
		{'=' ,  2},
		{'<' , 10},
		{'>' , 11},
		{'+' , 20},
		{'-' , 30},
		{'*' , 40},
	};
	//Code_Gen code_Gen(_printLog);
    }
	
	void Parser::ParserLog(std::string str) 
	{
		if (printLog) 
		{
			std::cout << "Parser " << str << std::endl;
		}
	}



}