//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_REWRITE_CSS_SHADER_BASE
#define COMPILER_REWRITE_CSS_SHADER_BASE

#include "GLSLANG/ShaderLang.h"

#include "compiler/intermediate.h"
#include "compiler/SymbolTable.h"

class TInfoSinkBase;

class RewriteCSSShaderBase {
public:
    RewriteCSSShaderBase(TIntermNode* treeRoot, const TSymbolTable& table, const TString& hiddenSymbolSuffix)
        : root(treeRoot)
        , symbolTable(table)
        , texCoordVaryingName(kTexCoordVaryingPrefix + hiddenSymbolSuffix) {}

    virtual void rewrite();
    TIntermNode* getNewTreeRoot() { return root; }

    virtual ~RewriteCSSShaderBase() {}

protected:
    static const char* const kTexCoordVaryingPrefix;
    static const char* const kMain;
    
    void insertAtBeginningOfShader(TIntermNode* node);
    void insertAtEndOfShader(TIntermNode* node);
    void insertAtBeginningOfFunction(TIntermAggregate* function, TIntermNode* node);
    void insertAtEndOfFunction(TIntermAggregate* function, TIntermNode* node);

    TIntermAggregate* findFunction(const TString& name);
    void renameFunction(const TString& oldFunctionName, const TString& newFunctionName);
    bool isSymbolUsed(const TString& symbolName);
    
    const TType& getBuiltinType(const TString& builtinName);

    TIntermNode* root;
    const TSymbolTable& symbolTable;
    TString texCoordVaryingName;

private:
    void createRootSequenceIfNeeded();
    TIntermAggregate* getOrCreateFunctionBody(TIntermAggregate* function);
};

#endif  // COMPILER_REWRITE_CSS_SHADER_BASE
