//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/RewriteCSSFragmentShader.h"
#include "ParseHelper.h"

static const char* kGLFragColor = "gl_FragColor";
static const char* kCSSGLFragColor = "css_gl_FragColor";
static const char* kCSSUTexture = "css_u_texture";
static const char* kCSSVTexCoord = "css_v_texCoord";
static const char* kTexture2D = "texture2D(s21;vf2;";
static const char* kMain = "main(";

void RewriteCSSFragmentShader::rewrite()
{
    insertTextureUniform();
    insertTexCoordVarying();
    insertCSSFragColorDeclaration();
    
    // Replace all kGLFragColor with kCSSGLFragColor.
    GlobalParseContext->treeRoot->traverse(this);
}

void RewriteCSSFragmentShader::insertAtTopOfShader(TIntermNode* node)
{
    TIntermSequence& globalSequence = GlobalParseContext->treeRoot->getAsAggregate()->getSequence();
    globalSequence.insert(globalSequence.begin(), node);
}

void RewriteCSSFragmentShader::insertAtEndOfFunction(TIntermNode* node, TIntermAggregate* function)
{
    TIntermAggregate* body = NULL;
    TIntermSequence& paramsAndBody = function->getSequence();
    
    // The function should have parameters and may have a body.
    ASSERT(paramsAndBody.size() == 1 || paramsAndBody.size() == 2);
    
    if (paramsAndBody.size() == 2) {
        body = paramsAndBody[1]->getAsAggregate();
    } else {
        // Make a function body if necessary.
        body = new TIntermAggregate(EOpSequence);
        paramsAndBody.push_back(body);
    }
    
    // The function body should be an aggregate node.
    ASSERT(body);
    
    body->getSequence().push_back(node);
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

TIntermBinary* RewriteCSSFragmentShader::createBinary(TOperator op, TIntermTyped* left, TIntermTyped* right)
{
    TIntermBinary* binary = new TIntermBinary(op);
    binary->setLeft(left);
    binary->setRight(right);
    return binary; 
}

TIntermAggregate* RewriteCSSFragmentShader::createTexture2DCall(const TString& textureUniformName, const TString& texCoordVaryingName)
{
    TIntermAggregate* texture2DCall = createFunctionCall(kTexture2D); // TODO: Maybe pool allocate strings?
    addArgument(createUniformSampler2D(textureUniformName), texture2DCall);
    addArgument(createVaryingVec2(texCoordVaryingName), texture2DCall);
    return texture2DCall;
}

TIntermAggregate* RewriteCSSFragmentShader::createDeclaration(TIntermNode* child)
{
    TIntermAggregate* declaration = new TIntermAggregate(EOpDeclaration);
    declaration->getSequence().push_back(child);
    return declaration;    
}

TIntermBinary* RewriteCSSFragmentShader::createGlobalVec4Initialization(const TString& symbolName, TIntermTyped* rhs)
{
    TIntermBinary* initialization = createBinary(EOpInitialize, createGlobalVec4(symbolName), rhs);
    initialization->setType(TType(EbtFloat, EbpHigh, EvqTemporary, 4)); // TODO: What precision?
    return initialization;
}

void RewriteCSSFragmentShader::addArgument(TIntermNode* argument, TIntermAggregate* functionCall)
{
    functionCall->getSequence().push_back(argument);
}

void RewriteCSSFragmentShader::insertCSSFragColorDeclaration()
{
    insertAtTopOfShader(createDeclaration(createGlobalVec4Initialization(kCSSGLFragColor, createVec4Constant(1.0f, 1.0f, 1.0f, 1.0f))));
}

// Inserts "uniform sampler2D css_u_texture".
void RewriteCSSFragmentShader::insertTextureUniform()
{
    insertAtTopOfShader(createDeclaration(createUniformSampler2D(kCSSUTexture)));
}

// Inserts "varying vec2 css_v_texCoord".
void RewriteCSSFragmentShader::insertTexCoordVarying()
{
    insertAtTopOfShader(createDeclaration(createVaryingVec2(kCSSVTexCoord)));
}

// TODO: Maybe add types to the function call, multiply, assign, etc. They don't seem to be necessary, but it might be good.
// TODO: Maybe assert that main function body is there and the right type.
// Inserts "gl_FragColor = css_FragColor * texture2D(s_texture, v_texCoord)"
void RewriteCSSFragmentShader::insertBlendingOp(TIntermAggregate* mainFunction)
{
    TIntermBinary* rhs = createBinary(EOpMul, createGlobalVec4(kCSSGLFragColor), createTexture2DCall(kCSSUTexture, kCSSVTexCoord));
    TIntermBinary* assign = createBinary(EOpAssign, createGlobalVec4(kGLFragColor), rhs);
    insertAtEndOfFunction(assign, mainFunction);
}

void RewriteCSSFragmentShader::visitSymbol(TIntermSymbol* node)
{
    if (node->getSymbol() == kGLFragColor) {  
        node->setId(0);
        node->getTypePointer()->setQualifier(EvqGlobal);
        node->setSymbol(kCSSGLFragColor);
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
    if (node->getOp() == EOpFunction && node->getName() == kMain)
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
