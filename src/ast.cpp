#include "ast.hpp"

std::map<std::string, llvm::Value *> NamedValues;
std::map<std::string, FunctionAST*> Functions;

ConstBoolAST::ConstBoolAST(int val_) : val(val_){}
llvm::Value* ConstBoolAST::execute(){
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(*TheContext),llvm::APInt(1,val));
}

BinopBoolAST::BinopBoolAST(ExprAST* l_, BoolOPT op_, ExprAST* r_) : l(l_), op(op_), r(r_){}

llvm::Value* BinopBoolAST::execute(){
    llvm::Value* left = l->execute();
    llvm::Value* right = r->execute();
    switch (op){
        case op_EQ:
            return Builder->CreateFCmpUEQ(left,right);
        case op_GE:
            return Builder->CreateFCmpUGE(left,right);
        case op_LE:
            return Builder->CreateFCmpULE(left,right);
        case op_NEQ:
            return Builder->CreateFCmpUNE(left,right);
        case op_GT:
            return Builder->CreateFCmpUGT(left,right);
        case op_LT:
            return Builder->CreateFCmpULT(left,right);
        default:
            printf("wtf\n");
            exit(1);
    }
}

NotBoolAST::NotBoolAST(BoolAST* val_) : val(val_){}

llvm::Value* NotBoolAST::execute(){
    llvm::Value* v = val->execute();
    return Builder->CreateNot(v);
}

LOPBoolAST::LOPBoolAST(BoolAST* l_, char op_, BoolAST* r_) : l(l_), op(op_), r(r_){}

llvm::Value* LOPBoolAST::execute(){
    llvm::Value* left = l->execute();
    llvm::Value* right = r->execute();
    switch (op){
        case '&':
            return Builder->CreateLogicalAnd(left,right);
        case '|':
            return Builder->CreateLogicalOr(left,right);
        default:
            printf("wtf\n");
            exit(1);
    }
}

NumberExprAST::NumberExprAST(double val_) : val(val_){}

llvm::Value* NumberExprAST::execute(){
    return llvm::ConstantFP::get(*TheContext, llvm::APFloat(val));
}

NamedExprAST::NamedExprAST(const char* name_){
    strcpy(name,name_);
}
llvm::Value* NamedExprAST::execute(){
    llvm::Value* out = NamedValues[name];
    if (!out){
        printf("No variabled named %s\n",name);
        exit(1);
    }
    return out;
}

BinopExprAST::BinopExprAST(ExprAST* l_, char op_, ExprAST* r_) : l(l_), op(op_), r(r_){}

llvm::Value* BinopExprAST::execute(){
    llvm::Value* left = l->execute();
    llvm::Value* right = r->execute();
    switch (op){
        case '+':
            return Builder->CreateFAdd(left,right);
        case '-':
            return Builder->CreateFSub(left,right);
        case '*':
            return Builder->CreateFMul(left,right);
        case '/':
            return Builder->CreateFDiv(left,right);
        default:
            printf("wtf\n");
            exit(1);
    }
}

IfExprAST::IfExprAST(BoolAST* cond_, ExprAST* t_, ExprAST* f_) : cond(cond_), t(t_), f(f_){}

llvm::Value* IfExprAST::execute(){

    llvm::Value* cond_expr = cond->execute();

    llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *ThenBB =
        llvm::BasicBlock::Create(*TheContext, "then", TheFunction);
    llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(*TheContext, "else");
    llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(*TheContext, "ifcont");

    Builder->CreateCondBr(cond_expr, ThenBB, ElseBB);

    Builder->SetInsertPoint(ThenBB);
    llvm::Value* then_expr = t->execute();

    Builder->CreateBr(MergeBB);
    ThenBB = Builder->GetInsertBlock();
    TheFunction->insert(TheFunction->end(), ElseBB);
    Builder->SetInsertPoint(ElseBB);

    llvm::Value *else_expr = f->execute();
    Builder->CreateBr(MergeBB);
    ElseBB = Builder->GetInsertBlock();
    TheFunction->insert(TheFunction->end(), MergeBB);

    Builder->SetInsertPoint(MergeBB);

    llvm::PHINode *PN = Builder->CreatePHI(llvm::Type::getDoubleTy(*TheContext), 2);

    PN->addIncoming(then_expr, ThenBB);
    PN->addIncoming(else_expr, ElseBB);

    return PN;
}

