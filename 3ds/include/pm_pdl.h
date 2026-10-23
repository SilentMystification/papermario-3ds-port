#ifndef PM_PDL_H
#define PM_PDL_H

/* C1/C4: generations for compiled display-list cache invalidation */
extern unsigned pm_pdl_heap_gen;
extern unsigned pm_pdl_entity_gen;
extern unsigned pm_pdl_genheap_gen;
extern unsigned pm_pdl_sprite_gen;

void pm_pdl_touch_range(const void* ptr, unsigned size);
void pm_pdl_genheap_bump(void);

#define PM_PSO_N 512
void pm_pso_bake(unsigned id);
unsigned pm_pdl_flush_pso(void);

#endif
