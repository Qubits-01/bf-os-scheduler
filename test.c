#include "types.h"
#include "user.h"
int main() {
  schedlog(10000);
  for (int i = 0; i < 3; i++) {
    if (nicefork(-5 + (5 * i)) == 0) {
      char *argv[] = {"loop", 0};
      exec("loop", argv);
    }
  }
  for (int i = 0; i < 3; i++) {
    wait();
  }
  shutdown();
}