llvm::Function *getFunction(std::string Name) {
  // First, see if the function has already been added to the current module.
  /*if (auto *F = TheModule->getFunction(Name))
    return F;

  // If not, check whether we can codegen the declaration from some existing
  // prototype.
  auto FI = FunctionProtos.find(Name);
  if (FI != FunctionProtos.end())
    return FI->second->codegen();*/

  // If no existing prototype exists, return null.
  return nullptr;
}

CallExprAST::CallExprAST(const char* name_, ExprList* args_) : args(args_){
    strcpy(name,name_);

    //llvm::Function *CalleeF = llvm::getFunction(name);
}

llvm::Value* CallExprAST::execute(){
    llvm::Function* to_call = TheModule->getFunction(name);
    if (!to_call){
        auto tmp = Functions[name];
        if(!tmp){
            printf("No function named %s\n",name);
            exit(1);
        }
        //printf("FOUND ORIGINAL!\n");
        to_call = tmp->generate_header();
    }
    if(!to_call){
        printf("No function named %s\n",name);
        exit(1);
    }
    if (args->size() != to_call->arg_size()){
        printf("mismatch arguments in %s\n",name);
        exit(1);
    }
    std::vector<llvm::Value*> inputs;
    for (auto arg : *args){
        inputs.push_back(arg->execute());
    }

    return Builder->CreateCall(to_call, inputs);
}

FunctionAST::FunctionAST(const char* name_, IdList* inputs_) : inputs(inputs_){
    strcpy(name,name_);
}

FunctionAST::FunctionAST(const char* name_) : inputs(make_object(IdList)){
    strcpy(name,name_);
}

void FunctionAST::add_expr(ExprAST* expr_){
    expr = expr_;
}

llvm::Function* FunctionAST::generate_header(){
    std::vector<llvm::Type *> Doubles(inputs->size(), llvm::Type::getDoubleTy(*TheContext));
    llvm::FunctionType *FT =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), Doubles, false);
    llvm::Function *F =
      llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, TheModule.get());
    return F;
}

void FunctionAST::execute(){
    
    std::vector<llvm::Type *> Doubles(inputs->size(), llvm::Type::getDoubleTy(*TheContext));
    llvm::FunctionType *FT =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), Doubles, false);
    llvm::Function *F =
      llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, TheModule.get());
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(*TheContext, "entry", F);
    Builder->SetInsertPoint(BB);

    Functions[name] = this;

    NamedValues.clear();
    int count = 0;
    for (auto &Arg : F->args()){
        NamedValues[(*inputs)[count]] = &Arg;
        count++;
    }

    llvm::Value* ret = expr->execute();
    Builder->CreateRet(ret);

    llvm::verifyFunction(*F);

    ExitOnErr(TheJIT->addModule(
          llvm::orc::ThreadSafeModule(std::move(TheModule), std::move(TheContext))));
    
    init_env();

}

TopLevelExprAST::TopLevelExprAST(ExprAST* expr_) : expr(expr_){}

void TopLevelExprAST::execute(){
    std::vector<llvm::Type *> Doubles(0, llvm::Type::getDoubleTy(*TheContext));
    llvm::FunctionType *FT =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), Doubles, false);
    llvm::Function *F =
      llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "__anon__", TheModule.get());
    NamedValues.clear();
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(*TheContext, "entry", F);
    Builder->SetInsertPoint(BB);

    llvm::Value* ret = expr->execute();
    Builder->CreateRet(ret);
    llvm::verifyFunction(*F);
    //verifyFunction(*F);
    // Run the optimizer on the function.
    //TheFPM->run(*F, *TheFAM);

    auto RT = TheJIT->getMainJITDylib().createResourceTracker();

    auto TSM = llvm::orc::ThreadSafeModule(std::move(TheModule), std::move(TheContext));

    ExitOnErr(TheJIT->addModule(std::move(TSM), RT));

    init_env();

    auto ExprSymbol = ExitOnErr(TheJIT->lookup("__anon__"));

    double (*FP)() = ExprSymbol.getAddress().toPtr<double (*)()>();
    printf("%f\n", FP());

    ExitOnErr(RT->remove());
}