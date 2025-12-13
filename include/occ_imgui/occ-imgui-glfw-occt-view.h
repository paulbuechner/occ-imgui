#pragma once

#include "occ-imgui-glfw-occt-window.h"

#include <opencascade/AIS_InteractiveContext.hxx>
#include <opencascade/AIS_ViewController.hxx>
#include <opencascade/V3d_View.hxx>

//! Sample class creating 3D Viewer within GLFW window.
class GlfwOcctView : protected AIS_ViewController
{
public:
  //! Default constructor.
  GlfwOcctView();

  //! Destructor.
  ~GlfwOcctView() override;

  //! Main application entry point.
  void run();

private:
  //! Create GLFW window.
  void initWindow(int theWidth, int theHeight, const char* theTitle);

  //! Create 3D Viewer.
  void initViewer();

  //! Init ImGui.
  void initGui() const;

  //! Render ImGUI.
  void renderGui();

  //! Fill 3D Viewer with a DEMO items.
  void initDemoScene() const;

  //! Application event loop.
  void mainloop();

  //! Clean up before .
  void cleanup() const;

  //! Handle view redraw.
  void handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                        const Handle(V3d_View)&               theView) override;

  //! @name GLWF callbacks
private:
  //! Window resize event.
  void onResize(int theWidth, int theHeight);

  //! Mouse scroll event.
  void onMouseScroll(double theOffsetX, double theOffsetY);

  //! Mouse click event.
  void onMouseButton(int theButton, int theAction, int theMods);

  //! Mouse move event.
  void onMouseMove(int thePosX, int thePosY);

  //! @name GLWF callbacks (static functions)
private:
  //! GLFW callback redirecting messages into Message::DefaultMessenger().
  static void errorCallback(int theError, const char* theDescription);

  //! Wrapper for glfwGetWindowUserPointer() returning this class instance.
  static GlfwOcctView* toView(GLFWwindow* theWin);

  //! Window resize callback.
  static void onResizeCallback(GLFWwindow* theWin, const int theWidth, const int theHeight)
  {
    toView(theWin)->onResize(theWidth, theHeight);
  }

  //! Frame-buffer resize callback.
  static void onFBResizeCallback(GLFWwindow* theWin, const int theWidth, const int theHeight)
  {
    toView(theWin)->onResize(theWidth, theHeight);
  }

  //! Mouse scroll callback.
  static void onMouseScrollCallback(GLFWwindow*  theWin,
                                    const double theOffsetX,
                                    const double theOffsetY)
  {
    toView(theWin)->onMouseScroll(theOffsetX, theOffsetY);
  }

  //! Mouse click callback.
  static void onMouseButtonCallback(GLFWwindow* theWin,
                                    const int   theButton,
                                    const int   theAction,
                                    const int   theMods)
  {
    toView(theWin)->onMouseButton(theButton, theAction, theMods);
  }

  //! Mouse move callback.
  static void onMouseMoveCallback(GLFWwindow* theWin, const double thePosX, const double thePosY)
  {
    toView(theWin)->onMouseMove(static_cast<int>(thePosX), static_cast<int>(thePosY));
  }

private:
  Handle(GlfwOcctWindow)         myOcctWindow;
  Handle(V3d_View)               myView;
  Handle(AIS_InteractiveContext) myContext;
  bool                           myToWaitEvents = true;

  // ImGui viewport dimensions
  int myViewportWidth  = 0;
  int myViewportHeight = 0;
};
