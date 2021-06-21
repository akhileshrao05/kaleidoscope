#include <iostream>

#include "Parser.h"


namespace myCompiler {

int main(int argc, char* argv[])
{
	bool printLog = false;
    
	std::string irFile = "ex.ll";
	char *fileName;
	bool srcProvided = false;
    for (int i = 1; i < argc; ++i) {
		if (std::string(argv[i]) == "--help") {
            std::cout << "Usage: ./a.exe --log<optional>:turn on logging\n"
			            << "               --ir-dump<optional> file for ir dump; ex.ll by default\n"
						<< "               --src<compulsory> source file\n";
			return 0;
        }
        else if (std::string(argv[i]) == "--log") {
            printLog = true;
        }
		else if (std::string(argv[i]) == "--ir-dump") {
			if (i + 1 < argc) {
				i++;
				irFile = std::string(argv[i]);
			}
			else {
				std::cerr << "--ir-dump requires a destination file" << std::endl;
				return 1;
			}
		}
		else if (std::string(argv[i]) == "--src") {
			srcProvided = true;
			if (i + 1 < argc) {
				i++;
				fileName = argv[i];
			}
			else {
				std::cerr << "--src requires a destination file" << std::endl;
				return 1;
			}
		}
    }
	
	if (!srcProvided)
	{
		std::cerr << "No source file provided" << std::endl;
		return 1;
	}
    
    Parser ps(fileName, printLog);
	//TheModule = make_unique<Module>("my cool jit", TheContext);
    ps.MainLoop();
	
	ps.code_Gen.WriteIRFile(irFile);
	
	if (!(ps.code_Gen).WriteObjectFile()) {
		std::cout << "Object file written to output.o" << std::endl;
	}
	else {
		std::cout << "Error writing object file" << std::endl;
	}
}

}