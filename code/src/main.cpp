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

// 2. Laad Raylib ALTIJD als allereerste in!
#include "raylib.h"

// 3. Vernietig de Windows-macro's die Raylib-functies/structs kapotmaken
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

// 4. Laad nu pas de rest van je project-headers veilig in
#include "ui.hpp"
#include "main.hpp"
#include "Workplane.hpp"
#include "URHandling.hpp"
#include "DXFHandling.hpp"
#include "robot_server.hpp"

#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

#include <ur_rtde/rtde_receive_interface.h>
#include <ur_rtde/rtde_control_interface.h>

// ... rest van je main() functie blijft exact hetzelfde


// class MyRobotArm
// {
// public:
//     void connect()
//     {
//         std::cout << "Robot connected\n";
//     }

//     void move_linear(const Pose& pose)
//     {
//         std::cout << "Moving robot to: "
//                   << pose.x << ", "
//                   << pose.y << ", "
//                   << pose.z << ", "
//                   << pose.rx << ", "
//                   << pose.ry << ", "
//                   << pose.rz << std::endl;

//         // REAL ROBOT SDK CALL GOES HERE:
//         //
//         // robot.moveLinear(
//         //     pose.x,
//         //     pose.y,
//         //     pose.z,
//         //     pose.rx,
//         //     pose.ry,
//         //     pose.rz
//         // );
//     }

//     Pose get_current_pose()
//     {
//         // REAL ROBOT SDK CALL GOES HERE:

//         //
//         // auto p = robot.getCurrentPose();
//         //
//         // return {
//         //     p.x,
//         //     p.y,
//         //     p.z,
//         //     p.rx,
//         //     p.ry,
//         //     p.rz
//         // };

//         return {100.0, 200.0, 300.0, 180.0, 0.0, 90.0};
//     }
// };

std::vector<double> test_pose = {0.1730, -0.8010, 0.4645, 1.307, -1.330, 1.134};

