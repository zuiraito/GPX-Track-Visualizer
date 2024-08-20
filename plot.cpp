#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <cmath>
#include <SDL2/SDL.h>
#include <tinyxml2.h>

namespace fs = std::filesystem;
using namespace tinyxml2;

struct Point {
    double lat, lon;
};

// Function to calculate the distance between two geographical points using the Haversine formula
double haversine(double lat1, double lon1, double lat2, double lon2) {
    constexpr double R = 6371.0; // Radius of Earth in kilometers
    double latRad1 = lat1 * M_PI / 180.0;
    double lonRad1 = lon1 * M_PI / 180.0;
    double latRad2 = lat2 * M_PI / 180.0;
    double lonRad2 = lon2 * M_PI / 180.0;

    double dLat = latRad2 - latRad1;
    double dLon = lonRad2 - lonRad1;
    double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
               std::cos(latRad1) * std::cos(latRad2) *
               std::sin(dLon / 2) * std::sin(dLon / 2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return R * c;
}

// Function to determine color based on distance
SDL_Color getColorForDistance(double distance) {
    if (distance <= 0.041) return {0, 0, 139, 255}; // Dark blue
    if (distance <= 0.083) return {173, 216, 230, 255}; // Light blue
    if (distance <= 0.166) return {0, 255, 0, 255}; // Green
    if (distance <= 0.250) return {255, 255, 0, 255}; // Yellow
    if (distance <= 0.416) return {255, 165, 0, 255}; // Orange
    return {150, 0, 0, 150}; // Red
}

std::vector<Point> parseGPX(const std::string &filename) {
    std::vector<Point> points;

    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS) {
        std::cerr << "Failed to load GPX file: " << filename << std::endl;
        return points;
    }

    XMLElement *trkpt = doc.FirstChildElement("gpx")->FirstChildElement("trk")->FirstChildElement("trkseg")->FirstChildElement("trkpt");
    while (trkpt != nullptr) {
        Point p;
        trkpt->QueryDoubleAttribute("lat", &p.lat);
        trkpt->QueryDoubleAttribute("lon", &p.lon);
        points.push_back(p);

        trkpt = trkpt->NextSiblingElement("trkpt");
    }

    return points;
}

