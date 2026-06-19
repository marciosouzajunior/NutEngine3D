#include "core/World.h"
#include "core/Camera.h"
#include "core/Mesh.h"
#include "core/Object3D.h"
#include "render/RaylibGraphics.h"
#include "render/WireframeRenderer.h"
#include <iostream>

int main() {
    std::cout << "Starting NutEngine3D V0..." << std::endl;

    // 1. Initialize Raylib Graphics backend
    const int screenWidth = 800;
    const int screenHeight = 600;
    nut::RaylibGraphics graphics(screenWidth, screenHeight, "NutEngine3D V0 - Wireframe Cube");

    // 2. Initialize Renderer
    nut::WireframeRenderer renderer(&graphics);

    // 3. Create a World and Camera
    nut::World world;
    nut::Camera camera;
    // Move the camera back so we can see the origin
    camera.transform.position = nut::math::Vec3(0, 0, -8);

    // 4. Create Geometry (Mesh)
    nut::Mesh cubeMesh = nut::Mesh::createCube();

    // 5. Create Objects and Scene Graph
    
    // Parent object (center cube)
    nut::Object3D parentCube;
    parentCube.mesh = &cubeMesh;
    parentCube.transform.position = nut::math::Vec3(0, 0, 0);
    parentCube.transform.scale = nut::math::Vec3(1.0f, 1.0f, 1.0f);
    
    // Child object (orbiting cube)
    nut::Object3D childCube;
    childCube.mesh = &cubeMesh;
    // Set position relative to the parent. It will be 3 units to the right of the parent.
    childCube.transform.position = nut::math::Vec3(3, 0, 0);
    childCube.transform.scale = nut::math::Vec3(0.5f, 0.5f, 0.5f); // Half the size

    // Link the child to the parent (Scene Graph)
    parentCube.addChild(&childCube);

    // Add ONLY the parent to the world. The renderer will find the child automatically.
    world.add(&parentCube);

    // 6. Main Loop
    while (!graphics.shouldClose()) {
        // Animation:
        // Rotate the parent. Because of the Scene Graph, the child will orbit the parent!
        parentCube.transform.rotation.y += 0.02f;
        parentCube.transform.rotation.x += 0.01f;

        // We can also rotate the child locally. It will spin on its own axis while orbiting.
        childCube.transform.rotation.x -= 0.05f;
        childCube.transform.rotation.z += 0.05f;

        // Render pass
        graphics.beginFrame();
        graphics.clear(); // Black background

        // The renderer calculates matrices and draws the lines
        renderer.render(world, camera);

        graphics.present(); // Swap buffers
    }

    std::cout << "Shutting down..." << std::endl;
    return 0;
}
