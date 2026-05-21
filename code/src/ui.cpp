// 1. Schakel Windows GDI en USER functies uit om botsingen te minimaliseren
#ifndef NOGDI
#define NOGDI
#endif
#ifndef NOUSER
#define NOUSER
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// 2. Laad Raylib ALTIJD als allereerste in deze file!
#include "raylib.h"

// 3. Vernietig de Windows-macro's DIRECT voordat ze Raylib kapotmaken
#ifdef DrawText
#undef DrawText
#endif
#ifdef CloseWindow
#undef CloseWindow
#endif
#ifdef ShowCursor
#undef ShowCursor
#endif
#ifdef Rectangle
#undef Rectangle
#endif

// 4. Laad nu pas de UI handler header in (die de UR-bibliotheken bevat)
#include "ui.hpp"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// De constructor start de robot intern op en zet de basisstatussen klaar
UIHandler::UIHandler(int windowWidth, int windowHeight, const char* title) {
    // 1. Window Setup
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(windowWidth, windowHeight, title);
    SetTargetFPS(60);

    // 2. Status initialisatie
    isRobotConnected = false; 
    isServerRunning = false;
    lastMaxGrensEditMode = false;

    maxGrens = 30.0;
    maxGrensEditMode = false;
    
    // Vul de buffer met de initiale waarde om parsing-crashes te voorkomen
    std::snprintf(maxGrensBuffer, sizeof(maxGrensBuffer), "%.1f", maxGrens);

    fb = { 0 };
    terminalScroll = { 0, 0 };
    camera = { 0 };

    // 3. FileBrowser Setup
    fb.show = false;
    fb.currentPath = GetWorkingDirectory();
    fb.scrollIndex = 0;
    fb.activeIndex = -1;
    fb.result = "";
    fb.files = { 0 };
    UpdateBrowserFiles();

    // 4. 3D World Camera Setup
    camera.position = (Vector3){ 0.0f, 50.0f, 0.0f }; 
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };   
    camera.up = (Vector3){ 0.0f, 0.0f, -1.0f };       
    camera.fovy = 45.0f;                             
    camera.projection = CAMERA_PERSPECTIVE;          

    LogInfo("Systeem, Camera en RobotManager succesvol geïnitialiseerd.");
}

// DESTRUCTOR: Regelt de automatische en veilige resource cleanup (RAII)
UIHandler::~UIHandler() {
    // 1. Stop de server (zodat netwerk-sockets netjes sluiten)
    if (robotServer.has_value()) {
        robotServer->stop();
    }

    // 2. Wacht netjes tot de achtergronddraad klaar is met afsluiten
    if (serverThread && serverThread->joinable()) {
        serverThread->join();
    }

    // 3. Ruim Raylib-specifieke directory buffers en het window op
    UnloadDirectoryFiles(fb.files);
    CloseWindow();
}

void UIHandler::LogInfo(std::string msg) { logs.push_back({msg, DARKGRAY}); }
void UIHandler::LogWarn(std::string msg) { logs.push_back({"[!] " + msg, ORANGE}); }
void UIHandler::LogError(std::string msg) { logs.push_back({"[X] " + msg, RED}); }

void UIHandler::UpdateBrowserFiles() {
    UnloadDirectoryFiles(fb.files);
    fb.files = LoadDirectoryFiles(fb.currentPath.c_str());
    
    fb.listText = "";
    for (unsigned int i = 0; i < fb.files.count; i++) {
        bool isDir = DirectoryExists(fb.files.paths[i]);
        fb.listText += (isDir ? "#01# " : "#218# ");
        fb.listText += GetFileName(fb.files.paths[i]);
        if (i < (int)fb.files.count - 1) fb.listText += ";";
    }
    fb.activeIndex = -1;
}

Camera3D UIHandler::GetCamera() { return camera; }
Camera3D* UIHandler::GetCameraPtr() { return &camera; }

