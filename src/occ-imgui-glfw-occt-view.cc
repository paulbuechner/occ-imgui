#include "occ_imgui/occ-imgui-glfw-occt-view.h"

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <opencascade/AIS_Shape.hxx>
#include <opencascade/AIS_ViewCube.hxx>
#include <opencascade/BRepPrimAPI_MakeBox.hxx>
#include <opencascade/BRepPrimAPI_MakeCone.hxx>
#include <opencascade/Message.hxx>
#include <opencascade/Message_Messenger.hxx>
#include <opencascade/OpenGl_GraphicDriver.hxx>
#include <opencascade/Graphic3d_GraphicDriver.hxx>
#include <opencascade/V3d_Viewer.hxx>

namespace
{
    //! Convert GLFW mouse button into Aspect_VKeyMouse.
    Aspect_VKeyMouse mouseButtonFromGlfw(const int theButton)
    {
        switch (theButton)
        {
        case GLFW_MOUSE_BUTTON_LEFT: return Aspect_VKeyMouse_LeftButton;
        case GLFW_MOUSE_BUTTON_RIGHT: return Aspect_VKeyMouse_RightButton;
        case GLFW_MOUSE_BUTTON_MIDDLE: return Aspect_VKeyMouse_MiddleButton;
        }
        return Aspect_VKeyMouse_NONE;
    }

    //! Convert GLFW key modifiers into Aspect_VKeyFlags.
    Aspect_VKeyFlags keyFlagsFromGlfw(const int theFlags)
    {
        Aspect_VKeyFlags aFlags = Aspect_VKeyFlags_NONE;
        if ((theFlags & GLFW_MOD_SHIFT) != 0)
        {
            aFlags |= Aspect_VKeyFlags_SHIFT;
        }
        if ((theFlags & GLFW_MOD_CONTROL) != 0)
        {
            aFlags |= Aspect_VKeyFlags_CTRL;
        }
        if ((theFlags & GLFW_MOD_ALT) != 0)
        {
            aFlags |= Aspect_VKeyFlags_ALT;
        }
        if ((theFlags & GLFW_MOD_SUPER) != 0)
        {
            aFlags |= Aspect_VKeyFlags_META;
        }
        return aFlags;
    }
}

// ================================================================
// Function : GlfwOcctView
// Purpose  :
// ================================================================
GlfwOcctView::GlfwOcctView() = default;

// ================================================================
// Function : ~GlfwOcctView
// Purpose  :
// ================================================================
GlfwOcctView::~GlfwOcctView() = default;

// ================================================================
// Function : toView
// Purpose  :
// ================================================================
GlfwOcctView* GlfwOcctView::toView(GLFWwindow* theWin)
{
    return static_cast<GlfwOcctView*>(glfwGetWindowUserPointer(theWin));
}

// ================================================================
// Function : errorCallback
// Purpose  :
// ================================================================
void GlfwOcctView::errorCallback(const int theError, const char* theDescription)
{
    Message::DefaultMessenger()->Send(TCollection_AsciiString("Error") + theError + ": " + theDescription,
                                      Message_Fail);
}

// ================================================================
// Function : run
// Purpose  :
// ================================================================
void GlfwOcctView::run()
{
    initWindow(1300, 900, "OCCT IMGUI");
    initViewer();
    initDemoScene();
    if (myView.IsNull())
    {
        return;
    }

    myView->MustBeResized();
    myOcctWindow->Map();
    initGui();
    mainloop();
    cleanup();
}

// ================================================================
// Function : initWindow
// Purpose  :
// ================================================================
void GlfwOcctView::initWindow(const int theWidth, const int theHeight, const char* theTitle)
{
    glfwSetErrorCallback(GlfwOcctView::errorCallback);
    glfwInit();

    const bool toAskCoreProfile = true;
    if (toAskCoreProfile)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#if defined (__APPLE__)
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        //glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, true);
        //glfwWindowHint(GLFW_DECORATED, GL_FALSE);
    }

    myOcctWindow = new GlfwOcctWindow(theWidth, theHeight, theTitle);
    glfwSetWindowUserPointer(myOcctWindow->getGlfwWindow(), this);

    // window callback
    glfwSetWindowSizeCallback(myOcctWindow->getGlfwWindow(), onResizeCallback);
    glfwSetFramebufferSizeCallback(myOcctWindow->getGlfwWindow(), onFBResizeCallback);

    // mouse callback
    glfwSetScrollCallback(myOcctWindow->getGlfwWindow(), onMouseScrollCallback);
    glfwSetMouseButtonCallback(myOcctWindow->getGlfwWindow(), onMouseButtonCallback);
    glfwSetCursorPosCallback(myOcctWindow->getGlfwWindow(), onMouseMoveCallback);
}

