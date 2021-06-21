
namespace myCompiler {
	
enum ExprType {
	exprTypeInvalid = -1,
	exprTypeNumber,
	exprTypeVariable,
	exprTypeBinaryExpr,
	exprTypeIf,
	exprTypeFor,
	exprTypeFunc,
	exprTypePrototype,
	exprTypeCall,
	exprTypeVarExpr
};
	
class ExprAST {
public:
    virtual ~ExprAST() {}
	virtual ExprType getType() {}

};


class NumberExprAST : public ExprAST {
	public:
	
    double Val;
	
    NumberExprAST (double _val) : Val(_val) {}
    double getVal() {return Val;}
	ExprType getType() override {return exprTypeNumber;};
	//Value* codegen();
};


class VariableExprAST : public ExprAST {
	public:
	
    std::string Name;

    VariableExprAST (std::string &_Name) : Name (_Name) {}
	std::string getName() {return Name;}
	ExprType getType() override {return exprTypeVariable;};
	//Value* codegen();
};

class BinaryExprAST : public ExprAST {
	public:
	
    char op;
    std::shared_ptr<ExprAST> LHS, RHS;


    BinaryExprAST (char _op, std::shared_ptr<ExprAST> _lhs, std::shared_ptr<ExprAST> _rhs) : op(_op), LHS(std::move(_lhs)), RHS(std::move(_rhs)) {}
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
	
	ExprType getType() override {return exprTypeBinaryExpr;};
	
	//Value* codegen();
	
};

class CallExprAST : public ExprAST {
	public:
	
    std::string Callee;
    std::vector<std::shared_ptr<ExprAST>> Args;


    CallExprAST (std::string &_callee, std::vector<std::shared_ptr<ExprAST>> _args) : Callee(_callee), Args(std::move(_args)) {}
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
	
	ExprType getType() override {return exprTypeCall;};
	//Value* codegen();
};

class PrototypeAST : public ExprAST {
	public:
	
    std::string Name;
    std::vector<std::string> Args;


    PrototypeAST (std::string _name, std::vector<std::string> _args) : Name(_name), Args(std::move (_args)) {}
    /*PrototypeAST (PrototypeAST&& other){
        this->Name = other.Name;
        this->Args = other.Args;
    }*/

    const std::string &getName() const { return Name; }
	
	ExprType getType() override {return exprTypePrototype;};
	//Value* codegen();

};

class FunctionAST : public ExprAST {
	public:
	
    std::shared_ptr<PrototypeAST> Proto;
    std::shared_ptr<ExprAST> Body;


    FunctionAST (FunctionAST&& other) {
        this->Proto = std::move(other.Proto);
        this->Body = std::move(other.Body);
    }
	
	FunctionAST (std::shared_ptr<PrototypeAST> _proto, std::shared_ptr<ExprAST> _body) : Proto(std::move(_proto)), Body(std::move(_body)) {}
	
	ExprType getType() override {return exprTypeFunc;};
	//Value* codegen();
};

class IfExprAST : public ExprAST {
	public:
	
	std::shared_ptr<ExprAST> Cond, Then, Else;
	
	
	IfExprAST(std::shared_ptr<ExprAST> _Cond, std::shared_ptr<ExprAST> _Then, std::shared_ptr<ExprAST> _Else)
	 : Cond(std::move(_Cond)),Then(std::move(_Then)),Else(std::move(_Else)) {}
	
	ExprType getType() override {return exprTypeIf;};
	//Value* codegen();
};

class ForExprAST : public ExprAST {
	public:
	
	std::string InductionVarName;
	std::shared_ptr<ExprAST> Start, End, Step, Body;
	
	
	ForExprAST (std::string _varName, std::shared_ptr<ExprAST> start, std::shared_ptr<ExprAST> end, std::shared_ptr<ExprAST> step, std::shared_ptr<ExprAST> body):
	InductionVarName(_varName), Start(std::move(start)), End(std::move(end)), Step(std::move(step)), Body(std::move(body)) {}
	
	ExprType getType() override {return exprTypeFor;};
	//Value* codegen();
};

class VarExprAST : public ExprAST {
	public:
	std::vector<std::pair<std::string, std::shared_ptr<ExprAST>>> VarNames;
	std::shared_ptr<ExprAST> Body;
	
	
	VarExprAST (std::vector<std::pair<std::string, std::shared_ptr<ExprAST>>> _varNames, std::shared_ptr<ExprAST> _body) : Body(std::move(_body)), VarNames(std::move(_varNames)) {}
	
	ExprType getType() override {return exprTypeVarExpr;};
	//Value* codegen();
	
};

}
