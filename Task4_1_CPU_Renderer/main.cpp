#include "Scene.h"
#include "Sphere.h"


int main()
{
   Camera camera;
   camera.position = {10.f, 10.f, 10.f};
   camera.direction = {-1.f, -1.f, -1.f};
   camera.direction = normalize(camera.direction);
   camera.fov = 90.f;

   {
      // Objects on stack
      Sphere sphere({0.f, 0.f, 3.f}, 1.f);

      Scene scene(camera);
      scene.addObject(&sphere);

      scene.renderScene(100, 100, "phong.bmp");
   }
   return 0;
}
