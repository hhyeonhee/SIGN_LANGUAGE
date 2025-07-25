#include "server.h"

using namespace std;

int main() {
    TcpServer server(12345);
    if (server.start()) {
        cout << "Press Enter to stop server..." << endl;
        cin.get();
        server.stop();
    }
    return 0;
}