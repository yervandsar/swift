//===--- CFGGenExpr.cpp - Implements Lowering of ASTs -> CFGs for Exprs ---===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2015 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "CFGGen.h"
#include "swift/AST/AST.h"
using namespace swift;
using namespace Lowering;

CFGValue CFGGen::visitApplyExpr(ApplyExpr *E) {
  CFGValue FnV = visit(E->getFn());
  llvm::SmallVector<CFGValue, 10> ArgsV;
  
  // Special case Arg being a TupleExpr, to inline the arguments and
  // not create another instruction.
  if (TupleExpr *TU = dyn_cast<TupleExpr>(E->getArg())) {
    for (auto arg : TU->getElements())
      ArgsV.push_back(visit(arg));
  } else {
    ArgsV.push_back(visit(E->getArg()));
  }
  
  return B.createApply(E, FnV, ArgsV);
}

CFGValue CFGGen::visitDeclRefExpr(DeclRefExpr *E) {
  // If this is a reference to a mutable decl, produce an lvalue.
  if (E->getType()->is<LValueType>()) {
    assert(VarLocs.count(E->getDecl()) && "VarDecl location not generated?");
    return VarLocs[E->getDecl()];
  }
  
  // Otherwise, we can only produce its value, use a ConstantRefInst.
  return B.createConstantRef(E);
}

CFGValue CFGGen::visitIntegerLiteralExpr(IntegerLiteralExpr *E) {
  return B.createIntegerLiteral(E);
}

CFGValue CFGGen::visitLoadExpr(LoadExpr *E) {
  CFGValue SubV = visit(E->getSubExpr());
  return B.createLoad(E, SubV);
}

CFGValue CFGGen::visitMaterializeExpr(MaterializeExpr *E) {
  // Evaluate the value, use it to initialize a new temporary and return the
  // temp's address.
  CFGValue Value = visit(E->getSubExpr());
  CFGValue TmpMem = B.createAllocTmp(E);
  B.createInitialization(E, Value, TmpMem);
  return TmpMem;
}


CFGValue CFGGen::visitRequalifyExpr(RequalifyExpr *E) {
  CFGValue SubV = visit(E->getSubExpr());
  return B.createRequalify(E, SubV);
}

CFGValue CFGGen::visitParenExpr(ParenExpr *E) {
  return visit(E->getSubExpr());
}

CFGValue CFGGen::visitTupleExpr(TupleExpr *E) {
  llvm::SmallVector<CFGValue, 10> ArgsV;
  for (auto &I : E->getElements())
    ArgsV.push_back(visit(I));
  return B.createTuple(E, ArgsV);
}

CFGValue CFGGen::visitScalarToTupleExpr(ScalarToTupleExpr *E) {
  return B.createScalarToTuple(E, visit(E->getSubExpr()));
}

CFGValue CFGGen::visitTupleElementExpr(TupleElementExpr *E) {
  return B.createTupleElement(E, visit(E->getBase()), E->getFieldNumber());
}

CFGValue CFGGen::visitTypeOfExpr(TypeOfExpr *E) {
  return B.createTypeOf(E);
}

