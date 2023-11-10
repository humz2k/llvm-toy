#include <stdio.h>
#include <stdlib.h>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "KaleidoscopeJIT.h"
#include <vector>
#include <memory>
#include <string.h>
#include <string>
#include <map>
#include "gc.h"

#define make_object(obj,...) new (GC_malloc(sizeof(obj))) obj(__VA_ARGS__)

extern std::unique_ptr<llvm::LLVMContext> TheContext;
extern std::unique_ptr<llvm::Module> TheModule;
extern std::unique_ptr<llvm::IRBuilder<>> Builder;
extern std::unique_ptr<llvm::FunctionPassManager> TheFPM;
extern std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM;
extern std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM;
extern std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC;
extern std::unique_ptr<llvm::StandardInstrumentations> TheSI;
extern std::unique_ptr<llvm::orc::KaleidoscopeJIT> TheJIT;
extern llvm::ExitOnError ExitOnErr;

void init_env();

typedef std::vector<std::string> IdList;

class ExprAST {
    public:
        virtual ~ExprAST() = default;
        virtual llvm::Value* execute() = 0;
};

typedef std::vector<ExprAST*> ExprList;

class BoolAST {
    public:
        virtual ~BoolAST() = default;
        virtual llvm::Value* execute() = 0;
};

class ConstBoolAST : public BoolAST{
    int val;
    public:
        ConstBoolAST(int val_);
        llvm::Value* execute() override;
};

class UnitAST{
    public:
        virtual ~UnitAST() = default;
        virtual void execute() = 0;
};

class TopLevelExprAST : public UnitAST{
    ExprAST* expr;
    public:
        TopLevelExprAST(ExprAST* expr_);
        void execute() override;
};

class FunctionAST : public UnitAST{
    char name[31];
    IdList* inputs;
    ExprAST* expr;
    public:
        FunctionAST(const char* name_, IdList* inputs_);
        FunctionAST(const char* name_);
        void add_expr(ExprAST* expr_);
        void execute() override;
        llvm::Function* generate_header();
};

typedef enum {
    op_LE,
    op_GE,
    op_EQ,
    op_NEQ,
    op_LT,
    op_GT
} BoolOPT;

class BinopBoolAST : public BoolAST{
    ExprAST* l;
    ExprAST* r;
    BoolOPT op;
    public:
        BinopBoolAST(ExprAST* l_, BoolOPT op_, ExprAST* r_);
        llvm::Value* execute() override;
};

class NotBoolAST : public BoolAST{
    BoolAST* val;
    public:
        NotBoolAST(BoolAST* val_);
        llvm::Value* execute() override;
};

class LOPBoolAST : public BoolAST{
    BoolAST* l;
    BoolAST* r;
    char op;
    public:
        LOPBoolAST(BoolAST* l_, char op_, BoolAST* r_);
        llvm::Value* execute() override;
};

class NumberExprAST : public ExprAST {
    double val;
    public:
        NumberExprAST(double val_);
        llvm::Value* execute() override;
};

class NamedExprAST : public ExprAST {
    char name[31];
    public:
        NamedExprAST(const char* name_);
        llvm::Value* execute() override;
};

class BinopExprAST : public ExprAST {
    ExprAST* l;
    ExprAST* r;
    char op;
    public:
        BinopExprAST(ExprAST* l_, char op_, ExprAST* r_);
        llvm::Value* execute() override;
};

class IfExprAST : public ExprAST {
    BoolAST* cond;
    ExprAST* t;
    ExprAST* f;
    public:
        IfExprAST(BoolAST* cond_, ExprAST* t_, ExprAST* f_ = NULL);
        llvm::Value* execute() override;
};

class CallExprAST : public ExprAST {
    char name[31];
    ExprList* args;
    public:
        CallExprAST(const char* name_, ExprList* args_);
        llvm::Value* execute() override;
};