#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include "ast.hpp"

int yyparse();
extern "C" int yylex();
extern FILE* yyin;

std::unique_ptr<llvm::LLVMContext> TheContext;
std::unique_ptr<llvm::Module> TheModule;
std::unique_ptr<llvm::IRBuilder<>> Builder;
std::unique_ptr<llvm::FunctionPassManager> TheFPM;
std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM;
std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM;
std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC;
std::unique_ptr<llvm::StandardInstrumentations> TheSI;
std::unique_ptr<llvm::orc::KaleidoscopeJIT> TheJIT;

llvm::ExitOnError ExitOnErr;

void init_env(){

    TheContext = std::make_unique<llvm::LLVMContext>();
    TheModule = std::make_unique<llvm::Module>("my cool jit", *TheContext);
    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

    TheFPM = std::make_unique<llvm::FunctionPassManager>();
    TheFAM = std::make_unique<llvm::FunctionAnalysisManager>();
    TheMAM = std::make_unique<llvm::ModuleAnalysisManager>();
    ThePIC = std::make_unique<llvm::PassInstrumentationCallbacks>();
    TheSI = std::make_unique<llvm::StandardInstrumentations>(*TheContext,
                                                        /*DebugLogging*/ true);
    TheSI->registerCallbacks(*ThePIC, TheMAM.get());

    //TheFPM->addPass(llvm::InstCombinePass());
    //TheFPM->addPass(llvm::ReassociatePass());
    //TheFPM->addPass(llvm::GVNPass());
    //TheFPM->addPass(llvm::SimplifyCFGPass());

    TheFAM->registerPass([&] { return llvm::AAManager(); });
    TheFAM->registerPass([&] { return llvm::DominatorTreeAnalysis(); });
    TheFAM->registerPass([&] { return llvm::LoopAnalysis(); });
    TheFAM->registerPass([&] { return llvm::MemoryDependenceAnalysis(); });
    TheFAM->registerPass([&] { return llvm::MemorySSAAnalysis(); });
    TheFAM->registerPass([&] { return llvm::OptimizationRemarkEmitterAnalysis(); });
    TheFAM->registerPass([&] {
        return llvm::OuterAnalysisManagerProxy<llvm::ModuleAnalysisManager, llvm::Function>(*TheMAM);
    });
    TheFAM->registerPass(
        [&] { return llvm::PassInstrumentationAnalysis(ThePIC.get()); });
    TheFAM->registerPass([&] { return llvm::TargetIRAnalysis(); });
    TheFAM->registerPass([&] { return llvm::TargetLibraryAnalysis(); });

    TheMAM->registerPass([&] { return llvm::ProfileSummaryAnalysis(); });

    TheModule->setDataLayout(TheJIT->getDataLayout());
}

int main(int argc, char *argv[]){
    if (argc == 1)return -1;

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    TheJIT = ExitOnErr(llvm::orc::KaleidoscopeJIT::Create());

    init_env();

    FILE* fp = fopen(argv[1], "r");
    yyin = fp;
    yyparse();
    fclose(fp);

    //TheModule->print(llvm::errs(), nullptr);

    return 0;
}