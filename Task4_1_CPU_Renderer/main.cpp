#include "Plane.h"
#include "RendererPBR.h"
#include "Scene.h"
#include "Sphere.h"
#include "RendererPhong.h"

#define SPHERES_X 5
#define SPHERES_Y 8
#define RESOLUTION 1024

int main()
{
   Camera camera;
   camera.position = {0.0f, 0.f, 4.5f};
   camera.direction = {0.f, 0.f, 1.0f};
   camera.direction = normalize(camera.direction);
   camera.fov = 145.f;

   Light light;
   light.color = {1.0f, 1.0f, 1.0f};
   light.direction = {0.5f, 0.0f, -1.0f};
   light.direction = normalize(light.direction);

   {
      Scene scene(light);

      Plane plane({0.f, 0.f, 1.f, 0.f}, {0.2f, 0.2f, 0.2f}, {0.1f, 0.1f, 0.1f}, 20.f, 0.f, 0.f);
      scene.addObject(&plane);

      Sphere sphere({0.f, 0.f, -1.f}, 0.8f, {1.f, 0.f, 0.f}, {0.4f, 0.3f, 0.4f}, 100.f, 0.f, 0.f);
      scene.addObject(&sphere);

      RendererPhong renderer(camera, {0.01f, 0.01f, 0.02f});
      renderer.renderScene(scene, RESOLUTION, RESOLUTION, "phong.bmp");
   }
   {
      Scene scene(light);

      Plane plane({0.f, 0.f, 1.f, 0.f}, {0.2f, 0.2f, 0.2f}, {0.3f, 0.3f, 0.3f}, 40.f, 0.11f, 1.0f);
      scene.addObject(&plane);

      Sphere spheres[SPHERES_X][SPHERES_Y];
      for (auto i = 0; i < SPHERES_X; ++i)
         for (auto j = 0; j < SPHERES_Y; ++j)
         {
            spheres[i][j] = Sphere({(static_cast<float>(i + 1) - (SPHERES_X + 1) / 2.f) * 2.f, (static_cast<float>(j + 1) - (SPHERES_Y + 1)/ 2.f) * 2.f, -1.f}, 0.8f,
                                   {1.f, 0.f, 0.f}, {0.2f, 0.2f, 0.2f}, 0.1f,
                                   static_cast<float>(i) / SPHERES_X, static_cast<float>(j) / SPHERES_Y);
            scene.addObject(&spheres[i][j]);
         }

      RendererPBR renderer(camera, {0.01f, 0.01f, 0.02f});
      renderer.renderScene(scene, RESOLUTION, RESOLUTION, "pbr.bmp");
   }
   return 0;
}