void UIHandler::Update() {
    // --- BESTANDSBEHEER LOGICA ---
    if (!fb.result.empty()) {
        LogInfo("Bestand laden: " + fb.result);
        if (dxfManager.loadFile(fb.result.c_str())) {
            dxfManager.processBlocks(maxGrens);
            dxfManager.groupAndSortByRails(2);
            dxfManager.printFinalResults();
            LogInfo("[OK] DXF succesvol ingelezen en gesorteerd.");
        } else {
            LogError("Kon het DXF-bestand niet openen of verwerken!");
        }
        fb.result = ""; 
        terminalScroll.y = -((float)logs.size() * 20.0f); 
    }

    // --- LIVE ROBOT STATUS CHECK ---
    if (isRobotConnected && robot.has_value()) {
        if (!robot->IsConnected()) {
            isRobotConnected = false;
            robot.reset(); 
            LogError("Robot verbinding onverwacht verbroken!");
        }
    }

    // --- LIVE ROBOT SERVER COMMUNICATIE ---
    if (isServerRunning && robotServer.has_value()) {
        if (isRobotConnected && robot.has_value()) {
            try {
                std::vector<double> iets = robot->GetRobotPose();
                Pose current_pose = VectorToPose(iets);
                robotServer->set_current_pose(current_pose);

                if (robotServer->has_new_target_pose()) {
                    Pose target_pose = robotServer->get_target_pose();
                    std::vector<double> vec = PoseToVector(target_pose);
                    robot->MoveToPose(vec); 
                }
            }
            catch (const std::exception& e) {
                LogError("Fout tijdens data-uitwisseling met server.");
            }
        }
    }

    // --- TEXTBOX INPUT VERWERKING ---
    if (!maxGrensEditMode && lastMaxGrensEditMode) {
        try {
            double ingevoerdeWaarde = std::stod(maxGrensBuffer);
            if (ingevoerdeWaarde != maxGrens) {
                maxGrens = ingevoerdeWaarde;
                LogInfo("Maximale grens aangepast naar: " + std::to_string(maxGrens));
            }
        }
        catch (const std::exception& e) {
            LogError("Ongeldige invoer! Grens hersteld.");
            std::snprintf(maxGrensBuffer, sizeof(maxGrensBuffer), "%.1f", maxGrens);
        }
    }
    lastMaxGrensEditMode = maxGrensEditMode;
}

void UIHandler::DrawDXF3D() {
    for (const auto& b : dxfManager.finalBlocks) {
        Vector3 center = { (float)b.centerX * DXF_SCALE, 0.0f, (float)b.centerY * DXF_SCALE };
        
        float width = (float)(b.maxX - b.minX) * DXF_SCALE;
        float length = (float)(b.maxY - b.minY) * DXF_SCALE;
        float height = 0.1f; 

        DrawCubeWires(center, width, height, length, LIME);
        DrawLine3D(center, {center.x, center.y + 0.5f, center.z}, RED);

        Vector2 screenPos = GetWorldToScreen(center, camera);
        if (screenPos.x > 0 && screenPos.y > 0) {
            DrawText(b.label.c_str(), (int)screenPos.x, (int)screenPos.y, 10, DARKGRAY);
        }
    }
}

