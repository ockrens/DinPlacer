// ==================== ui.hpp ====================
#ifndef UI_HPP
#define UI_HPP

#include "raylib.h"
#include <vector>
#include <string>
#include <optional>       // Vereist voor de connect-allocatie
#include "DXFHandling.hpp"
#include "URHandling.hpp" // Sluit je schone robot header hier aan

#define DXF_SCALE 0.1f

struct LogEntry { 
    std::string text;
    Color color; 
};
struct FileBrowser { bool show; std::string currentPath; std::string listText; int scrollIndex; int activeIndex; std::string result; FilePathList files; };

class UIHandler {
private:
    // De UI beheert de robot optioneel. Start als 'leeg' (niet verbonden)
    std::optional<RobotManager> robot; 
    bool isRobotConnected;
    DXFManager dxfManager;
    double maxGrens;
    char maxGrensBuffer[16]; // Buffer voor de tekst in het invoerveld
    bool maxGrensEditMode; 
    Font uiFont;
    
    FileBrowser fb;
    std::vector<LogEntry> logs;
    Vector2 terminalScroll;
    Camera3D camera;

    void UpdateBrowserFiles();
    
public:
    UIHandler(int windowWidth = 1024, int windowHeight = 600, const char* title = "Robot Viz - Integrated UI");
    void Update(); 
    void DrawDXF3D();
    void Render(); 
    void Cleanup(); 

    void LogInfo(std::string msg); void LogWarn(std::string msg); void LogError(std::string msg);
    Camera3D GetCamera(); Camera3D* GetCameraPtr();
};

#endif