int main() {
    // 1. DXF Inlezen
    // DXFManager manager;
    // std::cout << "DXF test gestart..." << std::endl;

    // if (manager.loadFile("../werkelijk.dxf")) {
    //     manager.processBlocks();
    //     manager.groupAndSortByRails(2);
    //     #ifdef DXFDebug    
    //         manager.printFinalResults(); 
    //     #endif
    // } 

    // else {
    //     std::cerr << "Kon het bestand niet openen." << std::endl;
    //     return 1;
    // }

    // 2. UIHandler initialisatie
    UIHandler UIHAND;
    // UIHAND.GetCameraPtr()->projection = CAMERA_PERSPECTIVE;
    // 3. 3D Objects Setup
    Vector3 cubePosition = { 0.0f, 0.0f, 0.0f };
    Mesh planeMesh = GenMeshCube(10.0f, 0.01f, 10.0f);
    Mesh cubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    Mesh sphereMesh = GenMeshSphere(0.5f, 16, 16);
    
    Material material = LoadMaterialDefault();
    
    BaseObject plane({ 0, 0, 0 }, { 0, 0, 1.5 }, GRAY);
    BaseObject arm({ 2, 0, 0 }, { 0, 0, 0 }, RED);   
    BaseObject hand({ 1.5, 0, 0 }, { 0, 0, 0 }, BLUE); 

    arm.parent = &plane;
    hand.parent = &arm;

    // Gebruik de First Person of Free mode om te kunnen vliegen/wandelen
    // We zetten de camera in de UIHandler op een goede startpositie
    // RobotManager robot("192.168.1.100");

    
    #ifdef UIEnable
    while (!WindowShouldClose()) {
        // --- 1. Update ---
        UIHAND.Update();

        // Camera besturing: Alleen als rechtermuisknop is ingedrukt
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            float moveSpeed = 0.5f; // Snelheid van het schuiven

            // Bewegen over de X-as (Links/Rechts)
            if (IsKeyDown(KEY_D)) { 
                UIHAND.GetCameraPtr()->position.x += moveSpeed; 
                UIHAND.GetCameraPtr()->target.x += moveSpeed; 
            }
            if (IsKeyDown(KEY_A)) { 
                UIHAND.GetCameraPtr()->position.x -= moveSpeed; 
                UIHAND.GetCameraPtr()->target.x -= moveSpeed; 
            }

            // Bewegen over de Z-as (Vooruit/Achteruit op je schema)
            // Omdat je naar beneden kijkt, is dit de W en S toets
            if (IsKeyDown(KEY_W)) { 
                UIHAND.GetCameraPtr()->position.z -= moveSpeed; 
                UIHAND.GetCameraPtr()->target.z -= moveSpeed; 
            }
            if (IsKeyDown(KEY_S)) { 
                UIHAND.GetCameraPtr()->position.z += moveSpeed; 
                UIHAND.GetCameraPtr()->target.z += moveSpeed; 
            }

            // Gebruik Q en E om echt te zoomen (omhoog/omlaag gaan)
            if (IsKeyDown(KEY_E)) UIHAND.GetCameraPtr()->position.y += moveSpeed;
            if (IsKeyDown(KEY_Q)) UIHAND.GetCameraPtr()->position.y -= moveSpeed;
        }

        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            float zoomSpeed = 2.0f; // Pas de snelheid naar wens aan
            // Zoom in (omlaag) of uit (omhoog)
            UIHAND.GetCameraPtr()->position.y -= wheel * zoomSpeed;
            
            // Optioneel: zorg dat de camera niet 'door de vloer' gaat
            if (UIHAND.GetCameraPtr()->position.y < 2.0f) UIHAND.GetCameraPtr()->position.y = 2.0f;
        }

        // --- 2. Render ---
        BeginDrawing();
            ClearBackground(RAYWHITE);

            // 3D Wereld renderen binnen de ScissorMode (rechts van sidebar, boven terminal)
            BeginScissorMode(220, 0, GetScreenWidth() - 220, GetScreenHeight() - 150);
                BeginMode3D(UIHAND.GetCamera());
                    
                    // Teken de berekende DXF blokken in 3D
                    #ifdef DXFEnable
                        UIHAND.DrawDXF3D();
                    #endif
                    
                    // Teken de robot/objecten
                    //plane.Draw(planeMesh, material);    
                    //arm.Draw(cubeMesh, material);
                    //hand.Draw(sphereMesh, material);
                    
                    DrawGrid(50, 1.0f);
                    // DrawLine3D({ -100.0f, 0.0f, 0.0f }, { 100.0f, 0.0f, 0.0f }, RED); //x as

                    // // Y-as (Groen) - Dit is de Z-as in Raylib 3D, maar de Y in je DXF
                    // DrawLine3D({ 0.0f, 0.0f, -100.0f }, { 0.0f, 0.0f, 100.0f }, GREEN); // y as
                    
                EndMode3D();
            EndScissorMode();

            // 2D UI (Sidebar, Terminal, Knoppen)
            UIHAND.Render();

        EndDrawing();
    }
    
    // 4. Cleanup
    CloseWindow();
    #endif

    // RobotServer server(5000);

    // if (!server.start())
    // {
    //     std::cerr << "Failed to start robot server\n";
    //     return 1;
    // }

    // std::thread server_thread(&RobotServer::run, &server);

    // while (server.is_running())
    // {   
    //     std::vector<double> iets =robot.GetRobotPose();
    //     Pose current_pose = VectorToPose(iets);
    //     server.set_current_pose(current_pose);

    //     if (server.has_new_target_pose())
    //     {
    //         Pose target_pose = server.get_target_pose();
    //         std::vector<double> vec = PoseToVector(target_pose);
    //         robot.MoveToPose(vec);
    //     }

    //     std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // }

    // server.stop();

    // if (server_thread.joinable())
    // {
    //     server_thread.join();
    // }
    
    return 0;
}
