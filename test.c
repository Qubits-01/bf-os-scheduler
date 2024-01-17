#include "types.h"
#include "user.h"
int main() {
  int nice_levels[3] = {-5, 0, 5};
  schedlog(10000);
  for (int i = 0; i < 3; i++) {
    if (nicefork(nice_levels[i]) == 0) {
      char *argv[] = {"loop", 0};
      exec("loop", argv);
    }
  }
  for (int i = 0; i < 3; i++) {
    wait();
  }
  shutdown();
}
