/*
 ***********************************************************************************************************************
 *
 *  Copyright (c) 2017-2019 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 **********************************************************************************************************************/
/**
 ***********************************************************************************************************************
 * @file  llpcPatchReturns.cpp
 * @brief LLPC source file: contains implementation of class Llpc::PatchReturns.
 ***********************************************************************************************************************
 */
#include "llvm/IR/Constant.h"
#include "llvm/IR/Instructions.h"
#include "llvm/PassSupport.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include <unordered_set>
#include "SPIRVInternal.h"
#include "llpcContext.h"
#include "llpcPatchReturns.h"

#define DEBUG_TYPE "llpc-patch-returns"

using namespace llvm;
using namespace Llpc;

namespace Llpc
{

// =====================================================================================================================
// Initializes static members.
char PatchReturns::ID = 0;

// =====================================================================================================================
// Pass creator, creates the pass of SPIR-V lowering operations for globals
ModulePass* CreatePatchReturns()
{
    return new PatchReturns();
}

// =====================================================================================================================
PatchReturns::PatchReturns()
    :
    Patch(ID),
    m_pRetBlock(nullptr)
{
    initializePatchReturnsPass(*PassRegistry::getPassRegistry());
}

// =====================================================================================================================
// Executes this SPIR-V lowering pass on the specified LLVM module.
bool PatchReturns::runOnModule(
    Module& module)  // [in,out] LLVM module to be run on
{
    LLVM_DEBUG(dbgs() << "Run the pass Spirv-Lower-Global\n");

    Patch::Init(&module);

    LowerOutput();

    return true;
}

// =====================================================================================================================
// Visits "return" instruction.
void PatchReturns::visitReturnInst(
    ReturnInst& retInst)    // [in] "Ret" instruction
{
    // We only handle the "return" in entry point
    if (retInst.getParent()->getParent()->getLinkage() == GlobalValue::InternalLinkage)
    {
        return;
    }

    LLPC_ASSERT(m_pRetBlock != nullptr); // Must have been created
    BranchInst::Create(m_pRetBlock, retInst.getParent());
    m_retInsts.insert(&retInst);
}

// =====================================================================================================================
// Visits "call" instruction.
void PatchReturns::visitCallInst(
    CallInst& callInst) // [in] "Call" instruction
{
    auto pCallee = callInst.getCalledFunction();
    if (pCallee == nullptr)
    {
        return;
    }

    auto mangledName = pCallee->getName();

    if (mangledName.startswith("llvm.amdgcn.exp"))
    {
        //m_emitCalls.insert(&callInst);
    }
}

// =====================================================================================================================
// Does lowering opertions for SPIR-V outputs, replaces outputs with proxy variables.
void PatchReturns::LowerOutput()
{
    m_pRetBlock = BasicBlock::Create(*m_pContext, "", m_pEntryPoint);

    auto pRetInst = ReturnInst::Create(*m_pContext, m_pRetBlock);

    visit(m_pModule);

    for (auto retInst : m_retInsts)
    {
        retInst->dropAllReferences();
        retInst->eraseFromParent();
    }
}

} // Llpc

// =====================================================================================================================
// Initializes the pass of patching returns.
INITIALIZE_PASS(PatchReturns, DEBUG_TYPE,
                "Unify returns", false, false)
