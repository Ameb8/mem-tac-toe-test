#include <stdio.h>
#include <unistd.h>

int main() {
    printf("I am a hanging submission, sleeping indefinitely...\n");
    fflush(stdout);
    while (1) {
        sleep(10);
    }
    return 0;
}
