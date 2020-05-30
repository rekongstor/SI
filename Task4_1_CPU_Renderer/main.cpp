#include "Plane.h"
#include "Scene.h"
#include "Sphere.h"
#include "RendererPhong.h"

int main()
{
   Camera camera;
   camera.position = {-3.f, 0.f, 6.f};
   camera.direction = {1.f, 0.f, 2.0f};
   camera.direction = normalize(camera.direction);
   camera.fov = 45.f;

   Light light;
   light.color = {1.0f, 1.0f, 1.0f};
   light.direction = {0.0f, 0.0f, -1.0f};
   light.direction = normalize(light.direction);

   {
      // Objects on stack
      Sphere sphere({0.f, 0.f, -3.f}, 2.f, {1.f, 0.f, 0.f}, { 0.4f, 0.3f, 0.4f }, 50.f);
      Plane plane({0.f, 0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.3f, 0.3f, 0.3f}, 10.f);

      Scene scene(light);
      scene.addObject(&sphere);
      scene.addObject(&plane);

      RendererPhong renderer(camera, {0.01f, 0.01f, 0.02f});
      renderer.renderScene(scene, 512, 512, "phong.bmp");
   }
   return 0;
}
