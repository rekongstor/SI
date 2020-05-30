#include "Plane.h"
#include "Scene.h"
#include "Sphere.h"
#include "RendererPhong.h"

#define SPHERES_X 5
#define SPHERES_Y 8

int main()
{
   Camera camera;
   camera.position = {-6.f, 2.f, 8.f};
   camera.direction = {1.f, -0.2f, 0.9f};
   camera.direction = normalize(camera.direction);
   camera.fov = 85.f;

   Light light;
   light.color = {1.0f, 1.0f, 1.0f};
   light.direction = {0.0f, 0.0f, -1.0f};
   light.direction = normalize(light.direction);

   {
      Scene scene(light);

      Plane plane({0.f, 0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.3f, 0.3f, 0.3f}, 40.f);
      scene.addObject(&plane);

      Sphere sphere({0.f, 0.f, -4.f}, 2.f, {1.f, 0.f, 0.f}, {0.4f, 0.3f, 0.4f}, 100.f);
      scene.addObject(&sphere);

      RendererPhong renderer(camera, {0.01f, 0.01f, 0.02f});
      renderer.renderScene(scene, 512, 512, "phong.bmp");
   }
   {
      Scene scene(light);

      Plane plane({0.f, 0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.3f, 0.3f, 0.3f}, 40.f);
      scene.addObject(&plane);

      Sphere spheres[SPHERES_X][SPHERES_Y];
      for (auto i = 0; i < SPHERES_X; ++i)
         for (auto j = 0; j < SPHERES_Y; ++j)
         {
            spheres[i][j] = Sphere({(i - SPHERES_X / 2.f) * 2.f, (j - SPHERES_Y / 2.f) * 2.f, -3.f}, 0.8f, {1.f, 0.f, 0.f}, {0.4f, 0.3f, 0.4f}, 100.f);
            scene.addObject(&spheres[i][j]);
         }

      RendererPhong renderer(camera, {0.01f, 0.01f, 0.02f});
      renderer.renderScene(scene, 1024, 1024, "pbr.bmp");
   }
   return 0;
}
