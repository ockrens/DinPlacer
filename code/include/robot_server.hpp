#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <atomic>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#endif

#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct Pose
{
    float x;
    float y;
    float z;
    float rx;
    float ry;
    float rz;
};
Pose VectorToPose(const std::vector<double>& vec)
{
    if (vec.size() != 6)
    {
        throw std::invalid_argument("Vector must contain exactly 6 elements");
    }

    return {
        static_cast<float>(vec[0]),
        static_cast<float>(vec[1]),
        static_cast<float>(vec[2]),
        static_cast<float>(vec[3]),
        static_cast<float>(vec[4]),
        static_cast<float>(vec[5])
    };
}
std::vector<double> PoseToVector(const Pose& pose)
{
    return {
        static_cast<double>(pose.x),
        static_cast<double>(pose.y),
        static_cast<double>(pose.z),
        static_cast<double>(pose.rx),
        static_cast<double>(pose.ry),
        static_cast<double>(pose.rz)
    };
}

class RobotServer
{
public:
    RobotServer(int port = 5000)
        : port(port)
    {
        current_pose = {0, 0, 0, 0, 0, 0};
        target_pose = {0, 0, 0, 0, 0, 0};
    }

    bool start()
    {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

        server_fd = socket(AF_INET, SOCK_STREAM, 0);

        if (server_fd < 0)
        {
            std::cerr << "Failed to create socket\n";
            return false;
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0)
        {
            std::cerr << "Bind failed\n";
            return false;
        }

        if (listen(server_fd, 1) < 0)
        {
            std::cerr << "Listen failed\n";
            return false;
        }

        std::cout << "Waiting for Python connection...\n";

        socklen_t addrlen = sizeof(address);
        client_fd = accept(server_fd, (sockaddr*)&address, &addrlen);

        if (client_fd < 0)
        {
            std::cerr << "Accept failed\n";
            return false;
        }

        std::cout << "Python client connected\n";

        running = true;
        return true;
    }

    void run()
    {
        std::string buffer;

        while (running)
        {
            std::string line = receive_line();

            if (line.empty())
            {
                std::cout << "Client disconnected\n";
                running = false;
                break;
            }

            try
            {
                json request = json::parse(line);
                handle_request(request);
            }
            catch (const std::exception& e)
            {
                send_error(e.what());
            }
        }
    }

    void stop()
    {
        running = false;

    #ifdef _WIN32
        shutdown(client_fd, SD_BOTH);  // 🔥 BELANGRIJK
        closesocket(client_fd);
        closesocket(server_fd);
        WSACleanup();
    #else
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        close(server_fd);
    #endif
    }

    bool is_running() const
    {
        return running;
    }

    bool has_new_target_pose() const
    {
        return new_target_pose_available;
    }

    Pose get_target_pose()
    {
        std::lock_guard<std::mutex> lock(pose_mutex);

        new_target_pose_available = false;
        return target_pose;
    }

    Pose get_current_pose()
    {
        std::lock_guard<std::mutex> lock(pose_mutex);
        return current_pose;
    }

    void set_current_pose(const Pose& pose)
    {
        std::lock_guard<std::mutex> lock(pose_mutex);
        current_pose = pose;
    }

private:
    int server_fd = -1;
    int client_fd = -1;
    int port;

    std::atomic<bool> running{false};
    std::atomic<bool> new_target_pose_available{false};

    Pose current_pose;
    Pose target_pose;

    mutable std::mutex pose_mutex;

    void handle_request(const json& request)
    {
        std::string cmd = request["cmd"];

        if (cmd == "get_pose")
        {
            send_pose();
        }
        else if (cmd == "set_pose")
        {
            if (!request.contains("pose") || request["pose"].size() != 6)
            {
                send_error("Pose must contain exactly 6 values: x, y, z, rx, ry, rz");
                return;
            }

            auto p = request["pose"];

            Pose new_pose;
            new_pose.x  = p[0];
            new_pose.y  = p[1];
            new_pose.z  = p[2];
            new_pose.rx = p[3];
            new_pose.ry = p[4];
            new_pose.rz = p[5];

            {
                std::lock_guard<std::mutex> lock(pose_mutex);
                target_pose = new_pose;
                new_target_pose_available = true;
            }

            send_ack();
        }
        else
        {
            send_error("Unknown command");
        }
    }

    void send_ack()
    {
        json response = {
            {"ok", true}
        };

        send_json(response);
    }

    void send_pose()
    {
        Pose pose;

        {
            std::lock_guard<std::mutex> lock(pose_mutex);
            pose = current_pose;
        }

        json response = {
            {"ok", true},
            {"pose", {
                pose.x,
                pose.y,
                pose.z,
                pose.rx,
                pose.ry,
                pose.rz
            }}
        };

        send_json(response);
    }

    void send_error(const std::string& error)
    {
        json response = {
            {"ok", false},
            {"error", error}
        };

        send_json(response);
    }

    void send_json(const json& data)
    {
        std::string message = data.dump() + "\n";
        send(client_fd, message.c_str(), message.size(), 0);
    }

    std::string receive_line()
    {
        std::string line;
        char c;

        while (true)
        {
            int bytes = recv(client_fd, &c, 1, 0);

            if (bytes <= 0)
            {
                return "";
            }

            if (c == '\n')
            {
                break;
            }

            line += c;
        }

        return line;
    }
};