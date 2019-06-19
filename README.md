```
#include "cyield.h"

void test(CYieldClientAPI* c) {
  puts("test start");
  c->Yield();
  puts("continue");
  c->Yield();
  puts("test end");
}

int main(int argc, char** argv) {
  CYield cy(4096);
  if (cy.Run(test)) {
    puts("yield");
    while(cy.Continue())
      puts("yield again");
  }
  return 0;
}
```