void drawPath(SDL_Renderer *renderer, const std::vector<Point> &points, int screenWidth, int screenHeight, double scale, int offsetX, int offsetY, double maxDistance, bool drawPoints, bool colorLines) {
    // Determine bounding box of the points
    double minLat = 90.0, maxLat = -90.0, minLon = 180.0, maxLon = -180.0;
    for (const auto &p : points) {
        if (p.lat < minLat) minLat = p.lat;
        if (p.lat > maxLat) maxLat = p.lat;
        if (p.lon < minLon) minLon = p.lon;
        if (p.lon > maxLon) maxLon = p.lon;
    }

    // Lambda function to convert geographic coordinates to screen coordinates
    auto toScreenCoords = [&](const Point &p) -> std::pair<int, int> {
        int x = static_cast<int>((p.lon - minLon) / (maxLon - minLon) * screenWidth * scale) + offsetX;
        int y = static_cast<int>((1.0 - (p.lat - minLat) / (maxLat - minLat)) * screenHeight * scale) + offsetY;
        return {x, y};
    };

    if (drawPoints) {
        // Draw points
        for (const auto &p : points) {
            auto [x, y] = toScreenCoords(p);
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for points
            SDL_RenderDrawPoint(renderer, x, y);
        }
    } else {
        // Draw lines
        for (size_t i = 1; i < points.size(); ++i) {
            double distance = haversine(points[i - 1].lat, points[i - 1].lon, points[i].lat, points[i].lon);
            if (distance <= maxDistance) {
                auto [x1, y1] = toScreenCoords(points[i - 1]);
                auto [x2, y2] = toScreenCoords(points[i]);
                SDL_Color color = colorLines ? getColorForDistance(distance) : SDL_Color{255, 255, 255, 255}; // White color for default
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
                SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <directory with GPX files>" << std::endl;
        return 1;
    }
    std::cerr << "P:       Lines / Points" << std::endl;
    std::cerr << "S:       Speed" << std::endl;
    std::cerr << "Up/Down: Change line distance threshold" << std::endl;
    std::cerr << "Dark blue:   0km/h" << std::endl;
    std::cerr << "Light Blue:  5km/h" << std::endl;
    std::cerr << "Green:      10km/h" << std::endl;
    std::cerr << "Yellow:     20km/h" << std::endl;
    std::cerr << "Orange:     30km/h" << std::endl;
    std::cerr << "Red:        50km/h" << std::endl;


    std::string directory = argv[1];

    std::vector<Point> allPoints;
    for (const auto &entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() == ".gpx") {
            std::vector<Point> points = parseGPX(entry.path().string());
            allPoints.insert(allPoints.end(), points.begin(), points.end());
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("GPX Track Visualizer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_SHOWN);
    if (win == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    int screenWidth = dm.w;
    int screenHeight = dm.h;

    double scale = 1.0;
    int offsetX = 0, offsetY = 0;
    double maxDistance = 1; // Initial distance threshold in kilometers
    bool drawPoints = false; // Initial mode is to draw lines
    bool colorLines = false; // Initial color mode is off
    bool fullscreen = true; // Start in fullscreen mode
    bool quit = false;
    SDL_Event e;

    bool dragging = false;
    int lastMouseX = 0, lastMouseY = 0;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_MOUSEWHEEL) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                double prevScale = scale;
                if (e.wheel.y > 0) {
                    scale *= 1.1;
                } else if (e.wheel.y < 0) {
                    scale /= 1.1;
                }

                // Calculate the center of the view
                int centerX = screenWidth / 2;
                int centerY = screenHeight / 2;

                // Convert mouse position to view coordinates
                double viewX = (mouseX - offsetX) / prevScale;
                double viewY = (mouseY - offsetY) / prevScale;

                // Recalculate the offset to center the zoom on the mouse position
                offsetX = mouseX - static_cast<int>(viewX * scale);
                offsetY = mouseY - static_cast<int>(viewY * scale);
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_UP) {
                    maxDistance *= 1.1; // Increase threshold
                } else if (e.key.keysym.sym == SDLK_DOWN) {
                    maxDistance = std::max(0.1, maxDistance / 1.1); // Decrease threshold
                } else if (e.key.keysym.sym == SDLK_p) {
                    drawPoints = !drawPoints; // Toggle between drawing points and lines
                    std::cout << (drawPoints ? "Drawing points" : "Drawing lines") << std::endl;
                } else if (e.key.keysym.sym == SDLK_s) {
                    colorLines = !colorLines; // Toggle color mode
                    std::cout << (colorLines ? "Color coding lines" : "Default line color") << std::endl;
                } else if (e.key.keysym.sym == SDLK_f) {
                    fullscreen = !fullscreen;
                    SDL_SetWindowFullscreen(win, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                    SDL_GetWindowSize(win, &screenWidth, &screenHeight);
                    std::cout << (fullscreen ? "Fullscreen mode" : "Windowed mode") << std::endl;
                }
                std::cout << "Distance Threshold: " << maxDistance << " km" << std::endl;
            } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                dragging = true;
                lastMouseX = e.button.x;
                lastMouseY = e.button.y;
            } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
                dragging = false;
            } else if (e.type == SDL_MOUSEMOTION && dragging) {
                int mouseX = e.motion.x;
                int mouseY = e.motion.y;

                offsetX += mouseX - lastMouseX;
                offsetY += mouseY - lastMouseY;

                lastMouseX = mouseX;
                lastMouseY = mouseY;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        drawPath(renderer, allPoints, screenWidth, screenHeight, scale, offsetX, offsetY, maxDistance, drawPoints, colorLines);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}

