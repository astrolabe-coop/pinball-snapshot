/***************************************************************************
                          OpenGLVisitor.h  -  description
                             -------------------
    begin                : Sat Jan 6 2001
    copyright            : (C) 2001 by Henrik Enqvist
    email                : henqvist@excite.com
 ***************************************************************************/


#ifndef OPENGLVISITOR_H
#define OPENGLVISITOR_H

#include "Private.h"

#include "Visitor.h"

class Group;
class Shape3D;

/**
  *@author Henrik Enqvist
  */

class OpenGLVisitor : public Visitor  {
	public:
		OpenGLVisitor();
		~OpenGLVisitor();
		void visit(Group* g);
	private:
		void visit(Shape3D* s, Group* g);
};

#endif // OPENGLVISITOR_H
