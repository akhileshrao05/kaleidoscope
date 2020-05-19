#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

class lexer {

    private:

    std::ifstream fileHandle;
	char LastChar;

    public:

    lexer (char fileName[])
    {
        std::cout << "Lexer::lexer(): create file handle" << std::endl << std::flush;
        fileHandle.open(fileName);
		LastChar = ' ';
    }

	void LexerLog(std::string str) {
		std::cout << "Lexer: " << str << std::endl;
	}

	/*~lexer()
	{
		fileHandle.close();
	}*/

    enum Token{
	    tok_eof = -1,
	    
	    tok_def = -2,
	    tok_extern = -3,
	    
	    tok_identifier = -4,
	    tok_number = -5,
        
        
		
		tok_if = -6,
		tok_then = -7,
		tok_else = -8,
		
		tok_for = -9,
		tok_in = -10,
		
		//unknown char like ')'
        tok_unknown = -20,
    };
    
    struct tokStruct{
	    Token tok;
	    double numVal;
	    std::string identifierStr;
        char unknownChar;

        tokStruct(Token _tok) : tok(_tok){
        }

        tokStruct(std::string identifier) : tok(tok_identifier), identifierStr(identifier){
        }

        tokStruct(char unknown) : tok(tok_unknown), unknownChar(unknown){
			std::cout << "tokStruct() " << this->unknownChar << std::endl << std::flush;
        }

        tokStruct(double number) : tok(tok_number), numVal(number){
        }

		tokStruct& operator=(const tokStruct& other){
			tok = other.tok;
			numVal = other.numVal;
			identifierStr = other.identifierStr;
			unknownChar = other.unknownChar;
			return *this;
		}
    };
    

    tokStruct getTok()
    {
        //std::cout << "Lexer::getTok()" << std::endl << std::flush;
        //std::string identifierStr;
		std:string msg = "Lexer::getTok() begin:LastChar:";
		msg += LastChar;
        LexerLog(msg);
		
			
        while (std::isspace(LastChar)) {
		    fileHandle.get(LastChar);
			msg = "Lexer::getTok():LastChar ";
			msg += LastChar;
            LexerLog(msg);
			if (!fileHandle)
			{
				LexerLog("Reached EOF");
				return tokStruct(tok_eof);
			}
        }
	
        std::string curString = "";
        if (isalpha(LastChar))
        {
            curString += LastChar;
            while(isalnum(LastChar))
            {
		        fileHandle.get(LastChar);
				if (std::isspace(LastChar) || (LastChar == '(') || (LastChar == ')') ||(LastChar == ',') || (LastChar == ';'))
					break;
                curString += LastChar;
            }
			msg = "Lexer::getTok(): ";
			msg += curString;
            LexerLog(msg);

            if (curString.compare("def") == 0)
                return tokStruct(Token::tok_def);
            else if (curString.compare("extern") == 0)
                return tokStruct(Token::tok_extern);
			else if (curString.compare("if") == 0)
                return tokStruct(Token::tok_if);
			else if (curString.compare("then") == 0)
                return tokStruct(Token::tok_then);
			else if (curString.compare("else") == 0)
                return tokStruct(Token::tok_else);
			else if (curString.compare("for") == 0)
                return tokStruct(Token::tok_for);
			else if (curString.compare("in") == 0)
                return tokStruct(Token::tok_in);
            return tokStruct(curString);
            
        }

        if (isdigit(LastChar) || LastChar == '.')
        {
            std::string NumStr;
            do {
                NumStr += LastChar;
		        fileHandle.get(LastChar);
            } while(isdigit(LastChar) || LastChar == '.');

            double NumVal = strtod(NumStr.c_str(), 0);
            return tokStruct(NumVal);
        }

        if (LastChar == '#') 
        {
           // Comment until end of line.
           do{
		        fileHandle.get(LastChar);
           } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');
        
           if (LastChar != EOF)
               return getTok();
  
        }

		
        //if unknown character, just return the character
		char tempChar = LastChar;
        fileHandle.get(LastChar);
		msg = "Lexer LastChar: ";
		msg += LastChar;
		LexerLog(msg);
		if (!fileHandle)
		{
			LexerLog("Reached EOF");
			return tokStruct(tok_eof);
		}
		msg = "Lexer:tempChar ";
		msg += tempChar;
		LexerLog(msg);
        return tokStruct(tempChar);
    }
};
