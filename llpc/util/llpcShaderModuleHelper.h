/*
 ***********************************************************************************************************************
 *
 *  Copyright (c) 2019-2020 Advanced Micro Devices, Inc. All Rights Reserved.
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
 * @file  llpcShaderModuleHelper.h
 * @brief LLPC header file: contains the definition of LLPC utility class ShaderModuleHelper.
 ***********************************************************************************************************************
 */

#pragma once
#include "llpc.h"
#include <vector>

namespace Llpc {

// Represents the special header of SPIR-V token stream (the first dword).
struct SpirvHeader {
  unsigned magicNumber;    // Magic number of SPIR-V module
  unsigned spvVersion;     // SPIR-V version number
  unsigned genMagicNumber; // Generator's magic number
  unsigned idBound;        // Upbound (X) of all IDs used in SPIR-V (0 < ID < X)
  unsigned reserved;       // Reserved word
};

// Represents the information of one shader entry in ShaderModuleData
struct ShaderModuleEntry {
  unsigned entryNameHash[4]; // Hash code of entry name
  unsigned entryOffset;      // Byte offset of the entry data in the binCode of ShaderModuleData
  unsigned entrySize;        // Byte size of the entry data
  unsigned passIndex;        // Indices of passes, It is only for internal debug.
};

// Represents the name map <stage, name> of shader entry-point
struct ShaderEntryName {
  ShaderStage stage; // Shader stage
  const char *name;  // Entry name
};

// =====================================================================================================================
// Represents LLPC shader module helper class
class ShaderModuleHelper {
public:
  static Result collectInfoFromSpirvBinary(const BinaryData *spvBinCode, ShaderModuleUsage *shaderModuleUsage,
                                           std::vector<ShaderEntryName> &shaderEntryNames, unsigned *debugInfoSize);

  static void trimSpirvDebugInfo(const BinaryData *spvBin, unsigned bufferSize, void *trimSpvBin);

  static Result optimizeSpirv(const BinaryData *spirvBinIn, BinaryData *spirvBinOut);

  static void cleanOptimizedSpirv(BinaryData *spirvBin);

  static unsigned getStageMaskFromSpirvBinary(const BinaryData *spvBin, const char *entryName);

  static const char *getEntryPointNameFromSpirvBinary(const BinaryData *spvBin);

  static Result verifySpirvBinary(const BinaryData *spvBin);

  static bool isSpirvBinary(const BinaryData *shaderBin);

  static bool isLlvmBitcode(const BinaryData *shaderBin);
};

} // namespace Llpc
