#ifndef COMMUNICATION_CONTEXT_H
#define COMMUNICATION_CONTEXT_H

#include "domain.h"
#include <stddef.h>

typedef struct CommunicationContext *CommunicationContext;

void init_communication_context(int* argc, char*** argv, CommunicationContext* ctx);
Domain init_domain_from_file(CommunicationContext ctx, const char* filename);
void init_halo_exchange(CommunicationContext ctx, const Domain* domain);
void exchange_halos(CommunicationContext ctx);
void finalize_halo_exchange(CommunicationContext ctx);
void save_domain_to_file(CommunicationContext ctx, const Domain* domain, const char* filename);
void free_communication_context(CommunicationContext* ctx);

#endif