// ================================================================
// Function : initViewer
// Purpose  :
// ================================================================
void GlfwOcctView::initViewer()
{
    if (myOcctWindow.IsNull()
        || myOcctWindow->getGlfwWindow() == nullptr)
    {
        return;
    }

    Handle(OpenGl_GraphicDriver) aGraphicDriver = new OpenGl_GraphicDriver(myOcctWindow->GetDisplay(), false);
    aGraphicDriver->SetBuffersNoSwap(true);

    // Cast to base driver type expected by V3d_Viewer constructor
    const Handle(Graphic3d_GraphicDriver) aBaseDriver = aGraphicDriver;
    const Handle(V3d_Viewer) aViewer = new V3d_Viewer(aBaseDriver);
    aViewer->SetDefaultLights();
    aViewer->SetLightOn();
    aViewer->SetDefaultTypeOfView(V3d_PERSPECTIVE);
    aViewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);

    myView = aViewer->CreateView();
    //myView->SetImmediateUpdate(false);
    myView->SetWindow(myOcctWindow, myOcctWindow->NativeGlContext());
    myView->ChangeRenderingParams().ToShowStats = true;

    myContext = new AIS_InteractiveContext(aViewer);

    Handle(AIS_ViewCube) aCube = new AIS_ViewCube();
    aCube->SetSize(55);
    aCube->SetFontHeight(12);
    aCube->SetAxesLabels("", "", "");
    aCube->SetTransformPersistence(
        new Graphic3d_TransformPers(Graphic3d_TMF_TriedronPers, Aspect_TOTP_LEFT_LOWER, Graphic3d_Vec2i(100, 100)));
    aCube->SetViewAnimation(this->ViewAnimation());
    aCube->SetFixedAnimationLoop(false);
    myContext->Display(aCube, false);
}

void GlfwOcctView::initGui() const
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& aIO = ImGui::GetIO();
    aIO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    aIO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui_ImplGlfw_InitForOpenGL(myOcctWindow->getGlfwWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Setup Dear ImGui style.
    ImGui::StyleColorsClassic();
}

void GlfwOcctView::renderGui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Create main dockspace
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    // 3D View Window - set to dock into the main area by default
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("3D View", nullptr, ImGuiWindowFlags_NoScrollbar))
    {
        // Get the available content region
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        // Store viewport dimensions for resize tracking
        bool viewportResized = (myViewportWidth != (int)viewportSize.x || myViewportHeight != (int)viewportSize.y);
        if (viewportResized && viewportSize.x > 0 && viewportSize.y > 0)
        {
            myViewportWidth = (int)viewportSize.x;
            myViewportHeight = (int)viewportSize.y;
            if (!myView.IsNull())
            {
                // Trigger viewport resize in OCCT
                myView->Window()->DoResize();
                myView->MustBeResized();
            }
        }

        // Draw the OCCT viewport as an invisible item to occupy space
        // The OCCT view renders directly to the framebuffer behind ImGui
        ImGui::Dummy(viewportSize);

        // Display debug info
        ImGui::SetCursorPosY(ImGui::GetWindowPos().y + 10);
        ImGui::SetCursorPosX(ImGui::GetWindowPos().x + 10);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "3D View: %.0f x %.0f", viewportSize.x, viewportSize.y);
    }
    ImGui::End();

    // Settings/Demo Window (dockable)
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings"))
    {
        if (ImGui::CollapsingHeader("Rendering Stats", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (!myView.IsNull())
            {
                const Graphic3d_RenderingParams& aParams = myView->ChangeRenderingParams();
                ImGui::Text("Stats Enabled: %s", aParams.ToShowStats ? "Yes" : "No");
            }
        }

        ImGui::Separator();
        if (ImGui::CollapsingHeader("ImGui Demo", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static bool show_demo = true;
            if (show_demo)
            {
                ImGui::ShowDemoWindow(&show_demo);
            }
        }
    }
    ImGui::End();

    // Control Panel (dockable)
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Controls"))
    {
        ImGui::Text("OpenCASCADE Viewer");
        ImGui::Separator();
        if (ImGui::Button("Reset View", ImVec2(-1, 0)))
        {
            if (!myView.IsNull())
            {
                myView->FitAll();
                myView->Invalidate();
            }
        }

        if (ImGui::Button("Zoom Fit", ImVec2(-1, 0)))
        {
            if (!myView.IsNull())
            {
                myView->ZFitAll();
                myView->Invalidate();
            }
        }

        ImGui::Separator();
        ImGui::TextWrapped("Use mouse to interact with the 3D view:");
        ImGui::BulletText("Left click + drag: Rotate");
        ImGui::BulletText("Right click + drag: Pan");
        ImGui::BulletText("Scroll: Zoom");
    }
    ImGui::End();

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Handle multi-viewport rendering
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(myOcctWindow->getGlfwWindow());
}

// ================================================================
// Function : initDemoScene
// Purpose  :
// ================================================================
void GlfwOcctView::initDemoScene() const
{
    if (myContext.IsNull())
    {
        return;
    }

    myView->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_GOLD, 0.08, V3d_WIREFRAME);

    gp_Ax2 anAxis;
    anAxis.SetLocation(gp_Pnt(0.0, 0.0, 0.0));
    Handle(AIS_Shape) aBox = new AIS_Shape(BRepPrimAPI_MakeBox(anAxis, 50, 50, 50).Shape());
    myContext->Display(aBox, AIS_Shaded, 0, false);
    anAxis.SetLocation(gp_Pnt(25.0, 125.0, 0.0));
    Handle(AIS_Shape) aCone = new AIS_Shape(BRepPrimAPI_MakeCone(anAxis, 25, 0, 50).Shape());
    myContext->Display(aCone, AIS_Shaded, 0, false);

    TCollection_AsciiString aGlInfo;
    {
        TColStd_IndexedDataMapOfStringString aRendInfo;
        myView->DiagnosticInformation(aRendInfo, Graphic3d_DiagnosticInfo_Basic);
        for (TColStd_IndexedDataMapOfStringString::Iterator aValueIter(aRendInfo); aValueIter.More(); aValueIter.Next())
        {
            if (!aGlInfo.IsEmpty()) { aGlInfo += "\n"; }
            aGlInfo += TCollection_AsciiString("  ") + aValueIter.Key() + ": " + aValueIter.Value();
        }
    }
    Message::DefaultMessenger()->Send(TCollection_AsciiString("OpenGL info:\n") + aGlInfo, Message_Info);
}

