
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_plaf_basic_BasicMenuItemUI$ClickAction__
#define __javax_swing_plaf_basic_BasicMenuItemUI$ClickAction__

#pragma interface

#include <javax/swing/AbstractAction.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
      namespace event
      {
          class ActionEvent;
      }
    }
  }
  namespace javax
  {
    namespace swing
    {
      namespace plaf
      {
        namespace basic
        {
            class BasicMenuItemUI;
            class BasicMenuItemUI$ClickAction;
        }
      }
    }
  }
}

class javax::swing::plaf::basic::BasicMenuItemUI$ClickAction : public ::javax::swing::AbstractAction
{

public: // actually package-private
  BasicMenuItemUI$ClickAction(::javax::swing::plaf::basic::BasicMenuItemUI *);
public:
  virtual void actionPerformed(::java::awt::event::ActionEvent *);
public: // actually package-private
  ::javax::swing::plaf::basic::BasicMenuItemUI * __attribute__((aligned(__alignof__( ::javax::swing::AbstractAction)))) this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_plaf_basic_BasicMenuItemUI$ClickAction__
