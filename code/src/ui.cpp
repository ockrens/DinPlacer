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

// De constructor is nu weer clean en start de robot intern op
UIHandler::UIHandler(int windowWidth, int windowHeight, const char* title) {
    // 1. Window Setup
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(windowWidth, windowHeight, title);
    SetTargetFPS(60);

    // 2. Status initialisatie
    isRobotConnected = false; // <--- Nu correct gedefinieerd via de header

    maxGrens = 30.0;
    maxGrensEditMode = false;

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
    if (!fb.result.empty()) {
        LogInfo("Bestand laden: " + fb.result);
        
        // Voer de DXF-inleeslogica dynamisch uit
        if (dxfManager.loadFile(fb.result.c_str())) {
            dxfManager.processBlocks(maxGrens);
            dxfManager.groupAndSortByRails(2);
            dxfManager.printFinalResults();
            
            LogInfo("[OK] DXF succesvol ingelezen en gesorteerd.");
        } else {
            LogError("Kon het DXF-bestand niet openen of verwerken!");
        }

        fb.result = ""; // Maak het resultaat leeg voor een volgende selectie
        terminalScroll.y = -((float)logs.size() * 20.0f); 
    }
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

    // --- SIDEBAR ---
    GuiPanel({ 0, 0, sidebarW, sh }, "CONTROLS");
    float btnW = sidebarW - 40.0f;
    float btnH = dynamicFontSize * 2.5f; 
    
    if (GuiButton({ 20, 50, btnW, btnH }, "Bestand Openen")) fb.show = true;

    // --- VERBINDINGSKNOP ---
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
        GuiDisable(); 
        GuiButton({ 20, connectBtnY, btnW, btnH }, "[ VERBONDEN ]");
        GuiEnable();  
    }
        // In ui.cpp binnen UIHandler::Render():
    // Bereken de Y-positie onder de verbindingsknop
    float labelGrensY = connectBtnY + btnH + 20.0f;
    float textBoxY = labelGrensY + dynamicFontSize + 5.0f;

    // 1. Teken een label boven de textbox zodat de operator weet wat het is
    DrawText("Maximale Grens:", 20, (int)labelGrensY, dynamicFontSize, DARKGRAY);

    // 2. Teken de Raygui TextBox
    // Als je op de textbox klikt, springt 'maxGrensEditMode' naar true en kun je typen.
    // Als je er buiten klikt of op Enter drukt, springt hij terug naar false.
    if (GuiTextBox({ 20, textBoxY, btnW, btnH }, maxGrensBuffer, sizeof(maxGrensBuffer), maxGrensEditMode)) {
        maxGrensEditMode = !maxGrensEditMode; // Schakel edit-mode om bij klik
    }

    // 3. Verwerk de invoer: Als de gebruiker klaar is met typen, zetten we de tekst om naar de double
    if (!maxGrensEditMode) {
        try {
            // Converteer de text in de buffer veilig terug naar een double
            double ingevoerdeWaarde = std::stod(maxGrensBuffer);
            
            // Controleer of de waarde echt veranderd is om onnodige logs te voorkomen
            if (ingevoerdeWaarde != maxGrens) {
                maxGrens = ingevoerdeWaarde;
                LogInfo("Maximale grens aangepast naar: " + std::to_string(maxGrens));
            }
        }
        catch (const std::exception& e) {
            // Als de gebruiker letters typt in plaats van cijfers, herstellen we de oude waarde
            std::snprintf(maxGrensBuffer, sizeof(maxGrensBuffer), "%.1f", maxGrens);
        }
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

    // --- FILE BROWSER WINDOW (HERSTELD & COMPLEET) ---
    if (fb.show) {
        DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, 0.3f));
        
        float fbW = sw * 0.6f; if (fbW < 520.0f) fbW = 520.0f;
        float fbH = sh * 0.7f; if (fbH < 400.0f) fbH = 400.0f;
        Rectangle winRect = { sw/2 - fbW/2, sh/2 - fbH/2, fbW, fbH };
        
        if (GuiWindowBox(winRect, "BESTANDSBEHEER")) fb.show = false;

        GuiLabel({ winRect.x + 10, winRect.y + 40, fbW - 20, (float)dynamicFontSize + 10 }, fb.currentPath.c_str());
        
        // De lijst krijgt dynamisch de ruimte boven de knoppenrij
        Rectangle listRect = { winRect.x + 10, winRect.y + 75, fbW - 20, fbH - 145 };
        GuiListView(listRect, fb.listText.c_str(), &fb.scrollIndex, &fb.activeIndex);

        // Dynamische Y-positie voor de knoppenrij onderaan het popup-venster
        float btnY = winRect.y + fbH - btnH - 15.0f;
        float browserBtnW = 130.0f;

        // Knop 1: Map Omhoog (Links uitgelijnd)
        if (GuiButton({ winRect.x + 10, btnY, browserBtnW, btnH }, "#114# Omhoog")) {
            fb.currentPath = GetPrevDirectoryPath(fb.currentPath.c_str());
            UpdateBrowserFiles();
        }

        // Knop 2: Contextknop voor Openen/Kiezen (Rechts uitgelijnd)
        if (fb.activeIndex >= 0 && fb.activeIndex < (int)fb.files.count) {
            const char* selectedPath = fb.files.paths[fb.activeIndex];
            bool isDir = DirectoryExists(selectedPath);

            // Verander tekst dynamisch op basis van selectie: Map of Bestand
            if (GuiButton({ winRect.x + fbW - browserBtnW - 10, btnY, browserBtnW, btnH }, isDir ? "Map Openen" : "Bestand Kiezen")) {
                if (isDir) {
                    fb.currentPath = selectedPath;
                    UpdateBrowserFiles();
                } else {
                    fb.result = selectedPath; // Dit triggert jouw DXFManager in Update()
                    fb.show = false;
                }
            }
        } else {
            // Toon een inactieve knop als er niks geselecteerd is
            GuiDisable();
            GuiButton({ winRect.x + fbW - browserBtnW - 10, btnY, browserBtnW, btnH }, "Kies Item");
            GuiEnable();
        }

        // Knop 3: Sluiten (Naast de Openen/Kiezen knop)
        if (GuiButton({ winRect.x + fbW - (browserBtnW * 2) - 20, btnY, browserBtnW, btnH }, "Sluiten")) {
            fb.show = false;
        }
    }
}


void UIHandler::Cleanup() {
    UnloadDirectoryFiles(fb.files);
}