void UIHandler::Render() {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    int dynamicFontSize = (int)(sh * (12.0f / 600.0f));
    if (dynamicFontSize < 11) dynamicFontSize = 11;
    if (dynamicFontSize > 24) dynamicFontSize = 24;

    GuiSetStyle(DEFAULT, TEXT_SIZE, dynamicFontSize);

    float sidebarW = sw * 0.20f;
    if (sidebarW < 180.0f) sidebarW = 180.0f;
    
    float termH = sh * 0.25f;
    if (termH < 100.0f) termH = 100.0f;

    if (fb.show) GuiLock();

    // --- SIDEBAR PANEL ---
    GuiPanel({ 0, 0, sidebarW, sh }, "CONTROLS");
    float btnW = sidebarW - 40.0f;
    float btnH = dynamicFontSize * 2.5f; 
    
    if (GuiButton({ 20, 50, btnW, btnH }, "Bestand Openen")) fb.show = true;

    // --- INTERACTIEVE VERBINDINGSKNOP (ROBOT) ---
    float connectBtnY = 50.0f + btnH + 15.0f; 

    if (!isRobotConnected) {
        if (GuiButton({ 20, connectBtnY, btnW, btnH }, "Verbind met Robot")) {
            LogInfo("Verbindingspoging gestart met 192.168.1.100...");
            try {
                robot.emplace("192.168.1.100"); 
                isRobotConnected = true; 
                LogInfo("[OK] Robot succesvol verbonden via RTDE.");
            } 
            catch (const std::exception& e) {
                LogError("Verbinding mislukt! Controleer het IP-adres.");
                robot.reset(); 
            }
        }
    } else {
        if (GuiButton({ 20, connectBtnY, btnW, btnH }, "[ DISCONNECT ROBOT ]")) {
            LogWarn("Verbinding met robot handmatig verbroken.");
            isRobotConnected = false;
            robot.reset(); 
        }
    }

    // --- INTERACTIEVE SERVER KNOP ---
    float serverBtnY = connectBtnY + btnH + 10.0f; 
    
    if (!isServerRunning) {
        if (GuiButton({ 20, serverBtnY, btnW, btnH }, "Start Server (Port 5000)")) {
            LogInfo("RobotServer opstarten op poort 5000...");
            try {
                robotServer.emplace(5000);
                if (robotServer->start()) {
                    serverThread = std::make_unique<std::thread>(&RobotServer::run, &(*robotServer));
                    isServerRunning = true;
                    LogInfo("[OK] Server draait succesvol op de achtergrond.");
                } else {
                    LogError("Server start mislukt! Is poort 5000 al bezet?");
                    robotServer.reset();
                }
            } 
            catch (const std::exception& e) {
                LogError("Fout bij starten server: " + std::string(e.what()));
                robotServer.reset();
            }
        }
    } else {
        if (GuiButton({ 20, serverBtnY, btnW, btnH }, "[ STOP SERVER ]")) {
            LogWarn("RobotServer handmatig stopgezet.");
            isServerRunning = false;
            
            // 1. Schiet eerst de sockets lek, zodat de recv/accept deblokkeert!
            if (robotServer.has_value()) {
                robotServer->stop();
            }

            // 2. Wacht nu pas tot de achtergrondthread daadwerkelijk stopt
            if (serverThread && serverThread->joinable()) {
                serverThread->join();
            }
            
            serverThread.reset();
            robotServer.reset(); 
        }
    }   

    // --- MAXIMALE GRENS INPUT ---
    float labelGrensY = serverBtnY + btnH + 20.0f;
    float textBoxY = labelGrensY + dynamicFontSize + 5.0f;

    DrawText("Maximale Grens:", 20, (int)labelGrensY, dynamicFontSize, DARKGRAY);

    if (GuiTextBox({ 20, textBoxY, btnW, btnH }, maxGrensBuffer, sizeof(maxGrensBuffer), maxGrensEditMode)) {
        maxGrensEditMode = !maxGrensEditMode; 
    }

    // --- TERMINAL OUTPUT ---
    Rectangle termBounds = { sidebarW, sh - termH, sw - sidebarW, termH };
    float lineSpacing = dynamicFontSize + 6.0f; 
    Rectangle content = { sidebarW, sh - termH, sw - sidebarW - 15, (float)logs.size() * lineSpacing + 20.0f };
    Rectangle view = { 0 };

    GuiPanel(termBounds, "TERMINAL OUTPUT");
    GuiScrollPanel(termBounds, NULL, content, &terminalScroll, &view);

    BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);
        for (size_t i = 0; i < logs.size(); i++) {
            float posY = view.y + terminalScroll.y + 10 + (i * lineSpacing);
            if (posY > view.y - lineSpacing && posY < view.y + view.height) {
                DrawText(logs[i].text.c_str(), (int)view.x + 10, (int)posY, dynamicFontSize, logs[i].color);
            }
        }
    EndScissorMode();
    
    if (fb.show) GuiUnlock();

    // --- FILE BROWSER WINDOW ---
    if (fb.show) {
        DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, 0.3f));
        
        float fbW = sw * 0.6f; if (fbW < 520.0f) fbW = 520.0f;
        float fbH = sh * 0.7f; if (fbH < 400.0f) fbH = 400.0f;
        Rectangle winRect = { sw/2 - fbW/2, sh/2 - fbH/2, fbW, fbH };
        
        if (GuiWindowBox(winRect, "BESTANDSBEHEER")) fb.show = false;

        GuiLabel({ winRect.x + 10, winRect.y + 40, fbW - 20, (float)dynamicFontSize + 10 }, fb.currentPath.c_str());
        
        Rectangle listRect = { winRect.x + 10, winRect.y + 75, fbW - 20, fbH - 145 };
        
        int oldActive = fb.activeIndex;
        GuiListView(listRect, fb.listText.c_str(), &fb.scrollIndex, &fb.activeIndex);

        if (fb.activeIndex != oldActive && fb.activeIndex >= 0 && GetGestureDetected() == GESTURE_DOUBLETAP) {
            const char* selectedPath = fb.files.paths[fb.activeIndex];
            if (DirectoryExists(selectedPath)) {
                fb.currentPath = selectedPath;
                UpdateBrowserFiles();
            }
        }

        float btnY = winRect.y + fbH - btnH - 15.0f;
        float browserBtnW = 130.0f;

        if (GuiButton({ winRect.x + 10, btnY, browserBtnW, btnH }, "#114# Omhoog")) {
            fb.currentPath = GetPrevDirectoryPath(fb.currentPath.c_str());
            UpdateBrowserFiles();
        }

        if (fb.activeIndex >= 0 && fb.activeIndex < (int)fb.files.count) {
            const char* selectedPath = fb.files.paths[fb.activeIndex];
            bool isDir = DirectoryExists(selectedPath);

            if (GuiButton({ winRect.x + fbW - browserBtnW - 10, btnY, browserBtnW, btnH }, isDir ? "Map Openen" : "Bestand Kiezen")) {
                if (isDir) {
                    fb.currentPath = selectedPath;
                    UpdateBrowserFiles();
                } else {
                    fb.result = selectedPath; 
                    fb.show = false;
                }
            }
        } else {
            GuiDisable();
            GuiButton({ winRect.x + fbW - browserBtnW - 10, btnY, browserBtnW, btnH }, "Kies Item");
            GuiEnable();
        }

        if (GuiButton({ winRect.x + fbW - (browserBtnW * 2) - 20, btnY, browserBtnW, btnH }, "Sluiten")) {
            fb.show = false;
        }
    }
}
