//#include <SDL.h>
//#include "vk_engine.h"
//
//int main(int argc, char** argv) {
//    SDL_SetMainReady(); // SDL'nin main tanýmýný devre dýþý býrakýr sonrasýnda bir de silip denemeyi unutma
//
//    VulkanEngine engine;
//
//    engine.init();
//    engine.run();
//    engine.cleanup();
//
//    return 0;
//}



//#define SDL_MAIN_HANDLED
//#include <SDL.h>
//#include "vk_engine.h"
//
//int main(int argc, char** argv) {
//    SDL_SetMainReady(); // SDL'nin main tanýmýný devre dýþý býrakýr
//
//    VulkanEngine engine;
//
//    engine.init();
//    engine.run();
//    engine.cleanup();
//
//    return 0;
//}




//#include <SDL.h>
//#include "vk_engine.h"
//
//int SDL_main(int argc, char** argv) {  // SDL'nin beklediði fonksiyon ismi
//    VulkanEngine engine;
//
//    engine.init();
//    engine.run();
//    engine.cleanup();
//
//    return 0;
//}


#include <SDL.h>
#include "vk_engine.h"

int main(int argc, char** argv) {
    //SDL_SetMainReady(); // SDL'nin main tanýmýný devre dýþý býrakýr sonrasýnda bir de silip denemeyi unutma

    VulkanEngine engine;

    engine.init();
    engine.run();
    engine.cleanup();

    return 0;
}