
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <nightingale.h>

int main() {
        if (fork()) {
                return EXIT_SUCCESS;
        }

        while (1) {
                sleep(1);
                printf("bg");
        }
}
