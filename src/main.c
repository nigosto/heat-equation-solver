#include "communication/communication_context.h"
#include "domain.h"
#include <assert.h>
#include <stdio.h>

int main(int argc, char** argv) {
  float alpha = 0.01, dh = 1.0, dt = 0.1;
  assert(dt < dh * dh / (4 * alpha));

  float r = alpha * dt / (dh * dh);

  if (argc != 3) {
    fprintf(stderr, "Invalid number of arguments. Please specify input and output file\n");
    return 1;
  }

  CommunicationContext ctx;
  init_communication_context(&argc, &argv, &ctx);

  Domain domain = init_domain_from_file(ctx, argv[1]);

  init_halo_exchange(ctx, &domain);
  for (size_t i = 0; i < 1e5; ++i) {
    exchange_halos(ctx);
    update_temperature(&domain, r);
  }
  finalize_halo_exchange(ctx);

  save_domain_to_file(ctx, &domain, argv[2]);
  free_communication_context(&ctx);

  return 0;
}