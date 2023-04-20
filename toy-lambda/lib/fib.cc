#include <cstdlib>
#include <string>

char msg[100] = {0};

int fib(int n) {
    if (n < 2) {
        return n;
    }
    return fib(n-2) + fib(n-1);
}

extern "C" const char* http_main() {
    int x = fib(20);
    sprintf(msg, "%d", x);
    return msg;
}
