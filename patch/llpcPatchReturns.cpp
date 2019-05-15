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
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include <unordered_set>
#include "llpcContext.h"
#include "llpcPatchReturns.h"
#include "llpcPipelineShaders.h"

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

    bool changed = false;
    auto pPipelineShaders = &getAnalysis<PipelineShaders>();
    for (uint32_t shaderStage = 0; shaderStage < ShaderStageCountInternal; ++shaderStage)
    {
        m_pEntryPoint = pPipelineShaders->GetEntryPoint(static_cast<ShaderStage>(shaderStage));
        if (m_pEntryPoint != nullptr)
        {
            m_shaderStage = static_cast<ShaderStage>(shaderStage);
            changed |= LowerOutput();
        }
    }


    return changed;
}

// =====================================================================================================================
// Visits "return" instruction.
void PatchReturns::visitReturnInst(
    ReturnInst& retInst)    // [in] "Ret" instruction
{
    // We only handle the "return" in entry point
    Function* function = retInst.getParent()->getParent();
    if (function->getLinkage() == GlobalValue::InternalLinkage
        || function != m_pEntryPoint)
    {
        return;
    }

    // Only handle the return if it is preceeded with a call to export with
    // the done flag set.
    auto it = retInst.getParent()->rbegin();
    it++;

    Instruction &beforeInst = *it;
    CallInst *beforeInstCall = dyn_cast<CallInst>(&beforeInst);
    if (beforeInstCall != nullptr)
    {
        auto mangledName = beforeInstCall->getCalledFunction()->getName();

        if (!mangledName.startswith("llvm.amdgcn.exp"))
        {
            return;
        }

        auto doneArg = dyn_cast<Constant>(beforeInstCall->getArgOperand(beforeInstCall->arg_size() - 2));
        if (!doneArg || doneArg->isZeroValue())
        {
            // Done is not set
            return;
        }

        m_retInsts[&retInst] = beforeInstCall;
    }
}

// =====================================================================================================================
// Does lowering opertions for SPIR-V outputs, replaces outputs with proxy variables.
bool PatchReturns::LowerOutput()
{
    m_pRetBlock = BasicBlock::Create(*m_pContext, "", m_pEntryPoint);

    visit(*m_pEntryPoint);

    if (m_retInsts.size() <= 1)
    {
        m_pRetBlock->eraseFromParent();
        return false;
    }

    CallInst *callInst0 = m_retInsts.begin()->second;

    // Create and insert phi nodes
    unsigned argSize = callInst0->arg_size();
    std::vector<PHINode*> phiInsts;
    phiInsts.reserve(argSize);

    for (auto& arg : callInst0->args())
    {
        auto phi = PHINode::Create(arg->getType(), argSize, "", m_pRetBlock);
        phiInsts.push_back(phi);
    }

    // Fill phi nodes
    for (auto retInst : m_retInsts)
    {
        auto *callInst = retInst.second;
        auto bb = callInst->getParent();
        for (size_t i = 0; i < argSize; i++)
        {
            phiInsts[i]->addIncoming(callInst->getArgOperand(i), bb);
        }
    }

    // Insert call
    std::vector<Value*> argsVec;
    argsVec.reserve(argSize);
    for (const auto arg : phiInsts)
    {
        argsVec.push_back(arg);
    }
    auto *pCallInst = CallInst::Create(callInst0->getFunctionType(), callInst0->getCalledValue(),
        argsVec, callInst0->getName(), m_pRetBlock);
    pCallInst->setTailCallKind(callInst0->getTailCallKind());
    pCallInst->setCallingConv(callInst0->getCallingConv());
    pCallInst->setAttributes(callInst0->getAttributes());
    pCallInst->setDebugLoc(callInst0->getDebugLoc());

    // Insert return
    auto pRetInst = ReturnInst::Create(*m_pContext, m_pRetBlock);

    for (auto retInst : m_retInsts)
    {
        BranchInst::Create(m_pRetBlock, retInst.first->getParent());
        retInst.first->dropAllReferences();
        retInst.first->eraseFromParent();
        retInst.second->dropAllReferences();
        retInst.second->eraseFromParent();
    }
    m_retInsts.clear();

    return true;
}

} // Llpc

// =====================================================================================================================
// Initializes the pass of patching returns.
INITIALIZE_PASS(PatchReturns, DEBUG_TYPE,
                "Unify returns", false, false)
