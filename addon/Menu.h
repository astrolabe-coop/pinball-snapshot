/***************************************************************************
                          Menu.h  -  description
                             -------------------
    begin                : Tue Feb 15 2000
    copyright            : (C) 2000 by Henrik Enqvist
    email                : henqvist@excite.com
 ***************************************************************************/

#ifndef MENU_H
#define MENU_H

#include <vector>

class Engine;
class EmFont;

#define EM_MENU_NOP         0
#define EM_MENU_EXIT        1
#define EM_MENU_RESUME      2
#define EM_MENU_BACK        3

#define EM_MENU             0
#define EM_MENU_SUB         1
#define EM_MENU_CHOOSE      2
#define EM_MENU_INSERT      3
#define EM_MENU_FCT         4

/** The base class for all menus. */
class MenuItem {
 public:
  MenuItem(Engine* e, int type);
  virtual ~MenuItem();
  virtual const char* getText() = 0;
  virtual int perform() = 0;
  inline int getType() { return m_iType; };
  inline Engine * getEngine() { return p_Engine; };
 protected:
  EmFont* p_EmFont;
  Engine* p_Engine;
  int m_iType;
};

/** A meny holding sub menys, can also be used for a back or exit item */
class MenuSub : public MenuItem {
 public:
  MenuSub(const char* name, Engine* e);
  ~MenuSub();
  void addMenuItem(MenuItem* m);
  int perform();
  void draw();
  /** Gives the number of submenus */
  inline int size() { return m_vMenuItem.size(); };
  inline void setAction(int a) { m_iAction = a; };
  inline const char* getText() { return m_Name; };
 protected:
  char m_Name[64];
  int m_iCurrent;
  int m_iAction;
  vector<MenuItem*> m_vMenuItem;
};

class MenuChoose : public MenuItem {
 public:
  MenuChoose(Engine* e);
  ~MenuChoose();
  void addText(const char * text);
  const char* getText();
  int perform();
  int next();
  int prev();
  inline int getCurrent() { return m_iCurrent; };
  inline void setCurrent(int c) { m_iCurrent = c; m_iPrevious = c; };
 protected:
  int m_iCurrent;
  /** Previous is the currently active alternative in the engine. */
  int m_iPrevious;
  vector<char*> m_vText;
  char t_Str[64];
};

/** You can create a MenuFct instance and pass i a function
 * or you can subclass it and override the 'perform' fucntion. 
 * The perform function should return EM_MENU_BACK, EM_MENU_NOP or EM_MENU_QUIT. */
class MenuFct : public MenuItem {
 public:
  MenuFct(const char * name, int (*fct)(void), Engine* e);
  ~MenuFct();
  int perform();
  virtual const char* getText() { return m_Name; };
 protected:
  char m_Name[64];
  int (*p_Fct)(void);
};

#endif // MENU_H