#include "../../include/coordinator.h"

int main(int argc, char **argv) {
  Coordinator coordinator("0.0.0.0", COORDINATOR_RPC_PORT,
                          "/home/cxm/cacheproject/happylrc/config.xml");
  coordinator.start();
  return 0;
}