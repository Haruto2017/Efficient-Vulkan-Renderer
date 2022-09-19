#include "app.h"

void unitTest()
{
    Mesh testMesh;
    testMesh.loadMesh("..\\extern\\common-3d-test-models\\data\\stanford-bunny.obj");
}

int main() {
    //unitTest();
    renderApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}