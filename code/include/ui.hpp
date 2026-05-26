// ==================== ui.hpp ====================
#ifndef UI_HPP
#define UI_HPP

#include "raylib.h"
#include <vector>
#include <string>
#include <optional>       // Vereist voor std::optional
#include <memory>         // Vereist voor std::unique_ptr
#include <thread>         // Vereist voor std::thread
#include "DXFHandling.hpp"
#include "URHandling.hpp" // Sluit je schone robot header hier aan
#include "robot_server.hpp"

#define DXF_SCALE 40.0f   // Schaal aangepast voor de meter-visualisatie

struct LogEntry { 
    std::string text;
    Color color; 
};

struct FileBrowser { 
    bool show; 
    std::string currentPath; 
    std::string listText; 
    int scrollIndex; 
    int activeIndex; 
    std::string result; 
    FilePathList files; 
};

class UIHandler {
private:
    // --- CAMERA STATUS EN ENUMS ---
    enum CameraMode { MODE_3D, MODE_2D_FRONT };
    CameraMode currentCameraMode;

    // --- ROBOT EN COMMUNICATIE STATUSSEN ---
    std::optional<RobotManager> robot; 
    bool isRobotConnected;
    
    bool isServerRunning;
    bool lastMaxGrensEditMode;
    
    std::optional<RobotServer> robotServer;
    std::unique_ptr<std::thread> serverThread;

    // --- INTERNE DESIGN EN INTERFACE VARIABELEN ---
    DXFManager dxfManager;
    double maxGrens;
    char maxGrensBuffer[16]; // Gecorrigeerd naar stabiele char array buffer
    bool maxGrensEditMode; 
    Font uiFont;
    
    FileBrowser fb;
    std::vector<LogEntry> logs;
    Vector2 terminalScroll;
    Camera3D camera;

    void UpdateBrowserFiles();
    
public:
    UIHandler(int windowWidth = 1024, int windowHeight = 600, const char* title = "Robot Viz - Integrated UI");
    ~UIHandler(); // Destructor vervangt de oude handmatige Cleanup()

    void Update(); 
    void DrawDXF3D();
    void Render(); 

    void LogInfo(std::string msg); 
    void LogWarn(std::string msg); 
    void LogError(std::string msg);
    
    Camera3D GetCamera(); 
    Camera3D* GetCameraPtr();

    // --- GEOMETRIE UTILITY VOOR SCISSOR MODE IN MAIN.CPP ---
    Rectangle GetViewportRect() const {
        float sw = (float)GetScreenWidth();
        float sh = (float)GetScreenHeight();
        
        float sidebarW = sw * 0.20f;
        if (sidebarW < 180.0f) sidebarW = 180.0f;
        
        float termH = sh * 0.25f;
        if (termH < 100.0f) termH = 100.0f;

        // Geeft exact de rechthoek van het vrije grid-scherm (boven terminal, rechts van menu)
        return Rectangle{ sidebarW, 0, sw - sidebarW, sh - termH };
    }
};

#endif
