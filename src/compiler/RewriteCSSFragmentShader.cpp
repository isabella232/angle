//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/RewriteCSSFragmentShader.h"
#include "ParseHelper.h"

void RewriteCSSFragmentShader::rewrite()
{
    insertTextureUniform();
    insertCSSFragColorDeclaration();
    
    // Replace all "gl_FragColor" with "css_FragColor".
    GlobalParseContext->treeRoot->traverse(this);
}

void RewriteCSSFragmentShader::insertAtTopOfShader(TIntermNode* node)
{
    TIntermSequence& globalSequence = GlobalParseContext->treeRoot->getAsAggregate()->getSequence();
    globalSequence.insert(globalSequence.begin(), node);
}

void RewriteCSSFragmentShader::insertAtEndOfFunction(TIntermNode* node, TIntermAggregate* function)
{
    // Insert at the end of the main function.
    // FIXME: Handle main with no body. Crashes.
    TIntermAggregate* functionBody = function->getSequence()[1]->getAsAggregate();
    functionBody->getSequence().push_back(node);
}

TIntermConstantUnion* RewriteCSSFragmentShader::createVec4Constant(float x, float y, float z, float w)
{
    ConstantUnion* constantArray = new ConstantUnion[4];
    constantArray[0].setFConst(x);
    constantArray[1].setFConst(y);
    constantArray[2].setFConst(z);
    constantArray[3].setFConst(w);
    return new TIntermConstantUnion(constantArray, TType(EbtFloat, EbpUndefined, EvqConst, 4));    
}

TIntermSymbol* RewriteCSSFragmentShader::createGlobalVec4(const TString& name)
{
    return new TIntermSymbol(0, name, TType(EbtFloat, EbpHigh, EvqGlobal, 4));
}

TIntermSymbol* RewriteCSSFragmentShader::createUniformSampler2D(const TString& name)
{
    return new TIntermSymbol(0, name, TType(EbtSampler2D, EbpUndefined, EvqUniform));
}

TIntermSymbol* RewriteCSSFragmentShader::createVaryingVec2(const TString& name)
{
    return new TIntermSymbol(0, name, TType(EbtFloat, EbpHigh, EvqVaryingIn, 2));
}

TIntermAggregate* RewriteCSSFragmentShader::createFunctionCall(const TString& name)
{
    TIntermAggregate* functionCall = new TIntermAggregate(EOpFunctionCall);
    functionCall->setName(name);
    return functionCall;
}

void RewriteCSSFragmentShader::addArgument(TIntermNode* argument, TIntermAggregate* functionCall)
{
    functionCall->getSequence().push_back(argument);
}

// TODO: Pool allocate strings.
// TODO: What precision?
void RewriteCSSFragmentShader::insertCSSFragColorDeclaration()
{
    // Declaration
    TIntermBinary* initialize = new TIntermBinary(EOpInitialize);
    initialize->setType(TType(EbtFloat, EbpHigh, EvqTemporary, 4));
    initialize->setLeft(createGlobalVec4("css_FragColor"));
    initialize->setRight(createVec4Constant(1.0f, 1.0f, 1.0f, 1.0f));
    
    TIntermAggregate* declaration = new TIntermAggregate(EOpDeclaration);
    declaration->getSequence().push_back(initialize);
    
    insertAtTopOfShader(declaration);
}

// Inserts "uniform sampler2D s_texture".
void RewriteCSSFragmentShader::insertTextureUniform()
{
    TIntermAggregate* declaration = new TIntermAggregate(EOpDeclaration);
    declaration->getSequence().push_back(createUniformSampler2D("s_texture"));

    insertAtTopOfShader(declaration);
}

// TODO: How should we manage v_texCoord. Is it safe to allow it to be custom defined? If so, do we enforce its type?
// Inserts "varying vec2 v_texCoord" if not present.
void RewriteCSSFragmentShader::insertTexCoordVarying()
{
    
}

// TODO: Maybe add types to the function call, multiply, assign, etc. They don't seem to be necessary, but it might be good.
// TODO: Maybe assert that main function body is there and the right type.
// Inserts "gl_FragColor = css_FragColor * texture2D(s_texture, v_texCoord)"
void RewriteCSSFragmentShader::insertBlendingOp(TIntermAggregate* mainFunction)
{
    TIntermAggregate* texture2DCall = createFunctionCall("texture2D(s21;vf2;");
    addArgument(createUniformSampler2D("s_texture"), texture2DCall);
    addArgument(createVaryingVec2("v_texCoord"), texture2DCall);
    
    // css_FragColor * texture2D...
    TIntermBinary* mul = new TIntermBinary(EOpMul);
    mul->setLeft(createGlobalVec4("css_FragColor"));
    mul->setRight(texture2DCall);
    
    // gl_FragColor = css_FragColor * ...
    TIntermBinary* assign = new TIntermBinary(EOpAssign);
    assign->setLeft(createGlobalVec4("gl_FragColor"));
    assign->setRight(mul);

    insertAtEndOfFunction(assign, mainFunction);
}

void RewriteCSSFragmentShader::visitSymbol(TIntermSymbol* node)
{
    const char* kFragColor = "gl_FragColor";
    if (node->getSymbol() == kFragColor) {  
        node->setId(0);
        node->getTypePointer()->setQualifier(EvqGlobal);
        node->setSymbol("css_FragColor");
    }
}

bool RewriteCSSFragmentShader::visitBinary(Visit visit, TIntermBinary* node)
{
    return true;
}

bool RewriteCSSFragmentShader::visitUnary(Visit visit, TIntermUnary* node)
{
    return true;
}

bool RewriteCSSFragmentShader::visitSelection(Visit visit, TIntermSelection* node)
{
    return true;
}

bool RewriteCSSFragmentShader::visitAggregate(Visit visit, TIntermAggregate* node)
{
    if (node->getOp() == EOpFunction && node->getName() == "main(")
        insertBlendingOp(node);
    
    return true;
}

bool RewriteCSSFragmentShader::visitLoop(Visit visit, TIntermLoop* node)
{
    return true;
}

bool RewriteCSSFragmentShader::visitBranch(Visit visit, TIntermBranch* node)
{
    return true;
}