// ================================================================
// Function : handleViewRedraw
// Purpose  :
// ================================================================
void GlfwOcctView::handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                    const Handle(V3d_View)& theView)
{
    AIS_ViewController::handleViewRedraw(theCtx, theView);
    myToWaitEvents = !myToAskNextFrame;
}

// ================================================================
// Function : mainloop
// Purpose  :
// ================================================================
void GlfwOcctView::mainloop()
{
    while (!glfwWindowShouldClose(myOcctWindow->getGlfwWindow()))
    {
        // glfwPollEvents() for continuous rendering (immediate return if there are no new events)
        // and glfwWaitEvents() for rendering on demand (something actually happened in the viewer)
        if (myToWaitEvents)
        {
            glfwWaitEvents();
        }
        else
        {
            glfwPollEvents();
        }
        if (!myView.IsNull())
        {
            myView->InvalidateImmediate(); // redraw view even if it wasn't modified
            FlushViewEvents(myContext, myView, true);

            renderGui();
        }
    }
}

// ================================================================
// Function : cleanup
// Purpose  :
// ================================================================
void GlfwOcctView::cleanup() const
{
    // Cleanup IMGUI.
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (!myView.IsNull())
    {
        myView->Remove();
    }
    if (!myOcctWindow.IsNull())
    {
        myOcctWindow->Close();
    }

    glfwTerminate();
}

// ================================================================
// Function : onResize
// Purpose  :
// ================================================================
void GlfwOcctView::onResize(const int theWidth, const int theHeight)
{
    if (theWidth != 0
        && theHeight != 0
        && !myView.IsNull())
    {
        myView->Window()->DoResize();
        myView->MustBeResized();
        myView->Invalidate();
        FlushViewEvents(myContext, myView, true);
        renderGui();
    }
}

// ================================================================
// Function : onMouseScroll
// Purpose  :
// ================================================================
void GlfwOcctView::onMouseScroll(const double theOffsetX, const double theOffsetY)
{
    if (const ImGuiIO& aIO = ImGui::GetIO(); !myView.IsNull() && !aIO.WantCaptureMouse)
    {
        UpdateZoom(Aspect_ScrollDelta(myOcctWindow->CursorPosition(), static_cast<int>(theOffsetY * 8.0)));
    }
}

// ================================================================
// Function : onMouseButton
// Purpose  :
// ================================================================
void GlfwOcctView::onMouseButton(const int theButton, const int theAction, const int theMods)
{
    if (const ImGuiIO& aIO = ImGui::GetIO(); myView.IsNull() || aIO.WantCaptureMouse)
    {
        return;
    }

    const Graphic3d_Vec2i aPos = myOcctWindow->CursorPosition();
    if (theAction == GLFW_PRESS)
    {
        PressMouseButton(aPos, mouseButtonFromGlfw(theButton), keyFlagsFromGlfw(theMods), false);
    }
    else
    {
        ReleaseMouseButton(aPos, mouseButtonFromGlfw(theButton), keyFlagsFromGlfw(theMods), false);
    }
}

// ================================================================
// Function : onMouseMove
// Purpose  :
// ================================================================
void GlfwOcctView::onMouseMove(int thePosX, int thePosY)
{
    if (myView.IsNull())
    {
        return;
    }

    if (const ImGuiIO& aIO = ImGui::GetIO(); aIO.WantCaptureMouse)
    {
        //myView->Redraw();
    }
    else
    {
        const Graphic3d_Vec2i aNewPos(thePosX, thePosY);
        UpdateMousePosition(aNewPos, PressedMouseButtons(), LastMouseFlags(), false);
    }
}
