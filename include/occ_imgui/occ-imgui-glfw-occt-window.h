#pragma once

#include <opencascade/Aspect_DisplayConnection.hxx>
#include <opencascade/Aspect_RenderingContext.hxx>
#include <opencascade/Aspect_Window.hxx>
#include <opencascade/Graphic3d_Vec.hxx>
#include <opencascade/TCollection_AsciiString.hxx>

struct GLFWwindow;

//! GLFWwindow wrapper implementing Aspect_Window interface.
class GlfwOcctWindow : public Aspect_Window
{
  DEFINE_STANDARD_RTTI_INLINE(GlfwOcctWindow, Aspect_Window)

public:
  //! Main constructor.
  GlfwOcctWindow(int theWidth, int theHeight, const TCollection_AsciiString& theTitle);

  //! Close the window.
  ~GlfwOcctWindow() override { Close(); }

  //! Close the window.
  void Close();

  //! Return X Display connection.
  const Handle(Aspect_DisplayConnection)& GetDisplay() const { return myDisplay; }

  //! Return GLFW window.
  GLFWwindow* getGlfwWindow() const { return myGlfwWindow; }

  //! Return native OpenGL context.
  Aspect_RenderingContext NativeGlContext() const;

  //! Return cursor position.
  Graphic3d_Vec2i CursorPosition() const;

public:
  //! Returns native Window handle
  Aspect_Drawable NativeHandle() const override;

  //! Returns parent of native Window handle.
  Aspect_Drawable NativeParentHandle() const override { return 0; }

  //! Applies the resizing to the window <me>
  Aspect_TypeOfResize DoResize() override;

  //! Returns True if the window <me> is opened and False if the window is closed.
  Standard_Boolean IsMapped() const override;

  //! Apply the mapping change to the window <me> and returns TRUE if the window is mapped at
  //! screen.
  Standard_Boolean DoMapping() const override { return Standard_True; }

  //! Opens the window <me>.
  void Map() const override;

  //! Closes the window <me>.
  void Unmap() const override;

  void Position(Standard_Integer& theX1,
                Standard_Integer& theY1,
                Standard_Integer& theX2,
                Standard_Integer& theY2) const override
  {
    theX1 = myXLeft;
    theX2 = myXRight;
    theY1 = myYTop;
    theY2 = myYBottom;
  }

  //! Returns The Window RATIO equal to the physical WIDTH/HEIGHT dimensions.
  Standard_Real Ratio() const override
  {
    return static_cast<double>(myXRight - myXLeft) / static_cast<double>(myYBottom - myYTop);
  }

  //! Return window size.
  void Size(Standard_Integer& theWidth, Standard_Integer& theHeight) const override
  {
    theWidth  = myXRight - myXLeft;
    theHeight = myYBottom - myYTop;
  }

  Aspect_FBConfig NativeFBConfig() const override { return nullptr; }

protected:
  Handle(Aspect_DisplayConnection) myDisplay;
  GLFWwindow*                      myGlfwWindow;
  Standard_Integer                 myXLeft;
  Standard_Integer                 myYTop;
  Standard_Integer                 myXRight;
  Standard_Integer                 myYBottom;
};
