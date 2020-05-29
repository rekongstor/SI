#include "Scene.h"
#include "Sphere.h"


int main()
{
   Camera camera;
   camera.position = {6.f, 6.f, 9.f};
   camera.direction = {-1.f, -1.f, -2.f};
   camera.direction = normalize(camera.direction);
   camera.fov = 55.f;

   Light light;
   light.color = { 0.6f,0.6f,0.6f };
   light.direction = { 1.f,2.f,4.f };
   light.direction = normalize(light.direction);

   {
      // Objects on stack
      Sphere sphere({0.f, 0.f, 0.f}, 10.f);

      Scene scene(camera, light);
      scene.addObject(&sphere);

      scene.renderScene(300, 300, "phong.bmp");
   }
   return 0;
}
