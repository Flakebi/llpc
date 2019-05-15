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
 * @file  llpcPatchReturns.h
 * @brief LLPC header file: contains declaration of class Llpc::PatchReturns.
 ***********************************************************************************************************************
 */
#pragma once

#include "llvm/IR/InstVisitor.h"

#include <unordered_map>
#include <unordered_set>
#include <list>
#include "llpcPatch.h"

namespace Llpc
{

// =====================================================================================================================
// Represents the pass of SPIR-V lowering opertions for globals (global variables, inputs, and outputs).
class PatchReturns:
    public Patch,
    public llvm::InstVisitor<PatchReturns>
{
public:
    PatchReturns();

    virtual bool runOnModule(llvm::Module& module);
    virtual void visitReturnInst(llvm::ReturnInst& retInst);
    virtual void visitCallInst(llvm::CallInst& callInst);

    // -----------------------------------------------------------------------------------------------------------------

    static char ID;   // ID of this pass

private:
    LLPC_DISALLOW_COPY_AND_ASSIGN(PatchReturns);

    void LowerOutput();

    // -----------------------------------------------------------------------------------------------------------------

    llvm::BasicBlock*   m_pRetBlock;    // The return block of entry point

    std::unordered_set<llvm::ReturnInst*>  m_retInsts;      // "Return" instructions to be removed
};

} // Llpc
